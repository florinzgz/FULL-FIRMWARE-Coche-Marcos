#ifndef OBSTACLE_CONFIG_H
#define OBSTACLE_CONFIG_H

#include <Arduino.h>

// ============================================================================
// TOFSense-M S Obstacle Detection - Configuration Constants
// ============================================================================
// 🔒 v2.12.0: Migración de VL53L5X I2C a TOFSense-M S UART
// - Sensor único LiDAR conectado por UART1 (115200 baud)
// - Protocolo: paquetes de 9 bytes, header 0x57
// - Manual: https://ftp.nooploop.com/software/products/tofsense_m/doc/TOFSense-M_User_Manual_V1.4_en.pdf
// - GPIO 18 (RX) recibe datos del sensor, GPIO 17 (TX) no usado
// ============================================================================

namespace ObstacleConfig {
    // Hardware configuration
    constexpr uint8_t NUM_SENSORS = 1;              // Un solo sensor TOFSense-M S
    constexpr uint32_t UART_BAUDRATE = 115200;      // Baudrate del TOFSense-M S
    constexpr uint8_t UART_NUM = 1;                 // UART1 (HardwareSerial)
    
    // TOFSense-M S UART Protocol
    // Packet format: [Header][Function][Length][Data_L][Data_H][...][Checksum]
    // Header: 0x57 (fixed)
    // Function: 0x00 for distance measurement
    // Length: 0x02 (2 bytes of distance data)
    // Distance: (Data_H << 8) | Data_L (millimeters)
    constexpr uint8_t PACKET_HEADER = 0x57;         // Packet header byte
    constexpr uint8_t PACKET_FUNC_DISTANCE = 0x00;  // Distance measurement function
    constexpr uint8_t PACKET_LENGTH = 9;            // Total packet length in bytes
    constexpr uint8_t PACKET_DATA_LENGTH = 0x02;    // Distance data length
    
    // Packet byte positions
    constexpr uint8_t POS_HEADER = 0;
    constexpr uint8_t POS_FUNCTION = 1;
    constexpr uint8_t POS_LENGTH = 2;
    constexpr uint8_t POS_DATA_L = 3;               // Distance low byte
    constexpr uint8_t POS_DATA_H = 4;               // Distance high byte
    constexpr uint8_t POS_CHECKSUM = 8;             // Checksum position
    
    // Sensor configuration (single zone - no grid)
    constexpr uint8_t ZONES_PER_SENSOR = 1;         // Single distance measurement
    
    // Distance thresholds (mm)
    constexpr uint16_t DISTANCE_CRITICAL = 200;     // 0-20cm: Emergency stop
    constexpr uint16_t DISTANCE_WARNING = 500;      // 20-50cm: Brake assist
    constexpr uint16_t DISTANCE_CAUTION = 1000;     // 50-100cm: Reduce speed
    constexpr uint16_t DISTANCE_MAX = 12000;        // Max detection range (12m for TOFSense-M S)
    constexpr uint16_t DISTANCE_INVALID = 65535;    // Invalid distance marker
    
    // Timing configuration
    constexpr uint32_t UPDATE_INTERVAL_MS = 66;     // 15Hz update rate
    constexpr uint32_t MEASUREMENT_TIMEOUT_MS = 500; // Max measurement time
    constexpr uint32_t UART_READ_TIMEOUT_MS = 100;  // UART read timeout
    
    // Calibration defaults
    constexpr int16_t DEFAULT_OFFSET_MM = 0;        // Distance offset calibration
    constexpr uint8_t MIN_CONFIDENCE = 50;          // Minimum valid confidence (0-100)
    
    // Error codes
    constexpr uint16_t ERROR_CODE_UART = 800;       // UART communication error
    constexpr uint16_t ERROR_CODE_CHECKSUM = 810;   // Checksum validation failed
    constexpr uint16_t ERROR_CODE_TIMEOUT = 820;    // Measurement timeout
    constexpr uint16_t ERROR_CODE_INVALID_DATA = 825; // Invalid packet data
}

#endif // OBSTACLE_CONFIG_H
