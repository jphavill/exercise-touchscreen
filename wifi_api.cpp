#include "wifi_api.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "config.h"

static constexpr size_t kDashboardJsonDocCapacity = 16384;

template <typename T>
static bool parse_required_number(JsonVariantConst root, const char* key, T& outValue) {
  JsonVariantConst field = root[key];
  if (!field.is<T>()) {
    Serial.print("[API] Missing or invalid ");
    Serial.println(key);
    return false;
  }

  outValue = field.as<T>();
  return true;
}

static bool parse_day_entry(JsonVariantConst dayValue, DayEntry& outDay, size_t index) {
  JsonObjectConst day = dayValue.as<JsonObjectConst>();
  if (day.isNull()) {
    Serial.print("[API] Invalid day entry object at index ");
    Serial.println(static_cast<int>(index));
    return false;
  }

  if (!day["count"].is<uint16_t>() || !day["heat_level"].is<uint8_t>()) {
    Serial.print("[API] Invalid day entry fields at index ");
    Serial.println(static_cast<int>(index));
    return false;
  }

  outDay.count = day["count"].as<uint16_t>();
  outDay.heatLevel = day["heat_level"].as<uint8_t>();
  return true;
}

static bool parse_payload(const String& payload, PullupDashboardData& outData) {
  outData.valid = false;

  // Sized for the current payload shape: 30 day entries plus metadata.
  DynamicJsonDocument doc(kDashboardJsonDocCapacity);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("[API] JSON parse error: ");
    Serial.println(err.c_str());
    return false;
  }

  if (!parse_required_number<uint32_t>(doc.as<JsonVariantConst>(), "year_total", outData.yearTotal) ||
      !parse_required_number<uint16_t>(doc.as<JsonVariantConst>(), "daily_goal", outData.dailyGoal)) {
    return false;
  }

  JsonArray last30 = doc["last_30_days"].as<JsonArray>();
  if (last30.isNull() || last30.size() != 30) {
    Serial.print("[API] last_30_days invalid size: ");
    Serial.println(last30.isNull() ? -1 : static_cast<int>(last30.size()));
    return false;
  }

  for (size_t i = 0; i < 30; i++) {
    if (!parse_day_entry(last30[i].as<JsonVariantConst>(), outData.days[i], i)) {
      return false;
    }
  }

  outData.valid = true;
  return true;
}

bool wifi_connect() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  Serial.print("[WIFI] Connecting to SSID: ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(WIFI_RETRY_DELAY_MS);
  }

  const bool connected = (WiFi.status() == WL_CONNECTED);
  if (connected) {
    Serial.print("[WIFI] Connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.print("[WIFI] Connect failed, status: ");
    Serial.println(static_cast<int>(WiFi.status()));
  }

  return connected;
}

bool api_fetch_data(PullupDashboardData& outData) {
  outData.valid = false;

  if (!wifi_connect()) {
    return false;
  }

  HTTPClient http;
  if (!http.begin(API_URL)) {
    Serial.print("[API] http.begin failed for URL: ");
    Serial.println(API_URL);
    return false;
  }

  http.addHeader("X-Timezone", "America/Halifax");

  Serial.print("[API] GET ");
  Serial.println(API_URL);
  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.print("[API] HTTP GET failed, code: ");
    Serial.println(code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  Serial.print("[API] Payload bytes: ");
  Serial.println(payload.length());

  return parse_payload(payload, outData);
}
