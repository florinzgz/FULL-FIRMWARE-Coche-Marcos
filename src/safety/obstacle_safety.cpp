// Obstacle-based Safety Systems
// Parking assist, collision avoidance, blind spot warning, adaptive cruise control
// Author: Copilot AI Assistant
// Date: 2025-11-23

#include "obstacle_safety.h"
#include "obstacle_detection.h"
#include "traction.h"
#include "logger.h"
#include "audio/dfplayer.h"
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
    
    Logger::info("Obstacle safety systems initialized");
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
    
    if (!obstStatus.systemHealthy) return;
    
    // Get distances
    uint16_t frontDist = ObstacleDetection::getMinDistance(ObstacleDetection::SENSOR_FRONT);
    uint16_t rearDist = ObstacleDetection::getMinDistance(ObstacleDetection::SENSOR_REAR);
    uint16_t leftDist = ObstacleDetection::getMinDistance(ObstacleDetection::SENSOR_LEFT);
    uint16_t rightDist = ObstacleDetection::getMinDistance(ObstacleDetection::SENSOR_RIGHT);
    
    // 1. COLLISION AVOIDANCE (highest priority)
    if (config.collisionAvoidanceEnabled) {
        if (frontDist < config.collisionCutoffDistanceMm) {
            state.collisionImminent = true;
            state.emergencyBrakeApplied = true;
            state.brakeForceFront = config.maxBrakeForce;
            
            // Cut power to motors
            Traction::emergencyStop();
            
            // Alert
            if (now - lastAlertMs[0] > ALERT_INTERVAL_MS) {
                Audio::playAlert(Audio::ALERT_COLLISION);
                Logger::warn("COLLISION IMMINENT - Emergency brake!");
                lastAlertMs[0] = now;
            }
        }
        
        if (rearDist < config.collisionCutoffDistanceMm) {
            state.collisionImminent = true;
            state.emergencyBrakeApplied = true;
            state.brakeForceRear = config.maxBrakeForce;
            
            if (now - lastAlertMs[1] > ALERT_INTERVAL_MS) {
                Audio::playAlert(Audio::ALERT_COLLISION);
                Logger::warn("REAR COLLISION - Emergency brake!");
                lastAlertMs[1] = now;
            }
        }
    }
    
    // 2. PARKING ASSIST
    if (config.parkingAssistEnabled && !state.collisionImminent) {
        if (frontDist < config.parkingBrakeDistanceMm) {
            state.parkingAssistActive = true;
            
            // Gradual brake force based on distance
            float brakeRatio = 1.0f - (float)frontDist / config.parkingBrakeDistanceMm;
            state.brakeForceFront = brakeRatio * config.maxBrakeForce;
            state.speedReductionFactor = std::min(state.speedReductionFactor, (float)frontDist / config.parkingBrakeDistanceMm);
            
            if (now - lastAlertMs[0] > ALERT_INTERVAL_MS) {
                Audio::playAlert(Audio::ALERT_PROXIMITY);
                lastAlertMs[0] = now;
            }
        }
        
        if (rearDist < config.parkingBrakeDistanceMm) {
            state.parkingAssistActive = true;
            
            float brakeRatio = 1.0f - (float)rearDist / config.parkingBrakeDistanceMm;
            state.brakeForceRear = brakeRatio * config.maxBrakeForce;
            state.speedReductionFactor = std::min(state.speedReductionFactor, (float)rearDist / config.parkingBrakeDistanceMm);
            
            if (now - lastAlertMs[1] > ALERT_INTERVAL_MS) {
                Audio::playAlert(Audio::ALERT_PROXIMITY);
                lastAlertMs[1] = now;
            }
        }
    }
    
    // 3. BLIND SPOT WARNING
    if (config.blindSpotEnabled) {
        if (leftDist < config.blindSpotDistanceMm) {
            state.blindSpotLeft = true;
            if (now - lastAlertMs[2] > ALERT_INTERVAL_MS) {
                Audio::playAlert(Audio::ALERT_WARNING);
                Logger::infof("Blind spot LEFT: %d mm", leftDist);
                lastAlertMs[2] = now;
            }
        }
        
        if (rightDist < config.blindSpotDistanceMm) {
            state.blindSpotRight = true;
            if (now - lastAlertMs[3] > ALERT_INTERVAL_MS) {
                Audio::playAlert(Audio::ALERT_WARNING);
                Logger::infof("Blind spot RIGHT: %d mm", rightDist);
                lastAlertMs[3] = now;
            }
        }
    }
    
    // 4. ADAPTIVE CRUISE CONTROL
    if (config.adaptiveCruiseEnabled && !state.collisionImminent && !state.parkingAssistActive) {
        float currentSpeed = Traction::getCurrentSpeed();  // km/h
        
        if (currentSpeed > config.minCruiseSpeed) {
            state.adaptiveCruiseActive = true;
            
            // Adjust speed based on front distance
            if (frontDist < config.cruiseFollowDistanceMm) {
                // Too close - reduce speed
                float targetRatio = (float)frontDist / config.cruiseFollowDistanceMm;
                state.speedReductionFactor = std::min(state.speedReductionFactor, targetRatio);
                state.targetSpeedKmh = currentSpeed * targetRatio;
                
                // Apply smooth deceleration
                Traction::setSpeedLimit(state.targetSpeedKmh);
            } else {
                // Maintain current speed
                state.targetSpeedKmh = currentSpeed;
            }
        }
    }
}

const SafetyState& getState() {
    return state;
}

const SafetyConfig& getConfig() {
    return config;
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

bool isBlindSpotActive(bool left) {
    return left ? state.blindSpotLeft : state.blindSpotRight;
}

} // namespace ObstacleSafety
