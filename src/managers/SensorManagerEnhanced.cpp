// SensorManagerEnhanced.cpp - Enhanced sensor manager with non-blocking I2C
#include "managers/SensorManager.h"
#include "current.h"
#include "logger.h"
#include "shared_data.h"
#include "temperature.h"
#include "wheels.h"

namespace SensorManager {

// I2C timeout configuration
constexpr uint32_t I2C_OPERATION_TIMEOUT_MS = 50; // 50ms timeout per I2C operation
static uint32_t lastI2cError = 0;
static uint8_t consecutiveI2cErrors = 0;
constexpr uint8_t MAX_CONSECUTIVE_I2C_ERRORS = 5;

// Non-blocking update with timeout handling
void updateNonBlocking() {
  // Call the standard update
  update();

  // Update shared sensor data
  SharedData::SensorData sensorData;

  // Read current sensors with timeout protection
  uint32_t startTime = millis();
  for (int i = 0; i < Sensors::NUM_CURRENTS && (millis() - startTime) < I2C_OPERATION_TIMEOUT_MS; i++) {
    sensorData.current[i] = Sensors::getCurrent(i);
    sensorData.voltage[i] = Sensors::getVoltage(i);
    sensorData.power[i] = Sensors::getPower(i);
    sensorData.currentOk[i] = Sensors::isCurrentSensorOk(i);
  }
  sensorData.currentTimestamp = millis();

  // Check for I2C timeout
  if ((millis() - startTime) >= I2C_OPERATION_TIMEOUT_MS) {
    consecutiveI2cErrors++;
    if (consecutiveI2cErrors >= MAX_CONSECUTIVE_I2C_ERRORS) {
      if (millis() - lastI2cError > 5000) { // Log every 5 seconds
        Logger::errorf("SensorManager: I2C operations timing out (%d consecutive errors)",
                       consecutiveI2cErrors);
        lastI2cError = millis();
      }
      sensorData.i2cBusOk = false;
      sensorData.i2cErrorCount = consecutiveI2cErrors;
      sensorData.lastI2cError = millis();
    }
  } else {
    consecutiveI2cErrors = 0;
    sensorData.i2cBusOk = true;
  }

  // Read temperature sensors (non-I2C, should be fast)
  for (int i = 0; i < Sensors::NUM_TEMPS; i++) {
    sensorData.temperature[i] = Sensors::getTemperature(i);
    sensorData.tempOk[i] = Sensors::isTemperatureSensorOk(i);
  }
  sensorData.tempTimestamp = millis();

  // Read wheel sensors
  for (int i = 0; i < Sensors::NUM_WHEELS; i++) {
    sensorData.wheelSpeed[i] = Sensors::getWheelSpeed(i);
    sensorData.wheelOk[i] = Sensors::isWheelSensorOk(i);
  }
  sensorData.wheelTimestamp = millis();

  // Update input devices (non-I2C)
  sensorData.pedalValue = Pedal::getValue();
  sensorData.steeringAngle = Steering::getAngle();
  sensorData.shifterPosition = Shifter::getPosition();
  // Note: Buttons are handled separately
  sensorData.inputTimestamp = millis();

  // Write to shared data structure
  if (!SharedData::writeSensorData(sensorData)) {
    Logger::warn("SensorManager: Failed to write sensor data to shared structure");
  }
}

bool isI2cHealthy() {
  return consecutiveI2cErrors < MAX_CONSECUTIVE_I2C_ERRORS;
}

uint8_t getI2cErrorCount() {
  return consecutiveI2cErrors;
}

} // namespace SensorManager
