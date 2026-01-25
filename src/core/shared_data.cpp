// shared_data.cpp - Implementation of thread-safe shared data structures
#include "shared_data.h"
#include "logger.h"

namespace SharedData {

// Static data storage
static SensorData sensorData;
static ControlState controlState;

// Mutexes for thread-safe access
static SemaphoreHandle_t sensorDataMutex = nullptr;
static SemaphoreHandle_t controlStateMutex = nullptr;

bool init() {
  Logger::info("SharedData: Initializing thread-safe data structures");

  // Create mutexes
  sensorDataMutex = xSemaphoreCreateMutex();
  controlStateMutex = xSemaphoreCreateMutex();

  if (sensorDataMutex == nullptr || controlStateMutex == nullptr) {
    Logger::error("SharedData: Failed to create mutexes");
    return false;
  }

  // Initialize shared data with safe defaults
  memset(&sensorData, 0, sizeof(SensorData));
  // Mark all sensors as initially invalid until first read
  for (int i = 0; i < 6; i++) {
    sensorData.currentOk[i] = false;
  }
  for (int i = 0; i < 4; i++) {
    sensorData.tempOk[i] = false;
    sensorData.wheelOk[i] = false;
  }
  sensorData.i2cBusOk = true;
  sensorData.currentTimestamp = millis();
  sensorData.tempTimestamp = millis();
  sensorData.wheelTimestamp = millis();
  sensorData.inputTimestamp = millis();

  // Initialize control state with safe defaults
  memset(&controlState, 0, sizeof(ControlState));
  controlState.lastHeartbeat = millis();

  Logger::info("SharedData: Initialization complete");
  return true;
}

bool readSensorData(SensorData &data) {
  if (sensorDataMutex == nullptr) {
    return false;
  }

  if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    memcpy(&data, &sensorData, sizeof(SensorData));
    xSemaphoreGive(sensorDataMutex);
    return true;
  }

  Logger::warn("SharedData: Timeout reading sensor data");
  return false;
}

bool writeSensorData(const SensorData &data) {
  if (sensorDataMutex == nullptr) {
    return false;
  }

  if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    memcpy(&sensorData, &data, sizeof(SensorData));
    xSemaphoreGive(sensorDataMutex);
    return true;
  }

  Logger::warn("SharedData: Timeout writing sensor data");
  return false;
}

bool readControlState(ControlState &state) {
  if (controlStateMutex == nullptr) {
    return false;
  }

  if (xSemaphoreTake(controlStateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    memcpy(&state, &controlState, sizeof(ControlState));
    xSemaphoreGive(controlStateMutex);
    return true;
  }

  Logger::warn("SharedData: Timeout reading control state");
  return false;
}

bool writeControlState(const ControlState &state) {
  if (controlStateMutex == nullptr) {
    return false;
  }

  if (xSemaphoreTake(controlStateMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
    memcpy(&controlState, &state, sizeof(ControlState));
    xSemaphoreGive(controlStateMutex);
    return true;
  }

  Logger::warn("SharedData: Timeout writing control state");
  return false;
}

bool isSensorDataStale(uint32_t maxAgeMs) {
  if (sensorDataMutex == nullptr) {
    return true;
  }

  bool stale = true;
  if (xSemaphoreTake(sensorDataMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    uint32_t now = millis();
    // Check if any critical sensor data is stale
    stale = (now - sensorData.currentTimestamp > maxAgeMs) ||
            (now - sensorData.wheelTimestamp > maxAgeMs) ||
            (now - sensorData.inputTimestamp > maxAgeMs);
    xSemaphoreGive(sensorDataMutex);
  }

  return stale;
}

bool isControlStateStale(uint32_t maxAgeMs) {
  if (controlStateMutex == nullptr) {
    return true;
  }

  bool stale = true;
  if (xSemaphoreTake(controlStateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
    uint32_t now = millis();
    stale = (now - controlState.lastHeartbeat > maxAgeMs);
    xSemaphoreGive(controlStateMutex);
  }

  return stale;
}

SemaphoreHandle_t getSensorDataMutex() { return sensorDataMutex; }

SemaphoreHandle_t getControlStateMutex() { return controlStateMutex; }

} // namespace SharedData
