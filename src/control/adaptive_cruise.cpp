// Adaptive Cruise Control Implementation v2.12.0
// PID-based speed regulation with person-following mode
// Author: Copilot AI Assistant
// Date: 2025-12-23

#include "adaptive_cruise.h"
#include "logger.h"
#include "obstacle_detection.h"
#include "pedal.h"
#include "system.h"

namespace AdaptiveCruise {

static ACCConfig config;
static ACCStatus status;
static float pidIntegral = 0.0f;
static float pidLastError = 0.0f;
static uint32_t lastUpdateMs = 0;
static uint32_t targetLostMs = 0;      // v2.12.0: Target lost timeout
static float childPedalLimit = 100.0f; // v2.12.0: Child pedal safety limit
static uint32_t lastDebugLogMs = 0;    // v2.12.0: Throttle debug logging

constexpr uint32_t UPDATE_INTERVAL_MS = 100;          // 10Hz
constexpr uint32_t TARGET_LOST_TIMEOUT_MS = 2000;     // 2 seconds
constexpr uint16_t EMERGENCY_BRAKE_DISTANCE_MM = 300; // 30cm

void init() {
  Logger::info("ACC v2.12.0: Initializing adaptive cruise control");
  config = ACCConfig(); // Load defaults
  // v2.12.0: Update PID tuning parameters
  config.pidKp = 0.3f;
  config.pidKi = 0.05f;
  config.pidKd = 0.15f;
  config.targetDistanceMm = 500; // 50cm target distance
  config.minDistanceMm = 300;    // 30cm emergency brake

  status.state = config.enabled ? ACC_STANDBY : ACC_DISABLED;
  pidIntegral = 0.0f;
  pidLastError = 0.0f;
  lastUpdateMs = millis();
  targetLostMs = 0;
  childPedalLimit = 100.0f;
}

void update() {
  uint32_t now = millis();
  if (now - lastUpdateMs < UPDATE_INTERVAL_MS) return;
  lastUpdateMs = now;

  if (!config.enabled) {
    status.state = ACC_DISABLED;
    status.speedAdjustment = 1.0f;
    return;
  }

  // v2.12.0: Get child pedal input as safety limit
  auto pedalState = Pedal::get();
  if (pedalState.valid) { childPedalLimit = pedalState.percent; }

  // Get front sensor distance
  uint16_t frontDist =
      ObstacleDetection::getMinDistance(ObstacleDetection::SENSOR_FRONT);
  status.currentDistance = frontDist;

  // Check if vehicle/person detected
  if (frontDist == ObstacleConfig::DISTANCE_INVALID || frontDist > 4000) {
    // No target detected
    if (status.vehicleDetected) {
      // Target just lost - reset PID state immediately
      if (targetLostMs == 0) {
        targetLostMs = now;
        pidIntegral = 0.0f;
        pidLastError = 0.0f;
        Logger::debug("ACC: Target lost, PID reset, starting timeout");
      } else if (now - targetLostMs >= TARGET_LOST_TIMEOUT_MS) {
        // Timeout exceeded - full speed
        status.vehicleDetected = false;
        status.state = ACC_STANDBY;
        status.speedAdjustment = 1.0f;
        targetLostMs = 0;
        Logger::info("ACC: Target lost timeout - returning to standby");
      }
    } else {
      status.state = ACC_STANDBY;
      status.speedAdjustment = 1.0f;
    }
    return;
  }

  // Target detected - reset timeout
  targetLostMs = 0;
  status.vehicleDetected = true;

  // v2.12.0: Emergency brake activation <30cm (check FIRST before PID)
  if (frontDist < EMERGENCY_BRAKE_DISTANCE_MM) {
    status.state = ACC_BRAKING;
    status.speedAdjustment = 0.0f;
    // Reset PID state to avoid stale control when ACC re-engages
    pidIntegral = 0.0f;
    pidLastError = 0.0f;
    Logger::warnf("ACC: Emergency brake! Distance=%dmm", frontDist);
    return;
  }

  status.state = ACC_ACTIVE;

  // PID controller for person-following mode
  float error = (float)(frontDist - config.targetDistanceMm);
  float dt = UPDATE_INTERVAL_MS / 1000.0f;
  pidIntegral += error * dt;
  float pidDerivative = (error - pidLastError) / dt;
  pidLastError = error;

  // Anti-windup: limit integral term
  pidIntegral = constrain(pidIntegral, -500.0f, 500.0f);

  // Calculate PID output
  float pidOutput = (config.pidKp * error) + (config.pidKi * pidIntegral) +
                    (config.pidKd * pidDerivative);

  // Convert to speed adjustment (0.0 = stop, 1.0 = full speed)
  float adjustment = 1.0f + (pidOutput / 1000.0f);

  // Normal regulation - clamp adjustment
  float minAdjust = 1.0f - (config.maxSpeedReduction / 100.0f);
  adjustment = constrain(adjustment, minAdjust, 1.0f);

  // v2.12.0: Respect child pedal input as safety limit
  // Never exceed what the child is demanding
  float pedalFactor = childPedalLimit / 100.0f;
  adjustment = min(adjustment, pedalFactor);

  status.speedAdjustment = adjustment;
  status.lastUpdateMs = now;

  // v2.12.0: Debug logging at 10Hz (throttled to once per second)
  if (now - lastDebugLogMs >= 1000) {
    Logger::debugf("ACC: dist=%dmm, target=%dmm, adj=%.2f, pedal=%.1f%%",
                   frontDist, config.targetDistanceMm, adjustment,
                   childPedalLimit);
    lastDebugLogMs = now;
  }
}

void setEnabled(bool enable) {
  config.enabled = enable;
  if (!enable) {
    status.state = ACC_DISABLED;
    status.speedAdjustment = 1.0f;
    pidIntegral = 0.0f;
    pidLastError = 0.0f;
  } else {
    status.state = ACC_STANDBY;
    Logger::info("ACC: Enabled");
  }
}

const ACCStatus &getStatus() { return status; }

const ACCConfig &getConfig() { return config; }

void setConfig(const ACCConfig &newConfig) {
  config = newConfig;
  status.state = config.enabled ? ACC_STANDBY : ACC_DISABLED;
  Logger::infof("ACC: Config updated - target=%dmm", config.targetDistanceMm);
}

float getSpeedAdjustment() { return status.speedAdjustment; }

void reset() {
  pidIntegral = 0.0f;
  pidLastError = 0.0f;
  status.state = config.enabled ? ACC_STANDBY : ACC_DISABLED;
  status.speedAdjustment = 1.0f;
}

} // namespace AdaptiveCruise
