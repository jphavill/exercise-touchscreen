#include "wifi_api.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <string.h>

#include "config.h"

static constexpr size_t kDashboardJsonDocCapacity = 16384;
static constexpr size_t kWorkoutsJsonDocCapacity = 49152;

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

static bool copy_required_string(JsonVariantConst field, char* outValue, size_t outSize,
                                 const char* fieldName) {
  if (!field.is<const char*>()) {
    Serial.print("[API] Missing or invalid ");
    Serial.println(fieldName);
    return false;
  }

  const char* value = field.as<const char*>();
  if (value == nullptr || value[0] == '\0') {
    Serial.print("[API] Empty ");
    Serial.println(fieldName);
    return false;
  }

  snprintf(outValue, outSize, "%s", value);
  return true;
}

static bool parse_workout_step_kind(const char* value, WorkoutStepKind& outKind) {
  if (strcmp(value, "timed_exercise") == 0) {
    outKind = WorkoutStepKind::TimedExercise;
    return true;
  }
  if (strcmp(value, "rest") == 0) {
    outKind = WorkoutStepKind::Rest;
    return true;
  }
  if (strcmp(value, "rep_exercise") == 0) {
    outKind = WorkoutStepKind::RepExercise;
    return true;
  }

  Serial.print("[API] Unknown workout step kind: ");
  Serial.println(value);
  return false;
}

static bool parse_workout_step(JsonVariantConst stepValue, WorkoutStep& outStep, size_t index) {
  JsonObjectConst step = stepValue.as<JsonObjectConst>();
  if (step.isNull()) {
    Serial.print("[API] Invalid workout step object at index ");
    Serial.println(static_cast<int>(index));
    return false;
  }

  char kindBuf[24];
  if (!copy_required_string(step["kind"], kindBuf, sizeof(kindBuf), "kind") ||
      !parse_workout_step_kind(kindBuf, outStep.kind) ||
      !copy_required_string(step["title"], outStep.title, sizeof(outStep.title), "title")) {
    return false;
  }

  outStep.durationSec = 0;
  outStep.reps = 0;
  outStep.repUnit[0] = '\0';

  switch (outStep.kind) {
    case WorkoutStepKind::TimedExercise:
    case WorkoutStepKind::Rest:
      if (!step["duration_sec"].is<uint16_t>() || step["duration_sec"].as<uint16_t>() == 0) {
        Serial.print("[API] Invalid duration_sec at step index ");
        Serial.println(static_cast<int>(index));
        return false;
      }
      outStep.durationSec = step["duration_sec"].as<uint16_t>();
      break;
    case WorkoutStepKind::RepExercise:
      if (!step["reps"].is<uint16_t>() || step["reps"].as<uint16_t>() == 0 ||
          !copy_required_string(step["rep_unit"], outStep.repUnit, sizeof(outStep.repUnit),
                                "rep_unit")) {
        Serial.print("[API] Invalid rep exercise fields at step index ");
        Serial.println(static_cast<int>(index));
        return false;
      }
      outStep.reps = step["reps"].as<uint16_t>();
      break;
  }

  return true;
}

static bool parse_workout_routine(JsonVariantConst routineValue, WorkoutRoutine& outRoutine,
                                  size_t routineIndex) {
  JsonObjectConst routine = routineValue.as<JsonObjectConst>();
  if (routine.isNull()) {
    Serial.print("[API] Invalid workout object at index ");
    Serial.println(static_cast<int>(routineIndex));
    return false;
  }

  if (!parse_required_number<uint16_t>(routine, "id", outRoutine.id) ||
      !copy_required_string(routine["name"], outRoutine.name, sizeof(outRoutine.name), "name") ||
      !parse_required_number<uint16_t>(routine, "duration_min", outRoutine.durationMin)) {
    return false;
  }

  JsonArrayConst steps = routine["steps"].as<JsonArrayConst>();
  if (steps.isNull() || steps.size() == 0 || steps.size() > kMaxWorkoutSteps) {
    Serial.print("[API] Invalid workout steps size at routine index ");
    Serial.println(static_cast<int>(routineIndex));
    return false;
  }

  outRoutine.stepCount = static_cast<uint8_t>(steps.size());
  for (size_t i = 0; i < steps.size(); i++) {
    if (!parse_workout_step(steps[i].as<JsonVariantConst>(), outRoutine.steps[i], i)) {
      return false;
    }
  }

  return true;
}

static bool parse_dashboard_payload(const String& payload, PullupDashboardData& outData) {
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

static bool parse_workouts_payload(const String& payload, WorkoutCache& outCache) {
  outCache = {};

  DynamicJsonDocument doc(kWorkoutsJsonDocCapacity);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.print("[API] Workouts JSON parse error: ");
    Serial.println(err.c_str());
    return false;
  }

  JsonVariantConst root = doc.as<JsonVariantConst>();
  JsonArrayConst workouts = root.as<JsonArrayConst>();
  if (workouts.isNull()) {
    workouts = root["workouts"].as<JsonArrayConst>();
  }

  if (workouts.isNull() || workouts.size() > kMaxWorkouts) {
    Serial.print("[API] Invalid workouts array size: ");
    Serial.println(workouts.isNull() ? -1 : static_cast<int>(workouts.size()));
    return false;
  }

  outCache.workoutCount = static_cast<uint8_t>(workouts.size());
  for (size_t i = 0; i < workouts.size(); i++) {
    if (!parse_workout_routine(workouts[i].as<JsonVariantConst>(), outCache.workouts[i], i)) {
      return false;
    }
  }

  outCache.hasLoadedOnce = true;
  outCache.refreshInProgress = false;
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

bool fetch_dashboard_data(PullupDashboardData& outData) {
  outData.valid = false;

  if (!wifi_connect()) {
    return false;
  }

  HTTPClient http;
  if (!http.begin(DASHBOARD_API_URL)) {
    Serial.print("[API] http.begin failed for URL: ");
    Serial.println(DASHBOARD_API_URL);
    return false;
  }

  http.setTimeout(API_HTTP_TIMEOUT_MS);
  http.addHeader("X-Timezone", "America/Halifax");

  Serial.print("[API] GET ");
  Serial.println(DASHBOARD_API_URL);
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

  return parse_dashboard_payload(payload, outData);
}

bool fetch_workouts(WorkoutCache& outCache) {
  if (!wifi_connect()) {
    return false;
  }

  HTTPClient http;
  if (!http.begin(WORKOUTS_API_URL)) {
    Serial.print("[API] http.begin failed for URL: ");
    Serial.println(WORKOUTS_API_URL);
    return false;
  }

  http.setTimeout(API_HTTP_TIMEOUT_MS);

  Serial.print("[API] GET ");
  Serial.println(WORKOUTS_API_URL);
  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.print("[API] Workouts HTTP GET failed, code: ");
    Serial.println(code);
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  Serial.print("[API] Workouts payload bytes: ");
  Serial.println(payload.length());

  return parse_workouts_payload(payload, outCache);
}
