#include "regen_ai.h"
#include "wheels.h"
#include "current.h"
#include "temperature.h"
#include "logger.h"
#include <Arduino.h>

namespace RegenAI {
    
    // Default configuration
    static Config config = {
        .enabled = true,
        .maxRegenPower = 80.0f,       // Max 80% regen power
        .batterySOCMin = 20.0f,       // Enable regen above 20% SOC
        .batterySOCMax = 95.0f,       // Disable regen above 95% SOC
        .tempMax = 45.0f,             // Max battery temp 45°C
        .aggressiveness = 0.7f        // 70% aggressiveness
    };
    
    // Current state
    static State state = {0};
    
    // Internal state
    static bool initialized = false;
    static uint32_t lastUpdateMs = 0;
    static uint32_t lastEnergyUpdateMs = 0;
    
    // Simple AI lookup table: Optimized regen power based on speed and deceleration
    // Rows: speed bins (0-10, 10-30, 30-50, 50-70, 70+ km/h)
    // Cols: decel bins (0-1, 1-2, 2-3, 3+ m/s²)
    static const float REGEN_LOOKUP[5][4] = {
        {10.0f, 20.0f, 30.0f, 40.0f},  // 0-10 km/h
        {20.0f, 35.0f, 50.0f, 65.0f},  // 10-30 km/h
        {30.0f, 50.0f, 70.0f, 85.0f},  // 30-50 km/h
        {40.0f, 60.0f, 80.0f, 95.0f},  // 50-70 km/h
        {45.0f, 65.0f, 85.0f, 100.0f}  // 70+ km/h
    };
    
    // Efficiency lookup: Expected energy recovery efficiency (%)
    static const float EFFICIENCY_LOOKUP[5][4] = {
        {40.0f, 45.0f, 50.0f, 55.0f},
        {55.0f, 60.0f, 65.0f, 70.0f},
        {65.0f, 70.0f, 75.0f, 80.0f},
        {70.0f, 75.0f, 80.0f, 82.0f},
        {72.0f, 76.0f, 80.0f, 82.0f}
    };
    
    // Get speed bin index (0-4)
    static int getSpeedBin(float speed) {
        if(speed < 10.0f) return 0;
        if(speed < 30.0f) return 1;
        if(speed < 50.0f) return 2;
        if(speed < 70.0f) return 3;
        return 4;
    }
    
    // Get decel bin index (0-3)
    static int getDecelBin(float decel) {
        decel = abs(decel);  // Make positive
        if(decel < 1.0f) return 0;
        if(decel < 2.0f) return 1;
        if(decel < 3.0f) return 2;
        return 3;
    }
    
    // Update input features from sensor readings
    static void updateFeatures() {
        // Get average speed from all wheels
        float speedSum = 0.0f;
        int validCount = 0;
        for(int i = 0; i < 4; i++) {
            if(Sensors::isWheelSensorOk(i)) {
                speedSum += Sensors::getWheelSpeed(i);
                validCount++;
            }
        }
        state.currentFeatures.speed = (validCount > 0) ? (speedSum / validCount) : 0.0f;
        
        // Estimate acceleration from speed change
        static float lastSpeed = 0.0f;
        static uint32_t lastSpeedTime = 0;
        uint32_t now = millis();
        if(now - lastSpeedTime > 0) {
            float dt = (now - lastSpeedTime) / 1000.0f;
            float dv = (state.currentFeatures.speed - lastSpeed) / 3.6f;  // Convert km/h to m/s
            state.currentFeatures.acceleration = dv / dt;
        }
        lastSpeed = state.currentFeatures.speed;
        lastSpeedTime = now;
        
        // Get battery state
        // Note: In real implementation, get actual SOC from battery management system
        // For now, estimate from voltage
        float voltage = Sensors::getVoltage(0);  // Battery voltage
        state.currentFeatures.batterySOC = constrain(
            ((voltage - 36.0f) / (42.0f - 36.0f)) * 100.0f, 
            0.0f, 100.0f
        );
        
        state.currentFeatures.batteryTemp = Sensors::getTemperature(0);
        state.currentFeatures.batteryCurrent = Sensors::getCurrent(0);
        
        // Estimate road slope from acceleration and power
        // Simplified: assume flat road for now (requires IMU for accurate slope)
        state.currentFeatures.roadSlope = 0.0f;
        
        // Get brake pedal position (0-100%)
        // Note: Implement actual brake pedal sensor reading
        state.currentFeatures.pedalPosition = 0.0f;  // Placeholder
    }
    
    void init() {
        Logger::info("Initializing AI Regenerative Braking system...");
        
        memset(&state, 0, sizeof(State));
        
        state.active = false;
        state.actualRegenPower = 0.0f;
        state.energyRecovered = 0.0f;
        state.cycleCount = 0;
        
        initialized = true;
        Logger::info("AI Regen system initialized");
    }
    
    void update() {
        if(!initialized || !config.enabled) {
            state.active = false;
            return;
        }
        
        uint32_t now = millis();
        
        // Update features
        updateFeatures();
        
        // Check if regen should be enabled
        bool canRegen = true;
        
        // Battery SOC limits
        if(state.currentFeatures.batterySOC < config.batterySOCMin || 
           state.currentFeatures.batterySOC > config.batterySOCMax) {
            canRegen = false;
        }
        
        // Battery temperature limit
        if(state.currentFeatures.batteryTemp > config.tempMax) {
            canRegen = false;
        }
        
        // Only regen when decelerating
        if(state.currentFeatures.acceleration >= -0.1f) {
            canRegen = false;
        }
        
        // Minimum speed for regen
        if(state.currentFeatures.speed < 5.0f) {
            canRegen = false;
        }
        
        if(!canRegen) {
            state.active = false;
            state.actualRegenPower = 0.0f;
            return;
        }
        
        // Evaluate optimal regen power
        state.prediction = {
            .regenPower = predictRegenPower(state.currentFeatures),
            .efficiency = 0.0f,
            .confidence = 0.8f
        };
        
        // Get efficiency estimate from lookup
        int speedBin = getSpeedBin(state.currentFeatures.speed);
        int decelBin = getDecelBin(state.currentFeatures.acceleration);
        state.prediction.efficiency = EFFICIENCY_LOOKUP[speedBin][decelBin];
        
        // Apply aggressiveness factor
        state.actualRegenPower = state.prediction.regenPower * config.aggressiveness;
        
        // Clamp to max regen power
        if(state.actualRegenPower > config.maxRegenPower) {
            state.actualRegenPower = config.maxRegenPower;
        }
        
        state.active = (state.actualRegenPower > 1.0f);
        
        // Update energy recovery statistics
        if(state.active && now - lastEnergyUpdateMs > 100) {
            float dt = (now - lastEnergyUpdateMs) / 1000.0f;
            float avgCurrent = abs(state.currentFeatures.batteryCurrent);
            float voltage = Sensors::getVoltage(0);
            float powerW = avgCurrent * voltage;
            
            updateEnergyStats(powerW, dt * 1000.0f);
            lastEnergyUpdateMs = now;
        }
        
        if(state.active) {
            state.cycleCount++;
        }
        
        lastUpdateMs = now;
    }
    
    void setEnabled(bool enabled) {
        config.enabled = enabled;
        if(!enabled) {
            state.active = false;
            state.actualRegenPower = 0.0f;
        }
        Logger::infof("AI Regen %s", enabled ? "enabled" : "disabled");
    }
    
    const State& getState() {
        return state;
    }
    
    bool isActive() {
        return state.active;
    }
    
    Config& getConfig() {
        return config;
    }
    
    void evaluateRegen() {
        update();
    }
    
    float getOptimalRegenPower() {
        if(!initialized || !config.enabled || !state.active) {
            return 0.0f;
        }
        return state.actualRegenPower;
    }
    
    float predictRegenPower(const Features& features) {
        // Use lookup table for prediction
        int speedBin = getSpeedBin(features.speed);
        int decelBin = getDecelBin(features.acceleration);
        
        float basePower = REGEN_LOOKUP[speedBin][decelBin];
        
        // Adjust based on battery SOC (reduce regen as SOC increases)
        float socFactor = 1.0f;
        if(features.batterySOC > 80.0f) {
            socFactor = 1.0f - ((features.batterySOC - 80.0f) / 20.0f) * 0.5f;
        }
        
        // Adjust based on battery temperature (reduce if hot)
        float tempFactor = 1.0f;
        if(features.batteryTemp > 35.0f) {
            tempFactor = 1.0f - ((features.batteryTemp - 35.0f) / 10.0f) * 0.3f;
        }
        
        float finalPower = basePower * socFactor * tempFactor;
        
        return constrain(finalPower, 0.0f, 100.0f);
    }
    
    void updateEnergyStats(float powerW, float durationMs) {
        // Convert to Wh: (W * ms) / (1000 ms/s * 3600 s/h)
        float energyWh = (powerW * durationMs) / 3600000.0f;
        state.energyRecovered += energyWh;
    }
}

// 🔒 v2.5.0: Estado de inicialización
bool RegenAI::initOK() {
    return initialized;
}
