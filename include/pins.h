#pragma once
#include <stdint.h>

// ============================================================================
// pins.h - Asignación de pines para ESP32-S3-DevKitC-1 (44 pines)
// Compatible con GPIOs 0-48 (adaptado de versión N16R8)
// ============================================================================
//
// HARDWARE COMPLETO INTEGRADO:
// - ESP32-S3-DevKitC-1 (44 pines, GPIOs 0-48)
// - 6x INA226 con shunts externos CG FL-2C (1x100A + 5x50A, 75mV, 0.5 Class)
// - 5x TCA9548A multiplexores I²C (para 6 INA226 sin conflicto dirección)
// - 1x PCA9685 PWM driver (dirección 0x41, motor dirección RS390)
// - 1x MCP23017 expansor GPIO I²C (16 pines, control relés)
// - 2x HY-M158 optoacopladores PC817 (8 canales c/u = 16 total)
// - 4x BTS7960 drivers motor 43A (tracción 4 ruedas)
// - 1x BTS7960 driver motor (dirección RS390 12V 6000RPM + reductora 1:50)
// - 1x Encoder E6B2-CWZ6C 1200PR (dirección, ratio 1:1 al volante)
// - 5x Sensores inductivos LJ12A3-4-Z/BX (4 ruedas + 1 señal Z encoder)
// - 1x Sensor Hall A1324LUA-T (pedal analógico)
// - 1x Pantalla ILI9488 480x320 + táctil XPT2046
// - 1x DFPlayer Mini (audio)
// - 1x Tira LEDs WS2812B (iluminación inteligente)
// - 3x Convertidores nivel I²C bidireccionales 4 canales (5V↔3.3V)
// - 1x Hub I²C divisor 8 vías
// - Relés: 1x módulo 2 canales 5V SRD-05VDC, 1x SPDT 100A 24V, 
//          1x TAXNELE TN606 RL280-12 (12V 100-200A), 
//          1x Contactor HCH8s-25z 25A 2NO DC12,
//          1x Automotive Waterproof Relay 100A 12V SPDT
// ============================================================================

// -----------------------
// Relés de potencia (4 relés SRD-05VDC-SL-C)
// GPIOs verificados para ESP32-S3-DevKitC-1 44 pines
// -----------------------
#define PIN_RELAY_MAIN    2   // Relé 1: Power Hold
#define PIN_RELAY_TRAC    4   // Relé 2: 12V Auxiliares  
#define PIN_RELAY_DIR     5   // Relé 3: 24V Motores Tracción
#define PIN_RELAY_SPARE   6   // Relé 4: Reserva (consolidado desde LIGHTS/MEDIA)

// -----------------------
// Pedal acelerador - Sensor Hall A1324LUA-T (analógico, 5V output)
// GPIO 35 tiene ADC disponible, requiere divisor tensión 5V→3.3V o HY-M158
// -----------------------
#define PIN_PEDAL         35  // GPIO 35 (ADC) - Compatible con ADC

// -----------------------
// Encoder dirección
// Remapeado para ESP32-S3-DevKitC-1
// Conectado vía HY-M158 optoacopladores para aislamiento
// -----------------------
#define PIN_ENCODER_A     37  // Canal A
#define PIN_ENCODER_B     38  // Canal B
#define PIN_ENCODER_Z     39  // Señal Z (centrado)

// -----------------------
// DFPlayer Mini (UART1)
// -----------------------
#define PIN_DFPLAYER_RX   44  // GPIO 44
#define PIN_DFPLAYER_TX   43  // GPIO 43

// -----------------------
// Pantalla TFT ILI9488 (SPI)
// Remapeado para ESP32-S3-DevKitC-1 44 pines
// -----------------------
#define PIN_TFT_CS        8   // Chip Select
#define PIN_TFT_DC        13  // Data/Command
#define PIN_TFT_RST       14  // Reset
#define PIN_TFT_MOSI      11  // SPI MOSI
#define PIN_TFT_MISO      12  // SPI MISO
#define PIN_TFT_SCK       10  // SPI Clock
#define PIN_TFT_BL        42  // Backlight PWM (0-255 brightness control via LEDC)

// -----------------------
// Táctil (XPT2046 SPI)
// -----------------------
#define PIN_TOUCH_CS      3   // Chip Select táctil (remapeado a GPIO3 - GPIO22 no disponible en placa)
#define PIN_TOUCH_IRQ     46  // Interrupción táctil

// -----------------------
// Botones físicos
// Remapeados para ESP32-S3-DevKitC-1 44 pines
// Multimedia (12V) vía HY-M158 Módulo #1 CH8
// 4x4/4x2 (12V) vía HY-M158 Módulo #2 CH6 - Switch 2 posiciones con pull-up
// -----------------------
#define PIN_BTN_LIGHTS    45  // Botón luces
#define PIN_BTN_MEDIA     40  // Botón multimedia (vía HY-M158 12V) 
#define PIN_BTN_4X4       41  // Botón 4x4/4x2 (vía HY-M158 12V, 2 posiciones, pull-up)

// -----------------------
// Palanca de cambios (Shifter) - 5 posiciones
// Conectada vía HY-M158 optoacopladores (señales 12V)
// -----------------------
#define PIN_SHIFTER_P     47  // Posición P (Park)
#define PIN_SHIFTER_D2    48  // Posición D2 (Drive 2)
#define PIN_SHIFTER_D1    7   // Posición D1 (Drive 1) (relocated)
#define PIN_SHIFTER_N     18  // Posición N (Neutral)
#define PIN_SHIFTER_R     19  // Posición R (Reverse)

// -----------------------
// Llave/Switch del sistema
// GPIO 0 (Boot button) - Usado para llave general del sistema
// -----------------------
#define PIN_KEY_SYSTEM    0   // GPIO 0: Llave/switch general del sistema

// -----------------------
// Sensores de rueda (entradas digitales/inductivas LJ12A3-4-Z/BX)
// Conectados vía HY-M158 optoacopladores (5V)
// Remapeados para ESP32-S3-DevKitC-1
// NOTA: GPIO21 liberado (no se usa botón batería)
// WHEEL1 (FR) movido a MCP23017 GPIOB0 para liberar GPIO
// -----------------------
#define PIN_WHEEL0        20  // FL (Frontal Izquierda) vía HY-M158
// PIN_WHEEL1 ahora en MCP23017 GPIOB0 (ver MCP_PIN_WHEEL1)
#define PIN_WHEEL2        36  // RL (Trasera Izquierda) vía HY-M158
#define PIN_WHEEL3        17  // RR (Trasera Derecha) vía HY-M158

// -----------------------
// DS18B20 (OneWire) - Sensor temperatura
// -----------------------
#define PIN_ONEWIRE       15  // GPIO 15

// -----------------------
// I2C (INA226 + PCA9685 + MCP23017 + TCA9548A)
// Remapeado para ESP32-S3-DevKitC-1
// -----------------------
#define PIN_I2C_SDA       16  // GPIO 16
#define PIN_I2C_SCL       9   // GPIO 9

// Direcciones I²C del sistema:
// 0x40 - PCA9685 (PWM motores tracción BTS7960, 8 canales usados)
// 0x41 - PCA9685 (PWM motor dirección RS390, cambiado para evitar conflicto con INA226)
// 0x20 - MCP23017 (control IN1/IN2 motores tracción BTS7960, 8 GPIOs usados)
// 0x70 - TCA9548A multiplexor I²C #1 (canales 0-7 para INA226)
// 0x71 - TCA9548A multiplexor I²C #2 (si se usa)
// 0x72 - TCA9548A multiplexor I²C #3 (si se usa)
// 0x73 - TCA9548A multiplexor I²C #4 (si se usa)
// 0x74 - TCA9548A multiplexor I²C #5 (si se usa)
//
// Asignación INA226 en TCA9548A (0x70):
// Canal 0: INA226 Motor FL (Frontal Izquierda) - Dirección 0x40 - Shunt 50A 75mV
// Canal 1: INA226 Motor FR (Frontal Derecha) - Dirección 0x40 - Shunt 50A 75mV
// Canal 2: INA226 Motor RL (Trasera Izquierda) - Dirección 0x40 - Shunt 50A 75mV
// Canal 3: INA226 Motor RR (Trasera Derecha) - Dirección 0x40 - Shunt 50A 75mV
// Canal 4: INA226 Batería 24V - Dirección 0x40 - Shunt 100A 75mV (CG FL-2C)
// Canal 5: INA226 Motor Dirección RS390 12V - Dirección 0x40 - Shunt 50A 75mV
//
// NOTA: Los INA226 todos usan dirección 0x40, pero están multiplexados vía TCA9548A
// para evitar conflicto. El PCA9685 de motores tracción también usa 0x40 pero NO
// está en el multiplexor, está directo en el bus I²C principal.

// -----------------------
// BTS7960 – Motores de rueda (controlados vía I²C)
// GPIOs 23-34 NO disponibles en ESP32-S3-DevKitC-1 44 pines
// SOLUCIÓN: Usar módulos I²C para control
// -----------------------
// Control PWM vía PCA9685 (I²C 0x40)
// - 16 canales PWM de 12 bits
// - Frecuencia configurable 24Hz - 1526Hz
// - Canales 0-7 usados para 4 motores (2 canales por motor para control bidireccional)
#define PCA9685_ADDR_MOTORS  0x40  // Dirección I²C PCA9685 para motores

// Asignación canales PCA9685 para PWM de motores
#define PCA_CH_FL_PWM     0   // Frontal Izquierda PWM (canal 0)
#define PCA_CH_FL_PWM_R   1   // Frontal Izquierda PWM Reverse (canal 1)
#define PCA_CH_FR_PWM     2   // Frontal Derecha PWM (canal 2)
#define PCA_CH_FR_PWM_R   3   // Frontal Derecha PWM Reverse (canal 3)
#define PCA_CH_RL_PWM     4   // Trasera Izquierda PWM (canal 4)
#define PCA_CH_RL_PWM_R   5   // Trasera Izquierda PWM Reverse (canal 5)
#define PCA_CH_RR_PWM     6   // Trasera Derecha PWM (canal 6)
#define PCA_CH_RR_PWM_R   7   // Trasera Derecha PWM Reverse (canal 7)

// Control IN1/IN2 vía MCP23017 (I²C 0x20)
// - 16 GPIOs digitales (GPIOA0-7, GPIOB0-7)
// - 8 pines usados para IN1/IN2 de 4 motores (GPIOA bank)
// - 1 pin usado para sensor rueda FR (GPIOB0)
#define MCP23017_ADDR_MOTORS 0x20  // Dirección I²C MCP23017 para motores

// Asignación pines MCP23017 para IN1/IN2 de BTS7960
#define MCP_PIN_FL_IN1    0   // GPIOA0: Frontal Izquierda IN1
#define MCP_PIN_FL_IN2    1   // GPIOA1: Frontal Izquierda IN2
#define MCP_PIN_FR_IN1    2   // GPIOA2: Frontal Derecha IN1
#define MCP_PIN_FR_IN2    3   // GPIOA3: Frontal Derecha IN2
#define MCP_PIN_RL_IN1    4   // GPIOA4: Trasera Izquierda IN1
#define MCP_PIN_RL_IN2    5   // GPIOA5: Trasera Izquierda IN2
#define MCP_PIN_RR_IN1    6   // GPIOA6: Trasera Derecha IN1
#define MCP_PIN_RR_IN2    7   // GPIOA7: Trasera Derecha IN2

// Asignación pines MCP23017 para sensores (GPIOB bank)
#define MCP_PIN_WHEEL1    8   // GPIOB0: Sensor rueda FR (Frontal Derecha)

// -----------------------
// LEDs WS2812B (Iluminación Inteligente)
// Asignados a GPIOs libres disponibles
// GPIO 0 reservado para llave/switch del sistema
// -----------------------
#define PIN_LED_FRONT     21  // GPIO 21: LEDs frontales (28 LEDs)
#define PIN_LED_REAR      1   // GPIO 1: LEDs traseros (16 LEDs)
#define NUM_LEDS_FRONT    28  // Cantidad LEDs frontales
#define NUM_LEDS_REAR     16  // Cantidad LEDs traseros

// -----------------------
// Helpers
// -----------------------
static inline bool pin_is_assigned(uint8_t gpio) {
  switch (gpio) {
    case PIN_KEY_SYSTEM:
    case PIN_RELAY_MAIN:
    case PIN_RELAY_TRAC:
    case PIN_RELAY_DIR:
    case PIN_RELAY_SPARE:
    case PIN_PEDAL:
    case PIN_ENCODER_A:
    case PIN_ENCODER_B:
    case PIN_ENCODER_Z:
    case PIN_SHIFTER_P:
    case PIN_SHIFTER_D2:
    case PIN_SHIFTER_D1:
    case PIN_SHIFTER_N:
    case PIN_SHIFTER_R:
    case PIN_DFPLAYER_RX:
    case PIN_DFPLAYER_TX:
    case PIN_TFT_CS:
    case PIN_TFT_DC:
    case PIN_TFT_RST:
    case PIN_TFT_MOSI:
    case PIN_TFT_MISO:
    case PIN_TFT_SCK:
    case PIN_TFT_BL:
    case PIN_TOUCH_CS:
    case PIN_TOUCH_IRQ:
    case PIN_BTN_LIGHTS:
    case PIN_BTN_MEDIA:
    case PIN_BTN_4X4:
    case PIN_WHEEL0:
    // PIN_WHEEL1 en MCP23017, no en ESP32 GPIO
    case PIN_WHEEL2:
    case PIN_WHEEL3:
    case PIN_ONEWIRE:
    case PIN_I2C_SDA:
    case PIN_I2C_SCL:
    case PIN_LED_FRONT:
    case PIN_LED_REAR:
      return true;
    default:
      return false;
  }
}