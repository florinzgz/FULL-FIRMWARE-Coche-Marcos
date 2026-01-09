#ifndef OBSTACLE_LOGGER_H
#define OBSTACLE_LOGGER_H

#include "obstacle_detection.h"
#include <Arduino.h>

// ============================================================================
// Obstacle Detection Data Logger
// 3D environment mapping and historical tracking
// ============================================================================

namespace ObstacleLogger {

// Single log entry
struct LogEntry {
  uint32_t timestamp;
  uint8_t sensorId;
  uint16_t minDistance;
  ObstacleDetection::ObstacleLevel level;
  uint8_t zoneCount;      // Number of valid zones
  float vehicleSpeed;     // km/h at time of measurement
  int16_t vehicleHeading; // Degrees from north (if available)

  LogEntry()
      : timestamp(0), sensorId(0), minDistance(0xFFFF),
        level(ObstacleDetection::LEVEL_INVALID), zoneCount(0),
        vehicleSpeed(0.0f), vehicleHeading(0) {}
};

// Logger configuration
struct LoggerConfig {
  bool enabled;
  bool autoRotate;
  uint32_t maxFileSize; // Bytes before rotation
  uint8_t maxFiles;     // Number of files to keep
  uint16_t logInterval; // ms between log entries

  LoggerConfig()
      : enabled(false), autoRotate(true), maxFileSize(10 * 1024 * 1024), // 10MB
        maxFiles(5), logInterval(200) {}                                 // 5Hz
};

// Logger status
struct LoggerStatus {
  bool active;
  uint32_t entriesLogged;
  uint32_t currentFileSize;
  uint8_t filesCreated;
  String currentFilename;
  uint32_t lastLogMs;

  LoggerStatus()
      : active(false), entriesLogged(0), currentFileSize(0), filesCreated(0),
        lastLogMs(0) {}
};

/**
 * Initialize logger system
 */
void init();

/**
 * Update logger (call in main loop)
 */
void update();

/**
 * Start logging
 * @return True if logging started successfully
 */
bool startLogging();

/**
 * Stop logging and close current file
 */
void stopLogging();

/**
 * Get logger status
 */
const LoggerStatus &getStatus();

/**
 * Get logger configuration
 */
const LoggerConfig &getConfig();

/**
 * Update configuration
 */
void setConfig(const LoggerConfig &config);

/**
 * Export data as CSV
 * @param filename Output filename
 * @return True if export successful
 */
bool exportCSV(const String &filename);

/**
 * Clear all log files
 * @return True if successful
 */
bool clearLogs();

/**
 * Get list of log files
 * @param files Output array
 * @param maxFiles Maximum files to return
 * @return Number of files found
 */
uint8_t getLogFiles(String *files, uint8_t maxFiles);
} // namespace ObstacleLogger

#endif // OBSTACLE_LOGGER_H
