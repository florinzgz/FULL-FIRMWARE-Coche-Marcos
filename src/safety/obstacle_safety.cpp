/**
 * @file obstacle_safety.cpp
 * @brief Intelligent 5-zone obstacle avoidance with child reaction detection and ACC coordination
 * @date 2025-12-23
 */

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

// Static variables
static ObstacleSafetyConfig config;
static ObstacleSafetyState state;
static unsigned long lastUpdateMs = 0;
static unsigned long lastAlertMs[6] = {0}; // For each zone type
static const unsigned long ALERT_INTERVAL_MS = 1000;
static const float PEDAL_REACTION_THRESHOLD = 10.0f;
static const unsigned long PEDAL_REACTION_WINDOW_MS = 500;
static const float EARLY_ALERT_DISTANCE_MM = 1500.0f;

/**
 * @brief Initialize obstacle safety system
 */
void init() {
    // Initialize config values
    config.enabled = true;
    config.parkingAssistEnabled = true;
    config.blindSpotEnabled = true;
    config.emergencyBrakeEnabled = true;
    config.audioAlertsEnabled = true;
    config.visualAlertsEnabled = true;
    
    // Distance thresholds (in mm)
    config.criticalDistance = 200;      // 20cm - ZONE 5
    config.warningDistance = 500;       // 50cm - ZONE 4
    config.cautionDistance = 1000;      // 100cm - ZONE 3
    config.earlyAlertDistance = 1500;   // 150cm - ZONE 2
    config.maxDetectionDistance = 4000; // 400cm - ZONE 1
    
    // Speed reduction factors
    config.criticalSpeedFactor = 0.2f;
    config.warningSpeedFactor = 0.5f;
    config.cautionSpeedFactor = 0.7f;
    config.earlyAlertSpeedFactor = 0.9f;
    
    // Aggressive factors for child reaction
    config.aggressiveWarningFactor = 0.3f;
    config.aggressiveCautionFactor = 0.5f;
    
    // Initialize state
    state.collisionImminent = false;
    state.emergencyBrakeApplied = false;
    state.parkingAssistActive = false;
    state.blindSpotActive = false;
    state.speedReductionFactor = 1.0f;
    state.frontObstacleDistance = 0xFFFF;
    state.rearObstacleDistance = 0xFFFF;
    state.leftBlindSpotDetected = false;
    state.rightBlindSpotDetected = false;
    state.pedalReactionDetected = false;
    state.lastPedalChangeMs = 0;
    state.lastPedalValue = 0.0f;
    state.activeZoneFront = ZONE_CLEAR;
    state.activeZoneRear = ZONE_CLEAR;
    
    Logger::info("ObstacleSafety", "Initialized with 5-zone intelligent detection");
}

/**
 * @brief Update obstacle safety system
 */
void update() {
    if (!config.enabled) {
        return;
    }
    
    unsigned long now = millis();
    
    // Get current pedal position
    float currentPedal = Pedal::get().percent;
    
    // Detect child reaction (sudden pedal release)
    if (state.lastPedalValue - currentPedal > PEDAL_REACTION_THRESHOLD) {
        state.pedalReactionDetected = true;
        state.lastPedalChangeMs = now;
        Logger::info("ObstacleSafety", "Child reaction detected - sudden brake");
    }
    
    // Reset reaction detection after window expires
    if (state.pedalReactionDetected && (now - state.lastPedalChangeMs > PEDAL_REACTION_WINDOW_MS)) {
        state.pedalReactionDetected = false;
    }
    
    // Update last pedal value
    state.lastPedalValue = currentPedal;
    
    // Reset state for this update cycle
    state.collisionImminent = false;
    state.emergencyBrakeApplied = false;
    state.parkingAssistActive = false;
    state.speedReductionFactor = 1.0f;
    state.activeZoneFront = ZONE_CLEAR;
    state.activeZoneRear = ZONE_CLEAR;
    
    // Get obstacle distances
    state.frontObstacleDistance = ObstacleDetection::getFrontDistance();
    state.rearObstacleDistance = ObstacleDetection::getRearDistance();
    
    // Check if ACC is active
    bool accActive = (AdaptiveCruise::getStatus().state == ACC_ACTIVE);
    
    // ========== FRONT SENSOR 5-ZONE LOGIC ==========
    if (state.frontObstacleDistance < config.criticalDistance) {
        // ZONE 5: EMERGENCY (< 20cm)
        state.activeZoneFront = ZONE_EMERGENCY;
        state.collisionImminent = true;
        state.emergencyBrakeApplied = true;
        state.speedReductionFactor = 0.0f;
        
        if (now - lastAlertMs[ZONE_EMERGENCY] > ALERT_INTERVAL_MS) {
            Alerts::triggerCritical("EMERGENCY STOP - Obstacle < 20cm!");
            lastAlertMs[ZONE_EMERGENCY] = now;
        }
        
    } else if (state.frontObstacleDistance < config.warningDistance) {
        // ZONE 4: CRITICAL (20-50cm)
        state.activeZoneFront = ZONE_CRITICAL;
        state.collisionImminent = true;
        
        // Calculate proportional reduction in critical zone
        float distanceRatio = (float)(state.frontObstacleDistance - config.criticalDistance) / 
                             (float)(config.warningDistance - config.criticalDistance);
        float criticalFactor = config.criticalSpeedFactor + 
                              (config.warningSpeedFactor - config.criticalSpeedFactor) * distanceRatio;
        state.speedReductionFactor = criticalFactor;
        
        if (now - lastAlertMs[ZONE_CRITICAL] > ALERT_INTERVAL_MS) {
            Alerts::triggerWarning("CRITICAL - Obstacle detected!");
            lastAlertMs[ZONE_CRITICAL] = now;
        }
        
    } else if (state.frontObstacleDistance < config.cautionDistance) {
        // ZONE 3: WARNING (50-100cm)
        state.activeZoneFront = ZONE_WARNING;
        
        if (accActive) {
            // ACC handles speed control
            state.speedReductionFactor = 1.0f;
        } else {
            // Manual mode - activate parking assist
            state.parkingAssistActive = true;
            
            // Use aggressive factor if child reaction detected
            if (state.pedalReactionDetected) {
                state.speedReductionFactor = config.aggressiveWarningFactor;
            } else {
                state.speedReductionFactor = config.warningSpeedFactor;
            }
        }
        
        if (now - lastAlertMs[ZONE_WARNING] > ALERT_INTERVAL_MS) {
            Alerts::triggerWarning("WARNING - Obstacle in range");
            lastAlertMs[ZONE_WARNING] = now;
        }
        
    } else if (state.frontObstacleDistance < config.earlyAlertDistance) {
        // ZONE 2: CAUTION (100-150cm)
        state.activeZoneFront = ZONE_CAUTION;
        
        if (accActive) {
            // ACC handles speed control
            state.speedReductionFactor = 1.0f;
        } else {
            // Manual mode - check for child reaction
            if (state.pedalReactionDetected) {
                state.speedReductionFactor = config.aggressiveCautionFactor;
            } else {
                state.speedReductionFactor = config.cautionSpeedFactor;
            }
        }
        
        if (now - lastAlertMs[ZONE_CAUTION] > ALERT_INTERVAL_MS) {
            Alerts::triggerInfo("CAUTION - Obstacle ahead");
            lastAlertMs[ZONE_CAUTION] = now;
        }
        
    } else if (state.frontObstacleDistance < config.maxDetectionDistance) {
        // ZONE 1: EARLY ALERT (150-400cm)
        state.activeZoneFront = ZONE_EARLY_ALERT;
        state.speedReductionFactor = 1.0f; // No speed reduction, audio only
        
        if (config.audioAlertsEnabled && now - lastAlertMs[ZONE_EARLY_ALERT] > ALERT_INTERVAL_MS) {
            Alerts::triggerInfo("Object detected ahead");
            lastAlertMs[ZONE_EARLY_ALERT] = now;
        }
    }
    
    // ========== REAR SENSOR 5-ZONE LOGIC ==========
    float rearSpeedFactor = 1.0f;
    
    if (state.rearObstacleDistance < config.criticalDistance) {
        // ZONE 5: EMERGENCY (< 20cm)
        state.activeZoneRear = ZONE_EMERGENCY;
        state.collisionImminent = true;
        state.emergencyBrakeApplied = true;
        rearSpeedFactor = 0.0f;
        
        if (now - lastAlertMs[ZONE_EMERGENCY] > ALERT_INTERVAL_MS) {
            Alerts::triggerCritical("EMERGENCY STOP - Rear obstacle < 20cm!");
        }
        
    } else if (state.rearObstacleDistance < config.warningDistance) {
        // ZONE 4: CRITICAL (20-50cm)
        state.activeZoneRear = ZONE_CRITICAL;
        state.collisionImminent = true;
        
        float distanceRatio = (float)(state.rearObstacleDistance - config.criticalDistance) / 
                             (float)(config.warningDistance - config.criticalDistance);
        float criticalFactor = config.criticalSpeedFactor + 
                              (config.warningSpeedFactor - config.criticalSpeedFactor) * distanceRatio;
        rearSpeedFactor = criticalFactor;
        
        if (now - lastAlertMs[ZONE_CRITICAL] > ALERT_INTERVAL_MS) {
            Alerts::triggerWarning("CRITICAL - Rear obstacle!");
        }
        
    } else if (state.rearObstacleDistance < config.cautionDistance) {
        // ZONE 3: WARNING (50-100cm)
        state.activeZoneRear = ZONE_WARNING;
        
        if (accActive) {
            rearSpeedFactor = 1.0f;
        } else {
            state.parkingAssistActive = true;
            
            if (state.pedalReactionDetected) {
                rearSpeedFactor = config.aggressiveWarningFactor;
            } else {
                rearSpeedFactor = config.warningSpeedFactor;
            }
        }
        
        if (now - lastAlertMs[ZONE_WARNING] > ALERT_INTERVAL_MS) {
            Alerts::triggerWarning("WARNING - Rear obstacle in range");
        }
        
    } else if (state.rearObstacleDistance < config.earlyAlertDistance) {
        // ZONE 2: CAUTION (100-150cm)
        state.activeZoneRear = ZONE_CAUTION;
        
        if (accActive) {
            rearSpeedFactor = 1.0f;
        } else {
            if (state.pedalReactionDetected) {
                rearSpeedFactor = config.aggressiveCautionFactor;
            } else {
                rearSpeedFactor = config.cautionSpeedFactor;
            }
        }
        
        if (now - lastAlertMs[ZONE_CAUTION] > ALERT_INTERVAL_MS) {
            Alerts::triggerInfo("CAUTION - Rear obstacle");
        }
        
    } else if (state.rearObstacleDistance < config.maxDetectionDistance) {
        // ZONE 1: EARLY ALERT (150-400cm)
        state.activeZoneRear = ZONE_EARLY_ALERT;
        rearSpeedFactor = 1.0f;
        
        if (config.audioAlertsEnabled && now - lastAlertMs[ZONE_EARLY_ALERT] > ALERT_INTERVAL_MS) {
            Alerts::triggerInfo("Object detected behind");
        }
    }
    
    // Apply minimum speed factor from front and rear
    state.speedReductionFactor = std::min(state.speedReductionFactor, rearSpeedFactor);
    
    // Update blind spot detection
    if (config.blindSpotEnabled) {
        state.leftBlindSpotDetected = ObstacleDetection::isLeftBlindSpotDetected();
        state.rightBlindSpotDetected = ObstacleDetection::isRightBlindSpotDetected();
        state.blindSpotActive = state.leftBlindSpotDetected || state.rightBlindSpotDetected;
    }
    
    lastUpdateMs = now;
}

/**
 * @brief Get current safety state
 * @return Current ObstacleSafetyState
 */
ObstacleSafetyState getState() {
    return state;
}

/**
 * @brief Get current configuration
 * @return Current ObstacleSafetyConfig
 */
ObstacleSafetyConfig getConfig() {
    return config;
}

/**
 * @brief Set configuration
 * @param newConfig New configuration to apply
 */
void setConfig(const ObstacleSafetyConfig& newConfig) {
    config = newConfig;
    Logger::info("ObstacleSafety", "Configuration updated");
}

/**
 * @brief Check if collision is imminent
 * @return true if collision detected
 */
bool isCollisionImminent() {
    return state.collisionImminent;
}

/**
 * @brief Check if parking assist is active
 * @return true if parking assist active
 */
bool isParkingAssistActive() {
    return state.parkingAssistActive && config.parkingAssistEnabled;
}

/**
 * @brief Check if blind spot detection is active
 * @return true if blind spot detected
 */
bool isBlindSpotActive() {
    return state.blindSpotActive && config.blindSpotEnabled;
}

/**
 * @brief Get current speed reduction factor
 * @return Speed reduction factor (0.0-1.0)
 */
float getSpeedReductionFactor() {
    return state.speedReductionFactor;
}

/**
 * @brief Enable obstacle safety system
 */
void enable() {
    config.enabled = true;
    Logger::info("ObstacleSafety", "System enabled");
}

/**
 * @brief Disable obstacle safety system
 */
void disable() {
    config.enabled = false;
    state.collisionImminent = false;
    state.emergencyBrakeApplied = false;
    state.speedReductionFactor = 1.0f;
    Logger::info("ObstacleSafety", "System disabled");
}

/**
 * @brief Enable parking assist
 */
void enableParkingAssist() {
    config.parkingAssistEnabled = true;
    Logger::info("ObstacleSafety", "Parking assist enabled");
}

/**
 * @brief Disable parking assist
 */
void disableParkingAssist() {
    config.parkingAssistEnabled = false;
    Logger::info("ObstacleSafety", "Parking assist disabled");
}

/**
 * @brief Enable blind spot detection
 */
void enableBlindSpot() {
    config.blindSpotEnabled = true;
    Logger::info("ObstacleSafety", "Blind spot detection enabled");
}

/**
 * @brief Disable blind spot detection
 */
void disableBlindSpot() {
    config.blindSpotEnabled = false;
    Logger::info("ObstacleSafety", "Blind spot detection disabled");
}

/**
 * @brief Enable emergency brake
 */
void enableEmergencyBrake() {
    config.emergencyBrakeEnabled = true;
    Logger::info("ObstacleSafety", "Emergency brake enabled");
}

/**
 * @brief Disable emergency brake
 */
void disableEmergencyBrake() {
    config.emergencyBrakeEnabled = false;
    Logger::info("ObstacleSafety", "Emergency brake disabled");
}

/**
 * @brief Trigger emergency stop
 */
void triggerEmergencyStop() {
    state.emergencyBrakeApplied = true;
    state.collisionImminent = true;
    state.speedReductionFactor = 0.0f;
    Traction::emergencyStop();
    Alerts::triggerCritical("EMERGENCY STOP ACTIVATED");
    Logger::warning("ObstacleSafety", "Emergency stop triggered");
}

/**
 * @brief Reset emergency stop
 */
void resetEmergencyStop() {
    state.emergencyBrakeApplied = false;
    state.collisionImminent = false;
    state.speedReductionFactor = 1.0f;
    Logger::info("ObstacleSafety", "Emergency stop reset");
}

} // namespace ObstacleSafety
