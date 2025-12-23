// Adaptive Cruise Control (ACC) - Person Following System
// Maintains target distance to obstacle using PID controller
// Author: Copilot AI Assistant
// Date: 2025-12-22
// v2.12.0: Intelligent person-following with child safety integration

#include "adaptive_cruise.h"
#include "obstacle_detection.h"
#include "obstacle_config.h"
#include "pedal.h"
#include "logger.h"
#include "alerts.h"
#include <cmath>
#include <algorithm>

namespace AdaptiveCruise {

// ACC state and configuration
static ACCConfig config;
static ACCStatus status;
static uint32_t lastUpdateMs = 0;

// PID controller state
static float errorIntegral = 0.0f;
static float lastError = 0.0f;
static uint32_t lastPIDUpdateMs = 0;

// Detection tracking
static uint32_t targetLostMs = 0;
static constexpr uint32_t TARGET_LOST_TIMEOUT_MS = 2000;
static constexpr uint32_t UPDATE_INTERVAL_MS = 100;

// Speed estimation
static uint16_t lastDistanceMm = 0;
static float estimatedTargetSpeedKmh = 0.0f;

// Safety limits
static constexpr float MAX_SPEED_ADJUSTMENT = 1.0f;
static constexpr float MIN_SPEED_ADJUSTMENT = 0.0f;
static constexpr uint16_t DETECTION_RANGE_MIN_MM = 300;
static constexpr uint16_t DETECTION_RANGE_MAX_MM = 4000;

void init() {
    Logger::info("Initializing Adaptive Cruise Control...");
    
    config.enabled = false;
    config.targetDistanceMm = 500;
    config.minDistanceMm = 300;
    config.maxSpeedReduction = 100;
    config.pidKp = 0.3f;
    config.pidKi = 0.05f;
    config.pidKd = 0.15f;
    
    status.state = ACC_DISABLED;
    status.currentDistance = 0xFFFF;
    status.speedAdjustment = 1.0f;
    status.vehicleDetected = false;
    status.lastUpdateMs = 0;
    
    errorIntegral = 0.0f;
    lastError = 0.0f;
    lastPIDUpdateMs = 0;
    targetLostMs = 0;
    lastDistanceMm = 0;
    estimatedTargetSpeedKmh = 0.0f;
    
    Logger::info("ACC initialized (person-following mode, target: 50cm)");
}

void update() {
    uint32_t now = millis();
    
    if (now - lastUpdateMs < UPDATE_INTERVAL_MS) return;
    float deltaTimeS = (now - lastUpdateMs) / 1000.0f;
    lastUpdateMs = now;
    
    if (!config.enabled) {
        status.state = ACC_DISABLED;
        status.speedAdjustment = 1.0f;
        status.vehicleDetected = false;
        errorIntegral = 0.0f;
        lastError = 0.0f;
        return;
    }
    
    ObstacleDetection::ObstacleStatus obstStatus;
    ObstacleDetection::getStatus(obstStatus);
    
    uint16_t frontDist = obstStatus.minDistanceFront;
    
    if (frontDist >= ::ObstacleConfig::DISTANCE_INVALID || 
        frontDist >= DETECTION_RANGE_MAX_MM) {
        status.vehicleDetected = false;
        
        if (status.state == ACC_ACTIVE || status.state == ACC_BRAKING) {
            if (targetLostMs == 0) {
                targetLostMs = now;
                Logger::warn("ACC: Target lost - entering standby");
            }
            
            if (now - targetLostMs > TARGET_LOST_TIMEOUT_MS) {
                status.state = ACC_STANDBY;
                status.speedAdjustment = 1.0f;
                errorIntegral = 0.0f;
                Logger::info("ACC: Standby mode (target lost timeout)");
            } else {
                Logger::debugf("ACC: Maintaining last adjustment (target lost %dms)", 
                              now - targetLostMs);
            }
        } else {
            status.state = ACC_STANDBY;
            status.speedAdjustment = 1.0f;
        }
        
        status.currentDistance = 0xFFFF;
        return;
    }
    
    status.vehicleDetected = true;
    status.currentDistance = frontDist;
    targetLostMs = 0;
    
    if (frontDist < DETECTION_RANGE_MIN_MM) {
        status.state = ACC_BRAKING;
        status.speedAdjustment = 0.0f;
        Logger::warnf("ACC: Emergency brake - distance %dmm < %dmm", 
                     frontDist, DETECTION_RANGE_MIN_MM);
        return;
    }
    
    status.state = ACC_ACTIVE;
    
    float targetDist = static_cast<float>(config.targetDistanceMm);
    float currentDist = static_cast<float>(frontDist);
    float error = currentDist - targetDist;
    
    if (lastDistanceMm > 0 && deltaTimeS > 0.001f) {
        float distanceChange = static_cast<float>(frontDist) - static_cast<float>(lastDistanceMm);
        float speedMmPerS = distanceChange / deltaTimeS;
        estimatedTargetSpeedKmh = (speedMmPerS * 3.6f) / 1000.0f;
        estimatedTargetSpeedKmh = std::max(-10.0f, std::min(10.0f, estimatedTargetSpeedKmh));
    }
    lastDistanceMm = frontDist;
    
    float P = error * config.pidKp;
    
    errorIntegral += error * deltaTimeS;
    float integralLimit = 1000.0f;
    errorIntegral = std::max(-integralLimit, std::min(integralLimit, errorIntegral));
    float I = errorIntegral * config.pidKi;
    
    float errorDerivative = 0.0f;
    if (deltaTimeS > 0.001f) {
        errorDerivative = (error - lastError) / deltaTimeS;
    }
    float D = errorDerivative * config.pidKd;
    lastError = error;
    
    float pidOutput = (P + I + D) / 1000.0f;
    
    float speedAdjustment = 1.0f + pidOutput;
    
    speedAdjustment = std::max(MIN_SPEED_ADJUSTMENT, 
                               std::min(MAX_SPEED_ADJUSTMENT, speedAdjustment));
    
    float pedalPercent = Pedal::get().percent;
    float maxAllowedAdjustment = pedalPercent / 100.0f;
    
    if (speedAdjustment > maxAllowedAdjustment) {
        speedAdjustment = maxAllowedAdjustment;
        Logger::debugf("ACC: Limited by pedal (%.1f%% → %.1f%%)", 
                      speedAdjustment * 100, maxAllowedAdjustment * 100);
    }
    
    if (frontDist < config.minDistanceMm) {
        float safetyFactor = static_cast<float>(frontDist - DETECTION_RANGE_MIN_MM) / 
                            static_cast<float>(config.minDistanceMm - DETECTION_RANGE_MIN_MM);
        safetyFactor = std::max(0.0f, std::min(1.0f, safetyFactor));
        
        if (safetyFactor < speedAdjustment) {
            speedAdjustment = safetyFactor;
            Logger::warnf("ACC: Safety override - distance %dmm, factor %.2f", 
                         frontDist, safetyFactor);
        }
    }
    
    status.speedAdjustment = speedAdjustment;
    
    static uint32_t lastLogMs = 0;
    if (now - lastLogMs > 1000) {
        Logger::infof("ACC: dist=%dmm, target=%dmm, err=%.0fmm, P=%.2f, I=%.2f, D=%.2f, adj=%.2f, pedal=%.1f%%",
                     frontDist, config.targetDistanceMm, error, P, I, D, 
                     speedAdjustment, pedalPercent);
        lastLogMs = now;
    }
}

void setEnabled(bool enable) {
    if (enable && !config.enabled) {
        Logger::info("ACC: Enabling person-following mode");
        config.enabled = true;
        status.state = ACC_STANDBY;
        
        errorIntegral = 0.0f;
        lastError = 0.0f;
        lastDistanceMm = 0;
        targetLostMs = 0;
        
        Alerts::play(Audio::AUDIO_MODULO_OK);
    } else if (!enable && config.enabled) {
        Logger::info("ACC: Disabling");
        config.enabled = false;
        status.state = ACC_DISABLED;
        status.speedAdjustment = 1.0f;
        
        Alerts::play(Audio::AUDIO_MODULO_OK);
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
    
    errorIntegral = 0.0f;
    lastError = 0.0f;
    
    Logger::infof("ACC config updated: target=%dmm, Kp=%.2f, Ki=%.2f, Kd=%.2f",
                 config.targetDistanceMm, config.pidKp, config.pidKi, config.pidKd);
}

float getSpeedAdjustment() {
    return status.speedAdjustment;
}

void reset() {
    Logger::info("ACC: Manual reset");
    
    if (config.enabled) {
        status.state = ACC_STANDBY;
    } else {
        status.state = ACC_DISABLED;
    }
    
    status.speedAdjustment = 1.0f;
    status.vehicleDetected = false;
    
    errorIntegral = 0.0f;
    lastError = 0.0f;
    lastDistanceMm = 0;
    targetLostMs = 0;
}

} // namespace AdaptiveCruise
