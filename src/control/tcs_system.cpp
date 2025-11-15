#include "tcs_system.h"
#include "wheels.h"
#include "steering.h"
#include "logger.h"
#include "alerts.h"
#include <cmath>

namespace TCSSystem {

static Config config = {
    .enabled = true,
    .slipThreshold = 15.0f,       // 15% slip triggers TCS
    .minSpeedKmh = 3.0f,           // Active above 3 km/h
    .aggressiveReduction = 40.0f,  // 40% power cut on detection
    .smoothReduction = 5.0f,       // 5% gradual reduction
    .recoveryRatePerSec = 25.0f,   // 25%/s power recovery
    .enabled4WD = true             // Differential control enabled
};

static State state;
static bool initialized = false;
static uint32_t lastUpdateMs = 0;

// Helper: clamp float value
static inline float clampf(float v, float lo, float hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// Calculate vehicle reference speed (average of slowest wheels)
static float calculateVehicleSpeed(const float wheelSpeeds[4]) {
    float minSpeed = 999.0f;
    int validCount = 0;
    
    for (int i = 0; i < 4; i++) {
        if (wheelSpeeds[i] > 0.1f && Sensors::isWheelSensorOk(i)) {
            if (wheelSpeeds[i] < minSpeed) {
                minSpeed = wheelSpeeds[i];
            }
            validCount++;
        }
    }
    
    // Use lowest wheel speed as reference (closest to ground truth)
    return (validCount > 0 && minSpeed < 999.0f) ? minSpeed : 0.0f;
}

// Calculate slip ratio for a wheel during acceleration
static float calculateSlipRatio(float wheelSpeed, float vehicleSpeed) {
    if (vehicleSpeed < 0.1f) return 0.0f;
    
    float slip = ((wheelSpeed - vehicleSpeed) / vehicleSpeed) * 100.0f;
    return clampf(slip, 0.0f, 100.0f);
}

// Estimate lateral G force from steering angle and speed
static float estimateLateralG(float speedKmh, float steeringDeg) {
    if (speedKmh < 5.0f) return 0.0f;
    
    // Simplified lateral acceleration calculation
    // a = v² / r, where r is turn radius
    float speedMs = speedKmh / 3.6f;
    float angleRad = (steeringDeg * M_PI) / 180.0f;
    
    // Approximate turn radius from steering angle (simplified)
    float turnRadius = 3.0f / (tan(fabs(angleRad)) + 0.001f); // wheelbase ~3m
    
    float lateralAccel = (speedMs * speedMs) / turnRadius;
    float lateralG = lateralAccel / 9.81f;
    
    return clampf(lateralG, 0.0f, 1.5f);
}

void init() {
    state = {};
    state.systemActive = false;
    state.totalActivations = 0;
    state.avgSlipRatio = 0.0f;
    state.vehicleSpeed = 0.0f;
    state.lateralG = 0.0f;
    state.efficiencyGain = 0.0f;
    
    for (int i = 0; i < 4; i++) {
        state.wheels[i].active = false;
        state.wheels[i].slipRatio = 0.0f;
        state.wheels[i].powerReduction = 0.0f;
        state.wheels[i].lastActiveMs = 0;
        state.wheels[i].lastActivationMs = 0;
        state.wheels[i].activationCount = 0;
    }
    
    lastUpdateMs = millis();
    initialized = true;
    
    Logger::infof("TCS initialized - Threshold: %.1f%%, MinSpeed: %.1f km/h", 
                  config.slipThreshold, config.minSpeedKmh);
}

void update() {
    if (!initialized || !config.enabled) {
        state.systemActive = false;
        return;
    }
    
    uint32_t now = millis();
    float deltaTime = (now - lastUpdateMs) / 1000.0f; // seconds
    lastUpdateMs = now;
    
    if (deltaTime <= 0.0f || deltaTime > 1.0f) deltaTime = 0.02f; // Guard
    
    // Get wheel speeds from sensors
    float wheelSpeeds[4];
    for (int i = 0; i < 4; i++) {
        wheelSpeeds[i] = Sensors::getWheelSpeed(i);
    }
    
    // Calculate reference vehicle speed
    float vehicleSpeed = calculateVehicleSpeed(wheelSpeeds);
    state.vehicleSpeed = vehicleSpeed;
    
    // Get steering angle
    float steeringAngle = Steering::get().angleDeg;
    state.lateralG = estimateLateralG(vehicleSpeed, steeringAngle);
    
    // Check if speed is above minimum threshold
    if (vehicleSpeed < config.minSpeedKmh) {
        state.systemActive = false;
        for (int i = 0; i < 4; i++) {
            state.wheels[i].active = false;
            state.wheels[i].powerReduction = 0.0f;
        }
        return;
    }
    
    // Update slip ratio and TCS state for each wheel
    bool anyWheelActive = false;
    float totalSlip = 0.0f;
    int validWheels = 0;
    
    for (int i = 0; i < 4; i++) {
        if (!Sensors::isWheelSensorOk(i)) continue;
        
        float slip = calculateSlipRatio(wheelSpeeds[i], vehicleSpeed);
        state.wheels[i].slipRatio = slip;
        totalSlip += slip;
        validWheels++;
        
        // Check if TCS should activate on this wheel
        if (slip > config.slipThreshold) {
            if (!state.wheels[i].active) {
                // TCS activation - aggressive initial reduction
                state.wheels[i].active = true;
                state.wheels[i].powerReduction = config.aggressiveReduction;
                state.wheels[i].lastActivationMs = now;
                state.wheels[i].activationCount++;
                state.totalActivations++;
                
                Logger::infof("TCS activated on wheel %d (slip: %.1f%%)", i, slip);
                Alerts::play(Audio::AUDIO_MODULO_OK);
            } else {
                // Already active - apply smooth reduction
                state.wheels[i].powerReduction += config.smoothReduction;
                state.wheels[i].powerReduction = clampf(state.wheels[i].powerReduction, 0.0f, 80.0f);
            }
            
            // Extra reduction in corners (high lateral G)
            if (state.lateralG > 0.3f) {
                state.wheels[i].powerReduction += 5.0f;
                state.wheels[i].powerReduction = clampf(state.wheels[i].powerReduction, 0.0f, 80.0f);
            }
            
            anyWheelActive = true;
        } else {
            // Gradually recover power if slip is under control
            if (state.wheels[i].powerReduction > 0.0f) {
                float recovery = config.recoveryRatePerSec * deltaTime;
                state.wheels[i].powerReduction -= recovery;
                state.wheels[i].powerReduction = clampf(state.wheels[i].powerReduction, 0.0f, 100.0f);
            }
            
            // Deactivate TCS if power fully recovered
            if (state.wheels[i].active && state.wheels[i].powerReduction < 1.0f) {
                state.wheels[i].active = false;
                Logger::infof("TCS deactivated on wheel %d", i);
            }
        }
    }
    
    state.systemActive = anyWheelActive;
    state.avgSlipRatio = (validWheels > 0) ? (totalSlip / validWheels) : 0.0f;
    
    // Estimate efficiency gain (simplified)
    if (state.systemActive) {
        state.efficiencyGain += 0.1f * deltaTime; // Accumulate gain
    }
    state.efficiencyGain = clampf(state.efficiencyGain, 0.0f, 100.0f);
}

void setEnabled(bool enabled) {
    config.enabled = enabled;
    Logger::infof("TCS %s", enabled ? "enabled" : "disabled");
}

const State& getState() {
    return state;
}

bool isActive() {
    return state.systemActive;
}

Config& getConfig() {
    return config;
}

float modulatePower(int wheel, float powerDemand) {
    if (!initialized || !config.enabled || wheel < 0 || wheel >= 4) {
        return powerDemand;
    }
    
    // If TCS is active on this wheel, reduce power
    if (state.wheels[wheel].powerReduction > 0.0f) {
        float reduction = state.wheels[wheel].powerReduction / 100.0f;
        return powerDemand * (1.0f - reduction);
    }
    
    return powerDemand;
}

void setDriveMode(uint8_t mode) {
    // Adjust TCS aggressiveness based on drive mode
    switch (mode) {
        case 0: // Eco
            config.slipThreshold = 10.0f;  // More aggressive TCS
            config.aggressiveReduction = 50.0f;
            break;
        case 1: // Normal
            config.slipThreshold = 15.0f;
            config.aggressiveReduction = 40.0f;
            break;
        case 2: // Sport
            config.slipThreshold = 20.0f;  // More permissive
            config.aggressiveReduction = 30.0f;
            break;
    }
    Logger::infof("TCS drive mode set to %d", mode);
}

} // namespace TCSSystem
