// Obstacle-based Safety Systems
// Parking assist, collision avoidance, blind spot warning, adaptive cruise control
// Author: Copilot AI Assistant
// Date: 2025-11-23
// v2.12.0: Intelligent obstacle avoidance with child reaction detection + ACC coordination

#include "obstacle_safety.h"
#include "obstacle_detection.h"
#include "obstacle_config.h"
#include "traction.h"
#include "pedal.h"
#include "adaptive_cruise.h"
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

// 🔒 v2.12.0: Child reaction detection thresholds
static constexpr float PEDAL_REACTION_THRESHOLD = 10.0f;    // 10% pedal reduction to detect reaction
static constexpr uint32_t PEDAL_REACTION_WINDOW_MS = 500;   // 500ms window for reaction
static constexpr uint16_t EARLY_ALERT_DISTANCE_MM = 1500;   // 150cm early warning

void init() {
    Logger::info("Initializing obstacle safety systems...");
    
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
    state.pedalReactionDetected = false;
    state.lastPedalChangeMs = 0;
    state.lastPedalValue = 0.0f;
    
    if (!config.blindSpotEnabled) {
        // Blind spot deshabilitado permanentemente (sin sensores laterales)
        state.blindSpotLeft = false;
        state.blindSpotRight = false;
    }
    
    Logger::info("Obstacle safety systems initialized (intelligent mode with ACC coordination)");
}

void update() {
    uint32_t now = millis();
    if (now - lastUpdateMs < 50) return;  // 20Hz update
    lastUpdateMs = now;
    
    // Get obstacle status
    ObstacleDetection::ObstacleStatus obstStatus;
    ObstacleDetection::getStatus(obstStatus);
    
    // Get current pedal value for reaction detection
    float currentPedal = Pedal::get().percent;
    
    // 🔒 v2.12.0: CHILD REACTION DETECTION
    // Detect if child has reduced pedal >10% recently (intelligent braking)
    if (state.lastPedalValue - currentPedal > PEDAL_REACTION_THRESHOLD) {
        state.pedalReactionDetected = true;
        state.lastPedalChangeMs = now;
        Logger::info("Obstacle: Child pedal reaction detected - allowing control");
    }
    
    // Reset reaction flag if >500ms without changes
    if (now - state.lastPedalChangeMs > PEDAL_REACTION_WINDOW_MS) {
        state.pedalReactionDetected = false;
    }
    
    state.lastPedalValue = currentPedal;
    
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
    
    // Ignore invalid distances
    if (frontDist >= ::ObstacleConfig::DISTANCE_INVALID) frontDist = ::ObstacleConfig::DISTANCE_MAX;
    if (rearDist >= ::ObstacleConfig::DISTANCE_INVALID) rearDist = ::ObstacleConfig::DISTANCE_MAX;
    
    // Find closest obstacle
    uint16_t minDist = ::ObstacleConfig::DISTANCE_MAX;
    if (frontDist < minDist) { minDist = frontDist; state.closestObstacleSensor = 0; }
    if (rearDist < minDist) { minDist = rearDist; state.closestObstacleSensor = 1; }
    state.closestObstacleDistanceMm = minDist;
    
    // ============================================================================
    // INTELLIGENT OBSTACLE AVOIDANCE - FRONT SENSOR (5 zones with reaction detection + ACC coordination)
    // ============================================================================
    if (config.collisionAvoidanceEnabled && frontDist < ::ObstacleConfig::DISTANCE_MAX) {
        
        // 🔴 ZONE 5: EMERGENCY <20cm → FULL CUT (no exceptions, overrides ACC)
        if (frontDist < ::ObstacleConfig::DISTANCE_CRITICAL) {
            state.collisionImminent = true;
            state.emergencyBrakeApplied = true;
            state.speedReductionFactor = 0.0f;
            
            if (now - lastAlertMs[0] > 200) {
                Alerts::play(Audio::AUDIO_EMERGENCIA);
                Logger::error("FRONT EMERGENCY <20cm - FULL STOP!");
                lastAlertMs[0] = now;
            }
        }
        // 🟠 ZONE 4: CRITICAL 20-50cm → Forced reduction (independent of reaction, overrides ACC)
        else if (frontDist < ::ObstacleConfig::DISTANCE_WARNING) {
            state.collisionImminent = true;
            float criticalFactor = 0.1f + ((float)(frontDist - ::ObstacleConfig::DISTANCE_CRITICAL) / 
                                           (float)(::ObstacleConfig::DISTANCE_WARNING - ::ObstacleConfig::DISTANCE_CRITICAL)) * 0.3f;
            state.speedReductionFactor = criticalFactor;
            
            if (now - lastAlertMs[0] > 300) {
                Alerts::play(Audio::AUDIO_ERROR_GENERAL);
                Logger::warnf("FRONT CRITICAL %dmm - Force brake %.0f%%", frontDist, criticalFactor * 100);
                lastAlertMs[0] = now;
            }
        }
        // 🟡 ZONE 3: WARNING 50-100cm → Reduction based on reaction (unless ACC active)
        else if (frontDist < ::ObstacleConfig::DISTANCE_CAUTION) {
            // 🔒 v2.12.0: Check if ACC is handling this (give ACC priority)
            bool accActive = (AdaptiveCruise::getStatus().state == AdaptiveCruise::ACC_ACTIVE);
            
            if (accActive) {
                // ACC is controlling speed - don't interfere
                state.speedReductionFactor = 1.0f;
                Logger::debugf("FRONT WARNING %dmm - ACC active, deferring control", frontDist);
            } else {
                // ACC not active, apply normal obstacle avoidance
                state.parkingAssistActive = true;
                
                if (state.pedalReactionDetected) {
                    // Child reacted: Soft assist (allow control)
                    float assistFactor = 0.6f + ((float)(frontDist - ::ObstacleConfig::DISTANCE_WARNING) / 
                                                 (float)(::ObstacleConfig::DISTANCE_CAUTION - ::ObstacleConfig::DISTANCE_WARNING)) * 0.1f;
                    state.speedReductionFactor = assistFactor;
                    Logger::debugf("FRONT WARNING %dmm - Child reacting, soft assist %.0f%%", frontDist, assistFactor * 100);
                } else {
                    // No reaction: Aggressive reduction
                    float aggressiveFactor = 0.4f + ((float)(frontDist - ::ObstacleConfig::DISTANCE_WARNING) / 
                                                     (float)(::ObstacleConfig::DISTANCE_CAUTION - ::ObstacleConfig::DISTANCE_WARNING)) * 0.2f;
                    state.speedReductionFactor = aggressiveFactor;
                    Logger::warnf("FRONT WARNING %dmm - No reaction, aggressive brake %.0f%%", frontDist, aggressiveFactor * 100);
                }
            }
            
            if (now - lastAlertMs[0] > 500) {
                Alerts::play(Audio::AUDIO_ERROR_GENERAL);
                lastAlertMs[0] = now;
            }
        }
        // 🟢 ZONE 2: CAUTION 100-150cm → Soft reduction only if NO reaction (unless ACC active)
        else if (frontDist < EARLY_ALERT_DISTANCE_MM) {
            // 🔒 v2.12.0: Check if ACC is handling this
            bool accActive = (AdaptiveCruise::getStatus().state == AdaptiveCruise::ACC_ACTIVE);
            
            if (accActive) {
                // ACC controlling - don't interfere
                state.speedReductionFactor = 1.0f;
                Logger::debugf("FRONT CAUTION %dmm - ACC active, deferring control", frontDist);
            } else if (!state.pedalReactionDetected) {
                // No reaction and ACC not active - apply gentle brake
                state.parkingAssistActive = true;
                float cautionFactor = 0.85f + ((float)(frontDist - ::ObstacleConfig::DISTANCE_CAUTION) / 
                                               (float)(EARLY_ALERT_DISTANCE_MM - ::ObstacleConfig::DISTANCE_CAUTION)) * 0.15f;
                state.speedReductionFactor = cautionFactor;
                Logger::debugf("FRONT CAUTION %dmm - No reaction, gentle brake %.0f%%", frontDist, cautionFactor * 100);
            } else {
                // Child reacting - full control
                state.speedReductionFactor = 1.0f;
                Logger::debugf("FRONT CAUTION %dmm - Child reacting, full control", frontDist);
            }
            
            if (now - lastAlertMs[0] > 1000) {
                Alerts::play(Audio::AUDIO_MODULO_OK);
                lastAlertMs[0] = now;
            }
        }
        // 🔵 ZONE 1: EARLY ALERT 150-400cm → Audio only, NO reduction
        else if (frontDist < ::ObstacleConfig::DISTANCE_MAX) {
            state.speedReductionFactor = 1.0f;
            
            if (now - lastAlertMs[0] > 2000) {
                Alerts::play(Audio::AUDIO_MODULO_OK);
                Logger::debugf("FRONT ALERT %dmm - Early warning", frontDist);
                lastAlertMs[0] = now;
            }
        }
    }
    
    // ============================================================================
    // INTELLIGENT OBSTACLE AVOIDANCE - REAR SENSOR (same logic with ACC coordination)
    // ============================================================================
    if (config.collisionAvoidanceEnabled && rearDist < ::ObstacleConfig::DISTANCE_MAX) {
        
        // 🔴 ZONE 5: EMERGENCY <20cm
        if (rearDist < ::ObstacleConfig::DISTANCE_CRITICAL) {
            state.collisionImminent = true;
            state.emergencyBrakeApplied = true;
            state.speedReductionFactor = 0.0f;
            
            if (now - lastAlertMs[1] > 200) {
                Alerts::play(Audio::AUDIO_EMERGENCIA);
                Logger::error("REAR EMERGENCY <20cm - FULL STOP!");
                lastAlertMs[1] = now;
            }
        }
        // 🟠 ZONE 4: CRITICAL 20-50cm
        else if (rearDist < ::ObstacleConfig::DISTANCE_WARNING) {
            state.collisionImminent = true;
            float criticalFactor = 0.1f + ((float)(rearDist - ::ObstacleConfig::DISTANCE_CRITICAL) / 
                                           (float)(::ObstacleConfig::DISTANCE_WARNING - ::ObstacleConfig::DISTANCE_CRITICAL)) * 0.3f;
            state.speedReductionFactor = std::min(state.speedReductionFactor, criticalFactor);
            
            if (now - lastAlertMs[1] > 300) {
                Alerts::play(Audio::AUDIO_ERROR_GENERAL);
                Logger::warnf("REAR CRITICAL %dmm - Force brake %.0f%%", rearDist, criticalFactor * 100);
                lastAlertMs[1] = now;
            }
        }
        // 🟡 ZONE 3: WARNING 50-100cm (with ACC coordination)
        else if (rearDist < ::ObstacleConfig::DISTANCE_CAUTION) {
            // 🔒 v2.12.0: ACC coordination (rear sensor less critical for ACC)
            bool accActive = (AdaptiveCruise::getStatus().state == AdaptiveCruise::ACC_ACTIVE);
            
            if (accActive) {
                // ACC controlling front - apply rear reduction cautiously
                Logger::debugf("REAR WARNING %dmm - ACC active, applying rear reduction", rearDist);
            }
            
            state.parkingAssistActive = true;
            
            if (state.pedalReactionDetected) {
                float assistFactor = 0.6f + ((float)(rearDist - ::ObstacleConfig::DISTANCE_WARNING) / 
                                             (float)(::ObstacleConfig::DISTANCE_CAUTION - ::ObstacleConfig::DISTANCE_WARNING)) * 0.1f;
                state.speedReductionFactor = std::min(state.speedReductionFactor, assistFactor);
                Logger::debugf("REAR WARNING %dmm - Child reacting, soft assist %.0f%%", rearDist, assistFactor * 100);
            } else {
                float aggressiveFactor = 0.4f + ((float)(rearDist - ::ObstacleConfig::DISTANCE_WARNING) / 
                                                 (float)(::ObstacleConfig::DISTANCE_CAUTION - ::ObstacleConfig::DISTANCE_WARNING)) * 0.2f;
                state.speedReductionFactor = std::min(state.speedReductionFactor, aggressiveFactor);
                Logger::warnf("REAR WARNING %dmm - No reaction, aggressive brake %.0f%%", rearDist, aggressiveFactor * 100);
            }
            
            if (now - lastAlertMs[1] > 500) {
                Alerts::play(Audio::AUDIO_ERROR_GENERAL);
                lastAlertMs[1] = now;
            }
        }
        // 🟢 ZONE 2: CAUTION 100-150cm
        else if (rearDist < EARLY_ALERT_DISTANCE_MM) {
            // 🔒 v2.12.0: ACC coordination
            bool accActive = (AdaptiveCruise::getStatus().state == AdaptiveCruise::ACC_ACTIVE);
            
            if (accActive) {
                Logger::debugf("REAR CAUTION %dmm - ACC active, applying rear caution", rearDist);
            }
            
            if (!state.pedalReactionDetected) {
                state.parkingAssistActive = true;
                float cautionFactor = 0.85f + ((float)(rearDist - ::ObstacleConfig::DISTANCE_CAUTION) / 
                                               (float)(EARLY_ALERT_DISTANCE_MM - ::ObstacleConfig::DISTANCE_CAUTION)) * 0.15f;
                state.speedReductionFactor = std::min(state.speedReductionFactor, cautionFactor);
                Logger::debugf("REAR CAUTION %dmm - No reaction, gentle brake %.0f%%", rearDist, cautionFactor * 100);
            } else {
                Logger::debugf("REAR CAUTION %dmm - Child reacting, full control", rearDist);
            }
            
            if (now - lastAlertMs[1] > 1000) {
                Alerts::play(Audio::AUDIO_MODULO_OK);
                lastAlertMs[1] = now;
            }
        }
        // 🔵 ZONE 1: EARLY ALERT 150-400cm
        else if (rearDist < ::ObstacleConfig::DISTANCE_MAX) {
            if (now - lastAlertMs[1] > 2000) {
                Alerts::play(Audio::AUDIO_MODULO_OK);
                Logger::debugf("REAR ALERT %dmm - Early warning", rearDist);
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
