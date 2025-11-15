// src/i2c.cpp
#include <Wire.h>
#include "addresses.h"

// Implementación segura de helper I2C para TCA9548A y acceso básico a INA226.
// Este módulo no hace suposiciones sobre Wire.begin(); System::init() debe
// inicializar el bus I2C antes de usar estas funciones.

void select_tca9548a_channel(uint8_t channel) {
    if (channel > 7) return;
    Wire.beginTransmission(I2C_MUX_ADDR);
    Wire.write(static_cast<uint8_t>(1u << channel));
    Wire.endTransmission();
    delay(1); // pequeño retardo para estabilizar el bus tras el cambio de canal
}

// Lectura simplificada de un registro de 16 bits desde un INA226 en el canal dado.
// Retorna true si la lectura fue exitosa y escribe el valor en out.
// No hace reintentos largos para evitar bloqueos; el llamador decide la política.
bool read_ina226_reg16(uint8_t tca_channel, uint8_t dev_addr, uint8_t reg, uint16_t &out) {
    if (tca_channel > 7) return false;
    select_tca9548a_channel(tca_channel);

    Wire.beginTransmission(dev_addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false; // no release bus on failure

    Wire.requestFrom(static_cast<int>(dev_addr), 2);
    if (Wire.available() < 2) return false;

    uint8_t hi = Wire.read();
    uint8_t lo = Wire.read();
    out = static_cast<uint16_t>((hi << 8) | lo);
    return true;
}

// Escritura simplificada de un registro de 16 bits en un INA226 en el canal dado.
// Retorna true en éxito.
bool write_ina226_reg16(uint8_t tca_channel, uint8_t dev_addr, uint8_t reg, uint16_t value) {
    if (tca_channel > 7) return false;
    select_tca9548a_channel(tca_channel);

    Wire.beginTransmission(dev_addr);
    Wire.write(reg);
    Wire.write(static_cast<uint8_t>(value >> 8));
    Wire.write(static_cast<uint8_t>(value & 0xFF));
    return (Wire.endTransmission() == 0);
}