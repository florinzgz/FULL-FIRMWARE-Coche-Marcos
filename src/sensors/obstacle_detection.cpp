// TOFSense-M S Obstacle Detection System Implementation
// 8x8 Matrix LiDAR sensor via UART protocol
// Author: Copilot AI Assistant
// Date: 2026-01-05
// Version: 2.13.0 - 8x8 Matrix Mode (64 distance points)
//
// Protocol Reference: TOFSense-M_User_Manual_V3.0_en.pdf
// https://ftp.nooploop.com/downloads/tofsense/TOFSense-M_User_Manual_V3.0_en.pdf
//
// Frame Format (400 bytes) - NLink_TOFSense_M_Frame0:
// [0-3] Header: 57 01 FF 00
// [4] ID: 0x01
// [5-6] Length: 0x0190 (400 bytes, little-endian)
// [7-10] System time: milliseconds (little-endian)
// [11-394] Matrix data: 64 pixels × 6 bytes each
//   Each pixel (6 bytes):
//     [0-2] Distance: 3 bytes (little-endian signed), value/256/1000 = meters
//     [3] Signal strength
//     [4] Status
//     [5] Reserved
// [395] Checksum: sum of all bytes
// [396-399] Reserved
//
// Matrix layout: 8x8 pixels (row-major order)
// Range: 4 meters, FOV: 65°, Update rate: ~15Hz

#include "obstacle_detection.h"
#include "logger.h"
#include "obstacle_config.h"
#include "pins.h"
#include "system.h"
#include "watchdog.h"
#include <HardwareSerial.h>

namespace ObstacleDetection {

// Hardware instances
static HardwareSerial
    TOFSerial(ObstacleConfig::UART_NUM); // UART0 for TOFSense-M S

// Sensor state
static ObstacleSensor sensorData[ObstacleConfig::NUM_SENSORS];
static ObstacleSettings config;
static bool initialized = false;
static uint32_t lastUpdateMs = 0;
static bool hardwarePresent = false;
static bool placeholderMode = true;

// UART frame buffer (400 bytes for 8x8 matrix mode)
static uint8_t frameBuffer[ObstacleConfig::FRAME_LENGTH];
static uint16_t bufferIndex = 0;
static uint32_t lastPacketMs = 0;
static bool frameInProgress = false;

// Helper function to calculate checksum for 400-byte frame
static uint8_t calculateChecksum(const uint8_t *data, uint16_t length) {
  uint8_t sum = 0;
  for (uint16_t i = 0; i < length; i++) {
    sum += data[i];
  }
  return sum;
}

// Helper function to validate 8x8 matrix frame header
static bool validateFrameHeader(const uint8_t *frame) {
  for (uint8_t i = 0; i < ObstacleConfig::HEADER_LENGTH; i++) {
    if (frame[i] != ObstacleConfig::FRAME_HEADER[i]) { return false; }
  }
  return true;
}

// Helper function to parse distance from 3-byte little-endian format
// Returns distance in millimeters (negative values indicate invalid/out of
// range)
static int16_t parsePixelDistance(const uint8_t *pixelData) {
  // Extract 3 bytes in little-endian format
  int32_t temp =
      (int32_t)(pixelData[0] | (pixelData[1] << 8) | (pixelData[2] << 16));

  // Sign extend from 24-bit to 32-bit
  if (temp & 0x800000) { temp |= 0xFF000000; }

  // Convert: divide by 256, then divide by 1000 to get meters, then *1000 for
  // mm
  int32_t distanceMm = temp / 256;

  // Return -1 for invalid/negative distances
  if (distanceMm < 0 || distanceMm > ObstacleConfig::DISTANCE_MAX) {
    return -1;
  }

  return (int16_t)distanceMm;
}

// Helper function to parse complete 8x8 matrix frame
static bool parseFrame() {
  // Validate header
  if (!validateFrameHeader(frameBuffer)) {
    Logger::warn("TOFSense: Invalid frame header");
    return false;
  }

  // 🔒 SECURITY FIX: Validate checksum position before access
  if (ObstacleConfig::POS_CHECKSUM >= ObstacleConfig::FRAME_LENGTH) {
    Logger::errorf("TOFSense: Invalid checksum position %u >= frame length %u",
                   ObstacleConfig::POS_CHECKSUM, ObstacleConfig::FRAME_LENGTH);
    System::logError(ObstacleConfig::ERROR_CODE_INVALID_DATA);
    return false;
  }

  // Validate checksum
  uint8_t expectedChecksum =
      calculateChecksum(frameBuffer, ObstacleConfig::POS_CHECKSUM);
  uint8_t receivedChecksum = frameBuffer[ObstacleConfig::POS_CHECKSUM];

  if (expectedChecksum != receivedChecksum) {
    Logger::warnf("TOFSense: Checksum mismatch (expected 0x%02X, got 0x%02X)",
                  expectedChecksum, receivedChecksum);
    System::logError(ObstacleConfig::ERROR_CODE_CHECKSUM);
    return false;
  }

  // Parse 64 distance values from matrix data
  ObstacleSensor &sensor = sensorData[SENSOR_FRONT];
  uint16_t minValidDistance = ObstacleConfig::DISTANCE_MAX;
  uint8_t validPixelCount = 0;

  for (uint8_t pixelIdx = 0; pixelIdx < ObstacleConfig::ZONES_PER_SENSOR;
       pixelIdx++) {
    // Calculate pixel data offset in frame
    uint16_t pixelOffset = ObstacleConfig::POS_MATRIX_DATA +
                           (pixelIdx * ObstacleConfig::BYTES_PER_PIXEL);

    // 🔒 SECURITY FIX: Bounds check before buffer access to prevent buffer
    // overflow Each pixel needs 6 bytes (3 distance + 1 signal + 1 status + 1
    // reserved)
    if (pixelOffset + ObstacleConfig::BYTES_PER_PIXEL >
        ObstacleConfig::FRAME_LENGTH) {
      Logger::errorf("TOFSense: Pixel %u offset %u exceeds frame bounds %u",
                     pixelIdx, pixelOffset + ObstacleConfig::BYTES_PER_PIXEL,
                     ObstacleConfig::FRAME_LENGTH);
      System::logError(ObstacleConfig::ERROR_CODE_INVALID_DATA);
      return false;
    }

    // Parse distance from 3-byte format
    int16_t distanceMm = parsePixelDistance(&frameBuffer[pixelOffset]);
    uint8_t signalStrength = frameBuffer[pixelOffset + 3];
    uint8_t status = frameBuffer[pixelOffset + 4];

    // Update zone data
    ObstacleZone &zone = sensor.zones[pixelIdx];

    if (distanceMm >= 0 && distanceMm <= ObstacleConfig::DISTANCE_MAX) {
      zone.distanceMm = (uint16_t)distanceMm;
      zone.confidence = (signalStrength > 100) ? 100 : signalStrength;
      zone.valid = true;
      validPixelCount++;

      // Track minimum distance across all valid pixels
      if (zone.distanceMm < minValidDistance) {
        minValidDistance = zone.distanceMm;
      }

      // Set proximity level for this zone
      if (zone.distanceMm < ObstacleConfig::DISTANCE_CRITICAL) {
        zone.level = LEVEL_CRITICAL;
      } else if (zone.distanceMm < ObstacleConfig::DISTANCE_WARNING) {
        zone.level = LEVEL_WARNING;
      } else if (zone.distanceMm < ObstacleConfig::DISTANCE_CAUTION) {
        zone.level = LEVEL_CAUTION;
      } else {
        zone.level = LEVEL_SAFE;
      }
    } else {
      // Invalid/out of range pixel
      zone.distanceMm = ObstacleConfig::DISTANCE_INVALID;
      zone.confidence = 0;
      zone.valid = false;
      zone.level = LEVEL_INVALID;
    }
  }

  // Update sensor minimum distance and overall proximity level
  if (validPixelCount > 0) {
    sensor.minDistance = minValidDistance;

    // Set overall proximity level based on closest point
    if (minValidDistance < ObstacleConfig::DISTANCE_CRITICAL) {
      sensor.proximityLevel = LEVEL_CRITICAL;
    } else if (minValidDistance < ObstacleConfig::DISTANCE_WARNING) {
      sensor.proximityLevel = LEVEL_WARNING;
    } else if (minValidDistance < ObstacleConfig::DISTANCE_CAUTION) {
      sensor.proximityLevel = LEVEL_CAUTION;
    } else {
      sensor.proximityLevel = LEVEL_SAFE;
    }
  } else {
    // No valid pixels
    sensor.minDistance = ObstacleConfig::DISTANCE_INVALID;
    sensor.proximityLevel = LEVEL_INVALID;
  }

  return true;
}

// Helper function to count valid zones in 8x8 matrix
static uint8_t countValidZones() {
  uint8_t count = 0;
  const ObstacleSensor &sensor = sensorData[SENSOR_FRONT];
  for (uint8_t i = 0; i < ObstacleConfig::ZONES_PER_SENSOR; i++) {
    if (sensor.zones[i].valid) { count++; }
  }
  return count;
}

// Update sensor data with new distance reading (legacy function for
// compatibility)
static void updateSensorData(uint8_t sensorIdx, uint16_t distanceMm) {
  if (sensorIdx >= ObstacleConfig::NUM_SENSORS) return;

  ObstacleSensor &sensor = sensorData[sensorIdx];

  // Apply calibration offset
  int32_t calibratedDistance =
      static_cast<int32_t>(distanceMm) + sensor.offsetMm;
  if (calibratedDistance < 0) calibratedDistance = 0;
  if (calibratedDistance > ObstacleConfig::DISTANCE_MAX) {
    calibratedDistance = ObstacleConfig::DISTANCE_INVALID;
  }

  // Update sensor state
  sensor.minDistance = static_cast<uint16_t>(calibratedDistance);
  sensor.lastUpdateMs = millis();

  // Sensor recovery: Mark as healthy when valid data received
  if (!sensor.healthy) {
    Logger::info("TOFSense: Sensor recovered, marking healthy");
  }
  sensor.healthy = true;
  sensor.errorCount = 0;

  // Update single zone data (for backward compatibility)
  ObstacleZone &zone = sensor.zones[0];
  zone.distanceMm = sensor.minDistance;
  zone.valid = (sensor.minDistance < ObstacleConfig::DISTANCE_INVALID);
  zone.confidence = 100; // TOFSense-M S doesn't provide confidence, assume 100%

  // Set proximity level
  if (sensor.minDistance < ObstacleConfig::DISTANCE_CRITICAL) {
    sensor.proximityLevel = LEVEL_CRITICAL;
    zone.level = LEVEL_CRITICAL;
  } else if (sensor.minDistance < ObstacleConfig::DISTANCE_WARNING) {
    sensor.proximityLevel = LEVEL_WARNING;
    zone.level = LEVEL_WARNING;
  } else if (sensor.minDistance < ObstacleConfig::DISTANCE_CAUTION) {
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
                ObstacleConfig::UART_NUM, PIN_TOFSENSE_RX, PIN_TOFSENSE_TX,
                ObstacleConfig::UART_BAUDRATE);

  hardwarePresent = false;
  placeholderMode = true;
  bufferIndex = 0;
  lastPacketMs = 0;

  // Initialize UART0 for TOFSense-M S (native UART pins)
  // Bidirectional UART: Both TX and RX configured for full communication
  // TX (GPIO43) → Sensor RX: Allows configuration commands
  // RX (GPIO44) ← Sensor TX: Receives 8x8 matrix data frames
  TOFSerial.begin(ObstacleConfig::UART_BAUDRATE, SERIAL_8N1, PIN_TOFSENSE_RX,
                  PIN_TOFSENSE_TX);

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

  while (millis() - startMs < ObstacleConfig::SENSOR_DETECTION_TIMEOUT_MS &&
         !dataReceived) {
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
    Logger::warn(
        "TOFSense-M S sensor not detected, running in simulation mode");
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

  // Read available UART data and accumulate frames
  // 🔒 SECURITY FIX: Limit UART read iterations to prevent infinite loop on
  // corrupted data
  uint16_t bytesRead = 0;
  const uint16_t MAX_BYTES_PER_UPDATE =
      ObstacleConfig::FRAME_LENGTH * 2; // Max 2 frames per update

  while (TOFSerial.available() > 0 && bytesRead < MAX_BYTES_PER_UPDATE) {
    uint8_t byte = TOFSerial.read();
    bytesRead++;

    // Look for frame header (4 bytes: 57 01 FF 00)
    if (bufferIndex == 0) {
      // Looking for first header byte
      if (byte == ObstacleConfig::FRAME_HEADER[0]) {
        frameBuffer[bufferIndex++] = byte;
        frameInProgress = true;
      }
    } else if (bufferIndex < ObstacleConfig::HEADER_LENGTH) {
      // Accumulating header bytes
      if (byte == ObstacleConfig::FRAME_HEADER[bufferIndex]) {
        frameBuffer[bufferIndex++] = byte;
      } else {
        // Header mismatch, restart
        bufferIndex = 0;
        frameInProgress = false;
        // Check if this byte could be start of new header
        if (byte == ObstacleConfig::FRAME_HEADER[0]) {
          frameBuffer[bufferIndex++] = byte;
          frameInProgress = true;
        }
      }
    } else {
      // Accumulating frame data after header
      frameBuffer[bufferIndex++] = byte;

      // 🔒 SECURITY FIX: Prevent buffer overflow if data exceeds expected frame
      // length
      if (bufferIndex > ObstacleConfig::FRAME_LENGTH) {
        Logger::warnf(
            "TOFSense: Buffer overflow detected at index %u, resetting",
            bufferIndex);
        bufferIndex = 0;
        frameInProgress = false;
        System::logError(ObstacleConfig::ERROR_CODE_INVALID_DATA);
        continue;
      }

      // Check if we have a complete frame (400 bytes)
      if (bufferIndex >= ObstacleConfig::FRAME_LENGTH) {
        // Complete frame received, parse it
        if (parseFrame()) {
          // Valid frame received and parsed
          ObstacleSensor &sensor = sensorData[SENSOR_FRONT];
          sensor.lastUpdateMs = now;
          sensor.healthy = true;
          sensor.errorCount = 0;
          lastPacketMs = now;

          // Log periodic readings
          static uint32_t lastLogMs = 0;
          if (now - lastLogMs > ObstacleConfig::LOG_INTERVAL_MS) {
            Logger::infof(
                "TOFSense 8x8: Min distance = %u mm (%d valid pixels)",
                sensor.minDistance, countValidZones());
            lastLogMs = now;
          }
        } else {
          // Invalid frame
          sensorData[SENSOR_FRONT].errorCount++;
          if (sensorData[SENSOR_FRONT].errorCount >
              ObstacleConfig::MAX_CONSECUTIVE_ERRORS) {
            sensorData[SENSOR_FRONT].healthy = false;
            Logger::warn("TOFSense: Too many errors, marking sensor unhealthy");
          }
        }

        // Reset buffer for next frame
        bufferIndex = 0;
        frameInProgress = false;
      }
    }
  }

  // Check for timeout (no data received for a while)
  if (now - lastPacketMs > ObstacleConfig::UART_READ_TIMEOUT_MS &&
      lastPacketMs > 0) {
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

const ObstacleSensor &getSensor(uint8_t sensorIdx) {
  static ObstacleSensor dummy = {};
  if (sensorIdx >= ObstacleConfig::NUM_SENSORS) return dummy;
  return sensorData[sensorIdx];
}

uint16_t getMinDistance(uint8_t sensorIdx) {
  if (sensorIdx >= ObstacleConfig::NUM_SENSORS)
    return ObstacleConfig::DISTANCE_INVALID;
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

void getStatus(ObstacleStatus &status) {
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
    if (level != LEVEL_INVALID && level > worstLevel) { worstLevel = level; }

    // Check for emergency conditions
    if (level == LEVEL_CRITICAL) { status.emergencyStopActive = true; }
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

const ObstacleSettings &getConfig() { return config; }

void setConfig(const ObstacleSettings &newConfig) {
  config = newConfig;

  // Apply new offsets to sensors
  for (uint8_t i = 0; i < ObstacleConfig::NUM_SENSORS; i++) {
    sensorData[i].offsetMm = config.offsetsMm[i];
    sensorData[i].enabled = config.sensorsEnabled[i];
  }
}

const ObstacleZone &getZone(uint8_t sensorIdx, uint8_t zoneIdx) {
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

bool isPlaceholderMode() { return placeholderMode; }

bool isHardwarePresent() { return hardwarePresent; }

} // namespace ObstacleDetection
