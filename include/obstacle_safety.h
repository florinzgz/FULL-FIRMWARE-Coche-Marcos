#ifndef OBSTACLE_SAFETY_H
#define OBSTACLE_SAFETY_H

#include <Arduino.h>

namespace ObstacleSafety {

// Safety configuration
struct SafetyConfig {
  bool parkingAssistEnabled;
  bool collisionAvoidanceEnabled;
  bool blindSpotEnabled;
  bool adaptiveCruiseEnabled;

  uint16_t parkingBrakeDistanceMm;    // Distance to trigger parking assist
                                      // (default: 500mm)
  uint16_t collisionCutoffDistanceMm; // Distance to trigger emergency stop
                                      // (default: 200mm)
  uint16_t
      blindSpotDistanceMm; // Lateral distance for blind spot (default: 1000mm)
  uint16_t
      cruiseFollowDistanceMm; // Following distance for ACC (default: 2000mm)

  float maxBrakeForce;  // Maximum brake force (0.0-1.0)
  float minCruiseSpeed; // Minimum speed for ACC (km/h)
};

// Safety system state
struct SafetyState {
  bool parkingAssistActive;
  bool collisionImminent;
  bool blindSpotLeft;  // Reserved: no lateral sensors, always false
  bool blindSpotRight; // Reserved: no lateral sensors, always false
  bool adaptiveCruiseActive;
  bool emergencyBrakeApplied;

  float speedReductionFactor;         // Speed reduction multiplier (0.0-1.0)
  uint16_t closestObstacleDistanceMm; // Distance to closest obstacle
  uint8_t closestObstacleSensor;      // Sensor with closest detection (0-3)

  // v2.12.0: Child reaction detection
  bool childReactionDetected;    // Child reduced pedal within 500ms
  uint32_t lastPedalReductionMs; // Timestamp of last pedal reduction
  float lastPedalValue;          // Previous pedal value for comparison

  // v2.12.0: ACC coordination
  bool accHasPriority;  // ACC system has priority (zones 2-3)
  uint8_t obstacleZone; // Current obstacle zone (1-5)
};

// Initialization
void init();

// Main update loop - call every iteration
void update();

// Configuration
void setConfig(const SafetyConfig &cfg);
void getConfig(SafetyConfig &cfg);

// State access
void getState(SafetyState &st);
bool isParkingAssistActive();
bool isCollisionImminent();
bool isBlindSpotActive();
float getSpeedReductionFactor();

// Manual overrides
void enableParkingAssist(bool enable);
void enableCollisionAvoidance(bool enable);
void enableBlindSpot(bool enable);
void enableAdaptiveCruise(bool enable);

// Emergency functions
void triggerEmergencyStop();
void resetEmergencyStop();

} // namespace ObstacleSafety

#endif // OBSTACLE_SAFETY_H
