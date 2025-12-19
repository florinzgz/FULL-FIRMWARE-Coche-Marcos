#ifndef OBSTACLE_CONFIG_H
#define OBSTACLE_CONFIG_H

#include <Arduino.h>

// ============================================================================
// VL53L5X Obstacle Detection - Configuration Constants
// ============================================================================

namespace ObstacleConfig {
    // Hardware configuration
    constexpr uint8_t NUM_SENSORS = 2;              // Front, Rear (laterales deshabilitados)
    constexpr uint8_t PCA9548A_ADDR = 0x71;         // I2C multiplexer address
    constexpr uint8_t VL53L5X_DEFAULT_ADDR = 0x29;  // VL53L5X default I2C address
    
    // GPIO pins for XSHUT (power control)
    // 🔒 v2.11.1: Sensores laterales eliminados para liberar GPIOs de las tiras LED
    //            Mantener solo detección frontal y trasera.
    //            Frente mueve su XSHUT a GPIO 46 (liberado tras eliminar sensor lateral derecho) y
    //            el trasero mantiene GPIO 19 (estable). GPIO 18 queda libre para LEDs.
    constexpr uint8_t PIN_XSHUT_FRONT = 46;         // Front sensor (strapping, mantener HIGH en boot)
    constexpr uint8_t PIN_XSHUT_REAR = 19;          // Rear sensor (GPIO libre)
    inline constexpr uint8_t XSHUT_PINS[NUM_SENSORS] = { PIN_XSHUT_FRONT, PIN_XSHUT_REAR };
    
    // PCA9548A multiplexer channels
    constexpr uint8_t MUX_CHANNEL_FRONT = 0;        // Front sensor
    constexpr uint8_t MUX_CHANNEL_REAR = 1;         // Rear sensor
    
    // VL53L5X configuration
    constexpr uint8_t ZONES_PER_SENSOR = 64;        // 8x8 grid
    constexpr uint8_t GRID_SIZE = 8;                // 8x8 zones
    
    // Distance thresholds (mm)
    constexpr uint16_t DISTANCE_CRITICAL = 200;     // 0-20cm: Emergency stop
    constexpr uint16_t DISTANCE_WARNING = 500;      // 20-50cm: Brake assist
    constexpr uint16_t DISTANCE_CAUTION = 1000;     // 50-100cm: Reduce speed
    constexpr uint16_t DISTANCE_MAX = 4000;         // Max detection range
    constexpr uint16_t DISTANCE_INVALID = 8191;     // Out of range marker
    
    // Timing configuration
    constexpr uint32_t UPDATE_INTERVAL_MS = 66;     // 15Hz update rate
    constexpr uint32_t MEASUREMENT_TIMEOUT_MS = 500; // Max measurement time
    constexpr uint32_t INIT_DELAY_MS = 50;          // Power stabilization delay
    constexpr uint32_t MUX_SWITCH_DELAY_US = 100;   // Multiplexer switching time
    
    // Sensor configuration
    constexpr uint8_t RANGING_FREQUENCY_HZ = 15;    // Measurement frequency
    constexpr uint8_t INTEGRATION_TIME_MS = 20;     // Integration time per measurement
    
    // Calibration defaults
    constexpr int16_t DEFAULT_OFFSET_MM = 0;        // Distance offset calibration
    constexpr uint8_t MIN_CONFIDENCE = 50;          // Minimum valid confidence (0-100)
    
    // Error codes
    constexpr uint16_t ERROR_CODE_MUX = 800;        // PCA9548A error
    constexpr uint16_t ERROR_CODE_SENSOR_BASE = 810; // VL53L5X errors (810-813)
    constexpr uint16_t ERROR_CODE_TIMEOUT = 820;    // Measurement timeout
    constexpr uint16_t ERROR_CODE_I2C = 825;        // I2C communication error
}

#endif // OBSTACLE_CONFIG_H
