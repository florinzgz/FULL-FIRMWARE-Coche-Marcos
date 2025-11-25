// Obstacle-based Safety Systems
// Parking assist, collision avoidance, blind spot warning, adaptive cruise control
// Author: Copilot AI Assistant
// Date: 2025-11-23
// Note: Placeholder implementation - VL53L5X sensors not yet integrated

#include "obstacle_safety.h"
#include "obstacle_detection.h"
#include "obstacle_config.h"
#include "traction.h"
#include "logger.h"
#include "alerts.h"
#include <cmath>

namespace ObstacleSafety {

// Safety state
static SafetyConfig config;
static SafetyState state;
static uint32_t lastUpdateMs = 0;
static uint32_t lastAlertMs[4] = {0};  // Per-sensor alert throttling

// Alert throttling
static constexpr uint32_t ALERT_INTERVAL_MS = 1000;  // 1 second between same alerts

void init() {
    Logger::info("Initializing obstacle safety systems...");
    
    // Default configuration
    config.parkingAssistEnabled = true;
    config.collisionAvoidanceEnabled = true;
    config.blindSpotEnabled = true;
    config.adaptiveCruiseEnabled = false;  // Disabled by default, enable in menu
    
    config.parkingBrakeDistanceMm = 500;  // 50cm
    config.collisionCutoffDistanceMm = 200;  // 20cm
    config.blindSpotDistanceMm = 1000;  // 1m laterally
    config.cruiseFollowDistanceMm = 2000;  // 2m for ACC
    
    config.maxBrakeForce = 0.8f;  // 80% max brake
    config.minCruiseSpeed = 10.0f;  // 10 km/h minimum for ACC
    
    // Initialize state
    state.parkingAssistActive = false;
    state.collisionImminent = false;
    state.blindSpotLeft = false;
    state.blindSpotRight = false;
    state.adaptiveCruiseActive = false;
    state.emergencyBrakeApplied = false;
    state.speedReductionFactor = 1.0f;
    state.closestObstacleDistanceMm = ::ObstacleConfig::DISTANCE_MAX;
    state.closestObstacleSensor = 0xFF;
    
    Logger::info("Obstacle safety systems initialized (placeholder mode)");
}

void update() {
    uint32_t now = millis();
    if (now - lastUpdateMs < 50) return;  // 20Hz update
    lastUpdateMs = now;
    
    // Get obstacle status
    ObstacleDetection::ObstacleStatus obstStatus;
    ObstacleDetection::getStatus(obstStatus);
    
    // Reset state
    state.parkingAssistActive = false;
    state.collisionImminent = false;
    state.blindSpotLeft = false;
    state.blindSpotRight = false;
    state.emergencyBrakeApplied = false;
    state.speedReductionFactor = 1.0f;
    state.closestObstacleDistanceMm = ::ObstacleConfig::DISTANCE_MAX;
    state.closestObstacleSensor = 0xFF;
    
    // Check if obstacle detection system is healthy
    if (obstStatus.sensorsHealthy == 0) return;
    
    // Get distances
    uint16_t frontDist = obstStatus.minDistanceFront;
    uint16_t rearDist = obstStatus.minDistanceRear;
    uint16_t leftDist = obstStatus.minDistanceLeft;
    uint16_t rightDist = obstStatus.minDistanceRight;
    
    // Ignore invalid distances
    if (frontDist >= ::ObstacleConfig::DISTANCE_INVALID) frontDist = ::ObstacleConfig::DISTANCE_MAX;
    if (rearDist >= ::ObstacleConfig::DISTANCE_INVALID) rearDist = ::ObstacleConfig::DISTANCE_MAX;
    if (leftDist >= ::ObstacleConfig::DISTANCE_INVALID) leftDist = ::ObstacleConfig::DISTANCE_MAX;
    if (rightDist >= ::ObstacleConfig::DISTANCE_INVALID) rightDist = ::ObstacleConfig::DISTANCE_MAX;
    
    // Find closest obstacle
    uint16_t minDist = ::ObstacleConfig::DISTANCE_MAX;
    if (frontDist < minDist) { minDist = frontDist; state.closestObstacleSensor = 0; }
    if (rearDist < minDist) { minDist = rearDist; state.closestObstacleSensor = 1; }
    if (leftDist < minDist) { minDist = leftDist; state.closestObstacleSensor = 2; }
    if (rightDist < minDist) { minDist = rightDist; state.closestObstacleSensor = 3; }
    state.closestObstacleDistanceMm = minDist;
    
    // 1. COLLISION AVOIDANCE (highest priority)
    if (config.collisionAvoidanceEnabled) {
        if (frontDist < config.collisionCutoffDistanceMm) {
            state.collisionImminent = true;
            state.emergencyBrakeApplied = true;
            
            // Alert using existing audio system
            if (now - lastAlertMs[0] > ALERT_INTERVAL_MS) {
                Alerts::play(Audio::AUDIO_EMERGENCIA);
                Logger::warn("COLLISION IMMINENT - Emergency brake!");
                lastAlertMs[0] = now;
            }
        }
        
        if (rearDist < config.collisionCutoffDistanceMm) {
            state.collisionImminent = true;
            state.emergencyBrakeApplied = true;
            
            if (now - lastAlertMs[1] > ALERT_INTERVAL_MS) {
                Alerts::play(Audio::AUDIO_EMERGENCIA);
                Logger::warn("REAR COLLISION - Emergency brake!");
                lastAlertMs[1] = now;
            }
        }
    }
    
    // 2. PARKING ASSIST
    if (config.parkingAssistEnabled && !state.collisionImminent) {
        if (frontDist < config.parkingBrakeDistanceMm) {
            state.parkingAssistActive = true;
            state.speedReductionFactor = std::min(state.speedReductionFactor, 
                                                   (float)frontDist / config.parkingBrakeDistanceMm);
            
            if (now - lastAlertMs[0] > ALERT_INTERVAL_MS) {
                Alerts::play(Audio::AUDIO_ERROR_GENERAL);  // Proximity beep
                lastAlertMs[0] = now;
            }
        }
        
        if (rearDist < config.parkingBrakeDistanceMm) {
            state.parkingAssistActive = true;
            state.speedReductionFactor = std::min(state.speedReductionFactor, 
                                                   (float)rearDist / config.parkingBrakeDistanceMm);
            
            if (now - lastAlertMs[1] > ALERT_INTERVAL_MS) {
                Alerts::play(Audio::AUDIO_ERROR_GENERAL);  // Proximity beep
                lastAlertMs[1] = now;
            }
        }
    }
    
    // 3. BLIND SPOT WARNING
    if (config.blindSpotEnabled) {
        if (leftDist < config.blindSpotDistanceMm) {
            state.blindSpotLeft = true;
            if (now - lastAlertMs[2] > ALERT_INTERVAL_MS) {
                Alerts::play(Audio::AUDIO_ERROR_GENERAL);  // Warning beep
                Logger::infof("Blind spot LEFT: %d mm", leftDist);
                lastAlertMs[2] = now;
            }
        }
        
        if (rightDist < config.blindSpotDistanceMm) {
            state.blindSpotRight = true;
            if (now - lastAlertMs[3] > ALERT_INTERVAL_MS) {
                Alerts::play(Audio::AUDIO_ERROR_GENERAL);  // Warning beep
                Logger::infof("Blind spot RIGHT: %d mm", rightDist);
                lastAlertMs[3] = now;
            }
        }
    }
    
    // 4. ADAPTIVE CRUISE CONTROL (disabled for now - requires speed sensor integration)
    state.adaptiveCruiseActive = false;
}

void getState(SafetyState& st) {
    st = state;
}

void getConfig(SafetyConfig& cfg) {
    cfg = config;
}

void setConfig(const SafetyConfig& newConfig) {
    config = newConfig;
    // Save to storage
}

bool isCollisionImminent() {
    return state.collisionImminent;
}

bool isParkingAssistActive() {
    return state.parkingAssistActive;
}

bool isBlindSpotActive() {
    return state.blindSpotLeft || state.blindSpotRight;
}

float getSpeedReductionFactor() {
    return state.speedReductionFactor;
}

void enableParkingAssist(bool enable) {
    config.parkingAssistEnabled = enable;
}

void enableCollisionAvoidance(bool enable) {
    config.collisionAvoidanceEnabled = enable;
}

void enableBlindSpot(bool enable) {
    config.blindSpotEnabled = enable;
}

void enableAdaptiveCruise(bool enable) {
    config.adaptiveCruiseEnabled = enable;
}

void triggerEmergencyStop() {
    state.emergencyBrakeApplied = true;
    state.collisionImminent = true;
    Alerts::play(Audio::AUDIO_EMERGENCIA);
    Logger::warn("MANUAL EMERGENCY STOP triggered");
}

void resetEmergencyStop() {
    state.emergencyBrakeApplied = false;
    Logger::info("Emergency stop reset");
}

} // namespace ObstacleSafety
