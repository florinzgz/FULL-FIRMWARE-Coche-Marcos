#ifndef OBSTACLE_CONFIG_H
#define OBSTACLE_CONFIG_H

#include <Arduino.h>

// ============================================================================
// TOFSense-M S Obstacle Detection - Configuration Constants
// ============================================================================
// 🔒 v2.13.0: TOFSense-M S con matriz 8x8 (64 puntos)
// - Sensor LiDAR con matriz 8x8 conectado por UART0 (921600 baud, pines
// nativos)
// - Protocolo: Frame de 400 bytes, header 0x57 0x01 0xFF 0x00
// - Manual:
// https://ftp.nooploop.com/software/products/tofsense_m/doc/TOFSense-M_User_Manual_V1.4_en.pdf
// - Rango: 4 metros, Campo de visión: 65°, Matriz: 8x8 = 64 puntos
// - GPIO 44 (RX): Sensor TX → ESP32 RX (recibe datos)
// - GPIO 43 (TX): ESP32 TX → Sensor RX (configuración bidireccional UART)
// ============================================================================

namespace ObstacleConfig {
// Hardware configuration
constexpr uint8_t NUM_SENSORS = 1; // Un solo sensor TOFSense-M S
constexpr uint32_t UART_BAUDRATE =
    921600;                     // Baudrate del TOFSense-M S (8x8 mode)
constexpr uint8_t UART_NUM = 0; // UART0 (HardwareSerial - pines nativos)

// TOFSense-M S 8x8 Matrix Mode Protocol (NLink_TOFSense_M_Frame0)
// Frame format: 400 bytes total
// Header: 57 01 FF 00 (4 bytes)
// ID: 1 byte
// Length: 2 bytes (little-endian, always 0x0190 = 400)
// System time: 4 bytes (milliseconds, little-endian)
// Matrix data: 64 pixels x 6 bytes each = 384 bytes
//   Each pixel: 3 bytes distance + 1 byte signal + 1 byte status + 1 byte
//   reserved
// Checksum: 1 byte (sum of all bytes)
// Reserved: 4 bytes
constexpr uint8_t FRAME_HEADER[] = {0x57, 0x01, 0xFF, 0x00}; // Frame header
constexpr uint16_t FRAME_LENGTH = 400;   // Total frame length
constexpr uint8_t HEADER_LENGTH = 4;     // Header bytes
constexpr uint8_t MATRIX_SIZE = 8;       // 8x8 matrix
constexpr uint8_t ZONES_PER_SENSOR = 64; // 8x8 = 64 distance measurements
constexpr uint8_t BYTES_PER_PIXEL =
    6; // 3 distance + 1 signal + 1 status + 1 reserved

// Frame byte positions
constexpr uint8_t POS_HEADER = 0;       // Header start (4 bytes)
constexpr uint8_t POS_ID = 4;           // ID byte
constexpr uint8_t POS_LENGTH = 5;       // Length (2 bytes, little-endian)
constexpr uint8_t POS_SYSTEM_TIME = 7;  // System time (4 bytes, little-endian)
constexpr uint8_t POS_MATRIX_DATA = 11; // Matrix data start (384 bytes)
constexpr uint16_t POS_CHECKSUM =
    395; // Checksum byte (uint16_t for value >255)
constexpr uint16_t POS_RESERVED = 396; // Reserved (4 bytes)

// Distance thresholds (mm)
constexpr uint16_t DISTANCE_CRITICAL = 200; // 0-20cm: Emergency stop
constexpr uint16_t DISTANCE_WARNING = 500;  // 20-50cm: Brake assist
constexpr uint16_t DISTANCE_CAUTION = 1000; // 50-100cm: Reduce speed
constexpr uint16_t DISTANCE_MAX =
    4000; // Max detection range (4m for TOFSense-M S)
constexpr uint16_t DISTANCE_INVALID = UINT16_MAX; // Invalid distance marker

// Timing configuration
constexpr uint32_t UPDATE_INTERVAL_MS =
    66; // 15Hz update rate (matches sensor output)
constexpr uint32_t MEASUREMENT_TIMEOUT_MS = 500; // Max measurement time
constexpr uint32_t UART_READ_TIMEOUT_MS =
    200; // UART read timeout (increased for large frames)
constexpr uint32_t SENSOR_DETECTION_TIMEOUT_MS =
    2000;                                  // Sensor detection timeout at init
constexpr uint32_t LOG_INTERVAL_MS = 2000; // Distance logging interval
constexpr uint32_t TIMEOUT_LOG_INTERVAL_MS =
    5000; // Timeout warning log interval

// Error handling
constexpr uint8_t MAX_CONSECUTIVE_ERRORS =
    10; // Max errors before marking unhealthy

// Calibration defaults
constexpr int16_t DEFAULT_OFFSET_MM = 0; // Distance offset calibration
constexpr uint8_t MIN_CONFIDENCE = 50;   // Minimum valid confidence (0-100)

// Error codes
constexpr uint16_t ERROR_CODE_UART = 800;         // UART communication error
constexpr uint16_t ERROR_CODE_CHECKSUM = 810;     // Checksum validation failed
constexpr uint16_t ERROR_CODE_TIMEOUT = 820;      // Measurement timeout
constexpr uint16_t ERROR_CODE_INVALID_DATA = 825; // Invalid packet data
} // namespace ObstacleConfig

#endif // OBSTACLE_CONFIG_H
