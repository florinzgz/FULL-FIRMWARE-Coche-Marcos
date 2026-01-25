// managers/SafetyManager.h
// Safety management - integrates ABS, TCS, and obstacle safety
#pragma once

#include "../../include/abs_system.h"
#include "../../include/obstacle_safety.h"
#include "../../include/tcs_system.h"

namespace SafetyManager {
inline bool init() {
  ABSSystem::init();
  TCSSystem::init();
  ObstacleSafety::init();

  // Verify critical safety systems initialized correctly
  bool absOK = ABSSystem::initOK();
  bool tcsOK = TCSSystem::initOK();

  // ObstacleSafety may not have initOK(), so we only check ABS/TCS
  return absOK && tcsOK;
}

inline void update() {
  ABSSystem::update();
  TCSSystem::update();
  ObstacleSafety::update();
}

// Enhanced functions for FreeRTOS multi-core operation
void updateWithHeartbeat();       // Update with heartbeat failsafe monitoring
bool isHeartbeatFailsafeActive(); // Check if heartbeat failsafe is active

} // namespace SafetyManager
