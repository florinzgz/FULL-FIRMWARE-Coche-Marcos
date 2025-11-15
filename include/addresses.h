#pragma once
#include <stdint.h>

// ======================================================
// Dirección base del multiplexor I2C (TCA9548A)
// ======================================================
constexpr uint8_t I2C_MUX_ADDR = 0x70u; // dirección estándar TCA9548A

// ======================================================
// Canales del TCA9548A (usar como índice 0..7)
// ======================================================
constexpr uint8_t TCA_CH_BATT     = 0; // INA226 batería
constexpr uint8_t TCA_CH_WHEEL_FL = 1; // INA226 rueda FL
constexpr uint8_t TCA_CH_WHEEL_FR = 2; // INA226 rueda FR
constexpr uint8_t TCA_CH_WHEEL_RL = 3; // INA226 rueda RL
constexpr uint8_t TCA_CH_WHEEL_RR = 4; // INA226 rueda RR
constexpr uint8_t TCA_CH_STEER    = 5; // INA226 dirección
// canales 6 y 7 libres para expansión

// ======================================================
// Dirección fija de los INA226 (misma en cada canal)
// ======================================================
constexpr uint8_t ADDR_INA226 = 0x40u;

// ======================================================
// Otros dispositivos I2C
// ======================================================
constexpr uint8_t PCA9685_ADDR   = 0x60u; // PCA9685 (si usas múltiples, gestionarlos por canal)
constexpr uint8_t TEMP_SENSOR_ADDR = 0x48u; // LM75 / TMP102 ejemplo
constexpr uint8_t EEPROM_ADDR      = 0x50u;
constexpr uint8_t DISPLAY_ADDR     = 0x3Cu; // SSD1306 típico

// ======================================================
// Prototipo helper para seleccionar canal del TCA9548A
// ======================================================
/**
 * Selecciona un canal del TCA9548A.
 * Pasar un valor 0..7. Internamente escribe (1 << channel) al multiplexor.
 */
void select_tca9548a_channel(uint8_t channel);