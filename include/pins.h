#pragma once
#include <stddef.h>
#include <stdint.h>

// ============================================================================
// pins.h - Asignación de pines para ESP32-S3-DevKitC-1 (44 pines)
// 🔒 ACTUALIZADO 2026-01-05 v2.15.0 - Power pins en GPIO 40/41 estables
// ============================================================================
//
// PINES REALES DISPONIBLES EN LA PLACA (36 GPIOs):
// LADO 1 (mirando desde arriba): GND,GND,19,20,21,47,48,45,0,35,36,37,38,39,40,41,42,2,1,RX(44),TX(43),GND
// LADO 2 (mirando desde arriba): GND,5V,14,13,12,11,10,9,46,3,8,18,17,16,15,7,6,5,4,RST,3V3,3V3
//
// ⚠️ STRAPPING PINS (EVITAR para funciones críticas):
// GPIO 0  - Boot mode (HIGH=SPI Boot, LOW=Download) → LIBRE (ya no se usa para power)
// GPIO 3  - JTAG (evitar si se usa JTAG)
// GPIO 45 - VDD_SPI voltage select → LIBRE (ya no se usa para power)
// GPIO 46 - Boot mode / ROM log → EN USO (Relé auxiliar desde v2.11.3)
// GPIO 43 - UART0 TX (TOFSense-M S, no conectado al sensor)
// GPIO 44 - UART0 RX (TOFSense-M S, recibe datos del sensor)
//
// 🔒 ⚠️ GPIO 46 (STRAPPING PIN) - AHORA EN USO:
// v2.11.3+: GPIO 46 asignado a PIN_RELAY_SPARE (Relé auxiliar).
// ANTES v2.11.3: GPIO 46 se usaba como XSHUT_FRONT del sensor VL53L5CX.
// v2.12.0: Con la migración a TOFSense-M S UART, este pin crítico de strapping quedó liberado.
//
// NOTA: Como strapping pin, el relé debe estar configurado como OUTPUT y manejado
// apropiadamente durante el boot para evitar activar modo ROM log.
//
// ✅ PINES MÁS SEGUROS Y ESTABLES:
// GPIO 19, 20, 21 → Muy estables, ideales para SPI/I²C periféricos
// GPIO 35-42 → Seguros, no afectan boot
// GPIO 4-18 → Disponibles con algunas restricciones
//
// HARDWARE COMPLETO INTEGRADO:
// - ESP32-S3-DevKitC-1 (44 pines, 36 GPIOs utilizables)
// - 6x INA226 con shunts externos CG FL-2C (1x100A batería + 4x50A motores + 1x50A dirección)
// - 1x TCA9548A multiplexor I²C (para 6 INA226 sin conflicto dirección)
// - 2x PCA9685 PWM driver motores tracción (0x40 delantero, 0x41 trasero)
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
// - 1x DFPlayer Mini (audio, UART1 GPIO18/17)
// - 1x TOFSense-M S LiDAR 8x8 (UART0 GPIO44/43)
// - 2x Tiras LEDs WS2812B (iluminación delantera 28 LEDs + trasera 16 LEDs)
// - Relés: 4x SRD-05VDC (control potencia, luces, tracción, dirección)
// ============================================================================

// ============================================================================
// COMUNICACIONES I²C
// ============================================================================

// -----------------------
// I2C (Bus principal) - Pines estándar ESP32-S3
// -----------------------
#define PIN_I2C_SDA       8   // GPIO 8  - Data (pin estable, no strapping)
#define PIN_I2C_SCL       9   // GPIO 9  - Clock (pin estable)

// Direcciones I²C del sistema:
#define I2C_ADDR_PCA9685_FRONT    0x40  // PCA9685 #1: Motores EJE DELANTERO (FL+FR)
#define I2C_ADDR_PCA9685_REAR     0x41  // PCA9685 #2: Motores EJE TRASERO (RL+RR)
#define I2C_ADDR_PCA9685_STEERING 0x42  // PCA9685 #3: Motor dirección
#define I2C_ADDR_MCP23017         0x20  // MCP23017: Expansor GPIO (control IN1/IN2 BTS7960 + Shifter D2)
#define I2C_ADDR_TCA9548A         0x70  // TCA9548A: Multiplexor I²C para 6x INA226

// Asignación canales TCA9548A (cada canal tiene un INA226 0x40):
// Canal 0: INA226 Motor FL (Frontal Izquierda) - Shunt 50A 75mV
// Canal 1: INA226 Motor FR (Frontal Derecha) - Shunt 50A 75mV
// Canal 2: INA226 Motor RL (Trasera Izquierda) - Shunt 50A 75mV
// Canal 3: INA226 Motor RR (Trasera Derecha) - Shunt 50A 75mV
// Canal 4: INA226 Batería 24V - Shunt 100A 75mV (CG FL-2C)
// Canal 5: INA226 Motor Dirección RS390 12V - Shunt 50A 75mV

// ============================================================================
// COMUNICACIONES SPI - PANTALLA TFT ST7796S 480x320
// ============================================================================

// Bus SPI ordenado en pines consecutivos (10-14)
#define PIN_TFT_SCK       10  // GPIO 10 - SPI Clock
#define PIN_TFT_MOSI      11  // GPIO 11 - SPI MOSI (Master Out)
#define PIN_TFT_MISO      12  // GPIO 12 - SPI MISO (Master In)
#define PIN_TFT_DC        13  // GPIO 13 - Data/Command
#define PIN_TFT_RST       14  // GPIO 14 - Reset
#define PIN_TFT_CS        16  // GPIO 16 - Chip Select TFT
#define PIN_TFT_BL        42  // GPIO 42 - Backlight PWM (LEDC)

// -----------------------
// Táctil (XPT2046 SPI) - ✅ OPTIMIZADO v2.3.0
// CS movido de GPIO 3 (strapping) → GPIO 21 (seguro)
// -----------------------
#define PIN_TOUCH_CS      21  // GPIO 21 - Chip Select Touch ✅ Pin seguro (antes GPIO 3)
#define PIN_TOUCH_IRQ     47  // GPIO 47 - Interrupción Touch (antes GPIO 46 strapping)

// ============================================================================
// COMUNICACIONES UART
// ============================================================================

// -----------------------
// UART0 (TOFSense-M S - Obstacle Detection LiDAR)
// Baudrate: 115200, protocolo de 9 bytes (header 0x57)
// El sensor solo tiene TX (salida), se conecta a GPIO44 RX del ESP32
// GPIO43 TX no se usa pero se asigna para completar el par UART0
// -----------------------
#define PIN_TOFSENSE_TX   43  // GPIO 43 - TX UART0 (ESP32 → Sensor RX, bidireccional)
#define PIN_TOFSENSE_RX   44  // GPIO 44 - RX UART0 (Sensor TX → ESP32, recibe datos)

// -----------------------
// UART1 (DFPlayer Mini - Audio)
// Movido de UART0 a UART1 para liberar los pines nativos para TOFSense
// -----------------------
#define PIN_DFPLAYER_TX   18  // GPIO 18 - TX UART1 (envía comandos al DFPlayer)
#define PIN_DFPLAYER_RX   17  // GPIO 17 - RX UART1 (recibe respuestas del DFPlayer)

// ============================================================================
// RELÉS DE POTENCIA (4x SRD-05VDC-SL-C)
// ✅ v2.9.1: RELAY_MAIN movido de GPIO 4 → GPIO 35 (GPIO 4 ahora es ADC para pedal)
// ============================================================================

#define PIN_RELAY_MAIN    35  // GPIO 35 - Relé principal (Power Hold) ✅ Movido de GPIO 4
#define PIN_RELAY_TRAC    5   // GPIO 5  - Relé tracción 24V
#define PIN_RELAY_DIR     6   // GPIO 6  - Relé dirección 12V
#define PIN_RELAY_SPARE   46  // GPIO 46 - Relé auxiliar (luces/media) ✅ Movido de GPIO 7

// ============================================================================
// ENTRADA SISTEMA - CONTROL DE ALIMENTACIÓN
// ============================================================================

// -----------------------
// Llave/Switch del sistema - ✅ v2.15.0: Movido a GPIO estables 40/41
// Anteriormente en GPIO 0/45 (strapping pins problemáticos)
// Ahora en GPIO 40/41 (liberados de botones multimedia/4x4, pines estables)
// -----------------------
#define PIN_KEY_ON        40  // GPIO 40 - Ignition ON detection (INPUT_PULLUP, LOW=ON) ✅ v2.15.0
#define PIN_KEY_OFF       41  // GPIO 41 - Shutdown request (INPUT_PULLUP, LOW=Shutdown) ✅ v2.15.0

// ⚠️ DEPRECATED - Ya no se usan (ahora GPIO 40/41):
// #define PIN_KEY_SYSTEM    0   // GPIO 0 - FREED (era strapping pin problemático)
// PIN_KEY_DETECT anteriormente en GPIO 45 (ahora libre)

// ============================================================================
// MOTORES TRACCIÓN (4x4) - Control vía I²C (PCA9685 + MCP23017)
// ============================================================================

// PCA9685 #1 - EJE DELANTERO (I²C 0x40)
// Canales PWM para BTS7960 motores FL y FR
#define PCA_FRONT_CH_FL_FWD    0   // Canal 0: FL Forward PWM
#define PCA_FRONT_CH_FL_REV    1   // Canal 1: FL Reverse PWM
#define PCA_FRONT_CH_FR_FWD    2   // Canal 2: FR Forward PWM
#define PCA_FRONT_CH_FR_REV    3   // Canal 3: FR Reverse PWM

// PCA9685 #2 - EJE TRASERO (I²C 0x41)
// Canales PWM para BTS7960 motores RL y RR
#define PCA_REAR_CH_RL_FWD     0   // Canal 0: RL Forward PWM
#define PCA_REAR_CH_RL_REV     1   // Canal 1: RL Reverse PWM
#define PCA_REAR_CH_RR_FWD     2   // Canal 2: RR Forward PWM
#define PCA_REAR_CH_RR_REV     3   // Canal 3: RR Reverse PWM

// MCP23017 (I²C 0x20) - Control IN1/IN2 de BTS7960 + Shifter completo
// GPIOA bank (0-7): Control dirección motores tracción
#define MCP_PIN_FL_IN1        0   // GPIOA0: FL IN1 (dirección motor)
#define MCP_PIN_FL_IN2        1   // GPIOA1: FL IN2
#define MCP_PIN_FR_IN1        2   // GPIOA2: FR IN1
#define MCP_PIN_FR_IN2        3   // GPIOA3: FR IN2
#define MCP_PIN_RL_IN1        4   // GPIOA4: RL IN1
#define MCP_PIN_RL_IN2        5   // GPIOA5: RL IN2
#define MCP_PIN_RR_IN1        6   // GPIOA6: RR IN1
#define MCP_PIN_RR_IN2        7   // GPIOA7: RR IN2

// GPIOB bank (8-15): Shifter completo (5 posiciones consecutivas) ✅ v2.3.0
// Todos los pines del shifter ahora en MCP23017 para mejor organización
#define MCP_PIN_SHIFTER_P     8   // GPIOB0: Shifter P (Park)
#define MCP_PIN_SHIFTER_R     9   // GPIOB1: Shifter R (Reverse)
#define MCP_PIN_SHIFTER_N     10  // GPIOB2: Shifter N (Neutral)
#define MCP_PIN_SHIFTER_D1    11  // GPIOB3: Shifter D1 (Drive 1 - baja velocidad)
#define MCP_PIN_SHIFTER_D2    12  // GPIOB4: Shifter D2 (Drive 2 - alta velocidad)

// MCP23017 pines para motor dirección (BTS7960)
// Usar GPIOB5-B6 (disponibles según tabla línea 333-334)
#define MCP_PIN_STEER_IN1     13  // GPIOB5: Steering R_EN
#define MCP_PIN_STEER_IN2     14  // GPIOB6: Steering L_EN
// GPIOB7 (pin 15) disponible para expansión futura

// ============================================================================
// MOTOR DIRECCIÓN
// ============================================================================

// PCA9685 #3 - DIRECCIÓN (I²C 0x42)
// RS390 12V 6000RPM con reductora 1:50
#define PCA_STEER_CH_PWM_FWD   0   // Canal 0: Steering Forward PWM
#define PCA_STEER_CH_PWM_REV   1   // Canal 1: Steering Reverse PWM

// ============================================================================
// SENSORES - ENCODER DIRECCIÓN
// Pines consecutivos 37-39 para encoder cuadratura
// ============================================================================

// Encoder E6B2-CWZ6C 1200PR (dirección)
// Conectado vía HY-M158 optoacopladores (12V → 3.3V)
#define PIN_ENCODER_A     37  // GPIO 37 - Canal A (cuadratura)
#define PIN_ENCODER_B     38  // GPIO 38 - Canal B (cuadratura)
#define PIN_ENCODER_Z     39  // GPIO 39 - Señal Z (centrado, 1 pulso/vuelta)

// ============================================================================
// SENSORES - PEDAL Y RUEDAS
// ============================================================================

// -----------------------
// Pedal acelerador (Sensor Hall A1324LUA-T)
// Salida analógica 5V → divisor resistivo → 3.3V
// ✅ v2.9.1: Cambiado de GPIO 35 → GPIO 4 (pin ADC válido en ESP32-S3)
// ESP32-S3 ADC válidos: GPIO 1-10 (ADC1), GPIO 11-20 (ADC2)
// -----------------------
#define PIN_PEDAL         4   // GPIO 4 - ADC1_CH3 (entrada analógica 0-3.3V) ✅ Corregido

// -----------------------
// Sensores inductivos ruedas (4x LJ12A3-4-Z/BX)
// Conectados vía HY-M158 optoacopladores (12V → 3.3V)
// 6 tornillos por rueda = 6 pulsos/revolución
// Ordenados: FL, FR, RL, RR
// ✅ v2.16.0: FIX CRÍTICO - WHEEL_RR movido GPIO 16 → GPIO 1
// GPIO 16 causaba conflicto con PIN_TFT_CS (SPI display)
// GPIO 46 es strapping pin (Boot mode/ROM log) - evitado
// GPIO 1 es seguro, estable, no afecta boot (antes libre)
// -----------------------
#define PIN_WHEEL_FL      7   // GPIO 7  - Wheel Front Left ✅ v2.17.2: Movido de GPIO 3 (strapping JTAG)
#define PIN_WHEEL_FR      36  // GPIO 36 - Wheel Front Right
#define PIN_WHEEL_RL      15  // GPIO 15 - Wheel Rear Left
#define PIN_WHEEL_RR      1   // GPIO 1  - Wheel Rear Right ✅ v2.16.0: Movido GPIO 16→46→1 (evitar strapping)

// -----------------------
// Temperatura motores (4x DS18B20 OneWire)
// Un sensor por motor de tracción, todos en bus paralelo
// -----------------------
#define PIN_ONEWIRE       20  // GPIO 20 - Bus OneWire (4 sensores en paralelo)

// ============================================================================
// ENTRADAS DIGITALES - SHIFTER (vía MCP23017)
// ============================================================================

// -----------------------
// Shifter (Palanca de cambios) - 5 posiciones
// ✅ v2.3.0: TODO el shifter migrado a MCP23017 GPIOB (pines consecutivos 8-12)
// Conectada vía HY-M158 optoacopladores (12V → 3.3V) → MCP23017
// -----------------------
// 🔒 NOTA DE POLARIDAD HARDWARE:
// Los optoacopladores HY-M158 (PC817) invierten la señal:
// - Shifter activo (posición seleccionada) → MCP23017 lee LOW (0)
// - Shifter inactivo → MCP23017 lee HIGH (1) por pull-up interno
//
// Pines MCP23017 asignados (ver sección MCP23017):
// MCP_PIN_SHIFTER_P  = 8  (GPIOB0) - Park
// MCP_PIN_SHIFTER_R  = 9  (GPIOB1) - Reverse  
// MCP_PIN_SHIFTER_N  = 10 (GPIOB2) - Neutral
// MCP_PIN_SHIFTER_D1 = 11 (GPIOB3) - Drive 1
// MCP_PIN_SHIFTER_D2 = 12 (GPIOB4) - Drive 2
// -----------------------
// 🆕 GPIOs liberados: 45 queda libre tras deshabilitar sensores laterales

// ============================================================================
// ENTRADAS DIGITALES - BOTONES
// ============================================================================

// -----------------------
// Botones físicos
// Conectados vía HY-M158 optoacopladores (12V → 3.3V)
// v2.14.0: Botones multimedia y 4x4 eliminados, control por touch screen
// v2.15.0: GPIO 40/41 reasignados a control de alimentación (PIN_KEY_ON/OFF)
// -----------------------
// #define PIN_BTN_MEDIA     40  // GPIO 40 - Ahora PIN_KEY_ON (power on detection)
// #define PIN_BTN_4X4       41  // GPIO 41 - Ahora PIN_KEY_OFF (shutdown request)
#define PIN_BTN_LIGHTS    2   // GPIO 2  - Botón luces ✅ Movido de GPIO 45

// ============================================================================
// SALIDAS - LEDs WS2812B (Iluminación Inteligente)
// ============================================================================
// 🔒 HISTORIAL DE CAMBIOS:
// - v2.3.0: PIN_LED_REAR movido de GPIO 19 → GPIO 48 (liberar GPIO 19)
// - v2.4.1: GPIO 19 reasignado a XSHUT_REAR (sensor obstáculos VL53L5X trasero)
// - v2.12.0: GPIO 18 liberado (UART1 para DFPlayer), PIN_LED_FRONT mantiene GPIO 19
// - v2.13.0: Confirmado GPIO 19 para LED_FRONT (8x8 matrix migration)

#define PIN_LED_FRONT     19  // GPIO 19 - LEDs frontales (28 LEDs)
#define PIN_LED_REAR      48  // GPIO 48 - LEDs traseros (16 LEDs) ✅ v2.3.0: movido desde GPIO 19
#define NUM_LEDS_FRONT    28  // Cantidad LEDs frontales (sin cambio)
#define NUM_LEDS_REAR     16  // Cantidad LEDs traseros (sin cambio)

// ============================================================================
// SENSORES OBSTÁCULOS - TOFSense-M S (LiDAR UART 8x8 Matrix)
// ============================================================================
// 🔒 v2.13.0: Migración a TOFSense-M S 8x8 Matrix Mode
// - Eliminados sensores VL53L5X (I2C) y multiplexador PCA9548A @ 0x71
// - Nuevo sensor único TOFSense-M S 8x8 matrix conectado por UART0 (nativo)
// - Protocolo: 400 bytes, header 57 01 FF 00, baudrate 921600, 64 puntos de distancia
// - Pines: GPIO44=RX (recibe datos), GPIO43=TX (no usado por sensor)
// - Rango: 4 metros, FOV: 65°, Update rate: ~15Hz
// - GPIO 46 EN USO (Relé auxiliar desde v2.11.3) ⚠️ Strapping pin
//
// NOTA: La configuración UART está en la sección COMUNICACIONES UART más arriba
// PIN_TOFSENSE_TX = 43 (GPIO 43 - ESP32 TX → Sensor RX, para configuración bidireccional)
// PIN_TOFSENSE_RX = 44 (GPIO 44 - Sensor TX → ESP32 RX, recibe datos)
//
// 🔒 ARQUITECTURA MULTIPLEXOR I2C (actualizada):
// El sistema ahora usa UN SOLO multiplexor I2C:
// 1. TCA9548A @ 0x70: Para 6x INA226 (sensores corriente, canales 0-5)
// 2. PCA9548A @ 0x71: ELIMINADO (era para VL53L5X, ya no se usa)

// ============================================================================
// TABLA RESUMEN DE USO DE PINES v2.13.0 (8x8 Matrix Update)
// ============================================================================
/*
┌──────┬─────────────────────────┬───────────┬─────────────────────────────────┐
│ GPIO │ Función                 │ Tipo      │ Notas                           │
├──────┼─────────────────────────┼───────────┼─────────────────────────────────┤
│  0   │ KEY_SYSTEM              │ Input     │ ⚠️ Strapping (Boot), pull-up ext │
│  1   │ WHEEL_RR                │ Input     │ ✅ v2.16.1: Rueda trasera der   │
│  2   │ BTN_LIGHTS              │ Input     │ Botón luces                     │
│  3   │ 🆓 LIBRE                │ -         │ ✅ v2.17.2: WHEEL_FL→GPIO 7     │
│  4   │ PEDAL (ADC)             │ Analog In │ ✅ v2.9.1: Sensor Hall pedal     │
│  5   │ RELAY_TRAC              │ Output    │ Relé tracción 24V               │
│  6   │ RELAY_DIR               │ Output    │ Relé dirección 12V              │
│  7   │ WHEEL_FL                │ Input     │ ✅ v2.17.2: Movido desde GPIO 3 │
│  8   │ I2C_SDA                 │ I/O       │ Bus I²C Data                    │
│  9   │ I2C_SCL                 │ I/O       │ Bus I²C Clock                   │
│ 10   │ TFT_SCK                 │ Output    │ SPI Clock                       │
│ 11   │ TFT_MOSI                │ Output    │ SPI MOSI                        │
│ 12   │ TFT_MISO                │ Input     │ SPI MISO                        │
│ 13   │ TFT_DC                  │ Output    │ Data/Command                    │
│ 14   │ TFT_RST                 │ Output    │ Reset pantalla                  │
│ 15   │ WHEEL_RL                │ Input     │ ✅ v2.12.0: Rueda trasera izq   │
│ 16   │ TFT_CS (SPI)            │ Output    │ ✅ Chip Select display          │
│ 17   │ DFPLAYER_RX (UART1)     │ Input     │ ✅ v2.12.0: Mini Audio RX       │
│ 18   │ DFPLAYER_TX (UART1)     │ Output    │ ✅ v2.12.0: Mini Audio TX       │
│ 19   │ LED_FRONT (WS2812B)     │ Output    │ 28 LEDs frontales               │
│ 20   │ ONEWIRE                 │ I/O       │ 4x DS18B20 temperatura          │
│ 21   │ TOUCH_CS                │ Output    │ ✅ CS Touch (seguro)             │
│ 35   │ RELAY_MAIN              │ Output    │ ✅ v2.9.1: Relé principal        │
│ 36   │ WHEEL_FR                │ Input     │ Sensor rueda delantera derecha  │
│ 37   │ ENCODER_A               │ Input     │ Encoder dirección A             │
│ 38   │ ENCODER_B               │ Input     │ Encoder dirección B             │
│ 39   │ ENCODER_Z               │ Input     │ Encoder dirección Z             │
│ 40   │ 🆓 LIBRE                │ -         │ ✅ v2.14.0: BTN_MEDIA eliminado │
│ 41   │ 🆓 LIBRE                │ -         │ ✅ v2.14.0: BTN_4X4 eliminado   │
│ 42   │ TFT_BL (PWM)            │ Output    │ Backlight pantalla              │
│ 43   │ TOFSENSE_TX (UART0)     │ Output    │ ✅ v2.12.0: TOFSense (no usado) │
│ 44   │ TOFSENSE_RX (UART0)     │ Input     │ ✅ v2.12.0: TOFSense RX LiDAR   │
│ 45   │ KEY_DETECT (power_mgmt) │ Input     │ ⚠️ STRAPPING PIN: VDD_SPI       │
│ 46   │ RELAY_SPARE             │ Output    │ ⚠️ STRAPPING: Relé auxiliar     │
│ 47   │ TOUCH_IRQ               │ Input     │ Interrupción táctil             │
│ 48   │ LED_REAR (WS2812B)      │ Output    │ 16 LEDs traseros                │
└──────┴─────────────────────────┴───────────┴─────────────────────────────────┘

MCP23017 (I²C 0x20) - Expansor GPIO:
┌──────┬─────────────────────────┬───────────┬─────────────────────────────────┐
│ Pin  │ Función                 │ Tipo      │ Notas                           │
├──────┼─────────────────────────┼───────────┼─────────────────────────────────┤
│ A0   │ FL_IN1                  │ Output    │ Motor FL dirección              │
│ A1   │ FL_IN2                  │ Output    │ Motor FL dirección              │
│ A2   │ FR_IN1                  │ Output    │ Motor FR dirección              │
│ A3   │ FR_IN2                  │ Output    │ Motor FR dirección              │
│ A4   │ RL_IN1                  │ Output    │ Motor RL dirección              │
│ A5   │ RL_IN2                  │ Output    │ Motor RL dirección              │
│ A6   │ RR_IN1                  │ Output    │ Motor RR dirección              │
│ A7   │ RR_IN2                  │ Output    │ Motor RR dirección              │
│ B0   │ SHIFTER_P               │ Input     │ ✅ Palanca Park (consecutivo)    │
│ B1   │ SHIFTER_R               │ Input     │ ✅ Palanca Reverse               │
│ B2   │ SHIFTER_N               │ Input     │ ✅ Palanca Neutral               │
│ B3   │ SHIFTER_D1              │ Input     │ ✅ Palanca Drive 1               │
│ B4   │ SHIFTER_D2              │ Input     │ ✅ Palanca Drive 2               │
│ B5   │ STEER_IN1               │ Output    │ ✅ Motor dirección R_EN         │
│ B6   │ STEER_IN2               │ Output    │ ✅ Motor dirección L_EN         │
│ B7   │ 🆓 LIBRE                │ -         │ Disponible para expansión       │
└──────┴─────────────────────────┴───────────┴─────────────────────────────────┘

MEJORAS v2.3.0:
✅ TOUCH_CS: GPIO 3 → GPIO 21 (evita strapping pin)
✅ LED_REAR: GPIO 19 → GPIO 48 (resuelve conflicto)
✅ TOUCH_IRQ: GPIO 46 → GPIO 47 (evita strapping pin)
✅ SHIFTER COMPLETO: GPIOs dispersos → MCP23017 GPIOB0-B4 (pines consecutivos)

MEJORAS v2.17.2:
✅ PIN_WHEEL_FL: GPIO 3 → GPIO 7 (evita strapping pin JTAG, mejora confiabilidad)
✅ GPIO 3 liberado (strapping pin, ahora disponible para uso futuro)

MEJORAS v2.4.1:
✅ VL53L5X XSHUT: Asignados a GPIO 18, 19, 45, 46 (antes libres)
✅ Corrección conflicto: GPIO 7,8,10,11 ya estaban en uso

MEJORAS v2.9.1:
✅ PIN_PEDAL: GPIO 35 → GPIO 4 (GPIO 35 no es ADC en ESP32-S3)
✅ PIN_RELAY_MAIN: GPIO 4 → GPIO 35 (intercambiado con pedal)

MEJORAS v2.12.0:
✅ Migración VL53L5X I2C → TOFSense-M S UART
✅ TOFSENSE UART0 nativo: GPIO 44=RX (datos sensor), GPIO 43=TX (no usado)
✅ DFPLAYER UART1: GPIO 18=TX, GPIO 17=RX (movido de UART0)
✅ WHEEL_RL/RR: Mantienen GPIO 15/16 (disponibles)
✅ LED_FRONT: Mantiene GPIO 19
✅ GPIO 43/44 ahora usados para TOFSense (UART0 nativo)
✅ GPIO 46 liberado (ya no se usa XSHUT)
✅ Eliminado multiplexor PCA9548A @ 0x71 (obstáculos)

TOTAL ESP32: 34/36 GPIOs utilizados (94% eficiencia)
TOTAL MCP23017: 13/16 pines utilizados (81% eficiencia)
*/

// ============================================================================
// HELPERS - Validación de pines
// ============================================================================

/**
 * @brief Verifica si un GPIO está asignado en el sistema
 * @param gpio Número de GPIO a verificar
 * @return true si el pin está en uso, false si está libre
 * @note El shifter ahora usa MCP23017, no GPIOs directos
 * @note v2.12.0: Eliminados XSHUT pins (VL53L5X), añadidos pines UART
 */
static inline bool pin_is_assigned(uint8_t gpio) {
    switch (gpio) {
        // Sistema y Boot
        case PIN_KEY_ON:
        case PIN_KEY_OFF:
        // LEDs
        case PIN_LED_FRONT:
        case PIN_LED_REAR:
        // Botones
        case PIN_BTN_LIGHTS:
        // case PIN_BTN_MEDIA:  // v2.14.0: FREED
        // case PIN_BTN_4X4:    // v2.14.0: FREED
        // Relés
        case PIN_RELAY_MAIN:
        case PIN_RELAY_TRAC:
        case PIN_RELAY_DIR:
        case PIN_RELAY_SPARE:
        // I²C
        case PIN_I2C_SDA:
        case PIN_I2C_SCL:
        // SPI TFT
        case PIN_TFT_SCK:
        case PIN_TFT_MOSI:
        case PIN_TFT_MISO:
        case PIN_TFT_DC:
        case PIN_TFT_RST:
        case PIN_TFT_CS:     // GPIO 16 - SPI Chip Select (no conflict now)
        case PIN_TFT_BL:
        // Touch
        case PIN_TOUCH_CS:
        case PIN_TOUCH_IRQ:
        // Sensores ruedas
        case PIN_WHEEL_FL:
        case PIN_WHEEL_FR:
        case PIN_WHEEL_RL:
        case PIN_WHEEL_RR:  // GPIO 1 - ✅ v2.16.1: Safe pin, avoids strapping pins
        // Encoder
        case PIN_ENCODER_A:
        case PIN_ENCODER_B:
        case PIN_ENCODER_Z:
        // Pedal y temperatura
        case PIN_PEDAL:
        case PIN_ONEWIRE:
        // UART (Audio y TOFSense)
        case PIN_DFPLAYER_TX:
        case PIN_DFPLAYER_RX:
        case PIN_TOFSENSE_TX:
        case PIN_TOFSENSE_RX:
        // NOTA: Shifter ahora en MCP23017, no en GPIOs directos
        // NOTA: XSHUT pins eliminados (VL53L5X ya no se usa)
            return true;
        default:
            return false;
    }
}

/**
 * @brief Verifica si un GPIO es un strapping pin (cuidado al usar)
 * @param gpio Número de GPIO a verificar
 * @return true si es strapping pin
 */
static inline bool pin_is_strapping(uint8_t gpio) {
    switch (gpio) {
        case 0:   // Boot mode
        case 3:   // JTAG
        case 45:  // VDD_SPI voltage
        case 46:  // Boot mode / ROM log
            return true;
        default:
            return false;
    }
}
