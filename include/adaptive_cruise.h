#ifndef ADAPTIVE_CRUISE_H
#define ADAPTIVE_CRUISE_H

#include "obstacle_detection.h"
#include <Arduino.h>

// ============================================================================
// Adaptive Cruise Control System
// ============================================================================

namespace AdaptiveCruise {

// ACC States
enum ACCState : uint8_t {
  ACC_DISABLED = 0,
  ACC_STANDBY = 1,
  ACC_ACTIVE = 2,
  ACC_BRAKING = 3,
  ACC_ERROR = 255
};

// ACC Configuration
struct ACCConfig {
  bool enabled;
  uint16_t targetDistanceMm; // Target following distance (default 1500mm)
  uint16_t minDistanceMm;    // Minimum safe distance (default 500mm)
  uint8_t maxSpeedReduction; // Max speed reduction % (default 50)
  float pidKp;               // PID proportional gain
  float pidKi;               // PID integral gain
  float pidKd;               // PID derivative gain

  ACCConfig()
      : enabled(false), targetDistanceMm(1500), minDistanceMm(500),
        maxSpeedReduction(50), pidKp(0.5f), pidKi(0.1f), pidKd(0.05f) {}
};

// ACC Status
struct ACCStatus {
  ACCState state;
  uint16_t currentDistance;
  float speedAdjustment; // Speed adjustment factor (0.0-1.0)
  bool vehicleDetected;
  uint32_t lastUpdateMs;

  ACCStatus()
      : state(ACC_DISABLED), currentDistance(0xFFFF), speedAdjustment(1.0f),
        vehicleDetected(false), lastUpdateMs(0) {}
};

/**
 * Initialize adaptive cruise control system
 */
void init();

/**
 * Update ACC (call in main loop at ~10Hz)
 */
void update();

/**
 * Enable/disable ACC
 */
void setEnabled(bool enable);

/**
 * Get current ACC status
 */
const ACCStatus &getStatus();

/**
 * Get current configuration
 */
const ACCConfig &getConfig();

/**
 * Update configuration
 */
void setConfig(const ACCConfig &config);

/**
 * Get speed adjustment factor for traction control
 * @return Multiplier 0.0-1.0 to apply to throttle demand
 */
float getSpeedAdjustment();

/**
 * Reset ACC state (call on manual brake input)
 */
void reset();
} // namespace AdaptiveCruise

#endif // ADAPTIVE_CRUISE_H
