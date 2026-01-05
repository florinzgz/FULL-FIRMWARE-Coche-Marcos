// TOFSense-M S Obstacle Detection System Implementation
// Single LiDAR sensor via UART protocol
// Author: Copilot AI Assistant
// Date: 2026-01-05
// Version: 2.12.0 - Migrated from VL53L5X I2C to TOFSense-M S UART
//
// Protocol Reference: TOFSense-M_User_Manual_V1.4_en.pdf
// https://ftp.nooploop.com/software/products/tofsense_m/doc/TOFSense-M_User_Manual_V1.4_en.pdf
//
// Packet Format (9 bytes):
// [0] Header: 0x57
// [1] Function: 0x00 (distance measurement)
// [2] Length: 0x02 (2 bytes of data)
// [3] Data_L: Distance low byte
// [4] Data_H: Distance high byte
// [5-7] Reserved/Additional data
// [8] Checksum: Sum of bytes [0-7] & 0xFF
//
// Distance = (Data_H << 8) | Data_L (in millimeters)

#include "obstacle_detection.h"
#include "obstacle_config.h"
#include "pins.h"
#include "logger.h"
#include "system.h"
#include "watchdog.h"
#include <HardwareSerial.h>

namespace ObstacleDetection {

// Hardware instances
static HardwareSerial TOFSerial(ObstacleConfig::UART_NUM);  // UART1 for TOFSense-M S

// Sensor state
static ObstacleSensor sensorData[ObstacleConfig::NUM_SENSORS];
static ObstacleSettings config;
static bool initialized = false;
static uint32_t lastUpdateMs = 0;
static bool hardwarePresent = false;
static bool placeholderMode = true;

// UART packet buffer
static uint8_t packetBuffer[ObstacleConfig::PACKET_LENGTH];
static uint8_t bufferIndex = 0;
static uint32_t lastPacketMs = 0;

// Helper function to calculate checksum
static uint8_t calculateChecksum(const uint8_t* data, uint8_t length) {
    uint8_t sum = 0;
    for (uint8_t i = 0; i < length; i++) {
        sum += data[i];
    }
    return sum;  // uint8_t automatically wraps at 255
}

// Helper function to validate and parse packet
static bool parsePacket(uint16_t& distance) {
    // Validate header
    if (packetBuffer[ObstacleConfig::POS_HEADER] != ObstacleConfig::PACKET_HEADER) {
        Logger::warn("TOFSense: Invalid packet header");
        return false;
    }
    
    // Validate function code
    if (packetBuffer[ObstacleConfig::POS_FUNCTION] != ObstacleConfig::PACKET_FUNC_DISTANCE) {
        Logger::warnf("TOFSense: Unexpected function code: 0x%02X", 
                     packetBuffer[ObstacleConfig::POS_FUNCTION]);
        return false;
    }
    
    // Validate length
    if (packetBuffer[ObstacleConfig::POS_LENGTH] != ObstacleConfig::PACKET_DATA_LENGTH) {
        Logger::warnf("TOFSense: Invalid data length: %d", 
                     packetBuffer[ObstacleConfig::POS_LENGTH]);
        return false;
    }
    
    // Validate checksum
    uint8_t expectedChecksum = calculateChecksum(packetBuffer, ObstacleConfig::POS_CHECKSUM);
    uint8_t receivedChecksum = packetBuffer[ObstacleConfig::POS_CHECKSUM];
    
    if (expectedChecksum != receivedChecksum) {
        Logger::warnf("TOFSense: Checksum mismatch (expected: 0x%02X, got: 0x%02X)", 
                     expectedChecksum, receivedChecksum);
        System::logError(ObstacleConfig::ERROR_CODE_CHECKSUM);
        return false;
    }
    
    // Extract distance (low byte + high byte)
    distance = (packetBuffer[ObstacleConfig::POS_DATA_H] << 8) | 
                packetBuffer[ObstacleConfig::POS_DATA_L];
    
    return true;
}

// Update sensor data with new distance reading
static void updateSensorData(uint8_t sensorIdx, uint16_t distanceMm) {
    if (sensorIdx >= ObstacleConfig::NUM_SENSORS) return;
    
    ObstacleSensor& sensor = sensorData[sensorIdx];
    
    // Apply calibration offset
    int32_t calibratedDistance = static_cast<int32_t>(distanceMm) + sensor.offsetMm;
    if (calibratedDistance < 0) calibratedDistance = 0;
    if (calibratedDistance > ObstacleConfig::DISTANCE_MAX) {
        calibratedDistance = ObstacleConfig::DISTANCE_INVALID;
    }
    
    // Update sensor state
    sensor.minDistance = static_cast<uint16_t>(calibratedDistance);
    sensor.lastUpdateMs = millis();
    sensor.healthy = true;
    sensor.errorCount = 0;
    
    // Update single zone data
    ObstacleZone& zone = sensor.zones[0];
    zone.distanceMm = sensor.minDistance;
    zone.valid = (sensor.minDistance < ObstacleConfig::DISTANCE_INVALID);
    zone.confidence = 100;  // TOFSense-M S doesn't provide confidence, assume 100%
    
    // Determine proximity level
    if (sensor.minDistance >= ObstacleConfig::DISTANCE_INVALID) {
        sensor.proximityLevel = LEVEL_INVALID;
        zone.level = LEVEL_INVALID;
    } else if (sensor.minDistance < config.thresholdCritical) {
        sensor.proximityLevel = LEVEL_CRITICAL;
        zone.level = LEVEL_CRITICAL;
    } else if (sensor.minDistance < config.thresholdWarning) {
        sensor.proximityLevel = LEVEL_WARNING;
        zone.level = LEVEL_WARNING;
    } else if (sensor.minDistance < config.thresholdCaution) {
        sensor.proximityLevel = LEVEL_CAUTION;
        zone.level = LEVEL_CAUTION;
    } else {
        sensor.proximityLevel = LEVEL_SAFE;
        zone.level = LEVEL_SAFE;
    }
}

void init() {
    Logger::info("Initializing TOFSense-M S obstacle detection system...");
    Logger::infof("  UART%d (native): RX=GPIO%d, TX=GPIO%d, Baudrate=%d", 
                 ObstacleConfig::UART_NUM,
                 PIN_TOFSENSE_RX, 
                 PIN_TOFSENSE_TX,
                 ObstacleConfig::UART_BAUDRATE);
    
    hardwarePresent = false;
    placeholderMode = true;
    bufferIndex = 0;
    lastPacketMs = 0;
    
    // Initialize UART0 for TOFSense-M S (native UART pins)
    // Note: Only RX is needed as sensor only transmits data
    TOFSerial.begin(ObstacleConfig::UART_BAUDRATE, SERIAL_8N1, 
                    PIN_TOFSENSE_RX, PIN_TOFSENSE_TX);
    
    // Wait for UART to stabilize
    delay(100);
    Watchdog::feed();
    
    // Initialize sensor data
    for (uint8_t i = 0; i < ObstacleConfig::NUM_SENSORS; i++) {
        sensorData[i].enabled = config.sensorsEnabled[i];
        sensorData[i].offsetMm = config.offsetsMm[i];
        sensorData[i].healthy = false;
        sensorData[i].lastUpdateMs = 0;
        sensorData[i].errorCount = 0;
        sensorData[i].minDistance = ObstacleConfig::DISTANCE_INVALID;
        sensorData[i].proximityLevel = LEVEL_INVALID;
    }
    
    // Try to read initial data to verify sensor presence
    // Give sensor some time to start sending data
    uint32_t startMs = millis();
    bool dataReceived = false;
    
    while (millis() - startMs < ObstacleConfig::SENSOR_DETECTION_TIMEOUT_MS && !dataReceived) {
        if (TOFSerial.available() > 0) {
            dataReceived = true;
            hardwarePresent = true;
            placeholderMode = false;
            Logger::info("TOFSense-M S sensor detected on UART0");
            break;
        }
        delay(10);
        Watchdog::feed();
    }
    
    if (!hardwarePresent) {
        Logger::warn("TOFSense-M S sensor not detected, running in simulation mode");
        placeholderMode = true;
    }
    
    initialized = true;
    Logger::info("Obstacle detection system ready");
}

void update() {
    if (!initialized) return;
    
    uint32_t now = millis();
    
    // In placeholder mode, just update timestamps
    if (placeholderMode) {
        if (now - lastUpdateMs < ObstacleConfig::UPDATE_INTERVAL_MS) return;
        lastUpdateMs = now;
        
        for (uint8_t i = 0; i < ObstacleConfig::NUM_SENSORS; i++) {
            if (!sensorData[i].enabled) continue;
            sensorData[i].lastUpdateMs = now;
        }
        return;
    }
    
    // Read available UART data
    while (TOFSerial.available() > 0) {
        uint8_t byte = TOFSerial.read();
        
        // Look for packet header
        if (bufferIndex == 0) {
            if (byte == ObstacleConfig::PACKET_HEADER) {
                packetBuffer[bufferIndex++] = byte;
            }
            // Discard bytes until we find a header
            continue;
        }
        
        // Accumulate packet bytes
        packetBuffer[bufferIndex++] = byte;
        
        // Check if we have a complete packet
        if (bufferIndex >= ObstacleConfig::PACKET_LENGTH) {
            uint16_t distance = 0;
            
            if (parsePacket(distance)) {
                // Valid packet received
                updateSensorData(SENSOR_FRONT, distance);
                lastPacketMs = now;
                
                // Log periodic readings
                static uint32_t lastLogMs = 0;
                if (now - lastLogMs > ObstacleConfig::LOG_INTERVAL_MS) {
                    Logger::infof("TOFSense: Distance = %u mm", distance);
                    lastLogMs = now;
                }
            } else {
                // Invalid packet
                sensorData[SENSOR_FRONT].errorCount++;
                if (sensorData[SENSOR_FRONT].errorCount > ObstacleConfig::MAX_CONSECUTIVE_ERRORS) {
                    sensorData[SENSOR_FRONT].healthy = false;
                    Logger::warn("TOFSense: Too many errors, marking sensor unhealthy");
                }
            }
            
            // Reset buffer for next packet
            bufferIndex = 0;
        }
    }
    
    // Check for timeout (no data received for a while)
    if (now - lastPacketMs > ObstacleConfig::UART_READ_TIMEOUT_MS && lastPacketMs > 0) {
        sensorData[SENSOR_FRONT].healthy = false;
        sensorData[SENSOR_FRONT].minDistance = ObstacleConfig::DISTANCE_INVALID;
        sensorData[SENSOR_FRONT].proximityLevel = LEVEL_INVALID;
        
        static uint32_t lastTimeoutLog = 0;
        if (now - lastTimeoutLog > ObstacleConfig::TIMEOUT_LOG_INTERVAL_MS) {
            Logger::warn("TOFSense: Communication timeout");
            System::logError(ObstacleConfig::ERROR_CODE_TIMEOUT);
            lastTimeoutLog = now;
        }
    }
    
    lastUpdateMs = now;
}

const ObstacleSensor& getSensor(uint8_t sensorIdx) {
    static ObstacleSensor dummy = {};
    if (sensorIdx >= ObstacleConfig::NUM_SENSORS) return dummy;
    return sensorData[sensorIdx];
}

uint16_t getMinDistance(uint8_t sensorIdx) {
    if (sensorIdx >= ObstacleConfig::NUM_SENSORS) return ObstacleConfig::DISTANCE_INVALID;
    return sensorData[sensorIdx].minDistance;
}

ObstacleLevel getProximityLevel(uint8_t sensorIdx) {
    if (sensorIdx >= ObstacleConfig::NUM_SENSORS) return LEVEL_INVALID;
    return sensorData[sensorIdx].proximityLevel;
}

bool enableSensor(uint8_t sensorIdx, bool enable) {
    if (sensorIdx >= ObstacleConfig::NUM_SENSORS) return false;
    sensorData[sensorIdx].enabled = enable;
    config.sensorsEnabled[sensorIdx] = enable;
    return true;
}

void setDistanceOffset(uint8_t sensorIdx, int16_t offsetMm) {
    if (sensorIdx >= ObstacleConfig::NUM_SENSORS) return;
    config.offsetsMm[sensorIdx] = offsetMm;
    sensorData[sensorIdx].offsetMm = offsetMm;
}

bool isHealthy(uint8_t sensorIdx) {
    if (sensorIdx >= ObstacleConfig::NUM_SENSORS) return false;
    return sensorData[sensorIdx].healthy && sensorData[sensorIdx].enabled;
}

void getStatus(ObstacleStatus& status) {
    status.sensorsHealthy = 0;
    status.sensorsEnabled = 0;
    status.overallLevel = LEVEL_INVALID;
    status.minDistanceFront = ObstacleConfig::DISTANCE_INVALID;
    status.minDistanceRear = ObstacleConfig::DISTANCE_INVALID;
    status.minDistanceLeft = ObstacleConfig::DISTANCE_INVALID;
    status.minDistanceRight = ObstacleConfig::DISTANCE_INVALID;
    status.emergencyStopActive = false;
    status.parkingAssistActive = false;
    status.lastUpdateMs = millis();
    
    ObstacleLevel worstLevel = LEVEL_SAFE;
    
    for (uint8_t i = 0; i < ObstacleConfig::NUM_SENSORS; i++) {
        if (sensorData[i].enabled) status.sensorsEnabled++;
        if (sensorData[i].healthy) status.sensorsHealthy++;
        
        uint16_t dist = sensorData[i].minDistance;
        ObstacleLevel level = sensorData[i].proximityLevel;
        
        // Store front distance (only one sensor)
        if (i == SENSOR_FRONT) status.minDistanceFront = dist;
        
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
    for (uint8_t i = 0; i < ObstacleConfig::NUM_SENSORS; i++) {
        sensorData[i].offsetMm = config.offsetsMm[i];
        sensorData[i].enabled = config.sensorsEnabled[i];
    }
}

const ObstacleZone& getZone(uint8_t sensorIdx, uint8_t zoneIdx) {
    static ObstacleZone dummy;
    if (sensorIdx >= ObstacleConfig::NUM_SENSORS) return dummy;
    if (zoneIdx >= ObstacleConfig::ZONES_PER_SENSOR) return dummy;
    return sensorData[sensorIdx].zones[zoneIdx];
}

void resetErrors() {
    for (uint8_t i = 0; i < ObstacleConfig::NUM_SENSORS; i++) {
        sensorData[i].errorCount = 0;
    }
}

bool runDiagnostics() {
    bool allPassed = true;
    
    for (uint8_t i = 0; i < ObstacleConfig::NUM_SENSORS; i++) {
        if (sensorData[i].enabled && !sensorData[i].healthy) {
            Logger::warnf("Obstacle sensor %d diagnostics FAILED", i);
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
