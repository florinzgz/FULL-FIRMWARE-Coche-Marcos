// shared_data.h - Thread-safe shared data structures for multi-core operation
// Provides synchronized access to sensor data between cores
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

namespace SharedData {

// Sensor data structure with timestamps for staleness detection
struct SensorData {
  // Current sensors (INA226)
  float current[6];  // 6 current sensors
  float voltage[6];  // 6 voltage sensors
  float power[6];    // 6 power calculations
  bool currentOk[6]; // Sensor validity flags
  uint32_t currentTimestamp;

  // Temperature sensors (DS18B20)
  float temperature[4]; // 4 temperature sensors
  bool tempOk[4];       // Sensor validity flags
  uint32_t tempTimestamp;

  // Wheel sensors
  float wheelSpeed[4]; // 4 wheel speeds
  bool wheelOk[4];     // Sensor validity flags
  uint32_t wheelTimestamp;

  // Input devices
  float pedalValue;
  float steeringAngle;
  uint8_t shifterPosition;
  uint8_t buttonStates;
  uint32_t inputTimestamp;

  // System status
  bool i2cBusOk;
  uint8_t i2cErrorCount;
  uint32_t lastI2cError;
};

// Control state shared from ControlManager to SafetyManager
struct ControlState {
  bool motorsActive;
  float targetSpeed;
  float targetSteering;
  uint32_t lastHeartbeat; // Timestamp of last control update
};

// Initialize shared data system
bool init();

// Thread-safe accessors for sensor data
bool readSensorData(SensorData &data);
bool writeSensorData(const SensorData &data);

// Thread-safe accessors for control state
bool readControlState(ControlState &state);
bool writeControlState(const ControlState &state);

// Check data staleness
bool isSensorDataStale(uint32_t maxAgeMs = 200);
bool isControlStateStale(uint32_t maxAgeMs = 200);

// Mutex access for advanced operations
SemaphoreHandle_t getSensorDataMutex();
SemaphoreHandle_t getControlStateMutex();

} // namespace SharedData
