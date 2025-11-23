// VL53L5X Obstacle Detection System Implementation
// Complete multi-sensor ToF obstacle detection with safety features
// Author: Copilot AI Assistant
// Date: 2025-11-23

#include "obstacle_detection.h"
#include "pins.h"
#include "logger.h"
#include "system.h"
#include <Wire.h>

namespace ObstacleDetection {

// Hardware instances
static VL53L5CX sensors[NUM_OBSTACLE_SENSORS];
static VL53L5CX_ResultsData results[NUM_OBSTACLE_SENSORS];

// Sensor state
static ObstacleSensor sensorData[NUM_OBSTACLE_SENSORS];
static ObstacleConfig config;
static bool initialized = false;
static uint32_t lastUpdateMs = 0;

// External I2C mutex (shared with current.cpp)
extern SemaphoreHandle_t i2cMutex;

// PCA9548A Multiplexer functions
static bool selectMuxChannel(uint8_t channel) {
    if (channel > 7) return false;
    
    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        Wire.beginTransmission(PCA9548A_ADDR);
        Wire.write(1 << channel);
        bool success = (Wire.endTransmission() == 0);
        xSemaphoreGive(i2cMutex);
        return success;
    }
    return false;
}

// Initialize single sensor
static bool initSensor(uint8_t idx) {
    if (idx >= NUM_OBSTACLE_SENSORS) return false;
    
    // Power cycle via XSHUT
    pinMode(OBSTACLE_XSHUT_PINS[idx], OUTPUT);
    digitalWrite(OBSTACLE_XSHUT_PINS[idx], LOW);
    delay(10);
    digitalWrite(OBSTACLE_XSHUT_PINS[idx], HIGH);
    delay(INIT_DELAY_MS);
    
    // Select multiplexer channel
    if (!selectMuxChannel(idx)) {
        Logger::errorf("Obstacle: Failed to select MUX channel %d", idx);
        return false;
    }
    
    // Initialize VL53L5X
    sensors[idx].begin();
    sensors[idx].init();
    
    // Configure ranging mode
    sensors[idx].set_ranging_mode(VL53L5CX_RANGING_MODE_CONTINUOUS);
    sensors[idx].set_ranging_frequency_hz(15);  // 15Hz update
    sensors[idx].set_resolution(VL53L5CX_RESOLUTION_8X8);
    
    // Start ranging
    sensors[idx].start_ranging();
    
    sensorData[idx].enabled = config.enabled[idx];
    sensorData[idx].healthy = true;
    sensorData[idx].lastUpdateMs = millis();
    
    Logger::infof("Obstacle sensor %d (%s) initialized", idx, SENSOR_NAMES[idx]);
    return true;
}

void init() {
    Logger::info("Initializing VL53L5X obstacle detection system...");
    
    // Load config from storage (implement later)
    config.enabled[SENSOR_FRONT] = true;
    config.enabled[SENSOR_REAR] = true;
    config.enabled[SENSOR_LEFT] = true;
    config.enabled[SENSOR_RIGHT] = true;
    
    for (int i = 0; i < NUM_OBSTACLE_SENSORS; i++) {
        config.distanceOffsetMm[i] = 0;
    }
    
    config.criticalDistanceMm = DISTANCE_CRITICAL;
    config.warningDistanceMm = DISTANCE_WARNING;
    config.cautionDistanceMm = DISTANCE_CAUTION;
    
    // Initialize all sensors
    bool allOk = true;
    for (uint8_t i = 0; i < NUM_OBSTACLE_SENSORS; i++) {
        if (!initSensor(i)) {
            allOk = false;
            sensorData[i].healthy = false;
        }
    }
    
    initialized = allOk;
    if (initialized) {
        Logger::info("Obstacle detection system ready - 4 sensors active");
    } else {
        Logger::warn("Obstacle detection system partially initialized");
    }
}

void update() {
    if (!initialized) return;
    
    uint32_t now = millis();
    if (now - lastUpdateMs < UPDATE_INTERVAL_MS) return;
    lastUpdateMs = now;
    
    // Update each enabled sensor
    for (uint8_t i = 0; i < NUM_OBSTACLE_SENSORS; i++) {
        if (!sensorData[i].enabled) continue;
        
        // Select multiplexer channel
        if (!selectMuxChannel(i)) {
            sensorData[i].healthy = false;
            continue;
        }
        
        // Check if data is ready
        uint8_t isReady = 0;
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            sensors[i].check_data_ready(&isReady);
            xSemaphoreGive(i2cMutex);
        }
        
        if (!isReady) {
            // Timeout check
            if (now - sensorData[i].lastUpdateMs > MEASUREMENT_TIMEOUT_MS) {
                sensorData[i].healthy = false;
                Logger::warnf("Obstacle sensor %d timeout", i);
            }
            continue;
        }
        
        // Get results
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            sensors[i].get_ranging_data(&results[i]);
            xSemaphoreGive(i2cMutex);
        }
        
        // Process 64 zones (8x8)
        uint16_t minDist = 4000;
        for (int z = 0; z < 64; z++) {
            uint16_t dist = results[i].distance_mm[z];
            uint8_t status = results[i].target_status[z];
            
            // Apply calibration offset
            int32_t calibrated = dist + config.distanceOffsetMm[i];
            if (calibrated < 0) calibrated = 0;
            if (calibrated > 4000) calibrated = 4000;
            
            sensorData[i].zones[z].distanceMm = calibrated;
            sensorData[i].zones[z].status = status;
            sensorData[i].zones[z].valid = (status == 5 || status == 9);  // Valid target statuses
            
            if (sensorData[i].zones[z].valid && calibrated < minDist) {
                minDist = calibrated;
            }
        }
        
        sensorData[i].minDistanceMm = minDist;
        sensorData[i].lastUpdateMs = now;
        sensorData[i].healthy = true;
    }
}

const ObstacleSensor& getSensor(uint8_t sensorIdx) {
    static ObstacleSensor dummy = {};
    if (sensorIdx >= NUM_OBSTACLE_SENSORS) return dummy;
    return sensorData[sensorIdx];
}

uint16_t getMinDistance(uint8_t sensorIdx) {
    if (sensorIdx >= NUM_OBSTACLE_SENSORS) return 4000;
    return sensorData[sensorIdx].minDistanceMm;
}

ObstacleLevel getProximityLevel(uint8_t sensorIdx) {
    uint16_t dist = getMinDistance(sensorIdx);
    
    if (dist < config.criticalDistanceMm) return LEVEL_CRITICAL;
    if (dist < config.warningDistanceMm) return LEVEL_WARNING;
    if (dist < config.cautionDistanceMm) return LEVEL_CAUTION;
    return LEVEL_SAFE;
}

bool enableSensor(uint8_t sensorIdx, bool enable) {
    if (sensorIdx >= NUM_OBSTACLE_SENSORS) return false;
    sensorData[sensorIdx].enabled = enable;
    config.enabled[sensorIdx] = enable;
    return true;
}

void setDistanceOffset(uint8_t sensorIdx, int16_t offsetMm) {
    if (sensorIdx >= NUM_OBSTACLE_SENSORS) return;
    config.distanceOffsetMm[sensorIdx] = offsetMm;
}

bool isHealthy(uint8_t sensorIdx) {
    if (sensorIdx >= NUM_OBSTACLE_SENSORS) return false;
    return sensorData[sensorIdx].healthy && sensorData[sensorIdx].enabled;
}

void getStatus(ObstacleStatus& status) {
    status.systemHealthy = initialized;
    status.sensorsActive = 0;
    status.obstacleDetected = false;
    status.minDistanceMm = 4000;
    status.criticalSensor = 0xFF;
    
    for (uint8_t i = 0; i < NUM_OBSTACLE_SENSORS; i++) {
        status.sensorHealthy[i] = sensorData[i].healthy;
        if (sensorData[i].enabled && sensorData[i].healthy) {
            status.sensorsActive++;
        }
        
        uint16_t dist = sensorData[i].minDistanceMm;
        if (dist < status.minDistanceMm) {
            status.minDistanceMm = dist;
            status.criticalSensor = i;
        }
        
        if (dist < config.warningDistanceMm) {
            status.obstacleDetected = true;
        }
    }
    
    status.proximityLevel = getProximityLevel(status.criticalSensor);
    status.timestamp = millis();
}

const ObstacleConfig& getConfig() {
    return config;
}

void setConfig(const ObstacleConfig& newConfig) {
    config = newConfig;
    // Save to storage (implement later)
}

} // namespace ObstacleDetection
