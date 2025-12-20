// src/i2c.cpp
#include <Wire.h>
#include "pins.h"   // Use centralized I2C addresses from pins.h
#include "logger.h" // 🔒 IMPROVEMENT: Use Logger for consistent error reporting

// Implementación segura de helper I2C para TCA9548A y acceso básico a INA226.
// Este módulo no hace suposiciones sobre Wire.begin(); System::init() debe
// inicializar el bus I2C antes de usar estas funciones.

void select_tca9548a_channel(uint8_t channel) {
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
        Logger::warnf("I2C: TCA9548A channel select failed ch=%d err=%d", channel, result);
    }
    // Non-blocking: removed 1ms delay - I2C hardware handles timing
    // If instability occurs, caller should add delayMicroseconds(10) if needed
}

// Lectura simplificada de un registro de 16 bits desde un INA226 en el canal dado.
// Retorna true si la lectura fue exitosa y escribe el valor en out.
// No hace reintentos largos para evitar bloqueos; el llamador decide la política.
bool read_ina226_reg16(uint8_t tca_channel, uint8_t dev_addr, uint8_t reg, uint16_t &out) {
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

    uint8_t received = Wire.requestFrom(static_cast<int>(dev_addr), 2);
    if (received < 2) {
        // 🔒 IMPROVEMENT: Log when not enough bytes received
        Logger::warnf("I2C: Read failed on ch%d addr=0x%02X, got %d bytes", 
                     tca_channel, dev_addr, received);
        return false;
    }

    uint8_t hi = Wire.read();
    uint8_t lo = Wire.read();
    out = static_cast<uint16_t>((hi << 8) | lo);
    return true;
}

// Escritura simplificada de un registro de 16 bits en un INA226 en el canal dado.
// Retorna true en éxito.
bool write_ina226_reg16(uint8_t tca_channel, uint8_t dev_addr, uint8_t reg, uint16_t value) {
    // 🔒 CRITICAL FIX: Validate channel before use
    if (tca_channel > 7) {
        Logger::errorf("I2C: Invalid TCA channel %d (must be 0-7)", tca_channel);
        return false;
    }
    
    if (!select_tca9548a_channel(tca_channel)) {
        Logger::errorf("I2C: Failed to select channel %d", tca_channel);
        return false;
    }

    Wire.beginTransmission(dev_addr);
    Wire.write(reg);
    Wire.write(static_cast<uint8_t>(value >> 8));
    Wire.write(static_cast<uint8_t>(value & 0xFF));
    uint8_t result = Wire.endTransmission();
    
    // 🔒 IMPROVEMENT: Log write failures for debugging
    if (result != 0) {
        Logger::warnf("I2C: Write failed on ch%d addr=0x%02X reg=0x%02X err=%d", 
                     tca_channel, dev_addr, reg, result);
    }
    
    return (result == 0);
}