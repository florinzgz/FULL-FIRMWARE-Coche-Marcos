#pragma once
#include <stdint.h>

// ============================================================================
// pins.h - Asignación de pines para ESP32-S3-DevKitC-1 (44 pines)
// 🔒 ACTUALIZADO 2025-11-24 - Auditoría pinout físico + validación strapping pins
// ============================================================================
//
// ╔═══════════════════════════════════════════════════════════════════════════╗
// ║              LAYOUT FÍSICO ESP32-S3-DevKitC-1 (VISTA SUPERIOR)            ║
// ║                           USB Type-C arriba                                ║
// ╠═══════════════════════════════════════════════════════════════════════════╣
// ║                                                                            ║
// ║  LADO 1 (DERECHO):                                                         ║
// ║  GND GND 19 20 21 47 48 45* 0* 35 36 37 38 39 40 41 42 2 1 RX(44) TX(43) GND ║
// ║                                                                            ║
// ║  LADO 2 (IZQUIERDO):                                                       ║
// ║  GND 5V 14 13 12 11 10 9 46* 3 8 18 17 16 15 7 6 5 4 RST 3V3 3V3          ║
// ║                                                                            ║
// ║  * = STRAPPING PINS (Afectan modo boot):                                   ║
// ║      GPIO 0  → Boot mode select (pull-up interno, LOW=Download)           ║
// ║      GPIO 45 → VDD_SPI voltage (input only, pull-down=3.3V default)       ║
// ║      GPIO 46 → Boot mode/ROM messages (input only)                        ║
// ║                                                                            ║
// ╚═══════════════════════════════════════════════════════════════════════════╝
//
// PINES REALES DISPONIBLES EN LA PLACA (36 GPIOs utilizables)
//
// HARDWARE COMPLETO INTEGRADO:
// - ESP32-S3-DevKitC-1 (44 pines, 36 GPIOs utilizables)
// - 6x INA226 con shunts externos CG FL-2C (1x100A batería + 4x50A motores + 1x50A dirección)
// - 1x TCA9548A multiplexor I²C (para 6 INA226 sin conflicto dirección)
// - 🆕 2x PCA9685 PWM driver motores tracción (0x40 delantero, 0x41 trasero)
// - 1x PCA9685 PWM driver motor dirección (0x42)
// - 1x MCP23017 expansor GPIO I²C (16 pines, 0x20)
// - 2x HY-M158 optoacopladores PC817 (8 canales c/u = 16 total, aislamiento 12V→3.3V)
// - 4x BTS7960 drivers motor 43A (tracción 4 ruedas independientes)
// - 1x BTS7960 driver motor dirección (RS390 12V 6000RPM + reductora 1:50)
// - 1x Encoder E6B2-CWZ6C 1200PR (dirección, ratio 1:1 al volante)
// - 4x Sensores inductivos LJ12A3-4-Z/BX (velocidad ruedas)
// - 1x Sensor inductivo LJ12A3-4-Z/BX (señal Z encoder centrado)
// - 1x Sensor Hall A1324LUA-T (pedal analógico)
// - 4x DS18B20 sensores temperatura (motores tracción)
// - 1x Pantalla ST7796S 480x320 + táctil XPT2046 (SPI)
// - 1x DFPlayer Mini (audio, UART)
// - 2x Tiras LEDs WS2812B (iluminación delantera 28 LEDs + trasera 16 LEDs)
// - Relés: 4x SRD-05VDC (control potencia, luces, tracción, dirección)
// ============================================================================

// ============================================================================
// COMUNICACIONES
// ============================================================================

// -----------------------
// I2C (Bus principal)
// -----------------------
#define PIN_I2C_SDA       16  // GPIO 16 - Data
#define PIN_I2C_SCL       9   // GPIO 9  - Clock

// Direcciones I²C del sistema:
#define I2C_ADDR_PCA9685_FRONT    0x40  // PCA9685 #1: Motores EJE DELANTERO (FL+FR)
#define I2C_ADDR_PCA9685_REAR     0x41  // PCA9685 #2: Motores EJE TRASERO (RL+RR)
#define I2C_ADDR_PCA9685_STEERING 0x42  // PCA9685 #3: Motor dirección
#define I2C_ADDR_MCP23017         0x20  // MCP23017: Expansor GPIO (control IN1/IN2 BTS7960)
#define I2C_ADDR_TCA9548A         0x70  // TCA9548A: Multiplexor I²C para 6x INA226

// Asignación canales TCA9548A (cada canal tiene un INA226 0x40):
// Canal 0: INA226 Motor FL (Frontal Izquierda) - Shunt 50A 75mV
// Canal 1: INA226 Motor FR (Frontal Derecha) - Shunt 50A 75mV
// Canal 2: INA226 Motor RL (Trasera Izquierda) - Shunt 50A 75mV
// Canal 3: INA226 Motor RR (Trasera Derecha) - Shunt 50A 75mV
// Canal 4: INA226 Batería 24V - Shunt 100A 75mV (CG FL-2C)
// Canal 5: INA226 Motor Dirección RS390 12V - Shunt 50A 75mV

// -----------------------
// SPI (Pantalla TFT ST7796S 480x320)
// -----------------------
#define PIN_TFT_CS        8   // GPIO 8  - Chip Select
#define PIN_TFT_DC        13  // GPIO 13 - Data/Command
#define PIN_TFT_RST       14  // GPIO 14 - Reset
#define PIN_TFT_MOSI      11  // GPIO 11 - SPI MOSI
#define PIN_TFT_MISO      12  // GPIO 12 - SPI MISO
#define PIN_TFT_SCK       10  // GPIO 10 - SPI Clock
#define PIN_TFT_BL        42  // GPIO 42 - Backlight PWM (0-255 brightness control via LEDC)

// -----------------------
// Táctil (XPT2046 SPI)
// -----------------------
#define PIN_TOUCH_CS      3   // GPIO 3  - Chip Select (pin seguro, no es strapping)
#define PIN_TOUCH_IRQ     46  // GPIO 46 - Interrupción (⚠️ STRAPPING PIN, input-only)

// -----------------------
// UART (DFPlayer Mini - Audio)
// -----------------------
#define PIN_DFPLAYER_TX   43  // GPIO 43 - TX (UART0)
#define PIN_DFPLAYER_RX   44  // GPIO 44 - RX (UART0)

// ============================================================================
// POTENCIA Y CONTROL
// ============================================================================

// -----------------------
// Relés de potencia (4x SRD-05VDC-SL-C)
// -----------------------
#define PIN_RELAY_MAIN    2   // GPIO 2  - Relé principal (Power Hold)
#define PIN_RELAY_TRAC    4   // GPIO 4  - Relé tracción 24V
#define PIN_RELAY_DIR     5   // GPIO 5  - Relé dirección 12V
#define PIN_RELAY_SPARE   6   // GPIO 6  - Relé auxiliar (luces/media)

// -----------------------
// Llave/Switch del sistema
// -----------------------
#define PIN_KEY_SYSTEM    0   // GPIO 0 - Boot button (strapping pin, usar con pull-up)

// ============================================================================
// MOTORES TRACCIÓN (4x4) - Control vía I²C
// ============================================================================

// 🆕 PCA9685 #1 - EJE DELANTERO (I²C 0x40)
// - Canales PWM para BTS7960 motores FL y FR
#define PCA_FRONT_CH_FL_FWD    0   // Canal 0: FL Forward PWM
#define PCA_FRONT_CH_FL_REV    1   // Canal 1: FL Reverse PWM
#define PCA_FRONT_CH_FR_FWD    2   // Canal 2: FR Forward PWM
#define PCA_FRONT_CH_FR_REV    3   // Canal 3: FR Reverse PWM

// 🆕 PCA9685 #2 - EJE TRASERO (I²C 0x41)
// - Canales PWM para BTS7960 motores RL y RR
#define PCA_REAR_CH_RL_FWD     0   // Canal 0: RL Forward PWM
#define PCA_REAR_CH_RL_REV     1   // Canal 1: RL Reverse PWM
#define PCA_REAR_CH_RR_FWD     2   // Canal 2: RR Forward PWM
#define PCA_REAR_CH_RR_REV     3   // Canal 3: RR Reverse PWM

// MCP23017 (I²C 0x20) - Control IN1/IN2 de BTS7960
// - GPIOA bank: Control dirección motores
#define MCP_PIN_FL_IN1    0   // GPIOA0: FL IN1 (dirección)
#define MCP_PIN_FL_IN2    1   // GPIOA1: FL IN2 (dirección)
#define MCP_PIN_FR_IN1    2   // GPIOA2: FR IN1
#define MCP_PIN_FR_IN2    3   // GPIOA3: FR IN2
#define MCP_PIN_RL_IN1    4   // GPIOA4: RL IN1
#define MCP_PIN_RL_IN2    5   // GPIOA5: RL IN2
#define MCP_PIN_RR_IN1    6   // GPIOA6: RR IN1
#define MCP_PIN_RR_IN2    7   // GPIOA7: RR IN2

// ============================================================================
// MOTOR DIRECCIÓN
// ============================================================================

// PCA9685 #3 - DIRECCIÓN (I²C 0x42)
// - RS390 12V 6000RPM con reductora 1:50
#define PCA_STEER_CH_PWM_FWD   0   // Canal 0: Steering Forward PWM
#define PCA_STEER_CH_PWM_REV   1   // Canal 1: Steering Reverse PWM

// ============================================================================
// SENSORES
// ============================================================================

// -----------------------
// Encoder dirección (E6B2-CWZ6C 1200PR)
// Conectado vía HY-M158 optoacopladores (12V → 3.3V)
// -----------------------
#define PIN_ENCODER_A     37  // GPIO 37 - Canal A (cuadratura)
#define PIN_ENCODER_B     38  // GPIO 38 - Canal B (cuadratura)
#define PIN_ENCODER_Z     39  // GPIO 39 - Señal Z (centrado, 1 pulso/vuelta)

// -----------------------
// Pedal acelerador (Sensor Hall A1324LUA-T)
// Salida analógica 5V → divisor resistivo → 3.3V
// -----------------------
#define PIN_PEDAL         35  // GPIO 35 - ADC1_CH4 (0-3.3V)

// -----------------------
// Sensores inductivos ruedas (4x LJ12A3-4-Z/BX)
// Conectados vía HY-M158 optoacopladores (12V → 3.3V)
// 6 tornillos por rueda = 6 pulsos/revolución
// -----------------------
#define PIN_WHEEL_FL      21  // GPIO 21 - Wheel Front Left (velocímetro)
#define PIN_WHEEL_FR      36  // GPIO 36 - Wheel Front Right
#define PIN_WHEEL_RL      17  // GPIO 17 - Wheel Rear Left
#define PIN_WHEEL_RR      15  // GPIO 15 - Wheel Rear Right

// -----------------------
// Temperatura motores (4x DS18B20 OneWire)
// Un sensor por motor de tracción
// -----------------------
#define PIN_ONEWIRE       20  // GPIO 20 - Bus OneWire (4 sensores en paralelo)

// ============================================================================
// ENTRADAS DIGITALES
// ============================================================================

// -----------------------
// Palanca de cambios (Shifter) - 5 posiciones
// Conectada vía HY-M158 optoacopladores (12V → 3.3V)
// -----------------------
#define PIN_SHIFTER_P     47  // GPIO 47 - Posición P (Park)
#define PIN_SHIFTER_D2    48  // GPIO 48 - Posición D2 (Drive 2 - alta velocidad)
#define PIN_SHIFTER_D1    7   // GPIO 7  - Posición D1 (Drive 1 - baja velocidad)
#define PIN_SHIFTER_N     18  // GPIO 18 - Posición N (Neutral)

// ⚠️ CONFLICTO RESUELTO: GPIO 19 compartido con LED_REAR
// PIN_SHIFTER_R movido a MCP23017 GPIOB0 para evitar conflicto hardware
// El código en shifter.cpp debe usar MCP23017 para leer R via I2C
#define MCP_PIN_SHIFTER_R 8   // MCP23017 GPIOB0: R (Reverse) - vía I2C

// Macro de compatibilidad - ADVERTENCIA: No usar GPIO 19 directamente
// Si se necesita el pin GPIO directo, reasignar LED_REAR a otra ubicación
#ifdef SHIFTER_USE_GPIO_LEGACY
#warning "SHIFTER_USE_GPIO_LEGACY está definido: GPIO 19 en conflicto con PIN_LED_REAR"
#define PIN_SHIFTER_R     19  // ⚠️ CONFLICTO con PIN_LED_REAR - Solo para compatibilidad
#endif

// -----------------------
// Botones físicos
// Conectados vía HY-M158 optoacopladores (12V → 3.3V)
// -----------------------
#define PIN_BTN_LIGHTS    45  // GPIO 45 - Botón luces (⚠️ STRAPPING PIN - input only, VDD_SPI)
#define PIN_BTN_MEDIA     40  // GPIO 40 - Botón multimedia
#define PIN_BTN_4X4       41  // GPIO 41 - Botón 4x4/4x2 (switch 2 posiciones)

// ============================================================================
// SALIDAS PWM/LED
// ============================================================================

// -----------------------
// LEDs WS2812B (Iluminación Inteligente)
// -----------------------
#define PIN_LED_FRONT     1   // GPIO 1  - LEDs frontales (28 LEDs)
#define PIN_LED_REAR      19  // GPIO 19 - LEDs traseros (16 LEDs) - ✅ Libre tras mover SHIFTER_R
#define NUM_LEDS_FRONT    28  // Cantidad LEDs frontales
#define NUM_LEDS_REAR     16  // Cantidad LEDs traseros (3L + 10C + 3R)

// ============================================================================
// TABLA RESUMEN DE USO DE PINES (Actualizada 2025-11-24)
// ============================================================================
/*
GPIO  | Función                    | Tipo      | Notas
------|----------------------------|-----------|----------------------------------
0     | KEY_SYSTEM                 | Input     | ⚠️ STRAPPING PIN (Boot), pull-up externo
1     | LED_FRONT (WS2812B)        | Output    | 28 LEDs
2     | RELAY_MAIN                 | Output    | Relé principal (Power Hold)
3     | TOUCH_CS                   | Output    | SPI CS táctil (pin seguro)
4     | RELAY_TRAC                 | Output    | Relé tracción 24V
5     | RELAY_DIR                  | Output    | Relé dirección 12V
6     | RELAY_SPARE                | Output    | Relé auxiliar
7     | SHIFTER_D1                 | Input     | Palanca D1 (via optoacoplador)
8     | TFT_CS                     | Output    | SPI CS pantalla
9     | I2C_SCL                    | I/O       | Bus I2C Clock (+ pull-up 4.7kΩ)
10    | TFT_SCK                    | Output    | SPI Clock
11    | TFT_MOSI                   | Output    | SPI MOSI
12    | TFT_MISO                   | Input     | SPI MISO
13    | TFT_DC                     | Output    | Data/Command pantalla
14    | TFT_RST                    | Output    | Reset pantalla
15    | WHEEL_RR                   | Input     | Sensor rueda trasera derecha
16    | I2C_SDA                    | I/O       | Bus I2C Data (+ pull-up 4.7kΩ)
17    | WHEEL_RL                   | Input     | Sensor rueda trasera izquierda
18    | SHIFTER_N                  | Input     | Palanca Neutral (via optoacoplador)
19    | LED_REAR (WS2812B)         | Output    | 16 LEDs traseros (SHIFTER_R → MCP23017)
20    | ONEWIRE                    | I/O       | 4x DS18B20 temperatura
21    | WHEEL_FL                   | Input     | Sensor rueda delantera izquierda
35    | PEDAL (ADC)                | Analog In | Sensor Hall pedal (ADC1_CH4)
36    | WHEEL_FR                   | Input     | Sensor rueda delantera derecha
37    | ENCODER_A                  | Input     | Encoder dirección A (cuadratura)
38    | ENCODER_B                  | Input     | Encoder dirección B (cuadratura)
39    | ENCODER_Z                  | Input     | Encoder dirección Z (índice)
40    | BTN_MEDIA                  | Input     | Botón multimedia
41    | BTN_4X4                    | Input     | Botón modo 4x4/4x2
42    | TFT_BL (PWM)               | Output    | Backlight pantalla (LEDC PWM)
43    | DFPLAYER_TX (UART)         | Output    | Audio TX (UART0)
44    | DFPLAYER_RX (UART)         | Input     | Audio RX (UART0)
45    | BTN_LIGHTS                 | Input     | ⚠️ STRAPPING PIN (input-only, VDD_SPI)
46    | TOUCH_IRQ                  | Input     | ⚠️ STRAPPING PIN (input-only)
47    | SHIFTER_P                  | Input     | Palanca Park (via optoacoplador)
48    | SHIFTER_D2                 | Input     | Palanca D2 (via optoacoplador)

MCP23017 GPIOB0 → SHIFTER_R (Reverse) - Movido desde GPIO 19 para evitar conflicto con LED_REAR

TOTAL: 34/36 pines GPIO + 1 MCP23017 (97% eficiencia)
STRAPPING PINS: GPIO 0 (boot), GPIO 45 (VDD_SPI), GPIO 46 (ROM messages)
*/

// ============================================================================
// HELPERS
// ============================================================================

static inline bool pin_is_assigned(uint8_t gpio) {
    switch (gpio) {
        case PIN_KEY_SYSTEM:
        case PIN_LED_FRONT:
        case PIN_RELAY_MAIN:
        case PIN_TOUCH_CS:
        case PIN_RELAY_TRAC:
        case PIN_RELAY_DIR:
        case PIN_RELAY_SPARE:
        case PIN_SHIFTER_D1:
        case PIN_TFT_CS:
        case PIN_I2C_SCL:
        case PIN_TFT_SCK:
        case PIN_TFT_MOSI:
        case PIN_TFT_MISO:
        case PIN_TFT_DC:
        case PIN_TFT_RST:
        case PIN_WHEEL_RR:
        case PIN_I2C_SDA:
        case PIN_WHEEL_RL:
        case PIN_SHIFTER_N:
        case PIN_LED_REAR:
        case PIN_ONEWIRE:
        case PIN_WHEEL_FL:
        case PIN_PEDAL:
        case PIN_WHEEL_FR:
        case PIN_ENCODER_A:
        case PIN_ENCODER_B:
        case PIN_ENCODER_Z:
        case PIN_BTN_MEDIA:
        case PIN_BTN_4X4:
        case PIN_TFT_BL:
        case PIN_DFPLAYER_TX:
        case PIN_DFPLAYER_RX:
        case PIN_BTN_LIGHTS:
        case PIN_TOUCH_IRQ:
        case PIN_SHIFTER_P:
        case PIN_SHIFTER_D2:
            return true;
        default:
            return false;
    }
}
