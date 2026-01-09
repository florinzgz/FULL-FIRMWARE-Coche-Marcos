#pragma once

#include <Arduino.h>
#include <Preferences.h>

// Namespace para acceso a Preferences (EEPROM en ESP32)
namespace ConfigManager {

// ============================================
// CONFIGURACIÓN PERSISTENTE EEPROM
// ============================================

// Estructura de configuración guardable
struct Config {
  // Power Management
  uint16_t powerHoldDelay = 5000; // ms
  uint16_t shutdownDelay = 3000;  // ms

  // Regenerative Braking
  uint8_t regenBrakeLevel = 30; // 0-100%

  // LEDs WS2812B
  uint8_t ledBrightness = 128; // 0-255
  uint8_t ledPattern = 0;      // 0-7 (patrones)
  uint8_t ledSpeed = 5;        // 1-10
  uint8_t ledColorR = 255;     // RGB custom
  uint8_t ledColorG = 255;
  uint8_t ledColorB = 255;

  // Encoder calibration
  int16_t encoderCenter = 600;    // Posición central volante
  int16_t encoderMinLeft = 0;     // Límite izquierdo
  int16_t encoderMaxRight = 1200; // Límite derecho

  // Sensor overrides (emergencia)
  bool sensorWheelsEnabled = true;
  bool sensorCurrentEnabled = true;
  bool sensorEncoderEnabled = true;

  // Menu config
  uint16_t menuTimeout = 30000; // ms (30s)
  uint16_t menuPIN = 8989;      // PIN protección

  // ABS/TCS/Regen thresholds
  uint8_t absThreshold = 15; // % slip máximo
  uint8_t tcsThreshold = 20; // % slip máximo

  // CRC para validación
  uint32_t crc;
};

// ============================================
// API DE PERSISTENCIA
// ============================================

void init();                     // Inicializar sistema EEPROM
bool load(Config &config);       // Cargar configuración guardada
bool save(const Config &config); // Guardar configuración
void reset();                    // Reset a valores por defecto
bool isValid();                  // Verifica si hay config válida

// Helper para calcular CRC32
uint32_t calculateCRC(const Config &config);

} // namespace ConfigManager
