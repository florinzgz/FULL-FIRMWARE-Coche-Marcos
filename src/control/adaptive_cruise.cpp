// Adaptive Cruise Control Implementation
// PID-based speed regulation for obstacle detection system
// Author: Copilot AI Assistant
// Date: 2025-11-24

#include "adaptive_cruise.h"
#include "obstacle_detection.h"
#include "logger.h"
#include "system.h"

namespace AdaptiveCruise {

static ACCConfig config;
static ACCStatus status;
static float pidIntegral = 0.0f;
static float pidLastError = 0.0f;
static uint32_t lastUpdateMs = 0;

constexpr uint32_t UPDATE_INTERVAL_MS = 100;  // 10Hz

void init() {
    Logger::info("ACC: Initializing adaptive cruise control");
    config = ACCConfig();  // Load defaults
    status.state = config.enabled ? ACC_STANDBY : ACC_DISABLED;
    pidIntegral = 0.0f;
    pidLastError = 0.0f;
    lastUpdateMs = millis();
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
    
    // Get front sensor distance
    uint16_t frontDist = ObstacleDetection::getMinDistance(ObstacleDetection::SENSOR_FRONT);
    status.currentDistance = frontDist;
    
    // Check if vehicle detected
    if (frontDist == ObstacleConfig::DISTANCE_INVALID || frontDist > 4000) {
        // No vehicle detected - full speed
        status.vehicleDetected = false;
        status.state = ACC_STANDBY;
        status.speedAdjustment = 1.0f;
        pidIntegral = 0.0f;
        pidLastError = 0.0f;
        return;
    }
    
    status.vehicleDetected = true;
    status.state = ACC_ACTIVE;
    
    // PID controller for speed adjustment
    float error = (float)(frontDist - config.targetDistanceMm);
    pidIntegral += error * (UPDATE_INTERVAL_MS / 1000.0f);
    float pidDerivative = (error - pidLastError) / (UPDATE_INTERVAL_MS / 1000.0f);
    pidLastError = error;
    
    // Calculate PID output
    float pidOutput = (config.pidKp * error) + 
                     (config.pidKi * pidIntegral) + 
                     (config.pidKd * pidDerivative);
    
    // Convert to speed adjustment (0.0 = stop, 1.0 = full speed)
    float adjustment = 1.0f + (pidOutput / 1000.0f);
    
    // Clamp adjustment
    if (frontDist < config.minDistanceMm) {
        // Emergency braking zone
        status.state = ACC_BRAKING;
        adjustment = 0.0f;
    } else {
        // Normal regulation
        float minAdjust = 1.0f - (config.maxSpeedReduction / 100.0f);
        adjustment = constrain(adjustment, minAdjust, 1.0f);
    }
    
    status.speedAdjustment = adjustment;
    status.lastUpdateMs = now;
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

const ACCStatus& getStatus() {
    return status;
}

const ACCConfig& getConfig() {
    return config;
}

void setConfig(const ACCConfig& newConfig) {
    config = newConfig;
    status.state = config.enabled ? ACC_STANDBY : ACC_DISABLED;
    Logger::infof("ACC: Config updated - target=%dmm", config.targetDistanceMm);
}

float getSpeedAdjustment() {
    return status.speedAdjustment;
}

void reset() {
    pidIntegral = 0.0f;
    pidLastError = 0.0f;
    status.state = config.enabled ? ACC_STANDBY : ACC_DISABLED;
    status.speedAdjustment = 1.0f;
}

} // namespace AdaptiveCruise
