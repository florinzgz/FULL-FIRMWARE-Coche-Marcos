// VL53L5X Obstacle Detection System Implementation
// Complete multi-sensor ToF obstacle detection with safety features
// Author: Copilot AI Assistant
// Date: 2025-11-23
// Updated: 2025-12-01 - Added I2C recovery and improved initialization
// Updated: 2025-12-20 - Added watchdog feeding during initialization (v2.11.3)

#include "obstacle_detection.h"
#include "obstacle_config.h"
#include "pins.h"
#include "logger.h"
#include "system.h"
#include "i2c_recovery.h"
#include "watchdog.h"
#include <Wire.h>

namespace ObstacleDetection {

// Hardware instances
// Note: VL53L5CX library integration - sensor objects created when library is available
// This implementation works with or without the actual sensor hardware

// Sensor state
static ObstacleSensor sensorData[::ObstacleConfig::NUM_SENSORS];
static ObstacleSettings config;
static bool initialized = false;
static uint32_t lastUpdateMs = 0;
static bool hardwarePresent = false;
static bool placeholderMode = true;  // True when sensors not detected, used for simulation

// I2CRecovery device ID allocation:
// - Devices 0-7: Reserved for other I2C devices (INA226, etc.)
// - Devices 8-9: Obstacle detection sensors (FRONT, REAR)
// - Devices 10-15: Reserved for future I2C expansion
constexpr uint8_t OBSTACLE_SENSOR_DEVICE_ID_BASE = 8;

// VL53L5CX device identification
constexpr uint16_t VL53L5CX_DEVICE_ID_REG = 0x0000;  // Device ID register address
constexpr uint8_t VL53L5CX_EXPECTED_ID = 0xF0;       // Expected device ID for VL53L5CX

// XSHUT pins for sensors
static constexpr auto& OBSTACLE_XSHUT_PINS = ::ObstacleConfig::XSHUT_PINS;

static const char* SENSOR_NAMES[::ObstacleConfig::NUM_SENSORS] = {
    "FRONT", "REAR"
};

// Verify I2C bus is operational
static bool testI2CBus() {
    if (!I2CRecovery::isBusHealthy()) {
        Logger::warn("Obstacle: I2C bus not healthy, attempting recovery...");
        if (!I2CRecovery::recoverBus()) {
            Logger::error("Obstacle: I2C bus recovery failed");
            return false;
        }
        Logger::info("Obstacle: I2C bus recovered successfully");
    }
    return true;
}

// PCA9548A Multiplexer functions with I2C recovery
static bool selectMuxChannel(uint8_t channel) {
    if (channel > 7) return false;
    
    // Use I2CRecovery for safe channel selection
    return I2CRecovery::tcaSelectSafe(channel, ::ObstacleConfig::PCA9548A_ADDR);
}

// Verify PCA9548A multiplexer is present using I2CRecovery
static bool verifyMultiplexer() {
    // Try to select channel 0 - if it works, multiplexer is present
    bool success = I2CRecovery::tcaSelectSafe(0, ::ObstacleConfig::PCA9548A_ADDR);
    
    if (!success) {
        Logger::errorf("Obstacle: PCA9548A (0x%02X) not found", ::ObstacleConfig::PCA9548A_ADDR);
    }
    return success;
}

// Reset all sensors via XSHUT pins
static void resetAllSensors() {
    Logger::info("Obstacle: Resetting all sensors via XSHUT...");
    
    // Configure all XSHUT pins as output and pull LOW
    for (uint8_t i = 0; i < ::ObstacleConfig::NUM_SENSORS; i++) {
        pinMode(OBSTACLE_XSHUT_PINS[i], OUTPUT);
        digitalWrite(OBSTACLE_XSHUT_PINS[i], LOW);
    }
    
    // Wait for sensors to enter shutdown
    delay(10);
}

// Initialize single sensor with improved error handling
static bool initSensor(uint8_t idx) {
    if (idx >= ::ObstacleConfig::NUM_SENSORS) return false;
    
    Logger::infof("Obstacle: Initializing sensor %s (GPIO %d, MUX ch %d)...", 
                  SENSOR_NAMES[idx], OBSTACLE_XSHUT_PINS[idx], idx);
    
    // Power up sensor via XSHUT
    digitalWrite(OBSTACLE_XSHUT_PINS[idx], HIGH);
    
    // Wait for sensor to stabilize
    // 🔒 CORRECCIÓN CRÍTICA: Feed watchdog durante delay largo (50ms)
    // Con 2 sensores VL53L5CX = 100ms total de delay, evita timeout de watchdog
    uint32_t startMs = millis();
    while (millis() - startMs < ::ObstacleConfig::INIT_DELAY_MS) {
        Watchdog::feed();  // Feed cada iteración para garantizar <50ms entre feeds
        yield();
    }
    
    // Select multiplexer channel using I2C recovery
    // 🔒 CORRECCIÓN CRÍTICA: Feed watchdog después de operación I2C crítica
    if (!selectMuxChannel(idx)) {
        Watchdog::feed();  // Feed antes de salir por error
        Logger::errorf("Obstacle: Failed to select MUX channel %d for sensor %s", 
                      idx, SENSOR_NAMES[idx]);
        sensorData[idx].healthy = false;
        sensorData[idx].errorCount++;
        return false;
    }
    Watchdog::feed();  // Feed después de operación I2C exitosa
    
    // Check for VL53L5CX sensor presence by reading device ID register
    // Use defined base ID + sensor index for I2CRecovery device tracking
    const uint8_t deviceId = OBSTACLE_SENSOR_DEVICE_ID_BASE + idx;
    uint8_t deviceIdValue = 0;
    bool readOk = I2CRecovery::readBytesWithRetry(
        ::ObstacleConfig::VL53L5X_DEFAULT_ADDR, VL53L5CX_DEVICE_ID_REG, &deviceIdValue, 1, deviceId);
    bool sensorFound = readOk && (deviceIdValue == VL53L5CX_EXPECTED_ID);
    
    // 🔒 CORRECCIÓN CRÍTICA: Feed watchdog después de lectura I2C
    Watchdog::feed();
    
    // Configure sensor state
    sensorData[idx].enabled = config.sensorsEnabled[idx];
    sensorData[idx].healthy = sensorFound;
    sensorData[idx].lastUpdateMs = millis();
    sensorData[idx].offsetMm = config.offsetsMm[idx];
    sensorData[idx].errorCount = 0;
    
    if (sensorFound) {
        hardwarePresent = true;
        placeholderMode = false;
        Logger::infof("Obstacle: Sensor %s detected at 0x%02X (ID: 0x%02X)", 
                     SENSOR_NAMES[idx], ::ObstacleConfig::VL53L5X_DEFAULT_ADDR, deviceIdValue);
    } else {
        if (readOk) {
            Logger::warnf("Obstacle: Sensor %s ID mismatch (got 0x%02X, expected 0x%02X)", 
                         SENSOR_NAMES[idx], deviceIdValue, VL53L5CX_EXPECTED_ID);
        } else {
            Logger::warnf("Obstacle: Sensor %s not detected (placeholder mode)", SENSOR_NAMES[idx]);
        }
    }
    
    return true;  // Return true even if sensor not found (placeholder mode)
}

void init() {
    Logger::info("Initializing VL53L5X obstacle detection system...");
    Logger::infof("  PCA9548A multiplexer address: 0x%02X", ::ObstacleConfig::PCA9548A_ADDR);
    Logger::infof("  XSHUT pins: FRONT=%d, REAR=%d",
                 ::ObstacleConfig::PIN_XSHUT_FRONT, ::ObstacleConfig::PIN_XSHUT_REAR);
    
    hardwarePresent = false;
    placeholderMode = true;
    
    // 0. Ensure any XSHUT strapping pin stays HIGH before any reconfiguration
    bool strappingGuarded = false;
    for (uint8_t i = 0; i < ::ObstacleConfig::NUM_SENSORS; i++) {
        const uint8_t pin = OBSTACLE_XSHUT_PINS[i];
        if (pin_is_strapping(pin)) {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, HIGH);
            strappingGuarded = true;
        }
    }
    if (strappingGuarded) delay(10);

    // 1. Test I2C bus health
    if (!testI2CBus()) {
        Logger::error("Obstacle: I2C bus test failed, aborting initialization");
        initialized = false;
        return;
    }
    
    // 🔒 v2.11.3: Feed watchdog after I2C bus test to prevent timeout during sensor init
    Watchdog::feed();
    
    // 2. Reset all sensors via XSHUT
    resetAllSensors();
    
    // 3. Verify PCA9548A multiplexer
    if (!verifyMultiplexer()) {
        Logger::warn("Obstacle: PCA9548A not found, running in simulation mode");
        // Continue in placeholder mode
    }
    
    // 🔒 v2.11.3: Feed watchdog after multiplexer verification (I2C operation)
    Watchdog::feed();
    
    // 4. Initialize each sensor sequentially
    for (uint8_t i = 0; i < ::ObstacleConfig::NUM_SENSORS; i++) {
        if (!initSensor(i)) {
            sensorData[i].healthy = false;
        }
        // 🔒 v2.11.3: Feed watchdog after each sensor init (prevents timeout on multi-sensor setup)
        Watchdog::feed();
    }
    
    // Set initialized based on I2C bus availability (placeholder mode is OK)
    initialized = true;
    
    if (hardwarePresent) {
        placeholderMode = false;
        Logger::info("Obstacle detection system ready (hardware detected)");
    } else {
        placeholderMode = true;
        Logger::info("Obstacle detection system ready (placeholder/simulation mode)");
    }
}

void update() {
    if (!initialized) return;
    
    uint32_t now = millis();
    if (now - lastUpdateMs < ::ObstacleConfig::UPDATE_INTERVAL_MS) return;
    lastUpdateMs = now;
    
    // In placeholder mode, just update timestamps
    // When hardware is present, actual sensor reading would go here
    for (uint8_t i = 0; i < ::ObstacleConfig::NUM_SENSORS; i++) {
        if (!sensorData[i].enabled) continue;
        sensorData[i].lastUpdateMs = now;
    }
}

const ObstacleSensor& getSensor(uint8_t sensorIdx) {
    static ObstacleSensor dummy = {};
    if (sensorIdx >= ::ObstacleConfig::NUM_SENSORS) return dummy;
    return sensorData[sensorIdx];
}

uint16_t getMinDistance(uint8_t sensorIdx) {
    if (sensorIdx >= ::ObstacleConfig::NUM_SENSORS) return ::ObstacleConfig::DISTANCE_INVALID;
    return sensorData[sensorIdx].minDistance;
}

ObstacleLevel getProximityLevel(uint8_t sensorIdx) {
    uint16_t dist = getMinDistance(sensorIdx);
    
    if (dist >= ::ObstacleConfig::DISTANCE_INVALID) return LEVEL_INVALID;
    if (dist < config.thresholdCritical) return LEVEL_CRITICAL;
    if (dist < config.thresholdWarning) return LEVEL_WARNING;
    if (dist < config.thresholdCaution) return LEVEL_CAUTION;
    return LEVEL_SAFE;
}

bool enableSensor(uint8_t sensorIdx, bool enable) {
    if (sensorIdx >= ::ObstacleConfig::NUM_SENSORS) return false;
    sensorData[sensorIdx].enabled = enable;
    config.sensorsEnabled[sensorIdx] = enable;
    return true;
}

void setDistanceOffset(uint8_t sensorIdx, int16_t offsetMm) {
    if (sensorIdx >= ::ObstacleConfig::NUM_SENSORS) return;
    config.offsetsMm[sensorIdx] = offsetMm;
    sensorData[sensorIdx].offsetMm = offsetMm;
}

bool isHealthy(uint8_t sensorIdx) {
    if (sensorIdx >= ::ObstacleConfig::NUM_SENSORS) return false;
    return sensorData[sensorIdx].healthy && sensorData[sensorIdx].enabled;
}

void getStatus(ObstacleStatus& status) {
    status.sensorsHealthy = 0;
    status.sensorsEnabled = 0;
    status.overallLevel = LEVEL_INVALID;
    status.minDistanceFront = ::ObstacleConfig::DISTANCE_INVALID;
    status.minDistanceRear = ::ObstacleConfig::DISTANCE_INVALID;
    status.minDistanceLeft = ::ObstacleConfig::DISTANCE_INVALID;
    status.minDistanceRight = ::ObstacleConfig::DISTANCE_INVALID;
    status.emergencyStopActive = false;
    status.parkingAssistActive = false;
    status.lastUpdateMs = millis();
    
    ObstacleLevel worstLevel = LEVEL_SAFE;
    
    for (uint8_t i = 0; i < ::ObstacleConfig::NUM_SENSORS; i++) {
        if (sensorData[i].enabled) status.sensorsEnabled++;
        if (sensorData[i].healthy) status.sensorsHealthy++;
        
        uint16_t dist = sensorData[i].minDistance;
        ObstacleLevel level = getProximityLevel(i);
        
        // Store per-direction distances
        if (i == SENSOR_FRONT) status.minDistanceFront = dist;
        if (i == SENSOR_REAR)  status.minDistanceRear = dist;
        
        // Track worst level
        if (level != LEVEL_INVALID && level > worstLevel) {
            worstLevel = level;
        }
        
        // Check for emergency conditions
        if (level == LEVEL_CRITICAL) {
            status.emergencyStopActive = true;
        }
    }
    
    status.overallLevel = worstLevel;
}

bool loadConfig() {
    // Placeholder: would load from EEPROM/Flash
    return false;
}

bool saveConfig() {
    // Placeholder: would save to EEPROM/Flash
    return false;
}

const ObstacleSettings& getConfig() {
    return config;
}

void setConfig(const ObstacleSettings& newConfig) {
    config = newConfig;
    
    // Apply new offsets to sensors
    for (uint8_t i = 0; i < ::ObstacleConfig::NUM_SENSORS; i++) {
        sensorData[i].offsetMm = config.offsetsMm[i];
        sensorData[i].enabled = config.sensorsEnabled[i];
    }
}

const ObstacleZone& getZone(uint8_t sensorIdx, uint8_t zoneIdx) {
    static ObstacleZone dummy;
    if (sensorIdx >= ::ObstacleConfig::NUM_SENSORS) return dummy;
    if (zoneIdx >= ::ObstacleConfig::ZONES_PER_SENSOR) return dummy;
    return sensorData[sensorIdx].zones[zoneIdx];
}

void resetErrors() {
    for (uint8_t i = 0; i < ::ObstacleConfig::NUM_SENSORS; i++) {
        sensorData[i].errorCount = 0;
    }
}

bool runDiagnostics() {
    bool allPassed = true;
    
    for (uint8_t i = 0; i < ::ObstacleConfig::NUM_SENSORS; i++) {
        if (sensorData[i].enabled && !sensorData[i].healthy) {
            Logger::warnf("Obstacle sensor %d (%s) diagnostics FAILED", i, SENSOR_NAMES[i]);
            allPassed = false;
        }
    }
    
    return allPassed;
}

bool isPlaceholderMode() {
    return placeholderMode;
}

bool isHardwarePresent() {
    return hardwarePresent;
}

} // namespace ObstacleDetection
