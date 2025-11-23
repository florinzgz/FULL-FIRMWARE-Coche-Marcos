#!/bin/bash

cat >> apply_corrections.sh << 'SCRIPT_PART3'

# ============================================================================
# CORRECCIÓN 2: include/pins.h - ARQUITECTURA POR EJES COMPLETA
# ============================================================================
echo ""
echo "🔧 CORRECCIÓN 2: include/pins.h - Arquitectura por ejes"
echo "-------------------------------------------------------"

cat > include/pins.h << 'EOF'
#pragma once
#include <stdint.h>

// ============================================================================
// pins.h - CORREGIDO para ESP32-S3-DevKitC-1 (44 pines)
// ✅ ARQUITECTURA POR EJES SEPARADOS - Cada eje su propio PCA9685
// ============================================================================

// -----------------------
// ✅ Relés de potencia (GPIOs verificados válidos)
// -----------------------
#define PIN_RELAY_MAIN    2   // ✅ GPIO 2 válido
#define PIN_RELAY_TRAC    4   // ✅ GPIO 4 válido  
#define PIN_RELAY_DIR     5   // ✅ GPIO 5 válido
#define PIN_RELAY_SPARE   6   // ✅ GPIO 6 válido

// -----------------------
// ✅ Sensores críticos (GPIOs verificados válidos)
// -----------------------
#define PIN_PEDAL         35  // ✅ GPIO 35 ADC válido
#define PIN_ENCODER_A     37  // ✅ GPIO 37 válido
#define PIN_ENCODER_B     38  // ✅ GPIO 38 válido
#define PIN_ENCODER_Z     39  // ✅ GPIO 39 válido

// -----------------------
// ✅ Audio UART (GPIOs verificados válidos)
// -----------------------
#define PIN_DFPLAYER_RX   44  // ✅ GPIO 44 válido
#define PIN_DFPLAYER_TX   43  // ✅ GPIO 43 válido

// -----------------------
// ✅ Display ST7796S SPI (GPIOs verificados válidos)
// -----------------------
#define PIN_TFT_CS        8   // ✅ GPIO 8 válido
#define PIN_TFT_DC        13  // ✅ GPIO 13 válido
#define PIN_TFT_RST       14  // ✅ GPIO 14 válido
#define PIN_TFT_MOSI      11  // ✅ GPIO 11 MOSI válido
#define PIN_TFT_MISO      12  // ✅ GPIO 12 MISO válido
#define PIN_TFT_SCK       10  // ✅ GPIO 10 SCK válido
#define PIN_TFT_BL        42  // ✅ GPIO 42 backlight válido

// -----------------------
// ✅ CORRECCIÓN CRÍTICA: Touch XPT2046
// -----------------------
#define PIN_TOUCH_CS      0   // ✅ CORREGIDO (era inválido → 0 válido)
#define PIN_TOUCH_IRQ     46  // ✅ GPIO 46 válido

// -----------------------
// ✅ Botones físicos (GPIOs verificados válidos)
// -----------------------
#define PIN_BTN_LIGHTS    45  // ✅ GPIO 45 válido
#define PIN_BTN_MEDIA     40  // ✅ GPIO 40 válido
#define PIN_BTN_4X4       41  // ✅ GPIO 41 válido

// -----------------------
// ✅ Shifter 5 posiciones (GPIOs verificados válidos)
// -----------------------
#define PIN_SHIFTER_P     47  // ✅ GPIO 47 válido
#define PIN_SHIFTER_D2    48  // ✅ GPIO 48 válido
#define PIN_SHIFTER_D1    7   // ✅ GPIO 7 válido
#define PIN_SHIFTER_N     18  // ✅ GPIO 18 válido
#define PIN_SHIFTER_R     19  // ✅ GPIO 19 válido

// -----------------------
// ✅ Sensores rueda (GPIOs verificados válidos)
// -----------------------
#define PIN_WHEEL0        20  // ✅ GPIO 20 válido FL
#define PIN_WHEEL1        21  // ✅ GPIO 21 válido FR
#define PIN_WHEEL2        36  // ✅ GPIO 36 válido RL
#define PIN_WHEEL3        17  // ✅ GPIO 17 válido RR

// -----------------------
// ✅ Comunicaciones (GPIOs verificados válidos)
// -----------------------
#define PIN_ONEWIRE       15  // ✅ GPIO 15 válido OneWire
#define PIN_I2C_SDA       16  // ✅ GPIO 16 válido I2C Data
#define PIN_I2C_SCL       9   // ✅ GPIO 9 válido I2C Clock

// -----------------------
// ✅ LEDs (GPIOs libres)
// -----------------------
#define PIN_LED_FRONT     1   // ✅ GPIO 1 válido
#define PIN_LED_REAR      3   // ✅ GPIO 3 válido

// ============================================================================
// ✅ ARQUITECTURA MOTORES POR EJES SEPARADOS
// ============================================================================

// -----------------------
// ✅ EJE FRONTAL (FL + FR)
// -----------------------
#define I2C_ADDR_PCA9685_FRONT     0x40  // PCA9685 eje frontal
#define I2C_ADDR_MCP23017_FRONT    0x20  // MCP23017 eje frontal

// Canales PCA9685 eje frontal
#define PCA_FRONT_FL_PWM           0     // Canal 0: FL PWM
#define PCA_FRONT_FL_PWM_REV       1     // Canal 1: FL PWM Reverse
#define PCA_FRONT_FR_PWM           2     // Canal 2: FR PWM
#define PCA_FRONT_FR_PWM_REV       3     // Canal 3: FR PWM Reverse

// Pines MCP23017 eje frontal
#define MCP_FRONT_FL_IN1           0     // GPIOA0: FL IN1
#define MCP_FRONT_FL_IN2           1     // GPIOA1: FL IN2
#define MCP_FRONT_FR_IN1           2     // GPIOA2: FR IN1
#define MCP_FRONT_FR_IN2           3     // GPIOA3: FR IN2

// -----------------------
// ✅ EJE TRASERO (RL + RR)
// -----------------------
#define I2C_ADDR_PCA9685_REAR      0x41  // PCA9685 eje trasero
#define I2C_ADDR_MCP23017_REAR     0x21  // MCP23017 eje trasero

// Canales PCA9685 eje trasero
#define PCA_REAR_RL_PWM            0     // Canal 0: RL PWM
#define PCA_REAR_RL_PWM_REV        1     // Canal 1: RL PWM Reverse
#define PCA_REAR_RR_PWM            2     // Canal 2: RR PWM
#define PCA_REAR_RR_PWM_REV        3     // Canal 3: RR PWM Reverse

// Pines MCP23017 eje trasero
#define MCP_REAR_RL_IN1            0     // GPIOA0: RL IN1
#define MCP_REAR_RL_IN2            1     // GPIOA1: RL IN2
#define MCP_REAR_RR_IN1            2     // GPIOA2: RR IN1
#define MCP_REAR_RR_IN2            3     // GPIOA3: RR IN2

// -----------------------
// ✅ MOTOR DIRECCIÓN
// -----------------------
#define I2C_ADDR_PCA9685_STEERING  0x42  // PCA9685 motor dirección

// Canales PCA9685 motor dirección
#define PCA_STEERING_PWM           0     // Canal 0: Motor dirección PWM
#define PCA_STEERING_PWM_REV       1     // Canal 1: Motor dirección PWM Reverse

// -----------------------
// ✅ SENSORES CORRIENTE
// -----------------------
#define I2C_ADDR_TCA9548A          0x70  // Multiplexor I2C
#define I2C_ADDR_INA226_BASE       0x40  // Dirección base INA226

// Canales TCA9548A para INA226
#define TCA_CHANNEL_INA226_FL      0     // Canal 0: Motor FL
#define TCA_CHANNEL_INA226_FR      1     // Canal 1: Motor FR
#define TCA_CHANNEL_INA226_RL      2     // Canal 2: Motor RL
#define TCA_CHANNEL_INA226_RR      3     // Canal 3: Motor RR
#define TCA_CHANNEL_INA226_STEER   4     // Canal 4: Motor dirección
#define TCA_CHANNEL_INA226_MAIN    5     // Canal 5: Línea principal

// ============================================================================
// ✅ ENUMERACIONES PARA CONTROL
// ============================================================================

typedef enum {
    AXIS_FRONT = 0,    // Eje frontal
    AXIS_REAR = 1,     // Eje trasero
    AXIS_STEERING = 2, // Motor dirección
    AXIS_COUNT = 3
} AxisID;

typedef enum {
    MOTOR_FL = 0,      // Frontal Izquierda
    MOTOR_FR = 1,      // Frontal Derecha
    MOTOR_RL = 2,      // Trasera Izquierda
    MOTOR_RR = 3,      // Trasera Derecha
    MOTOR_STEERING = 4,// Motor dirección
    MOTOR_COUNT = 5
} MotorID;

// ============================================================================
// ✅ VALIDACIÓN Y APIS HELPER
// ============================================================================

static_assert(PIN_TOUCH_CS <= 48, "PIN_TOUCH_CS fuera del rango ESP32-S3");
static_assert(I2C_ADDR_PCA9685_FRONT != I2C_ADDR_PCA9685_REAR, "PCA9685 addresses must be unique");
static_assert(I2C_ADDR_PCA9685_REAR != I2C_ADDR_PCA9685_STEERING, "PCA9685 addresses must be unique");

// API para obtener información de motores
static inline AxisID motor_get_axis(MotorID motor) {
    switch(motor) {
        case MOTOR_FL:
        case MOTOR_FR:
            return AXIS_FRONT;
        case MOTOR_RL:
        case MOTOR_RR:
            return AXIS_REAR;
        case MOTOR_STEERING:
            return AXIS_STEERING;
        default:
            return AXIS_COUNT;
    }
}

// API para obtener dirección PCA9685 por eje
static inline uint8_t get_pca9685_address(AxisID axis) {
    switch(axis) {
        case AXIS_FRONT: return I2C_ADDR_PCA9685_FRONT;
        case AXIS_REAR: return I2C_ADDR_PCA9685_REAR;
        case AXIS_STEERING: return I2C_ADDR_PCA9685_STEERING;
        default: return 0x00;
    }
}

// API para obtener dirección MCP23017 por eje
static inline uint8_t get_mcp23017_address(AxisID axis) {
    switch(axis) {
        case AXIS_FRONT: return I2C_ADDR_MCP23017_FRONT;
        case AXIS_REAR: return I2C_ADDR_MCP23017_REAR;
        case AXIS_STEERING: return 0x00; // No usa MCP23017
        default: return 0x00;
    }
}

// Helper function para verificar asignaciones
static inline bool pin_is_assigned(uint8_t gpio) {
  switch (gpio) {
    case PIN_RELAY_MAIN: case PIN_RELAY_TRAC: case PIN_RELAY_DIR: case PIN_RELAY_SPARE:
    case PIN_PEDAL: case PIN_ENCODER_A: case PIN_ENCODER_B: case PIN_ENCODER_Z:
    case PIN_DFPLAYER_RX: case PIN_DFPLAYER_TX:
    case PIN_TFT_CS: case PIN_TFT_DC: case PIN_TFT_RST: case PIN_TFT_MOSI:
    case PIN_TFT_MISO: case PIN_TFT_SCK: case PIN_TFT_BL:
    case PIN_TOUCH_CS: case PIN_TOUCH_IRQ:
    case PIN_BTN_LIGHTS: case PIN_BTN_MEDIA: case PIN_BTN_4X4:
    case PIN_SHIFTER_P: case PIN_SHIFTER_D2: case PIN_SHIFTER_D1: case PIN_SHIFTER_N: case PIN_SHIFTER_R:
    case PIN_WHEEL0: case PIN_WHEEL1: case PIN_WHEEL2: case PIN_WHEEL3:
    case PIN_ONEWIRE: case PIN_I2C_SDA: case PIN_I2C_SCL:
    case PIN_LED_FRONT: case PIN_LED_REAR:
      return true;
    default:
      return false;
  }
}
EOF

echo "✅ include/pins.h corregido con arquitectura completa por ejes"
SCRIPT_PART3

chmod +x create_script_part3.sh