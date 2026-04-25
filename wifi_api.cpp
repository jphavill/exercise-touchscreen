#include "wifi_api.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <cstring>

#include "config.h"

static bool parse_payload(const String& payload, PullupDashboardData& outData) {
  outData.valid = false;

  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    return false;
  }

  if (!doc["year_total"].is<uint32_t>() || !doc["daily_goal"].is<uint16_t>()) {
    return false;
  }

  JsonArray last30 = doc["last_30_days"].as<JsonArray>();
  if (last30.isNull() || last30.size() != 30) {
    return false;
  }

  outData.yearTotal = doc["year_total"].as<uint32_t>();
  outData.dailyGoal = doc["daily_goal"].as<uint16_t>();

  for (size_t i = 0; i < 30; i++) {
    JsonObject day = last30[i].as<JsonObject>();
    if (day.isNull() || !day["date"].is<const char*>() || !day["count"].is<uint16_t>() ||
        !day["heat_level"].is<uint8_t>()) {
      return false;
    }

    const char* date = day["date"].as<const char*>();
    strncpy(outData.days[i].date, date, sizeof(outData.days[i].date) - 1);
    outData.days[i].date[sizeof(outData.days[i].date) - 1] = '\0';
    outData.days[i].count = day["count"].as<uint16_t>();
    outData.days[i].heatLevel = day["heat_level"].as<uint8_t>();
  }

  outData.valid = true;
  return true;
}

bool wifi_connect() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(WIFI_RETRY_DELAY_MS);
  }

  return (WiFi.status() == WL_CONNECTED);
}

bool api_fetch_data(PullupDashboardData& outData) {
  outData.valid = false;

  if (!wifi_connect()) {
    return false;
  }

  HTTPClient http;
  if (!http.begin(API_URL)) {
    Serial.println("[API] Request setup failed");
    return false;
  }

  http.addHeader("X-Timezone", "America/Halifax");

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.print("[API] Request failed, code: ");
    Serial.println(code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  const bool parsed = parse_payload(payload, outData);
  if (!parsed) {
    Serial.println("[API] Invalid response payload");
  }
  return parsed;
}
