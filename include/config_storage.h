#pragma once

#include <Arduino.h>
#include <Preferences.h>

namespace ConfigStorage {
// Storage namespace
const char *const NAMESPACE = "car_config";

// EEPROM Configuration structure
struct Config {
  // Sensor enable/disable states
  bool sensorFL_enabled;
  bool sensorFR_enabled;
  bool sensorRL_enabled;
  bool sensorRR_enabled;
  bool ina226_enabled;

  // Encoder calibration
  int16_t encoder_center;
  int16_t encoder_left_limit;
  int16_t encoder_right_limit;

  // LED configuration
  uint8_t led_pattern;    // 0-7 pattern index
  uint8_t led_brightness; // 0-255
  uint8_t led_speed;      // 0-255
  uint32_t led_color;     // RGB color

  // Power management
  uint16_t power_hold_delay; // ms
  uint16_t aux_delay;        // ms
  uint16_t traction_delay;   // ms

  // System flags
  bool abs_enabled;
  bool tcs_enabled;
  bool regen_enabled;
  bool wifi_enabled;

  // Bluetooth controller
  char bt_mac_address[18]; // MAC address of paired controller
                           // (XX:XX:XX:XX:XX:XX)
  bool bt_auto_reconnect;  // Auto-reconnect on power up

  // 🔒 v2.13.1: Obstacle detection configuration
  // Note: Single front sensor (TOFSense-M S). Structure supports future
  // expansion to multiple sensors.
  uint16_t obstacle_critical_distance; // Critical distance threshold (mm)
  uint16_t obstacle_warning_distance;  // Warning distance threshold (mm)
  uint16_t obstacle_caution_distance;  // Caution distance threshold (mm)
  bool obstacle_sensor_enabled;        // Front sensor enabled (index 0)
  bool obstacle_audio_alerts;          // Audio alerts enabled
  bool obstacle_visual_alerts;         // Visual alerts enabled

  // PHASE 7: Shadow Mode validation
  bool shadowHudEnabled; // Enable shadow sprite validation

  // Checksum for validation
  uint32_t checksum;
};

// Default configuration values
const Config DEFAULT_CONFIG = {
    true, // sensorFL_enabled
    true, // sensorFR_enabled
    true, // sensorRL_enabled
    true, // sensorRR_enabled
    true, // ina226_enabled

    600,  // encoder_center
    0,    // encoder_left_limit
    1200, // encoder_right_limit

    0,        // led_pattern (Rainbow)
    128,      // led_brightness (50%)
    128,      // led_speed (Medium)
    0xFF0000, // led_color (Red)

    5000, // power_hold_delay
    100,  // aux_delay
    500,  // traction_delay

    true, // abs_enabled
    true, // tcs_enabled
    true, // regen_enabled
    true, // wifi_enabled

    "0",  // bt_mac_address (empty until paired)
    true, // bt_auto_reconnect

    200,  // obstacle_critical_distance (default from ObstacleConfig)
    500,  // obstacle_warning_distance
    1000, // obstacle_caution_distance
    true, // obstacle_sensor_enabled
    true, // obstacle_audio_alerts
    true, // obstacle_visual_alerts

    false, // shadowHudEnabled (PHASE 7: disabled by default for production)

    0 // checksum
};

// Initialize storage system
void init();

// Load configuration from EEPROM
bool load(Config &config);

// Save configuration to EEPROM
bool save(const Config &config);

// Reset to factory defaults
void resetToDefaults();

// Validate configuration checksum
bool isValid(const Config &config);

// Calculate checksum
uint32_t calculateChecksum(const Config &config);

// Get current configuration
Config &getCurrentConfig();
} // namespace ConfigStorage
