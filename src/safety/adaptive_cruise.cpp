/**
 * @file adaptive_cruise.cpp
 * @brief Adaptive Cruise Control implementation with PID controller for person following
 * @date 2025-12-23
 */

#include "safety/adaptive_cruise.h"
#include <Arduino.h>

namespace AdaptiveCruise {

// PID Controller parameters
static float kP = 0.8f;  // Proportional gain
static float kI = 0.1f;  // Integral gain
static float kD = 0.3f;  // Derivative gain

// Target distance for person following (in cm)
static const float TARGET_DISTANCE = 50.0f;

// PID state variables
static float previousError = 0.0f;
static float integralError = 0.0f;
static unsigned long lastUpdateTime = 0;

// System state
static bool enabled = false;
static float currentDistance = 0.0f;
static float speedAdjustment = 0.0f;

// Configuration
static ACCConfig config = {
    .targetDistance = TARGET_DISTANCE,
    .minDistance = 20.0f,
    .maxDistance = 200.0f,
    .maxSpeed = 100.0f,
    .minSpeed = 0.0f,
    .kP = 0.8f,
    .kI = 0.1f,
    .kD = 0.3f
};

// Status
static ACCStatus status = {
    .enabled = false,
    .currentDistance = 0.0f,
    .targetDistance = TARGET_DISTANCE,
    .speedAdjustment = 0.0f,
    .error = 0.0f,
    .lastUpdateTime = 0
};

/**
 * @brief Initialize the Adaptive Cruise Control system
 */
void init() {
    // Initialize PID state
    previousError = 0.0f;
    integralError = 0.0f;
    lastUpdateTime = millis();
    
    // Initialize system state
    enabled = false;
    currentDistance = 0.0f;
    speedAdjustment = 0.0f;
    
    // Update status
    status.enabled = false;
    status.currentDistance = 0.0f;
    status.targetDistance = config.targetDistance;
    status.speedAdjustment = 0.0f;
    status.error = 0.0f;
    status.lastUpdateTime = lastUpdateTime;
    
    Serial.println("[ACC] Adaptive Cruise Control initialized");
    Serial.print("[ACC] Target distance: ");
    Serial.print(config.targetDistance);
    Serial.println(" cm");
}

/**
 * @brief Update the ACC system with new distance measurement
 * @param distance Current distance to object/person in cm
 */
void update(float distance) {
    if (!enabled) {
        speedAdjustment = 0.0f;
        status.speedAdjustment = 0.0f;
        return;
    }
    
    currentDistance = distance;
    unsigned long currentTime = millis();
    float deltaTime = (currentTime - lastUpdateTime) / 1000.0f; // Convert to seconds
    
    // Prevent division by zero
    if (deltaTime <= 0.0f) {
        deltaTime = 0.001f;
    }
    
    // Calculate error (positive means too close, negative means too far)
    float error = config.targetDistance - distance;
    
    // Update integral term with anti-windup
    integralError += error * deltaTime;
    // Anti-windup: clamp integral term
    float maxIntegral = 50.0f;
    if (integralError > maxIntegral) {
        integralError = maxIntegral;
    } else if (integralError < -maxIntegral) {
        integralError = -maxIntegral;
    }
    
    // Calculate derivative term
    float derivative = (error - previousError) / deltaTime;
    
    // Calculate PID output
    speedAdjustment = (config.kP * error) + 
                      (config.kI * integralError) + 
                      (config.kD * derivative);
    
    // Clamp speed adjustment to reasonable limits
    if (speedAdjustment > config.maxSpeed) {
        speedAdjustment = config.maxSpeed;
    } else if (speedAdjustment < -config.maxSpeed) {
        speedAdjustment = -config.maxSpeed;
    }
    
    // Emergency stop if too close
    if (distance < config.minDistance) {
        speedAdjustment = -config.maxSpeed; // Full brake
    }
    
    // No adjustment if too far
    if (distance > config.maxDistance) {
        speedAdjustment = 0.0f;
    }
    
    // Update state
    previousError = error;
    lastUpdateTime = currentTime;
    
    // Update status
    status.currentDistance = currentDistance;
    status.speedAdjustment = speedAdjustment;
    status.error = error;
    status.lastUpdateTime = currentTime;
}

/**
 * @brief Enable or disable the ACC system
 * @param enable True to enable, false to disable
 */
void setEnabled(bool enable) {
    if (enable && !enabled) {
        // Enabling ACC - reset PID state
        reset();
        Serial.println("[ACC] Adaptive Cruise Control ENABLED");
    } else if (!enable && enabled) {
        Serial.println("[ACC] Adaptive Cruise Control DISABLED");
    }
    
    enabled = enable;
    status.enabled = enable;
    
    if (!enabled) {
        speedAdjustment = 0.0f;
        status.speedAdjustment = 0.0f;
    }
}

/**
 * @brief Get current ACC status
 * @return Current status structure
 */
ACCStatus getStatus() {
    return status;
}

/**
 * @brief Get current ACC configuration
 * @return Current configuration structure
 */
ACCConfig getConfig() {
    return config;
}

/**
 * @brief Set ACC configuration
 * @param newConfig New configuration structure
 */
void setConfig(const ACCConfig& newConfig) {
    config = newConfig;
    
    // Update PID gains
    kP = config.kP;
    kI = config.kI;
    kD = config.kD;
    
    // Update status target distance
    status.targetDistance = config.targetDistance;
    
    Serial.println("[ACC] Configuration updated");
    Serial.print("[ACC] Target: ");
    Serial.print(config.targetDistance);
    Serial.print(" cm, kP: ");
    Serial.print(config.kP);
    Serial.print(", kI: ");
    Serial.print(config.kI);
    Serial.print(", kD: ");
    Serial.println(config.kD);
}

/**
 * @brief Get current speed adjustment value
 * @return Speed adjustment (-100 to +100, where negative means slow down/brake)
 */
float getSpeedAdjustment() {
    return speedAdjustment;
}

/**
 * @brief Reset the ACC system state
 */
void reset() {
    previousError = 0.0f;
    integralError = 0.0f;
    lastUpdateTime = millis();
    speedAdjustment = 0.0f;
    
    status.speedAdjustment = 0.0f;
    status.error = 0.0f;
    status.lastUpdateTime = lastUpdateTime;
    
    Serial.println("[ACC] System reset");
}

} // namespace AdaptiveCruise
