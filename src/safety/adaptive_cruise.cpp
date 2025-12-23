/**
 * @file adaptive_cruise.cpp
 * @brief Adaptive Cruise Control System Implementation
 * @author FULL-FIRMWARE Team
 * @date 2025-12-22
 * 
 * Complete implementation of Adaptive Cruise Control (ACC) system with:
 * - PID controller for precise distance maintenance
 * - Person-following mode with object detection
 * - Multi-sensor fusion (ultrasonic, lidar, camera)
 * - Speed adaptation and emergency braking
 * - Smooth acceleration/deceleration profiles
 */

#include "adaptive_cruise.h"
#include <Arduino.h>
#include <math.h>

// Safety constants
#define MIN_SAFE_DISTANCE_CM 50.0f
#define MAX_DETECTION_RANGE_CM 500.0f
#define EMERGENCY_STOP_DISTANCE_CM 30.0f
#define PERSON_DETECTION_CONFIDENCE 0.75f

// PID tuning parameters
#define PID_KP 2.5f
#define PID_KI 0.1f
#define PID_KD 1.8f
#define PID_OUTPUT_MIN -100.0f
#define PID_OUTPUT_MAX 100.0f
#define PID_INTEGRAL_LIMIT 50.0f

// Speed control constants
#define MAX_CRUISE_SPEED_MS 8.0f
#define MIN_CRUISE_SPEED_MS 0.5f
#define ACCELERATION_RATE_MS2 1.5f
#define DECELERATION_RATE_MS2 3.0f
#define EMERGENCY_DECEL_RATE_MS2 6.0f

// Filtering and timing
#define SENSOR_FILTER_ALPHA 0.3f
#define CONTROL_LOOP_FREQ_HZ 50
#define CONTROL_LOOP_PERIOD_MS (1000 / CONTROL_LOOP_FREQ_HZ)

/**
 * @brief Constructor - Initializes ACC system
 */
AdaptiveCruiseControl::AdaptiveCruiseControl() :
    enabled(false),
    personFollowingMode(false),
    targetDistance(100.0f),
    currentSpeed(0.0f),
    setSpeed(0.0f),
    measuredDistance(0.0f),
    filteredDistance(0.0f),
    pidError(0.0f),
    pidIntegral(0.0f),
    pidDerivative(0.0f),
    lastError(0.0f),
    pidOutput(0.0f),
    lastUpdateTime(0),
    objectDetected(false),
    personDetected(false),
    detectionConfidence(0.0f),
    emergencyBrakeActive(false),
    speedControlOutput(0.0f)
{
    // Initialize sensor weights for fusion
    sensorWeights[0] = 0.4f;  // Ultrasonic
    sensorWeights[1] = 0.4f;  // Lidar
    sensorWeights[2] = 0.2f;  // Camera depth
}

/**
 * @brief Destructor
 */
AdaptiveCruiseControl::~AdaptiveCruiseControl() {
    disable();
}

/**
 * @brief Initialize the ACC system
 * @return true if initialization successful
 */
bool AdaptiveCruiseControl::begin() {
    Serial.println("[ACC] Initializing Adaptive Cruise Control System...");
    
    // Reset all state variables
    reset();
    
    // Initialize PID controller
    initializePID();
    
    // Set default target distance
    setTargetDistance(150.0f);
    
    Serial.println("[ACC] System initialized successfully");
    return true;
}

/**
 * @brief Initialize PID controller parameters
 */
void AdaptiveCruiseControl::initializePID() {
    pidError = 0.0f;
    pidIntegral = 0.0f;
    pidDerivative = 0.0f;
    lastError = 0.0f;
    pidOutput = 0.0f;
    
    Serial.println("[ACC] PID controller initialized");
    Serial.printf("[ACC] PID Parameters - Kp: %.2f, Ki: %.2f, Kd: %.2f\n", 
                  PID_KP, PID_KI, PID_KD);
}

/**
 * @brief Enable ACC system
 * @param speed Initial cruise speed in m/s
 */
void AdaptiveCruiseControl::enable(float speed) {
    if (speed < MIN_CRUISE_SPEED_MS || speed > MAX_CRUISE_SPEED_MS) {
        Serial.printf("[ACC] Invalid speed: %.2f m/s\n", speed);
        return;
    }
    
    enabled = true;
    setSpeed = speed;
    currentSpeed = 0.0f;
    reset();
    
    Serial.printf("[ACC] System enabled - Target speed: %.2f m/s\n", setSpeed);
}

/**
 * @brief Disable ACC system
 */
void AdaptiveCruiseControl::disable() {
    enabled = false;
    personFollowingMode = false;
    emergencyBrakeActive = false;
    speedControlOutput = 0.0f;
    
    Serial.println("[ACC] System disabled");
}

/**
 * @brief Reset ACC system state
 */
void AdaptiveCruiseControl::reset() {
    pidIntegral = 0.0f;
    pidDerivative = 0.0f;
    lastError = 0.0f;
    pidOutput = 0.0f;
    measuredDistance = MAX_DETECTION_RANGE_CM;
    filteredDistance = MAX_DETECTION_RANGE_CM;
    objectDetected = false;
    personDetected = false;
    emergencyBrakeActive = false;
    lastUpdateTime = millis();
}

/**
 * @brief Main control loop - call at regular intervals
 */
void AdaptiveCruiseControl::update() {
    if (!enabled) {
        return;
    }
    
    unsigned long currentTime = millis();
    unsigned long deltaTime = currentTime - lastUpdateTime;
    
    if (deltaTime < CONTROL_LOOP_PERIOD_MS) {
        return;  // Maintain consistent loop timing
    }
    
    float dt = deltaTime / 1000.0f;  // Convert to seconds
    lastUpdateTime = currentTime;
    
    // Sensor fusion and filtering
    updateSensorFusion();
    
    // Object and person detection
    updateObjectDetection();
    
    // Emergency brake check
    checkEmergencyConditions();
    
    if (emergencyBrakeActive) {
        applyEmergencyBrake();
        return;
    }
    
    // PID control for distance maintenance
    float pidControlOutput = calculatePID(dt);
    
    // Speed adaptation based on distance and detection
    adaptSpeed(dt, pidControlOutput);
    
    // Generate motor control output
    generateControlOutput();
}

/**
 * @brief Fuse multiple sensor readings
 */
void AdaptiveCruiseControl::updateSensorFusion() {
    // Weighted average of sensor readings
    float fusedDistance = 0.0f;
    float totalWeight = 0.0f;
    
    for (int i = 0; i < 3; i++) {
        if (sensorReadings[i] > 0 && sensorReadings[i] < MAX_DETECTION_RANGE_CM) {
            fusedDistance += sensorReadings[i] * sensorWeights[i];
            totalWeight += sensorWeights[i];
        }
    }
    
    if (totalWeight > 0) {
        measuredDistance = fusedDistance / totalWeight;
    } else {
        measuredDistance = MAX_DETECTION_RANGE_CM;
    }
    
    // Exponential moving average filter
    filteredDistance = SENSOR_FILTER_ALPHA * measuredDistance + 
                      (1.0f - SENSOR_FILTER_ALPHA) * filteredDistance;
}

/**
 * @brief Update object and person detection status
 */
void AdaptiveCruiseControl::updateObjectDetection() {
    objectDetected = (filteredDistance < MAX_DETECTION_RANGE_CM * 0.8f);
    
    if (personFollowingMode && objectDetected) {
        // In person-following mode, check detection confidence
        personDetected = (detectionConfidence >= PERSON_DETECTION_CONFIDENCE);
    } else {
        personDetected = false;
    }
}

/**
 * @brief Check for emergency conditions
 */
void AdaptiveCruiseControl::checkEmergencyConditions() {
    if (filteredDistance < EMERGENCY_STOP_DISTANCE_CM && currentSpeed > 0.1f) {
        emergencyBrakeActive = true;
        Serial.println("[ACC] EMERGENCY BRAKE ACTIVATED!");
    } else if (emergencyBrakeActive && filteredDistance > EMERGENCY_STOP_DISTANCE_CM * 1.5f) {
        emergencyBrakeActive = false;
        Serial.println("[ACC] Emergency brake deactivated");
    }
}

/**
 * @brief Apply emergency braking
 */
void AdaptiveCruiseControl::applyEmergencyBrake() {
    speedControlOutput = -100.0f;  // Full brake
    currentSpeed = max(0.0f, currentSpeed - EMERGENCY_DECEL_RATE_MS2 * 0.02f);
    
    Serial.printf("[ACC] Emergency brake - Distance: %.1f cm, Speed: %.2f m/s\n",
                  filteredDistance, currentSpeed);
}

/**
 * @brief Calculate PID control output
 * @param dt Time delta in seconds
 * @return PID control output
 */
float AdaptiveCruiseControl::calculatePID(float dt) {
    // Calculate error (desired - actual)
    pidError = targetDistance - filteredDistance;
    
    // Proportional term
    float pTerm = PID_KP * pidError;
    
    // Integral term with anti-windup
    pidIntegral += pidError * dt;
    pidIntegral = constrain(pidIntegral, -PID_INTEGRAL_LIMIT, PID_INTEGRAL_LIMIT);
    float iTerm = PID_KI * pidIntegral;
    
    // Derivative term
    pidDerivative = (pidError - lastError) / dt;
    float dTerm = PID_KD * pidDerivative;
    
    // Calculate total output
    pidOutput = pTerm + iTerm + dTerm;
    pidOutput = constrain(pidOutput, PID_OUTPUT_MIN, PID_OUTPUT_MAX);
    
    lastError = pidError;
    
    return pidOutput;
}

/**
 * @brief Adapt speed based on distance and PID output
 * @param dt Time delta in seconds
 * @param pidControl PID controller output
 */
void AdaptiveCruiseControl::adaptSpeed(float dt, float pidControl) {
    float targetSpeed = setSpeed;
    
    if (objectDetected) {
        // Reduce speed based on distance
        float distanceRatio = filteredDistance / targetDistance;
        
        if (personFollowingMode && personDetected) {
            // In person-following mode, match person's speed
            targetSpeed = setSpeed * constrain(distanceRatio, 0.2f, 1.0f);
        } else {
            // Normal ACC mode
            targetSpeed = setSpeed * constrain(distanceRatio, 0.3f, 1.0f);
        }
        
        // Apply PID correction
        if (pidControl < 0) {
            // Too close - reduce speed
            targetSpeed *= (1.0f + pidControl / 100.0f);
        }
    }
    
    // Smooth speed transition
    float speedDifference = targetSpeed - currentSpeed;
    float maxSpeedChange;
    
    if (speedDifference > 0) {
        maxSpeedChange = ACCELERATION_RATE_MS2 * dt;
    } else {
        maxSpeedChange = DECELERATION_RATE_MS2 * dt;
    }
    
    currentSpeed += constrain(speedDifference, -maxSpeedChange, maxSpeedChange);
    currentSpeed = constrain(currentSpeed, 0.0f, MAX_CRUISE_SPEED_MS);
}

/**
 * @brief Generate motor control output
 */
void AdaptiveCruiseControl::generateControlOutput() {
    // Map speed to motor output percentage
    speedControlOutput = (currentSpeed / MAX_CRUISE_SPEED_MS) * 100.0f;
    speedControlOutput = constrain(speedControlOutput, 0.0f, 100.0f);
}

/**
 * @brief Set target following distance
 * @param distance Distance in centimeters
 */
void AdaptiveCruiseControl::setTargetDistance(float distance) {
    targetDistance = constrain(distance, MIN_SAFE_DISTANCE_CM, MAX_DETECTION_RANGE_CM);
    Serial.printf("[ACC] Target distance set to: %.1f cm\n", targetDistance);
}

/**
 * @brief Set cruise speed
 * @param speed Speed in m/s
 */
void AdaptiveCruiseControl::setCruiseSpeed(float speed) {
    setSpeed = constrain(speed, MIN_CRUISE_SPEED_MS, MAX_CRUISE_SPEED_MS);
    Serial.printf("[ACC] Cruise speed set to: %.2f m/s\n", setSpeed);
}

/**
 * @brief Enable/disable person-following mode
 * @param enable True to enable
 */
void AdaptiveCruiseControl::setPersonFollowingMode(bool enable) {
    personFollowingMode = enable;
    
    if (enable) {
        Serial.println("[ACC] Person-following mode enabled");
        setTargetDistance(120.0f);  // Closer following distance
    } else {
        Serial.println("[ACC] Person-following mode disabled");
        setTargetDistance(150.0f);  // Standard distance
    }
}

/**
 * @brief Update sensor reading
 * @param sensorIndex Sensor index (0=ultrasonic, 1=lidar, 2=camera)
 * @param distance Distance reading in cm
 */
void AdaptiveCruiseControl::updateSensorReading(int sensorIndex, float distance) {
    if (sensorIndex >= 0 && sensorIndex < 3) {
        sensorReadings[sensorIndex] = distance;
    }
}

/**
 * @brief Update person detection confidence
 * @param confidence Confidence level (0.0 - 1.0)
 */
void AdaptiveCruiseControl::updatePersonDetection(float confidence) {
    detectionConfidence = constrain(confidence, 0.0f, 1.0f);
}

/**
 * @brief Get current motor control output
 * @return Motor output percentage (-100 to 100)
 */
float AdaptiveCruiseControl::getControlOutput() const {
    return speedControlOutput;
}

/**
 * @brief Get current system status
 * @return Status structure
 */
ACCStatus AdaptiveCruiseControl::getStatus() const {
    ACCStatus status;
    status.enabled = enabled;
    status.currentSpeed = currentSpeed;
    status.targetSpeed = setSpeed;
    status.measuredDistance = filteredDistance;
    status.targetDistance = targetDistance;
    status.objectDetected = objectDetected;
    status.personDetected = personDetected;
    status.emergencyBrakeActive = emergencyBrakeActive;
    status.pidError = pidError;
    status.controlOutput = speedControlOutput;
    
    return status;
}

/**
 * @brief Print debug information
 */
void AdaptiveCruiseControl::printDebugInfo() const {
    Serial.println("========== ACC Debug Info ==========");
    Serial.printf("Enabled: %s\n", enabled ? "YES" : "NO");
    Serial.printf("Person Following: %s\n", personFollowingMode ? "YES" : "NO");
    Serial.printf("Current Speed: %.2f m/s\n", currentSpeed);
    Serial.printf("Target Speed: %.2f m/s\n", setSpeed);
    Serial.printf("Measured Distance: %.1f cm\n", filteredDistance);
    Serial.printf("Target Distance: %.1f cm\n", targetDistance);
    Serial.printf("PID Error: %.2f\n", pidError);
    Serial.printf("PID Output: %.2f\n", pidOutput);
    Serial.printf("Object Detected: %s\n", objectDetected ? "YES" : "NO");
    Serial.printf("Person Detected: %s (conf: %.2f)\n", 
                  personDetected ? "YES" : "NO", detectionConfidence);
    Serial.printf("Emergency Brake: %s\n", emergencyBrakeActive ? "ACTIVE" : "INACTIVE");
    Serial.printf("Control Output: %.1f%%\n", speedControlOutput);
    Serial.println("====================================");
}
