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
    Serial.print("[API] JSON parse error: ");
    Serial.println(err.c_str());
    return false;
  }

  if (!doc["year_total"].is<uint32_t>() || !doc["daily_goal"].is<uint16_t>()) {
    Serial.println("[API] Missing or invalid year_total/daily_goal");
    return false;
  }

  JsonArray last30 = doc["last_30_days"].as<JsonArray>();
  if (last30.isNull() || last30.size() != 30) {
    Serial.print("[API] last_30_days invalid size: ");
    Serial.println(last30.isNull() ? -1 : static_cast<int>(last30.size()));
    return false;
  }

  outData.yearTotal = doc["year_total"].as<uint32_t>();
  outData.dailyGoal = doc["daily_goal"].as<uint16_t>();

  for (size_t i = 0; i < 30; i++) {
    JsonObject day = last30[i].as<JsonObject>();
    if (day.isNull() || !day["date"].is<const char*>() || !day["count"].is<uint16_t>() ||
        !day["heat_level"].is<uint8_t>()) {
      Serial.print("[API] Invalid day entry at index ");
      Serial.println(static_cast<int>(i));
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
