// src/i2c.cpp
#include "boot_guard.h"
#include "i2c_recovery.h"
#include "logger.h" // 🔒 IMPROVEMENT: Use Logger for consistent error reporting
#include "pins.h"   // Use centralized I2C addresses from pins.h
#include <Wire.h>

// Implementación segura de helper I2C para TCA9548A y acceso básico a INA226.
// Este módulo no hace suposiciones sobre Wire.begin(); System::init() debe
// inicializar el bus I2C antes de usar estas funciones.

// ============================================================================
// I2C Timeout Constants - v2.11.5
// ============================================================================
// Wire.requestFrom() puede colgar indefinidamente si un sensor I2C no responde.
// Estos timeouts previenen hang del sistema completo.
namespace I2CConstants {
constexpr uint32_t READ_TIMEOUT_MS = 100; // Timeout lectura I2C
constexpr uint16_t RETRY_DELAY_MS =
    1; // Delay entre reintentos (permite task switching)
} // namespace I2CConstants

void select_tca9548a_channel(uint8_t channel) {
  if (!I2CRecovery::isInitialized()) {
    BootGuard::setResetMarker(BootGuard::RESET_MARKER_I2C_PREINIT);
    Logger::error("I2C: Wire.begin() missing before channel select");
    return;
  }

  // 🔒 CRITICAL FIX: Validate channel and log errors
  if (channel > 7) {
    Logger::errorf("I2C: Invalid TCA9548A channel %d (must be 0-7)", channel);
    return;
  }

  Wire.beginTransmission(I2C_ADDR_TCA9548A);
  Wire.write(static_cast<uint8_t>(1u << channel));
  uint8_t result = Wire.endTransmission();

  // 🔒 IMPROVEMENT: Log TCA9548A communication failures
  if (result != 0) {
    Logger::warnf("I2C: TCA9548A channel select failed ch=%d err=%d", channel,
                  result);
  }
  // Non-blocking: removed 1ms delay - I2C hardware handles timing
  // If instability occurs, caller should add delayMicroseconds(10) if needed
}

// Lectura simplificada de un registro de 16 bits desde un INA226 en el canal
// dado. Retorna true si la lectura fue exitosa y escribe el valor en out. No
// hace reintentos largos para evitar bloqueos; el llamador decide la política.
bool read_ina226_reg16(uint8_t tca_channel, uint8_t dev_addr, uint8_t reg,
                       uint16_t &out) {
  if (!I2CRecovery::isInitialized()) {
    BootGuard::setResetMarker(BootGuard::RESET_MARKER_I2C_PREINIT);
    Logger::error("I2C: Wire.begin() missing before INA226 read");
    return false;
  }
  // 🔒 CRITICAL FIX: Validate channel before use
  if (tca_channel > 7) {
    Logger::errorf("I2C: Invalid TCA channel %d (must be 0-7)", tca_channel);
    return false;
  }

  select_tca9548a_channel(tca_channel);

  Wire.beginTransmission(dev_addr);
  Wire.write(reg);
  uint8_t result = Wire.endTransmission(false); // keep bus for read
  if (result != 0) {
    // 🔒 IMPROVEMENT: Log I2C error for debugging
    Logger::warnf("I2C: Write failed on ch%d addr=0x%02X reg=0x%02X err=%d",
                  tca_channel, dev_addr, reg, result);
    return false;
  }

  // ✅ CRITICAL FIX v2.11.5: Timeout manual para requestFrom()
  // Wire.requestFrom() puede colgar si sensor no responde
  // Añadir timeout de 100ms para prevenir hang del sistema
  uint32_t timeoutStart = millis();

  uint8_t received = Wire.requestFrom(static_cast<int>(dev_addr), 2);

  // Esperar con timeout a que lleguen los bytes
  // Usar yield() en lugar de delay() para permitir RTOS task switching
  while (Wire.available() < 2) {
    uint32_t elapsed = millis() - timeoutStart;
    if (elapsed >= I2CConstants::READ_TIMEOUT_MS) {
      Logger::warnf(
          "I2C: Read timeout on ch%d addr=0x%02X (got %d bytes in %lums)",
          tca_channel, dev_addr, Wire.available(), elapsed);
      return false;
    }
    yield(); // Non-blocking - permite task switching
  }

  received = Wire.available();
  if (received < 2) {
    // Defensive check - no debería ocurrir tras el while loop
    Logger::warnf("I2C: Partial read on ch%d addr=0x%02X (got %d bytes)",
                  tca_channel, dev_addr, received);
    return false;
  }

  uint8_t hi = Wire.read();
  uint8_t lo = Wire.read();
  out = static_cast<uint16_t>((hi << 8) | lo);
  return true;
}

// Escritura simplificada de un registro de 16 bits en un INA226 en el canal
// dado. Retorna true en éxito.
bool write_ina226_reg16(uint8_t tca_channel, uint8_t dev_addr, uint8_t reg,
                        uint16_t value) {
  if (!I2CRecovery::isInitialized()) {
    BootGuard::setResetMarker(BootGuard::RESET_MARKER_I2C_PREINIT);
    Logger::error("I2C: Wire.begin() missing before INA226 write");
    return false;
  }
  // 🔒 CRITICAL FIX: Validate channel before use
  if (tca_channel > 7) {
    Logger::errorf("I2C: Invalid TCA channel %d (must be 0-7)", tca_channel);
    return false;
  }

  select_tca9548a_channel(tca_channel);

  Wire.beginTransmission(dev_addr);
  Wire.write(reg);
  Wire.write(static_cast<uint8_t>(value >> 8));
  Wire.write(static_cast<uint8_t>(value & 0xFF));

  // endTransmission tiene timeout interno del Wire library
  uint8_t result = Wire.endTransmission();

  // 🔒 IMPROVEMENT: Log write failures for debugging
  if (result != 0) {
    Logger::warnf("I2C: Write failed on ch%d addr=0x%02X reg=0x%02X err=%d",
                  tca_channel, dev_addr, reg, result);
  }

  return (result == 0);
}
