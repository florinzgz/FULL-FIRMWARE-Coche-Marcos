#include "limp_mode.h"
#include "current.h"
#include "logger.h"
#include "pedal.h"
#include "steering.h"
#include "system.h"
#include "temperature.h"
#include <Arduino.h>

namespace LimpMode {

// ==========================================
// State machine
// ==========================================
static LimpState currentState = LimpState::NORMAL;
static LimpState forcedState = LimpState::NORMAL;
static bool stateForced = false;

// ==========================================
// Timing
// ==========================================
static uint32_t timeEnteredState = 0;
static uint32_t lastTransition = 0;

// ==========================================
// Safety thresholds
// ==========================================
namespace Thresholds {
constexpr float BATTERY_UNDERVOLTAGE = 20.0f; // 24V system, critical at 20V
constexpr float BATTERY_CRITICAL = 18.0f;     // Below this = CRITICAL
constexpr float TEMP_WARNING = 80.0f;         // Motor temp warning (°C)
constexpr float TEMP_CRITICAL = 90.0f;        // Motor temp critical (°C)
constexpr uint8_t ERROR_COUNT_DEGRADED = 1;   // 1+ errors = DEGRADED
constexpr uint8_t ERROR_COUNT_LIMP = 3;       // 3+ errors = LIMP
constexpr uint8_t ERROR_COUNT_CRITICAL = 5;   // 5+ errors = CRITICAL
} // namespace Thresholds

// ==========================================
// Power limits per state
// ==========================================
namespace Limits {
// Power limits (multiplier for pedal output)
constexpr float POWER_NORMAL = 1.0f;    // 100% power
constexpr float POWER_DEGRADED = 0.7f;  // 70% power
constexpr float POWER_LIMP = 0.4f;      // 40% power (drive-home mode)
constexpr float POWER_CRITICAL = 0.15f; // 15% power (minimal operation)

// Steering assist limits
constexpr float STEERING_NORMAL = 1.0f;
constexpr float STEERING_DEGRADED = 0.8f;
constexpr float STEERING_LIMP = 0.5f;
constexpr float STEERING_CRITICAL = 0.3f;

// Speed limits (fraction of normal max speed)
constexpr float SPEED_NORMAL = 1.0f;    // No limit
constexpr float SPEED_DEGRADED = 0.8f;  // 80% max speed
constexpr float SPEED_LIMP = 0.5f;      // 50% max speed
constexpr float SPEED_CRITICAL = 0.25f; // 25% max speed
} // namespace Limits

// ==========================================
// State transition hysteresis (ms)
// ==========================================
constexpr uint32_t STATE_HYSTERESIS_MS = 500; // Prevent rapid state changes

// ==========================================
// Cached sensor readings
// ==========================================
struct SensorCache {
  bool pedalValid;
  bool steeringValid;
  float pedalPercent;
  float steeringAngle;
  uint8_t systemErrorCount;
  float batteryVoltage;
  bool temperatureWarning;
  bool temperatureCritical;
  bool steeringCentered;
  uint32_t lastUpdateMs;
};

static SensorCache cache;

// ==========================================
// Forward declarations
// ==========================================
static void transitionToState(LimpState newState);
static LimpState evaluateConditions();
static void updateSensorCache();
static float getPowerLimit(LimpState state);
static float getSteeringLimit(LimpState state);
static float getSpeedLimit(LimpState state);

// ==========================================
// Initialization
// ==========================================
void init() {
  currentState = LimpState::NORMAL;
  stateForced = false;
  timeEnteredState = millis();
  lastTransition = millis();

  memset(&cache, 0, sizeof(cache));

  Logger::info("[LimpMode] Initialized in NORMAL state");
}

// ==========================================
// Main update loop
// ==========================================
void update() {
  uint32_t now = millis();

  // Update sensor readings
  updateSensorCache();

  // If state is forced (for testing), skip evaluation
  if (stateForced) {
    currentState = forcedState;
    return;
  }

  // Evaluate conditions to determine required state
  LimpState requiredState = evaluateConditions();

  // Apply hysteresis to prevent rapid transitions
  if (requiredState != currentState) {
    uint32_t timeSinceTransition = now - lastTransition;
    if (timeSinceTransition >= STATE_HYSTERESIS_MS) {
      transitionToState(requiredState);
    }
  }
}

// ==========================================
// State evaluation logic
// ==========================================
static LimpState evaluateConditions() {
  // Check for CRITICAL conditions first (highest priority)

  // Both pedal AND steering invalid = CRITICAL
  if (!cache.pedalValid && !cache.steeringValid) { return LimpState::CRITICAL; }

  // Battery critically low = CRITICAL
  if (cache.batteryVoltage > 0.0f &&
      cache.batteryVoltage < Thresholds::BATTERY_CRITICAL) {
    return LimpState::CRITICAL;
  }

  // Temperature critical = CRITICAL
  if (cache.temperatureCritical) { return LimpState::CRITICAL; }

  // Too many errors = CRITICAL
  if (cache.systemErrorCount >= Thresholds::ERROR_COUNT_CRITICAL) {
    return LimpState::CRITICAL;
  }

  // Check for LIMP conditions (medium priority)

  // Moderate error count = LIMP
  if (cache.systemErrorCount >= Thresholds::ERROR_COUNT_LIMP) {
    return LimpState::LIMP;
  }

  // Battery undervoltage = LIMP
  if (cache.batteryVoltage > 0.0f &&
      cache.batteryVoltage < Thresholds::BATTERY_UNDERVOLTAGE) {
    return LimpState::LIMP;
  }

  // Steering not centered (encoder not initialized) = LIMP
  if (!cache.steeringCentered) { return LimpState::LIMP; }

  // Either pedal OR steering invalid = LIMP
  if (!cache.pedalValid || !cache.steeringValid) { return LimpState::LIMP; }

  // Check for DEGRADED conditions (low priority)

  // Any errors present = DEGRADED
  if (cache.systemErrorCount >= Thresholds::ERROR_COUNT_DEGRADED) {
    return LimpState::DEGRADED;
  }

  // Temperature warning = DEGRADED
  if (cache.temperatureWarning) { return LimpState::DEGRADED; }

  // All conditions OK = NORMAL
  return LimpState::NORMAL;
}

// ==========================================
// State transition
// ==========================================
static void transitionToState(LimpState newState) {
  if (newState == currentState) { return; }

  // Log state change
  const char *stateNames[] = {"NORMAL", "DEGRADED", "LIMP", "CRITICAL"};
  Logger::warnf("[LimpMode] State transition: %s -> %s",
                stateNames[(int)currentState], stateNames[(int)newState]);

  currentState = newState;
  timeEnteredState = millis();
  lastTransition = millis();
}

// ==========================================
// Sensor cache update
// ==========================================
static void updateSensorCache() {
  cache.lastUpdateMs = millis();

  // Pedal state
  const Pedal::State &pedalState = Pedal::get();
  cache.pedalValid = pedalState.valid;
  cache.pedalPercent = pedalState.percent;

  // Steering state
  const Steering::State &steeringState = Steering::get();
  cache.steeringValid = steeringState.valid;
  cache.steeringAngle = steeringState.angleDeg;
  cache.steeringCentered = steeringState.centered;

  // System error count
  cache.systemErrorCount = (uint8_t)System::getErrorCount();

  // Battery voltage (from INA226 channel 4 - battery monitor)
  // Channel 4 is typically the battery monitor channel
  cache.batteryVoltage = Sensors::getVoltage(4);

  // Temperature warnings
  // Check all motor temperatures (indices 0-3)
  cache.temperatureWarning = false;
  cache.temperatureCritical = false;

  for (int i = 0; i < 4; i++) { // Check 4 motor temps
    float temp = Sensors::getTemperature(i);
    if (temp > Thresholds::TEMP_WARNING) { cache.temperatureWarning = true; }
    if (temp > Thresholds::TEMP_CRITICAL) { cache.temperatureCritical = true; }
  }
}

// ==========================================
// Limit getters
// ==========================================
static float getPowerLimit(LimpState state) {
  switch (state) {
  case LimpState::NORMAL:
    return Limits::POWER_NORMAL;
  case LimpState::DEGRADED:
    return Limits::POWER_DEGRADED;
  case LimpState::LIMP:
    return Limits::POWER_LIMP;
  case LimpState::CRITICAL:
    return Limits::POWER_CRITICAL;
  default:
    return Limits::POWER_CRITICAL;
  }
}

static float getSteeringLimit(LimpState state) {
  switch (state) {
  case LimpState::NORMAL:
    return Limits::STEERING_NORMAL;
  case LimpState::DEGRADED:
    return Limits::STEERING_DEGRADED;
  case LimpState::LIMP:
    return Limits::STEERING_LIMP;
  case LimpState::CRITICAL:
    return Limits::STEERING_CRITICAL;
  default:
    return Limits::STEERING_CRITICAL;
  }
}

static float getSpeedLimit(LimpState state) {
  switch (state) {
  case LimpState::NORMAL:
    return Limits::SPEED_NORMAL;
  case LimpState::DEGRADED:
    return Limits::SPEED_DEGRADED;
  case LimpState::LIMP:
    return Limits::SPEED_LIMP;
  case LimpState::CRITICAL:
    return Limits::SPEED_CRITICAL;
  default:
    return Limits::SPEED_CRITICAL;
  }
}

// ==========================================
// Public API
// ==========================================

LimpState getState() { return currentState; }

Diagnostics getDiagnostics() {
  Diagnostics diag;
  diag.state = currentState;

  // Input validity
  diag.pedalValid = cache.pedalValid;
  diag.steeringValid = cache.steeringValid;

  // System health
  diag.systemErrorCount = cache.systemErrorCount;
  diag.batteryUndervoltage =
      (cache.batteryVoltage > 0.0f &&
       cache.batteryVoltage < Thresholds::BATTERY_UNDERVOLTAGE);
  diag.temperatureWarning = cache.temperatureWarning;
  diag.steeringCentered = cache.steeringCentered;

  // Applied limits
  diag.powerLimit = getPowerLimit(currentState);
  diag.steeringLimit = getSteeringLimit(currentState);
  diag.maxSpeedLimit = getSpeedLimit(currentState);

  // Timing
  diag.timeInStateMs = millis() - timeEnteredState;
  diag.lastTransitionMs = lastTransition;

  return diag;
}

float limitPower(float pedalPercent) {
  // Clamp input to valid range
  if (pedalPercent < 0.0f) pedalPercent = 0.0f;
  if (pedalPercent > 100.0f) pedalPercent = 100.0f;

  // Apply limit
  float limit = getPowerLimit(currentState);
  return pedalPercent * limit;
}

float limitSteering(float assistLevel) {
  // Clamp input to valid range
  if (assistLevel < 0.0f) assistLevel = 0.0f;
  if (assistLevel > 1.0f) assistLevel = 1.0f;

  // Apply limit
  float limit = getSteeringLimit(currentState);
  return assistLevel * limit;
}

float limitSpeed(float speedKmh) {
  // Clamp input to valid range
  if (speedKmh < 0.0f) speedKmh = 0.0f;

  // Apply limit
  float limit = getSpeedLimit(currentState);
  return speedKmh * limit;
}

bool isSafeToOperate() {
  return currentState == LimpState::NORMAL ||
         currentState == LimpState::DEGRADED;
}

bool isFullPowerAllowed() { return currentState == LimpState::NORMAL; }

void forceState(LimpState state) {
  forcedState = state;
  stateForced = true;
  transitionToState(state);
  Logger::infof("[LimpMode] State forced to %d for testing", (int)state);
}

void reset() {
  stateForced = false;
  currentState = LimpState::NORMAL;
  timeEnteredState = millis();
  lastTransition = millis();
  Logger::info("[LimpMode] Reset to NORMAL state");
}

} // namespace LimpMode
