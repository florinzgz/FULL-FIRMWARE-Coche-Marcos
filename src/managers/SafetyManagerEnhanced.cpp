// SafetyManagerEnhanced.cpp - Enhanced safety manager with heartbeat failsafe
#include "logger.h"
#include "managers/SafetyManager.h"
#include "shared_data.h"
#include "steering_motor.h"
#include "traction.h"

namespace SafetyManager {

// Heartbeat monitoring configuration
constexpr uint32_t HEARTBEAT_TIMEOUT_MS = 200; // 200ms timeout as specified
static bool heartbeatFailsafeActive = false;
static uint32_t lastHeartbeatWarning = 0;

// Enhanced update function with heartbeat monitoring
void updateWithHeartbeat() {
  // Call original safety systems update
  update();

  // Check control manager heartbeat
  SharedData::ControlState controlState;
  if (SharedData::readControlState(controlState)) {
    uint32_t now = millis();
    uint32_t timeSinceHeartbeat = now - controlState.lastHeartbeat;

    if (timeSinceHeartbeat > HEARTBEAT_TIMEOUT_MS) {
      // Heartbeat timeout detected
      if (!heartbeatFailsafeActive) {
        Logger::errorf("SafetyManager: Heartbeat timeout (%lu ms) - EMERGENCY "
                       "MOTOR STOP",
                       timeSinceHeartbeat);
        heartbeatFailsafeActive = true;

        // Emergency stop: Set all motor demands to 0
        Traction::setDemand(0.0f);
        SteeringMotor::setDemandAngle(0.0f); // Also stop steering motor

        // Log detailed state for diagnostics
        Logger::errorf(
            "SafetyManager: ControlManager not responding - failsafe "
            "activated");
      }

      // Periodic warning (every 5 seconds while in failsafe)
      if (now - lastHeartbeatWarning > 5000) {
        Logger::warn("SafetyManager: Still in heartbeat failsafe mode");
        lastHeartbeatWarning = now;
      }
    } else {
      // Heartbeat OK - clear failsafe if it was active
      if (heartbeatFailsafeActive) {
        Logger::info("SafetyManager: Heartbeat restored - failsafe cleared");
        heartbeatFailsafeActive = false;
      }
    }
  } else {
    // Failed to read control state - enter failsafe
    if (!heartbeatFailsafeActive) {
      Logger::error(
          "SafetyManager: Cannot read control state - EMERGENCY MOTOR STOP");
      heartbeatFailsafeActive = true;
      Traction::setDemand(0.0f);
      SteeringMotor::setDemandAngle(0.0f);
    }
  }
}

bool isHeartbeatFailsafeActive() { return heartbeatFailsafeActive; }

} // namespace SafetyManager
