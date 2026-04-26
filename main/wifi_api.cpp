#include "wifi_api.h"

#include <cstring>
#include <string>

#include "cJSON.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

#include "config.h"

static const char* TAG = "wifi_api";
static EventGroupHandle_t s_wifiEventGroup = nullptr;
static bool s_wifiInitialized = false;
static esp_event_handler_instance_t s_wifiAnyIdHandler;
static esp_event_handler_instance_t s_wifiGotIpHandler;

static constexpr int WIFI_CONNECTED_BIT = BIT0;

struct HttpResponseBuffer {
  std::string body;
};

static bool request_wifi_connect() {
  const esp_err_t err = esp_wifi_connect();
  if (err == ESP_OK || err == ESP_ERR_WIFI_CONN) {
    return true;
  }

  ESP_LOGW(TAG, "esp_wifi_connect failed: %s", esp_err_to_name(err));
  return false;
}

static esp_err_t on_http_event(esp_http_client_event_t* evt) {
  auto* response = static_cast<HttpResponseBuffer*>(evt->user_data);
  if (response == nullptr) {
    return ESP_OK;
  }

  if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data != nullptr && evt->data_len > 0) {
    response->body.append(static_cast<const char*>(evt->data), evt->data_len);
  }

  return ESP_OK;
}

static void on_wifi_event(void* arg,
                          esp_event_base_t event_base,
                          int32_t event_id,
                          void* event_data) {
  (void)arg;
  (void)event_data;

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    request_wifi_connect();
    return;
  }

  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
    xEventGroupClearBits(s_wifiEventGroup, WIFI_CONNECTED_BIT);
    request_wifi_connect();
    return;
  }

  if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    xEventGroupSetBits(s_wifiEventGroup, WIFI_CONNECTED_BIT);
  }
}

static bool ensure_wifi_stack_initialized() {
  if (s_wifiInitialized) {
    return true;
  }

  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "nvs_flash_init failed: %s", esp_err_to_name(err));
    return false;
  }

  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));

  s_wifiEventGroup = xEventGroupCreate();
  if (s_wifiEventGroup == nullptr) {
    ESP_LOGE(TAG, "Failed to create Wi-Fi event group");
    return false;
  }

  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      WIFI_EVENT, ESP_EVENT_ANY_ID, &on_wifi_event, nullptr, &s_wifiAnyIdHandler));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
      IP_EVENT, IP_EVENT_STA_GOT_IP, &on_wifi_event, nullptr, &s_wifiGotIpHandler));

  wifi_config_t wifiConfig = {};
  std::strncpy(reinterpret_cast<char*>(wifiConfig.sta.ssid), WIFI_SSID,
               sizeof(wifiConfig.sta.ssid) - 1);
  std::strncpy(reinterpret_cast<char*>(wifiConfig.sta.password), WIFI_PASSWORD,
               sizeof(wifiConfig.sta.password) - 1);
  wifiConfig.sta.pmf_cfg.capable = true;
  wifiConfig.sta.pmf_cfg.required = false;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));
  ESP_ERROR_CHECK(esp_wifi_start());

  s_wifiInitialized = true;
  return true;
}

static bool parse_payload(const std::string& payload, PullupDashboardData& outData) {
  outData.valid = false;

  cJSON* root = cJSON_Parse(payload.c_str());
  if (root == nullptr) {
    return false;
  }

  cJSON* yearTotal = cJSON_GetObjectItemCaseSensitive(root, "year_total");
  cJSON* dailyGoal = cJSON_GetObjectItemCaseSensitive(root, "daily_goal");
  cJSON* last30 = cJSON_GetObjectItemCaseSensitive(root, "last_30_days");

  if (!cJSON_IsNumber(yearTotal) || !cJSON_IsNumber(dailyGoal) || !cJSON_IsArray(last30) ||
      cJSON_GetArraySize(last30) != 30) {
    cJSON_Delete(root);
    return false;
  }

  outData.yearTotal = static_cast<uint32_t>(yearTotal->valuedouble);
  outData.dailyGoal = static_cast<uint16_t>(dailyGoal->valuedouble);

  for (size_t i = 0; i < 30; i++) {
    cJSON* day = cJSON_GetArrayItem(last30, i);
    if (!cJSON_IsObject(day)) {
      cJSON_Delete(root);
      return false;
    }

    cJSON* dateItem = cJSON_GetObjectItemCaseSensitive(day, "date");
    cJSON* countItem = cJSON_GetObjectItemCaseSensitive(day, "count");
    cJSON* heatLevelItem = cJSON_GetObjectItemCaseSensitive(day, "heat_level");

    if (!cJSON_IsString(dateItem) || !cJSON_IsNumber(countItem) || !cJSON_IsNumber(heatLevelItem)) {
      cJSON_Delete(root);
      return false;
    }

    const char* date = dateItem->valuestring;
    strncpy(outData.days[i].date, date, sizeof(outData.days[i].date) - 1);
    outData.days[i].date[sizeof(outData.days[i].date) - 1] = '\0';
    outData.days[i].count = static_cast<uint16_t>(countItem->valuedouble);
    outData.days[i].heatLevel = static_cast<uint8_t>(heatLevelItem->valuedouble);
  }

  cJSON_Delete(root);
  outData.valid = true;
  return true;
}

bool wifi_connect() {
  if (std::strlen(WIFI_SSID) == 0) {
    ESP_LOGE(TAG, "CONFIG_EXERCISE_WIFI_SSID is not set");
    return false;
  }

  if (!ensure_wifi_stack_initialized()) {
    return false;
  }

  wifi_ap_record_t apInfo = {};
  if (esp_wifi_sta_get_ap_info(&apInfo) == ESP_OK) {
    return true;
  }

  xEventGroupClearBits(s_wifiEventGroup, WIFI_CONNECTED_BIT);
  if (!request_wifi_connect()) {
    return false;
  }

  const EventBits_t bits = xEventGroupWaitBits(
      s_wifiEventGroup, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE,
      pdMS_TO_TICKS(WIFI_CONNECT_TIMEOUT_MS));

  if ((bits & WIFI_CONNECTED_BIT) != 0) {
    ESP_LOGI(TAG, "Wi-Fi connected");
    return true;
  }

  ESP_LOGW(TAG, "Wi-Fi connect timed out");
  return false;
}

bool api_fetch_data(PullupDashboardData& outData) {
  outData.valid = false;

  if (!wifi_connect()) {
    return false;
  }

  HttpResponseBuffer response;
  esp_http_client_config_t httpConfig = {};
  httpConfig.url = API_URL;
  httpConfig.timeout_ms = static_cast<int>(WIFI_CONNECT_TIMEOUT_MS);
  httpConfig.event_handler = on_http_event;
  httpConfig.user_data = &response;

  esp_http_client_handle_t client = esp_http_client_init(&httpConfig);
  if (client == nullptr) {
    ESP_LOGE(TAG, "Request setup failed");
    return false;
  }

  esp_http_client_set_method(client, HTTP_METHOD_GET);
  esp_http_client_set_header(client, "X-Timezone", API_TIMEZONE);

  const esp_err_t requestErr = esp_http_client_perform(client);
  if (requestErr != ESP_OK) {
    ESP_LOGW(TAG, "Request failed: %s", esp_err_to_name(requestErr));
    esp_http_client_cleanup(client);
    return false;
  }

  const int code = esp_http_client_get_status_code(client);
  esp_http_client_cleanup(client);

  if (code != 200) {
    ESP_LOGW(TAG, "Request failed, HTTP code: %d", code);
    return false;
  }

  const bool parsed = parse_payload(response.body, outData);
  if (!parsed) {
    ESP_LOGW(TAG, "Invalid response payload");
  }

  return parsed;
}
