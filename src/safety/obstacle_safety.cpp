// Obstacle-based Safety Systems v2.12.0
// Parking assist, collision avoidance, blind spot warning, adaptive cruise control
// v2.12.0: Intelligent 5-zone obstacle avoidance with child reaction detection
// Author: Copilot AI Assistant
// Date: 2025-12-23
// Note: Placeholder implementation - VL53L5X sensors not yet integrated

#include "obstacle_safety.h"
#include "obstacle_detection.h"
#include "obstacle_config.h"
#include "adaptive_cruise.h"
#include "pedal.h"
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

// v2.12.0: 5-Zone thresholds (mm)
static constexpr uint16_t ZONE_5_THRESHOLD = 200;   // <20cm - Emergency
static constexpr uint16_t ZONE_4_THRESHOLD = 500;   // 20-50cm - Critical
static constexpr uint16_t ZONE_3_THRESHOLD = 1000;  // 50-100cm - Warning
static constexpr uint16_t ZONE_2_THRESHOLD = 1500;  // 100-150cm - Caution
static constexpr uint16_t ZONE_1_THRESHOLD = 4000;  // 150-400cm - Alert

// v2.12.0: Child reaction detection
static constexpr float CHILD_REACTION_THRESHOLD = 10.0f;  // 10% pedal reduction
static constexpr uint32_t CHILD_REACTION_WINDOW_MS = 500;  // 500ms window

void init() {
    Logger::info("Obstacle safety v2.12.0: Initializing with 5-zone detection...");
    
    // Default configuration
    config.parkingAssistEnabled = true;
    config.collisionAvoidanceEnabled = true;
    config.blindSpotEnabled = false;  // Deshabilitado: sin sensores laterales
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
    
    // v2.12.0: Initialize child reaction detection
    state.childReactionDetected = false;
    state.lastPedalReductionMs = 0;
    state.lastPedalValue = 0.0f;
    
    // v2.12.0: Initialize ACC coordination
    state.accHasPriority = false;
    state.obstacleZone = 0;
    
    if (!config.blindSpotEnabled) {
        // Blind spot deshabilitado permanentemente (sin sensores laterales)
        state.blindSpotLeft = false;
        state.blindSpotRight = false;
    }
    
    Logger::info("Obstacle safety v2.12.0 initialized with 5-zone detection");
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
    
    // 🔒 CRITICAL SAFETY: If sensors are unhealthy (timeout/error), apply emergency brake
    // This is fail-safe behavior - if we lose sensor data, assume danger and stop
    if (obstStatus.sensorsHealthy == 0) {
        if (config.collisionAvoidanceEnabled) {
            state.emergencyBrakeApplied = true;
            state.collisionImminent = true;
            state.speedReductionFactor = 0.0f;  // Full stop
            
            // Alert periodically
            static uint32_t lastSensorFailAlertMs = 0;
            if (now - lastSensorFailAlertMs > ALERT_INTERVAL_MS * 3) {  // Every 3 seconds
                Alerts::play(Audio::AUDIO_EMERGENCIA);
                Logger::error("SENSOR FAILURE: Emergency brake applied (fail-safe)");
                lastSensorFailAlertMs = now;
            }
        }
        return;  // No valid sensor data, maintain emergency state
    }
    
    // Get distances
    uint16_t frontDist = obstStatus.minDistanceFront;
    uint16_t rearDist = obstStatus.minDistanceRear;
    
    // Ignore invalid distances
    if (frontDist >= ::ObstacleConfig::DISTANCE_INVALID) frontDist = ::ObstacleConfig::DISTANCE_MAX;
    if (rearDist >= ::ObstacleConfig::DISTANCE_INVALID) rearDist = ::ObstacleConfig::DISTANCE_MAX;
    
    // Find closest obstacle
    uint16_t minDist = ::ObstacleConfig::DISTANCE_MAX;
    if (frontDist < minDist) { minDist = frontDist; state.closestObstacleSensor = 0; }
    if (rearDist < minDist) { minDist = rearDist; state.closestObstacleSensor = 1; }
    state.closestObstacleDistanceMm = minDist;
    
    // v2.12.0: Determine obstacle zone (1-5)
    uint8_t zone = 0;
    if (minDist < ZONE_5_THRESHOLD) {
        zone = 5;  // Emergency
    } else if (minDist < ZONE_4_THRESHOLD) {
        zone = 4;  // Critical
    } else if (minDist < ZONE_3_THRESHOLD) {
        zone = 3;  // Warning
    } else if (minDist < ZONE_2_THRESHOLD) {
        zone = 2;  // Caution
    } else if (minDist < ZONE_1_THRESHOLD) {
        zone = 1;  // Alert
    }
    state.obstacleZone = zone;
    
    // v2.12.0: Child reaction detection
    auto pedalState = Pedal::get();
    if (pedalState.valid) {
        float pedalReduction = state.lastPedalValue - pedalState.percent;
        if (pedalReduction > CHILD_REACTION_THRESHOLD) {
            // Child reduced pedal
            state.lastPedalReductionMs = now;
            state.childReactionDetected = true;
        } else if (pedalReduction <= 0.0f) {
            // Pedal increased or stayed constant - clear reaction immediately
            state.childReactionDetected = false;
        } else if (now - state.lastPedalReductionMs > CHILD_REACTION_WINDOW_MS) {
            // Small reduction but outside reaction window - clear reaction
            state.childReactionDetected = false;
        }
        state.lastPedalValue = pedalState.percent;
    }
    
    // v2.12.0: ACC Coordination
    // ACC has priority in zones 2-3 (50-150cm)
    // Obstacle safety overrides in zones 4-5 (<50cm)
    auto accStatus = AdaptiveCruise::getStatus();
    state.adaptiveCruiseActive = (accStatus.state == AdaptiveCruise::ACC_ACTIVE || 
                                   accStatus.state == AdaptiveCruise::ACC_BRAKING);
    state.accHasPriority = state.adaptiveCruiseActive && (zone == 2 || zone == 3);
    
    // v2.12.0: 5-Zone Detection System
    if (config.collisionAvoidanceEnabled) {
        switch (zone) {
            case 5:  // ZONE 5: Emergency (<20cm) - Full stop (0%)
                state.collisionImminent = true;
                state.emergencyBrakeApplied = true;
                state.speedReductionFactor = 0.0f;
                if (now - lastAlertMs[0] > ALERT_INTERVAL_MS) {
                    Alerts::play(Audio::AUDIO_EMERGENCIA);
                    Logger::warnf("ZONE 5: EMERGENCY STOP! Distance=%dmm", minDist);
                    lastAlertMs[0] = now;
                }
                break;
                
            case 4:  // ZONE 4: Critical (20-50cm) - Forced reduction (10-40%)
                state.collisionImminent = true;
                // Linear interpolation: 20cm=10%, 50cm=40%
                state.speedReductionFactor = 0.1f + (minDist - ZONE_5_THRESHOLD) * 0.3f / 
                                             (ZONE_4_THRESHOLD - ZONE_5_THRESHOLD);
                state.speedReductionFactor = constrain(state.speedReductionFactor, 0.1f, 0.4f);
                if (now - lastAlertMs[0] > ALERT_INTERVAL_MS) {
                    Alerts::play(Audio::AUDIO_EMERGENCIA);
                    Logger::warnf("ZONE 4: CRITICAL! Distance=%dmm, factor=%.2f", 
                                 minDist, state.speedReductionFactor);
                    lastAlertMs[0] = now;
                }
                break;
                
            case 3:  // ZONE 3: Warning (50-100cm) - Reaction-based (40-70%)
                if (state.childReactionDetected) {
                    // Child reacted - soft assist
                    state.speedReductionFactor = 0.7f;
                    Logger::debugf("ZONE 3: Child reacted, soft assist (70%%)");
                } else {
                    // No reaction - moderate brake
                    state.speedReductionFactor = 0.4f + (minDist - ZONE_4_THRESHOLD) * 0.3f / 
                                                 (ZONE_3_THRESHOLD - ZONE_4_THRESHOLD);
                    state.speedReductionFactor = constrain(state.speedReductionFactor, 0.4f, 0.7f);
                }
                if (now - lastAlertMs[0] > ALERT_INTERVAL_MS) {
                    Alerts::play(Audio::AUDIO_ERROR_GENERAL);
                    lastAlertMs[0] = now;
                }
                break;
                
            case 2:  // ZONE 2: Caution (100-150cm) - Gentle brake if no reaction (85-100%)
                if (!state.childReactionDetected) {
                    // No reaction - gentle brake (regardless of ACC priority)
                    state.speedReductionFactor = 0.85f + (minDist - ZONE_3_THRESHOLD) * 0.15f / 
                                                 (ZONE_2_THRESHOLD - ZONE_3_THRESHOLD);
                    state.speedReductionFactor = constrain(state.speedReductionFactor, 0.85f, 1.0f);
                } else {
                    // Child reacted - full speed (ACC may manage additional control)
                    state.speedReductionFactor = 1.0f;
                }
                break;
                
            case 1:  // ZONE 1: Alert (150-400cm) - Audio only (100%)
                state.speedReductionFactor = 1.0f;
                if (now - lastAlertMs[0] > ALERT_INTERVAL_MS * 2) {
                    Alerts::play(Audio::AUDIO_ERROR_GENERAL);
                    lastAlertMs[0] = now;
                }
                break;
                
            default:  // No obstacle
                state.speedReductionFactor = 1.0f;
                break;
        }
    }
    
    // 2. PARKING ASSIST (legacy support)
    if (config.parkingAssistEnabled && zone == 0) {
        if (frontDist < config.parkingBrakeDistanceMm) {
            state.parkingAssistActive = true;
            float parkingFactor = (float)frontDist / config.parkingBrakeDistanceMm;
            state.speedReductionFactor = std::min(state.speedReductionFactor, parkingFactor);
            
            if (now - lastAlertMs[0] > ALERT_INTERVAL_MS) {
                Alerts::play(Audio::AUDIO_ERROR_GENERAL);
                lastAlertMs[0] = now;
            }
        }
        
        if (rearDist < config.parkingBrakeDistanceMm) {
            state.parkingAssistActive = true;
            float parkingFactor = (float)rearDist / config.parkingBrakeDistanceMm;
            state.speedReductionFactor = std::min(state.speedReductionFactor, parkingFactor);
            
            if (now - lastAlertMs[1] > ALERT_INTERVAL_MS) {
                Alerts::play(Audio::AUDIO_ERROR_GENERAL);
                lastAlertMs[1] = now;
            }
        }
    }
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
