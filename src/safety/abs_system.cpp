#include "abs_system.h"
#include "alerts.h"
#include "logger.h"
#include "wheels.h"
#include <Arduino.h>
#include <cmath> // 🔒 For std::isfinite validation

namespace ABSSystem {

// Default configuration
static Config config = {
    .enabled = true,
    .slipThreshold = 15.0f,   // Trigger ABS at 15% slip
    .minSpeedKmh = 10.0f,     // Only activate above 10 km/h
    .cycleMs = 100,           // 10 Hz ABS cycles
    .pressureReduction = 0.3f // Reduce brake pressure by 30%
};

// Current state
static State state = {0};

// Internal state
static bool initialized = false;
static float vehicleSpeedKmh = 0.0f;
static uint32_t lastUpdateMs = 0;

// Calculate vehicle reference speed (average of non-locked wheels)
static float calculateVehicleSpeed() {
  float sum = 0.0f;
  int count = 0;

  for (int i = 0; i < 4; i++) {
    float wheelSpeed = Sensors::getWheelSpeed(i);
    // 🔒 SECURITY FIX: Validate wheel speed before using
    if (!std::isfinite(wheelSpeed) || wheelSpeed < 0.0f) {
      Logger::warnf("ABS: Invalid wheel speed %.2f on wheel %d", wheelSpeed, i);
      continue;
    }
    if (Sensors::isWheelSensorOk(i) && wheelSpeed > 0.1f) {
      // Only count wheels that are moving
      if (!state.wheels[i].active || state.wheels[i].slipRatio < 50.0f) {
        sum += wheelSpeed;
        count++;
      }
    }
  }

  return (count > 0) ? (sum / count) : 0.0f;
}

// Calculate slip ratio for a wheel
// Slip ratio = (vehicle_speed - wheel_speed) / vehicle_speed * 100
static float calculateSlipRatio(int wheel, float vehSpeed) {
  // 🔒 SECURITY FIX: Validate inputs before calculation
  if (!std::isfinite(vehSpeed) || vehSpeed < 0.1f) return 0.0f;
  if (wheel < 0 || wheel >= 4) return 0.0f;

  float wheelSpeed = Sensors::getWheelSpeed(wheel);
  if (!Sensors::isWheelSensorOk(wheel)) return 0.0f;

  // 🔒 SECURITY FIX: Validate wheel speed
  if (!std::isfinite(wheelSpeed) || wheelSpeed < 0.0f) {
    Logger::warnf("ABS: Invalid wheel speed %.2f on wheel %d for slip calc",
                  wheelSpeed, wheel);
    return 0.0f;
  }

  float slip = ((vehSpeed - wheelSpeed) / vehSpeed) * 100.0f;
  return constrain(slip, -100.0f, 100.0f);
}

void init() {
  Logger::info("Initializing ABS system...");

  memset(&state, 0, sizeof(State));

  for (int i = 0; i < 4; i++) {
    state.wheels[i].active = false;
    state.wheels[i].slipRatio = 0.0f;
    state.wheels[i].lastCycleMs = 0;
    state.wheels[i].pulsesApplied = 0;
  }

  state.systemActive = false;
  state.activationCount = 0;
  state.avgSlipRatio = 0.0f;

  initialized = true;
  Logger::info("ABS system initialized");
}

void update() {
  if (!initialized || !config.enabled) {
    state.systemActive = false;
    return;
  }

  uint32_t now = millis();

  // Update vehicle reference speed
  vehicleSpeedKmh = calculateVehicleSpeed();

  // Don't activate ABS below minimum speed
  if (vehicleSpeedKmh < config.minSpeedKmh) {
    state.systemActive = false;
    for (int i = 0; i < 4; i++) {
      state.wheels[i].active = false;
    }
    return;
  }

  // Check each wheel for excessive slip
  bool anyActive = false;
  float totalSlip = 0.0f;
  int activeCount = 0;

  for (int i = 0; i < 4; i++) {
    if (!Sensors::isWheelSensorOk(i)) {
      state.wheels[i].active = false;
      continue;
    }

    // Calculate current slip ratio
    float slip = calculateSlipRatio(i, vehicleSpeedKmh);
    state.wheels[i].slipRatio = slip;

    // Check if slip exceeds threshold
    if (slip > config.slipThreshold) {
      if (!state.wheels[i].active) {
        // ABS activation on this wheel
        state.wheels[i].active = true;
        state.wheels[i].lastCycleMs = now;
        state.wheels[i].pulsesApplied = 0;
        state.activationCount++;

        Logger::infof("ABS activated on wheel %d (slip: %.1f%%)", i, slip);
      }

      anyActive = true;
      activeCount++;
      totalSlip += slip;
    } else if (slip < config.slipThreshold * 0.7f) {
      // Deactivate ABS when slip drops below 70% of threshold
      if (state.wheels[i].active) {
        Logger::infof("ABS deactivated on wheel %d", i);
      }
      state.wheels[i].active = false;
    }

    // Apply ABS cycling if active
    if (state.wheels[i].active) {
      if (now - state.wheels[i].lastCycleMs >= config.cycleMs) {
        state.wheels[i].lastCycleMs = now;
        state.wheels[i].pulsesApplied++;
      }
    }
  }

  // Update system state
  bool wasActive = state.systemActive;
  state.systemActive = anyActive;
  state.avgSlipRatio = (activeCount > 0) ? (totalSlip / activeCount) : 0.0f;

  // Audio feedback on activation/deactivation
  if (state.systemActive && !wasActive) {
    Alerts::play(Audio::AUDIO_ERROR_GENERAL);
  }

  lastUpdateMs = now;
}

void setEnabled(bool enabled) {
  config.enabled = enabled;
  if (!enabled) {
    state.systemActive = false;
    for (int i = 0; i < 4; i++) {
      state.wheels[i].active = false;
    }
  }
  Logger::infof("ABS %s", enabled ? "enabled" : "disabled");
}

const State &getState() { return state; }

bool isActive() { return state.systemActive; }

Config &getConfig() { return config; }

float modulateBrake(int wheel, float brakeDemand) {
  if (!initialized || !config.enabled || wheel < 0 || wheel >= 4) {
    return brakeDemand;
  }

  if (!state.wheels[wheel].active) { return brakeDemand; }

  // Apply pressure reduction during ABS pulse
  uint32_t now = millis();
  uint32_t cyclePhase =
      (now - state.wheels[wheel].lastCycleMs) % config.cycleMs;

  // Pulse pattern: reduce for first 50% of cycle, allow for second 50%
  if (cyclePhase < (config.cycleMs / 2)) {
    return brakeDemand * (1.0f - config.pressureReduction);
  }

  return brakeDemand;
}
} // namespace ABSSystem

// 🔒 v2.4.0: Estado de inicialización
bool ABSSystem::initOK() { return initialized; }
