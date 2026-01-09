#ifndef OBSTACLE_DETECTION_H
#define OBSTACLE_DETECTION_H

#include "obstacle_config.h"
#include <Arduino.h>

// ============================================================================
// TOFSense-M S Obstacle Detection System - Public API
// ============================================================================
// 🔒 v2.12.0: Migrado de VL53L5X I2C a TOFSense-M S UART
// - Sensor único LiDAR conectado por UART0 (115200 baud, pines nativos)
// - Protocolo: paquetes de 9 bytes según manual oficial
// - Manual:
// https://ftp.nooploop.com/software/products/tofsense_m/doc/TOFSense-M_User_Manual_V1.4_en.pdf
//
// 🔒 FAIL-SAFE BEHAVIOR (Seguridad crítica):
// - Si el sensor no responde (timeout 100ms): sensor.healthy = false
// - Si hay errores consecutivos (>10): sensor.healthy = false
// - El sistema de seguridad (obstacle_safety.cpp) aplica freno de emergencia
//   cuando sensorsHealthy == 0 (fail-safe: detener si perdemos sensores)
// - Recuperación automática cuando se reciben datos válidos nuevamente
// ============================================================================

namespace ObstacleDetection {

// Sensor identifiers (solo un sensor frontal)
enum SensorID : uint8_t { SENSOR_FRONT = 0, SENSOR_COUNT = 1 };

// Validación estática de configuración
static_assert(
    SENSOR_COUNT == ::ObstacleConfig::NUM_SENSORS,
    "ObstacleDetection::SENSOR_COUNT must match ObstacleConfig::NUM_SENSORS");

// Proximity levels based on distance
enum ObstacleLevel : uint8_t {
  LEVEL_SAFE = 0,     // >100cm - No obstacle
  LEVEL_CAUTION = 1,  // 50-100cm - Reduce speed
  LEVEL_WARNING = 2,  // 20-50cm - Brake assist
  LEVEL_CRITICAL = 3, // 0-20cm - Emergency stop
  LEVEL_INVALID = 255 // No valid data
};

// Single zone measurement data (TOFSense-M S provides single distance)
struct ObstacleZone {
  uint16_t distanceMm; // Distance in millimeters
  uint8_t confidence;  // Measurement confidence (0-100)
  ObstacleLevel level; // Proximity level
  bool valid;          // Data validity flag

  ObstacleZone()
      : distanceMm(::ObstacleConfig::DISTANCE_INVALID), confidence(0),
        level(LEVEL_INVALID), valid(false) {}
};

// Complete sensor state (single sensor, single measurement)
struct ObstacleSensor {
  ObstacleZone zones[::ObstacleConfig::ZONES_PER_SENSOR]; // Single zone
  uint16_t minDistance;         // Current distance (mm)
  ObstacleLevel proximityLevel; // Current proximity level
  bool enabled;                 // Sensor enabled flag
  bool healthy;                 // Sensor health status
  int16_t offsetMm;             // Calibration offset
  uint32_t lastUpdateMs;        // Last successful update timestamp
  uint8_t errorCount;           // Consecutive error counter

  ObstacleSensor()
      : minDistance(::ObstacleConfig::DISTANCE_INVALID),
        proximityLevel(LEVEL_INVALID), enabled(true), healthy(false),
        offsetMm(0), lastUpdateMs(0), errorCount(0) {}
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
  ObstacleSettings()
      : thresholdCritical(::ObstacleConfig::DISTANCE_CRITICAL),
        thresholdWarning(::ObstacleConfig::DISTANCE_WARNING),
        thresholdCaution(::ObstacleConfig::DISTANCE_CAUTION),
        audioAlertsEnabled(true), visualAlertsEnabled(true) {
    for (uint8_t i = 0; i < ::ObstacleConfig::NUM_SENSORS; i++) {
      sensorsEnabled[i] = true;
      offsetsMm[i] = 0;
    }
  }
};

// System-wide obstacle detection status
struct ObstacleStatus {
  uint8_t sensorsHealthy;     // Number of healthy sensors
  uint8_t sensorsEnabled;     // Number of enabled sensors
  ObstacleLevel overallLevel; // Current proximity level
  uint16_t minDistanceFront;  // Front distance
  uint16_t
      minDistanceRear; // RESERVED: always DISTANCE_INVALID (no rear sensor)
  uint16_t minDistanceLeft;  // RESERVED: always DISTANCE_INVALID
  uint16_t minDistanceRight; // RESERVED: always DISTANCE_INVALID
  bool emergencyStopActive;  // Emergency stop triggered
  bool parkingAssistActive;  // Parking assist active
  uint32_t lastUpdateMs;     // Last system update

  ObstacleStatus()
      : sensorsHealthy(0), sensorsEnabled(0), overallLevel(LEVEL_INVALID),
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
 * - Initializes UART1 for TOFSense-M S communication
 * - Configures 115200 baudrate
 * - Loads configuration from storage
 */
void init();

/**
 * Update obstacle detection (call in main loop)
 * - Reads UART data packets
 * - Parses TOFSense-M S protocol
 * - Updates sensor data
 * - Non-blocking operation
 */
void update();

/**
 * Enable/disable sensor
 * @param sensorIdx Sensor index (0 for single sensor)
 * @param enable True to enable, false to disable
 * @return True if successful
 */
bool enableSensor(uint8_t sensorIdx, bool enable);

/**
 * Set distance offset for calibration
 * @param sensorIdx Sensor index (0 for single sensor)
 * @param offsetMm Offset in millimeters (+/- adjustment)
 */
void setDistanceOffset(uint8_t sensorIdx, int16_t offsetMm);

/**
 * Get complete sensor data
 * @param sensorIdx Sensor index (0 for single sensor)
 * @return Reference to sensor data structure
 */
const ObstacleSensor &getSensor(uint8_t sensorIdx);

/**
 * Get current distance from sensor
 * @param sensorIdx Sensor index (0 for single sensor)
 * @return Distance in mm (DISTANCE_INVALID if no valid data)
 */
uint16_t getMinDistance(uint8_t sensorIdx);

/**
 * Get proximity level for sensor
 * @param sensorIdx Sensor index (0 for single sensor)
 * @return Proximity level (SAFE/CAUTION/WARNING/CRITICAL)
 */
ObstacleLevel getProximityLevel(uint8_t sensorIdx);

/**
 * Check if sensor is healthy
 * @param sensorIdx Sensor index (0 for single sensor)
 * @return True if sensor is responding correctly
 */
bool isHealthy(uint8_t sensorIdx);

/**
 * Get overall system status
 * @param status Output status structure
 */
void getStatus(ObstacleStatus &status);

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
const ObstacleSettings &getConfig();

/**
 * Update configuration
 * @param config New configuration to apply
 */
void setConfig(const ObstacleSettings &config);

/**
 * Get specific zone data
 * @param sensorIdx Sensor index (0 for single sensor)
 * @param zoneIdx Zone index (0 for single zone)
 * @return Zone data structure
 */
const ObstacleZone &getZone(uint8_t sensorIdx, uint8_t zoneIdx);

/**
 * Reset error counters for all sensors
 */
void resetErrors();

/**
 * Run diagnostic test on sensor
 * @return True if sensor passed
 */
bool runDiagnostics();

/**
 * Check if running in placeholder/simulation mode
 * @return True if no real sensor hardware detected
 */
bool isPlaceholderMode();

/**
 * Check if sensor hardware is present
 * @return True if TOFSense-M S sensor was detected
 */
bool isHardwarePresent();
} // namespace ObstacleDetection

#endif // OBSTACLE_DETECTION_H
