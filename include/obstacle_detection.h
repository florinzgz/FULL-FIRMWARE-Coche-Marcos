#ifndef OBSTACLE_DETECTION_H
#define OBSTACLE_DETECTION_H

#include <Arduino.h>
#include "obstacle_config.h"

// ============================================================================
// VL53L5X Obstacle Detection System - Public API
// ============================================================================

namespace ObstacleDetection {
    
    // Sensor identifiers
    enum SensorID : uint8_t {
        SENSOR_FRONT = 0,
        SENSOR_REAR = 1,
        SENSOR_LEFT = 2,
        SENSOR_RIGHT = 3,
        SENSOR_COUNT = 4
    };
    
    // Proximity levels based on distance
    enum ObstacleLevel : uint8_t {
        LEVEL_SAFE = 0,         // >100cm - No obstacle
        LEVEL_CAUTION = 1,      // 50-100cm - Reduce speed
        LEVEL_WARNING = 2,      // 20-50cm - Brake assist
        LEVEL_CRITICAL = 3,     // 0-20cm - Emergency stop
        LEVEL_INVALID = 255     // No valid data
    };
    
    // Single zone measurement data
    struct ObstacleZone {
        uint16_t distanceMm;    // Distance in millimeters
        uint8_t confidence;     // Measurement confidence (0-100)
        ObstacleLevel level;    // Proximity level
        bool valid;             // Data validity flag
        
        ObstacleZone() : distanceMm(::ObstacleConfig::DISTANCE_INVALID), 
                        confidence(0), level(LEVEL_INVALID), valid(false) {}
    };
    
    // Complete sensor state (8x8 grid)
    struct ObstacleSensor {
        ObstacleZone zones[::ObstacleConfig::ZONES_PER_SENSOR];
        uint16_t minDistance;           // Closest obstacle in all zones (mm)
        ObstacleLevel proximityLevel;   // Overall proximity level
        bool enabled;                   // Sensor enabled flag
        bool healthy;                   // Sensor health status
        int16_t offsetMm;               // Calibration offset
        uint32_t lastUpdateMs;          // Last successful update timestamp
        uint8_t errorCount;             // Consecutive error counter
        
        ObstacleSensor() : minDistance(::ObstacleConfig::DISTANCE_INVALID),
                          proximityLevel(LEVEL_INVALID), enabled(true),
                          healthy(false), offsetMm(0), lastUpdateMs(0), errorCount(0) {}
    };
    
    // Persistent configuration (saved to storage)
    struct ObstacleSettings {
        bool sensorsEnabled[::ObstacleConfig::NUM_SENSORS];
        int16_t offsetsMm[::ObstacleConfig::NUM_SENSORS];
        uint16_t thresholdCritical;
        uint16_t thresholdWarning;
        uint16_t thresholdCaution;
        bool audioAlertsEnabled;
        bool visualAlertsEnabled;
        
        // Initialize with defaults
        ObstacleSettings() : thresholdCritical(::ObstacleConfig::DISTANCE_CRITICAL),
                          thresholdWarning(::ObstacleConfig::DISTANCE_WARNING),
                          thresholdCaution(::ObstacleConfig::DISTANCE_CAUTION),
                          audioAlertsEnabled(true),
                          visualAlertsEnabled(true) {
            for (uint8_t i = 0; i < ::ObstacleConfig::NUM_SENSORS; i++) {
                sensorsEnabled[i] = true;
                offsetsMm[i] = 0;
            }
        }
    };
    
    // System-wide obstacle detection status
    struct ObstacleStatus {
        uint8_t sensorsHealthy;         // Number of healthy sensors
        uint8_t sensorsEnabled;         // Number of enabled sensors
        ObstacleLevel overallLevel;     // Worst proximity level
        uint16_t minDistanceFront;      // Front min distance
        uint16_t minDistanceRear;       // Rear min distance
        uint16_t minDistanceLeft;       // Left min distance
        uint16_t minDistanceRight;      // Right min distance
        bool emergencyStopActive;       // Emergency stop triggered
        bool parkingAssistActive;       // Parking assist active
        uint32_t lastUpdateMs;          // Last system update
        
        ObstacleStatus() : sensorsHealthy(0), sensorsEnabled(0), 
                          overallLevel(LEVEL_INVALID),
                          minDistanceFront(::ObstacleConfig::DISTANCE_INVALID),
                          minDistanceRear(::ObstacleConfig::DISTANCE_INVALID),
                          minDistanceLeft(::ObstacleConfig::DISTANCE_INVALID),
                          minDistanceRight(::ObstacleConfig::DISTANCE_INVALID),
                          emergencyStopActive(false), parkingAssistActive(false),
                          lastUpdateMs(0) {}
    };
    
    // ========================================================================
    // Public API Functions
    // ========================================================================
    
    /**
     * Initialize obstacle detection system
     * - Powers up sensors with XSHUT sequencing
     * - Initializes PCA9548A multiplexer
     * - Configures VL53L5X sensors
     * - Loads configuration from storage
     */
    void init();
    
    /**
     * Update obstacle detection (call in main loop)
     * - Non-blocking async updates
     * - 15Hz update rate per sensor
     * - Updates all sensor data
     */
    void update();
    
    /**
     * Enable/disable individual sensor
     * @param sensorIdx Sensor index (0-3)
     * @param enable True to enable, false to disable
     * @return True if successful
     */
    bool enableSensor(uint8_t sensorIdx, bool enable);
    
    /**
     * Set distance offset for calibration
     * @param sensorIdx Sensor index (0-3)
     * @param offsetMm Offset in millimeters (+/- adjustment)
     */
    void setDistanceOffset(uint8_t sensorIdx, int16_t offsetMm);
    
    /**
     * Get complete sensor data
     * @param sensorIdx Sensor index (0-3)
     * @return Reference to sensor data structure
     */
    const ObstacleSensor& getSensor(uint8_t sensorIdx);
    
    /**
     * Get minimum distance from sensor (all zones)
     * @param sensorIdx Sensor index (0-3)
     * @return Minimum distance in mm (DISTANCE_INVALID if no valid data)
     */
    uint16_t getMinDistance(uint8_t sensorIdx);
    
    /**
     * Get proximity level for sensor
     * @param sensorIdx Sensor index (0-3)
     * @return Proximity level (SAFE/CAUTION/WARNING/CRITICAL)
     */
    ObstacleLevel getProximityLevel(uint8_t sensorIdx);
    
    /**
     * Check if sensor is healthy
     * @param sensorIdx Sensor index (0-3)
     * @return True if sensor is responding correctly
     */
    bool isHealthy(uint8_t sensorIdx);
    
    /**
     * Get overall system status
     * @param status Output status structure
     */
    void getStatus(ObstacleStatus& status);
    
    /**
     * Load configuration from storage
     * @return True if loaded successfully
     */
    bool loadConfig();
    
    /**
     * Save configuration to storage
     * @return True if saved successfully
     */
    bool saveConfig();
    
    /**
     * Get current configuration
     * @return Reference to configuration structure
     */
    const ObstacleSettings& getConfig();
    
    /**
     * Update configuration
     * @param config New configuration to apply
     */
    void setConfig(const ObstacleSettings& config);
    
    /**
     * Get specific zone data
     * @param sensorIdx Sensor index (0-3)
     * @param zoneIdx Zone index (0-63)
     * @return Zone data structure
     */
    const ObstacleZone& getZone(uint8_t sensorIdx, uint8_t zoneIdx);
    
    /**
     * Reset error counters for all sensors
     */
    void resetErrors();
    
    /**
     * Run diagnostic test on all sensors
     * @return True if all enabled sensors passed
     */
    bool runDiagnostics();
    
    /**
     * Check if running in placeholder/simulation mode
     * @return True if no real sensor hardware detected
     */
    bool isPlaceholderMode();
    
    /**
     * Check if any real sensor hardware is present
     * @return True if at least one VL53L5CX sensor was detected
     */
    bool isHardwarePresent();
}

#endif // OBSTACLE_DETECTION_H
