# Manual Técnico de Conexiones y Firmware
## ESP32-S3 DevKitC-1 - Sistema de Control Vehicular Inteligente

---

## 1. Introducción

### 1.1 Descripción del Proyecto

Este proyecto implementa un **sistema de control vehicular avanzado** basado en el microcontrolador **ESP32-S3 DevKitC-1 (44 pines)**, diseñado para gestionar:

- **Control de tracción** en las 4 ruedas mediante drivers BTS7960 de alta corriente (43A)
- **Sistema de dirección asistida** con motor RS390 12V y encoder absoluto
- **Monitorización en tiempo real** de corriente, temperatura y velocidad
- **Interfaz HUD** con pantalla ILI9488 táctil (480x320)
- **Sistemas de seguridad avanzados**: ABS, TCS, regeneración inteligente
- **Audio** vía DFPlayer Mini con alertas y confirmaciones
- **Iluminación inteligente** con LEDs WS2812B programables

### 1.2 Objetivo

Documentar de forma exhaustiva:
- Todas las conexiones físicas entre el ESP32-S3 y los periféricos
- Asignación de GPIOs y compatibilidad con la placa DevKitC-1
- Configuración del firmware y librerías utilizadas
- Protecciones eléctricas y recomendaciones de seguridad
- Checklist de verificación antes de energizar el sistema

---

## 2. Mapa de Pines (pins.h)

### 2.1 GPIOs Disponibles en ESP32-S3-DevKitC-1

La placa expone los siguientes GPIOs en sus headers físicos:

**Lado 1 (22 pines):**
```
GND, GND, 19, 20, 21, 47, 48, 45, 0, 35, 36, 37, 38, 39, 40, 41, 42, 2, 1, RX(44), TX(43), GND
```

**Lado 2 (22 pines):**
```
GND, 5V, 14, 13, 12, 11, 10, 9, 46, 3, 8, 18, 17, 16, 15, 7, 6, 5, 4, RST, 3V3, 3V3
```

**Total GPIOs disponibles:** 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48

**⚠️ IMPORTANTE:** GPIOs 22-34 NO están expuestos físicamente en esta placa.

### 2.2 Tabla de Asignación de Pines

| Periférico/Función | GPIO | Descripción | Notas |
|-------------------|------|-------------|-------|
| **Relés de Potencia** |
| Power Hold | 2 | Relé 1: Mantiene buck 5V activo | SRD-05VDC |
| 12V Auxiliares | 4 | Relé 2: Alimenta sensores/encoder | SRD-05VDC |
| 24V Motores | 5 | Relé 3: Alimenta motores tracción | SRD-05VDC |
| Reserva | 6 | Relé 4: Función configurable | SRD-05VDC |
| **Pantalla ILI9488 (SPI)** |
| TFT_CS | 8 | Chip Select pantalla | |
| TFT_SCK | 10 | SPI Clock | Compartido |
| TFT_MOSI | 11 | SPI MOSI | Compartido |
| TFT_MISO | 12 | SPI MISO | Compartido |
| TFT_DC | 13 | Data/Command | |
| TFT_RST | 14 | Reset | |
| TFT_BL | 42 | Backlight PWM | LEDC 0-255 |
| **Táctil XPT2046 (SPI)** |
| TOUCH_CS | 3 | Chip Select táctil | ✅ Remapeado de GPIO22 |
| TOUCH_IRQ | 46 | Interrupción táctil (PEN) | INPUT_PULLUP |
| **I²C (Sensores/Multiplexores)** |
| I2C_SDA | 16 | Serial Data | Con convertidor 5V↔3.3V |
| I2C_SCL | 9 | Serial Clock | Con convertidor 5V↔3.3V |
| **Encoder Dirección E6B2-CWZ6C** |
| ENCODER_A | 37 | Canal A | Vía HY-M158 |
| ENCODER_B | 38 | Canal B | Vía HY-M158 |
| ENCODER_Z | 39 | Señal Z (centrado) | Vía HY-M158 |
| **Sensores de Rueda LJ12A3-4-Z/BX** |
| WHEEL0 (FL) | 20 | Frontal Izquierda | Vía HY-M158 |
| WHEEL1 (FR) | 21 | Frontal Derecha | Vía HY-M158 |
| WHEEL2 (RL) | 36 | Trasera Izquierda | Vía HY-M158 |
| WHEEL3 (RR) | 17 | Trasera Derecha | Vía HY-M158 |
| **Pedal Acelerador Hall A1324LUA-T** |
| PEDAL | 35 | Entrada analógica ADC | Divisor 5V→3.3V |
| **Palanca de Cambios (Shifter)** |
| SHIFTER_P | 47 | Posición Park | Vía HY-M158 |
| SHIFTER_D2 | 48 | Posición Drive 2 | Vía HY-M158 |
| SHIFTER_D1 | 7 | Posición Drive 1 | Vía HY-M158 |
| SHIFTER_N | 18 | Posición Neutral | Vía HY-M158 |
| SHIFTER_R | 19 | Posición Reverse | Vía HY-M158 |
| **Botones Físicos** |
| BTN_LIGHTS | 45 | Botón luces | INPUT_PULLUP |
| BTN_MEDIA | 40 | Botón multimedia | Vía HY-M158 12V |
| BTN_4X4 | 41 | Botón 4x4/4x2 | Vía HY-M158 12V |
| **DFPlayer Mini (UART)** |
| DFPLAYER_TX | 43 | UART TX | TX ESP → RX DFPlayer |
| DFPLAYER_RX | 44 | UART RX | RX ESP ← TX DFPlayer |
| **Temperatura DS18B20 (OneWire)** |
| ONEWIRE | 15 | Bus OneWire | Pull-up 4.7kΩ |
| **LEDs Inteligentes WS2812B** |
| LED_FRONT | 0 | LEDs frontales | 28 unidades |
| LED_REAR | 1 | LEDs traseros | 16 unidades |
| **Drivers BTS7960 (Motores) - Controlados vía I²C** |
| FL_PWM | PCA_CH0 | PWM Motor FL | ✅ PCA9685 @ 0x40 Canal 0 |
| FL_PWM_R | PCA_CH1 | PWM Motor FL Reverse | ✅ PCA9685 @ 0x40 Canal 1 |
| FL_IN1 | MCP_A0 | Dirección FL IN1 | ✅ MCP23017 @ 0x20 GPIOA0 |
| FL_IN2 | MCP_A1 | Dirección FL IN2 | ✅ MCP23017 @ 0x20 GPIOA1 |
| FR_PWM | PCA_CH2 | PWM Motor FR | ✅ PCA9685 @ 0x40 Canal 2 |
| FR_PWM_R | PCA_CH3 | PWM Motor FR Reverse | ✅ PCA9685 @ 0x40 Canal 3 |
| FR_IN1 | MCP_A2 | Dirección FR IN1 | ✅ MCP23017 @ 0x20 GPIOA2 |
| FR_IN2 | MCP_A3 | Dirección FR IN2 | ✅ MCP23017 @ 0x20 GPIOA3 |
| RL_PWM | PCA_CH4 | PWM Motor RL | ✅ PCA9685 @ 0x40 Canal 4 |
| RL_PWM_R | PCA_CH5 | PWM Motor RL Reverse | ✅ PCA9685 @ 0x40 Canal 5 |
| RL_IN1 | MCP_A4 | Dirección RL IN1 | ✅ MCP23017 @ 0x20 GPIOA4 |
| RL_IN2 | MCP_A5 | Dirección RL IN2 | ✅ MCP23017 @ 0x20 GPIOA5 |
| RR_PWM | PCA_CH6 | PWM Motor RR | ✅ PCA9685 @ 0x40 Canal 6 |
| RR_PWM_R | PCA_CH7 | PWM Motor RR Reverse | ✅ PCA9685 @ 0x40 Canal 7 |
| RR_IN1 | MCP_A6 | Dirección RR IN1 | ✅ MCP23017 @ 0x20 GPIOA6 |
| RR_IN2 | MCP_A7 | Dirección RR IN2 | ✅ MCP23017 @ 0x20 GPIOA7 |

### 2.3 Pines Libres Restantes

**GPIOs disponibles sin asignar:**
- Ninguno - Todos asignados

**✅ PROBLEMA RESUELTO:**
Los GPIOs 23-34 asignados originalmente a los drivers BTS7960 **NO están físicamente disponibles** en los headers de la placa ESP32-S3-DevKitC-1. 

**✅ SOLUCIÓN IMPLEMENTADA:** 
- **PCA9685 @ 0x40**: Control PWM de 4 motores (8 canales)
- **MCP23017 @ 0x20**: Control IN1/IN2 de 4 motores (8 GPIOs)
- **GPIO 0**: LEDs frontales WS2812B (28 unidades)
- **GPIO 1**: LEDs traseros WS2812B (16 unidades)

---

## 3. Conexiones Principales

### 3.1 Pantalla ILI9488 (480x320, SPI)

| Pin Pantalla | Pin ESP32-S3 | GPIO | Descripción |
|--------------|--------------|------|-------------|
| VCC | 3.3V | - | Alimentación |
| GND | GND | - | Tierra |
| CS | TFT_CS | 8 | Chip Select |
| RESET | TFT_RST | 14 | Reset (LOW = reset) |
| DC/RS | TFT_DC | 13 | Data/Command |
| SDI/MOSI | TFT_MOSI | 11 | Serial Data In |
| SCK | TFT_SCK | 10 | Serial Clock |
| LED/BL | TFT_BL | 42 | Backlight (PWM) |
| SDO/MISO | TFT_MISO | 12 | Serial Data Out |

**Configuración SPI:**
- Velocidad: 27 MHz (configurado en TFT_eSPI)
- Modo: SPI_MODE0
- Orden bits: MSB First

### 3.2 Táctil XPT2046 (SPI compartido)

| Pin Táctil | Pin ESP32-S3 | GPIO | Descripción |
|------------|--------------|------|-------------|
| VCC | 3.3V | - | Alimentación |
| GND | GND | - | Tierra |
| CS | TOUCH_CS | 3 | Chip Select (⚠️ remapeado de GPIO22) |
| DIN/MOSI | TFT_MOSI | 11 | Compartido con pantalla |
| DOUT/MISO | TFT_MISO | 12 | Compartido con pantalla |
| CLK | TFT_SCK | 10 | Compartido con pantalla |
| IRQ/PEN | TOUCH_IRQ | 46 | Interrupción (activo bajo) |

**Notas:**
- Comparte bus SPI con la pantalla ILI9488
- CS debe estar en HIGH cuando no se use para evitar conflictos
- IRQ requiere resistencia pull-up (ya integrada en placa táctil)

### 3.3 Relés de Potencia

| Relé | GPIO | Función | Carga Máxima | Tipo |
|------|------|---------|--------------|------|
| Relé 1 | 2 | Power Hold Buck 5V | 10A @ 250VAC | SRD-05VDC-SL-C |
| Relé 2 | 4 | 12V Auxiliares | 10A @ 250VAC | SRD-05VDC-SL-C |
| Relé 3 | 5 | 24V Motores | 10A @ 250VAC | SRD-05VDC-SL-C |
| Relé 4 | 6 | Reserva | 10A @ 250VAC | SRD-05VDC-SL-C |

**Esquema de Conexión:**
```
ESP32-S3 GPIO → Transistor NPN (BC547) → Bobina Relé → +5V
                     ↓
                  Diodo 1N4007 (flyback)
```

**Protección Obligatoria:**
- Diodo 1N4007 en antiparalelo con la bobina (protección contra picos inductivos)
- Resistencia base transistor: 1kΩ
- Transistor: BC547 o 2N2222 (suficiente para bobina 70mA)

### 3.4 Sensores INA226 (Corriente/Tensión)

**Configuración I²C con Multiplexor TCA9548A:**

| Sensor | Canal TCA | Dirección I²C | Shunt | Corriente Máx |
|--------|-----------|---------------|-------|---------------|
| Motor FL | CH0 (0x70) | 0x40 | 75mV 50A | 50A |
| Motor FR | CH1 (0x70) | 0x40 | 75mV 50A | 50A |
| Motor RL | CH2 (0x70) | 0x40 | 75mV 50A | 50A |
| Motor RR | CH3 (0x70) | 0x40 | 75mV 50A | 50A |
| Batería 24V | CH4 (0x70) | 0x40 | 75mV 100A | 100A |
| Motor Dirección | CH5 (0x70) | 0x40 | 75mV 50A | 50A |

**Multiplexor TCA9548A:**
- Dirección: 0x70 (A0=A1=A2=GND)
- SDA/SCL conectados a GPIO 16/9 vía convertidor de nivel
- Permite usar 8 dispositivos con la misma dirección I²C

### 3.5 DFPlayer Mini (Audio)

| Pin DFPlayer | Pin ESP32-S3 | GPIO | Notas |
|--------------|--------------|------|-------|
| VCC | 5V | - | Regulado 5V |
| GND | GND | - | Común |
| TX | DFPLAYER_RX | 44 | TX DFPlayer → RX ESP |
| RX | DFPLAYER_TX | 43 | RX DFPlayer ← TX ESP |
| SPK1 | - | - | A altavoz/amplificador |
| SPK2 | - | - | A altavoz/amplificador |

**Configuración UART:**
- Baudrate: 9600 bps
- 8N1 (8 bits, sin paridad, 1 stop bit)
- Tarjeta SD con archivos MP3 numerados (001.mp3, 002.mp3, etc.)

### 3.6 LEDs WS2812B (Iluminación Inteligente)

| Parámetro | Valor |
|-----------|-------|
| Pin de datos | ⚠️ **No definido en pins.h** |
| Cantidad LEDs frontales | 28 |
| Cantidad LEDs traseros | 16 |
| Alimentación | 5V externa (NO desde ESP32) |
| Nivel lógico | 3.3V compatible |

**⚠️ ACCIÓN REQUERIDA:** Definir GPIO para control de LEDs WS2812B.

---

## 4. Componentes Añadidos

### 4.1 Multiplexores I²C TCA9548A (x5)

**Función:** Expandir bus I²C para conectar hasta 40 dispositivos con la misma dirección.

| Multiplexor | Dirección I²C | Uso |
|-------------|---------------|-----|
| TCA #1 | 0x70 | 6x INA226 (sensores corriente) |
| TCA #2 | 0x71 | Reserva |
| TCA #3 | 0x72 | Reserva |
| TCA #4 | 0x73 | Reserva |
| TCA #5 | 0x74 | Reserva |

**Conexión:**
- SDA/SCL a bus principal con convertidor 5V↔3.3V
- RESET a HIGH (3.3V) para operación normal
- Pines A0/A1/A2 configuran dirección (ver tabla)

### 4.2 PCA9685 (PWM Driver, 16 canales)

**Función:** Generar señales PWM para motor de dirección RS390.

| Parámetro | Valor |
|-----------|-------|
| Dirección I²C | 0x41 (A0=HIGH, resto GND) |
| Frecuencia PWM | 50Hz - 1.6kHz |
| Resolución | 12 bits (4096 pasos) |
| Canales usados | 1 (motor dirección) |

### 4.3 MCP23017 (Expansor GPIO I²C, 16 pines)

**Función:** Expansión de GPIOs para control de relés adicionales o BTS7960.

| Parámetro | Valor |
|-----------|-------|
| Dirección I²C | 0x20 (A0=A1=A2=GND) |
| GPIOs disponibles | 16 (GPIOA0-7, GPIOB0-7) |
| Corriente máx/pin | 25mA |
| Uso sugerido | Control BTS7960 (GPIOs 23-34) |

**⚠️ RECOMENDACIÓN CRÍTICA:**
Usar el MCP23017 para controlar los 12 pines de los drivers BTS7960, ya que los GPIOs 23-34 del ESP32-S3 no están físicamente disponibles.

### 4.4 Optoacopladores HY-M158 (PC817, x2 módulos)

**Función:** Aislar señales de 12V/24V hacia entradas 3.3V del ESP32-S3.

| Módulo | Canales | Señales Aisladas |
|--------|---------|------------------|
| HY-M158 #1 | 8 | Encoder A/B/Z, Shifter, Buttons |
| HY-M158 #2 | 8 | Sensores rueda (x4), Signals 12V |

**Esquema por Canal:**
```
Entrada 12V → Resistencia 1kΩ → LED Optoacoplador → GND
Transistor → Pull-up 10kΩ a 3.3V → GPIO ESP32-S3
```

**Características:**
- Aislamiento galvánico: 5000V
- Tiempo respuesta: <18µs
- Compatible con señales 5V-24V

### 4.5 Drivers BTS7960 (43A, x4 unidades)

**Función:** Control bidireccional de motores de tracción (4 ruedas).

| Motor | PWM | IN1 | IN2 | Corriente Máx |
|-------|-----|-----|-----|---------------|
| FL | 23 | 24 | 25 | 43A |
| FR | 26 | 27 | 28 | 43A |
| RL | 29 | 30 | 31 | 43A |
| RR | 32 | 33 | 34 | 43A |

**⚠️ PROBLEMA:** GPIOs 23-34 no disponibles.

**SOLUCIÓN:** Controlar vía MCP23017:
- GPIOA0-2 → FL (PWM, IN1, IN2)
- GPIOA3-5 → FR
- GPIOA6-7 + GPIOB0 → RL
- GPIOB1-3 → RR

### 4.6 Encoder E6B2-CWZ6C (1200 PPR)

**Función:** Medición de ángulo absoluto del volante.

| Señal | GPIO | Aislamiento |
|-------|------|-------------|
| A (cuadratura) | 37 | Vía HY-M158 |
| B (cuadratura) | 38 | Vía HY-M158 |
| Z (índice) | 39 | Vía HY-M158 |
| Alimentación | 12V | Externa |

**Especificaciones:**
- Resolución: 1200 pulsos/revolución
- Ratio volante: 1:1 (directo)
- Señal: NPN Open Collector → requiere pull-up

### 4.7 Sensores Inductivos LJ12A3-4-Z/BX (x5)

**Función:** Detectar paso de dientes metálicos (ruedas) para medir velocidad.

| Sensor | GPIO | Aislamiento | Uso |
|--------|------|-------------|-----|
| Wheel FL | 20 | HY-M158 | Velocidad FL |
| Wheel FR | 21 | HY-M158 | Velocidad FR |
| Wheel RL | 36 | HY-M158 | Velocidad RL |
| Wheel RR | 17 | HY-M158 | Velocidad RR |
| Encoder Z | 39 | HY-M158 | Índice encoder |

**Especificaciones:**
- Distancia detección: 4mm
- Alimentación: 6-36V DC (usar 12V)
- Salida: NPN NO (normalmente abierto)
- Carga máxima: 200mA

### 4.8 Sensor Hall A1324LUA-T (Pedal Analógico)

**Función:** Medir posición del pedal acelerador.

| Parámetro | Valor |
|-----------|-------|
| GPIO | 35 (ADC) |
| Alimentación | 5V |
| Salida | 0.5V - 4.5V (ratiométrico) |
| Divisor tensión | Necesario: 5V → 3.3V |

**Divisor de Tensión Requerido:**
```
5V → R1 (2.7kΩ) → GPIO35 → R2 (4.7kΩ) → GND
Vout = 5V * (4.7 / (2.7 + 4.7)) = 3.18V max
```

---

## 5. Protecciones y Recomendaciones

### 5.1 Resistencias Pull-up/Pull-down

| Señal | Tipo | Valor | Ubicación |
|-------|------|-------|-----------|
| I²C SDA | Pull-up | 4.7kΩ | Bus I²C principal |
| I²C SCL | Pull-up | 4.7kΩ | Bus I²C principal |
| OneWire (DS18B20) | Pull-up | 4.7kΩ | GPIO 15 |
| Encoder A/B/Z | Pull-up | 10kΩ | Interno HY-M158 |
| Botones físicos | Pull-up | 10kΩ | Interno ESP32 (INPUT_PULLUP) |
| TOUCH_IRQ | Pull-up | 10kΩ | Módulo táctil |

### 5.2 Fusibles Requeridos

| Línea | Fusible | Tipo | Ubicación |
|-------|---------|------|-----------|
| 12V Principal | 15A | Auto Blade | Entrada 12V |
| 24V Motores | 50A | ANL | Batería 24V |
| 5V Buck | 5A | Auto Mini | Salida Buck |
| 12V Auxiliares | 5A | Auto Mini | Post-relé |

### 5.3 Diodos de Protección

**Diodos Anti-Retorno (Inversión Polaridad):**
- 12V entrada: Schottky 15A (SS54)
- 24V batería: Schottky 50A (MBR5045)

**Diodos Flyback (Cargas Inductivas):**
- Bobinas relés: 1N4007 (cada relé)
- Motores: Integrado en drivers BTS7960

### 5.4 Divisor de Tensión (Pedal Hall 5V→3.3V)

**Esquema:**
```
Pedal 5V Output → R1 (2.7kΩ) → GPIO35 (ADC) → R2 (4.7kΩ) → GND
```

**Cálculo:**
- V_max = 5V × (4.7kΩ / 7.4kΩ) = 3.18V ✅
- Margen seguridad: 3.3V - 3.18V = 120mV ✅

**Alternativa:** Usar HY-M158 con salida 3.3V.

### 5.5 Convertidores de Nivel I²C (5V ↔ 3.3V)

**Necesario para:**
- TCA9548A (trabaja a 5V)
- INA226 (compatible 3.3V-5V, usar 5V para mejor precisión)
- PCA9685 (compatible 3.3V-5V)
- MCP23017 (compatible 3.3V-5V)

**Módulo recomendado:**
- **TXS0108E** (8 canales, bidireccional)
- Lado A: 3.3V (ESP32-S3)
- Lado B: 5V (periféricos I²C)

### 5.6 Optoacopladores HY-M158 (Aislamiento 12V/24V)

**Uso obligatorio para:**
- Encoder E6B2-CWZ6C (señales 12V)
- Sensores inductivos LJ12A3 (salida 12V)
- Botones 12V (BTN_MEDIA, BTN_4X4)
- Shifter (señales 12V)

**Configuración por canal:**
```
IN+ ─┬─ Resistencia 1kΩ ─┬─ LED Opto ─── IN-
     │                    │
   Señal 12V            GND señal
```

---

## 6. Firmware

### 6.1 Archivo platformio.ini

```ini
[env:esp32-s3-devkitc]
platform = espressif32
board = esp32-s3-devkitc-1
framework = arduino

board_build.mcu = esp32s3
board_build.f_cpu = 240000000L
board_build.flash_size = 16MB
board_build.partitions = default.csv

monitor_speed = 115200
upload_speed = 921600
upload_port = COM3
monitor_port = COM3

lib_deps =
    bodmer/TFT_eSPI @ ^2.5.43
    dfrobot/DFRobotDFPlayerMini @ ^1.0.6
    milesburton/DallasTemperature@^4.0.5
    paulstoffregen/OneWire@^2.3.8
    https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library.git
    RobTillaart/INA226 @ ^0.6.4
    https://github.com/PaulStoffregen/XPT2046_Touchscreen.git
    fastled/FastLED @ 3.6.0

build_flags =
    -DCORE_DEBUG_LEVEL=5
    -DUSER_SETUP_LOADED
    -std=gnu++17
    -DLOAD_GLCD
    -DLOAD_FONT2
    -DLOAD_FONT4
    -DLOAD_FONT6
    -DLOAD_FONT7
    -DLOAD_FONT8
    -DSMOOTH_FONT
    -Iinclude
    
    ; TFT_eSPI configuration for ILI9488 (480x320)
    -DILI9488_DRIVER
    -DTFT_WIDTH=480
    -DTFT_HEIGHT=320
    -DTOUCH_CS=3   ; ✅ remapeado al GPIO3
    
    ; Suprimir warnings de librerías externas
    -w
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-unknown-pragmas
    -Wno-macro-redefined
```

### 6.2 Librerías Utilizadas

| Librería | Versión | Función |
|----------|---------|---------|
| TFT_eSPI | 2.5.43 | Control pantalla ILI9488 |
| DFRobotDFPlayerMini | 1.0.6 | Control DFPlayer Mini audio |
| DallasTemperature | 4.0.5 | Lectura sensores DS18B20 |
| OneWire | 2.3.8 | Protocolo OneWire (DS18B20) |
| Adafruit PWM Servo Driver | Latest | Control PCA9685 (PWM motor dirección) |
| INA226 | 0.6.4 | Lectura sensores corriente/tensión |
| XPT2046_Touchscreen | Latest | Control táctil capacitivo |
| FastLED | 3.6.0 | Control LEDs WS2812B |

### 6.3 Configuración TFT_eSPI

**En `platformio.ini` (build_flags):**
```cpp
-DILI9488_DRIVER         // Driver específico ILI9488
-DTFT_WIDTH=480          // Ancho pantalla
-DTFT_HEIGHT=320         // Alto pantalla
-DTOUCH_CS=3             // CS táctil en GPIO3
```

**Pines definidos en pins.h (usados automáticamente):**
- TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_MISO, TFT_SCK, TFT_BL

### 6.4 Configuración XPT2046 Touch

**Inicialización en código:**
```cpp
#include <XPT2046_Touchscreen.h>
#include "pins.h"

XPT2046_Touchscreen touch(PIN_TOUCH_CS, PIN_TOUCH_IRQ);

void setup() {
    touch.begin();
    touch.setRotation(1);  // Coincidir con rotación de pantalla
}
```

### 6.5 Configuración INA226 (Sensores Corriente)

**Parámetros de calibración:**
```cpp
// Shunt 75mV, 50A → Resistencia = 75mV / 50A = 0.0015Ω = 1.5mΩ
// Shunt 75mV, 100A → Resistencia = 75mV / 100A = 0.00075Ω = 0.75mΩ

INA226 ina_motor_fl(0x40);  // En canal 0 de TCA9548A

void setup() {
    selectTCAChannel(0);  // Seleccionar canal TCA
    ina_motor_fl.begin();
    ina_motor_fl.configure(INA226_AVERAGES_16, INA226_BUS_CONV_TIME_1100US, 
                           INA226_SHUNT_CONV_TIME_1100US, INA226_MODE_SHUNT_BUS_CONT);
    ina_motor_fl.calibrate(0.0015, 50.0);  // R_shunt, I_max
}
```

### 6.6 Configuración DFPlayer Mini

```cpp
#include <DFRobotDFPlayerMini.h>
#include <HardwareSerial.h>

HardwareSerial dfSerial(1);  // UART1
DFRobotDFPlayerMini player;

void setup() {
    dfSerial.begin(9600, SERIAL_8N1, PIN_DFPLAYER_RX, PIN_DFPLAYER_TX);
    player.begin(dfSerial);
    player.volume(25);  // Volumen 0-30
}
```

### 6.7 Configuración OneWire/DallasTemperature

```cpp
#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(PIN_ONEWIRE);
DallasTemperature sensors(&oneWire);

void setup() {
    sensors.begin();
    sensors.setResolution(12);  // 12 bits precisión
}

void loop() {
    sensors.requestTemperatures();
    float temp = sensors.getTempCByIndex(0);
}
```

### 6.8 Configuración FastLED (WS2812B)

**⚠️ PENDIENTE:** Definir GPIO para LEDs en pins.h

```cpp
#include <FastLED.h>

#define LED_PIN      XX  // ⚠️ A DEFINIR
#define NUM_LEDS_FRONT 28
#define NUM_LEDS_REAR  16

CRGB leds_front[NUM_LEDS_FRONT];
CRGB leds_rear[NUM_LEDS_REAR];

void setup() {
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds_front, NUM_LEDS_FRONT);
    FastLED.setBrightness(50);
}
```

---

## 7. Esquemas y Diagramas

### 7.1 Diagrama de Bloques Principal

```
┌────────────────────────────────────────────────────────────────┐
│                     ESP32-S3 DevKitC-1                         │
│                       (240 MHz, 16MB)                          │
├─────────┬──────────┬───────────┬──────────┬──────────┬─────────┤
│   SPI   │  UART1   │    I²C    │   ADC    │  GPIO    │  PWM    │
└────┬────┴─────┬────┴─────┬─────┴─────┬────┴────┬─────┴────┬────┘
     │          │          │           │         │          │
     ├──────────┤          │           │         │          │
     │          │          │           │         │          │
┌────▼───┐  ┌───▼────┐ ┌──▼─────┐ ┌───▼───┐ ┌──▼────┐ ┌───▼────┐
│ILI9488 │  │DFPlayer│ │TCA9548A│ │ Hall  │ │ Relés │ │ Buzzer │
│+Touch  │  │  Mini  │ │ (x5)   │ │Sensor │ │  (x4) │ │        │
│480x320 │  │        │ │        │ │Pedal  │ │       │ │        │
└────────┘  └────────┘ └───┬────┘ └───────┘ └───────┘ └────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
    ┌───▼────┐      ┌──────▼─────┐    ┌──────▼──────┐
    │ INA226 │      │  PCA9685   │    │  MCP23017   │
    │  (x6)  │      │ (PWM x16)  │    │ (GPIO x16)  │
    │Current │      │Motor Dir.  │    │BTS7960 Ctrl │
    └────────┘      └────────────┘    └──────┬──────┘
                                              │
                    ┌─────────────────────────┼─────────────┐
                    │                         │             │
              ┌─────▼──────┐          ┌───────▼─────┐  ┌───▼──────┐
              │  BTS7960   │          │  BTS7960    │  │ BTS7960  │
              │  Motor FL  │          │  Motor FR   │  │ Motor RL │
              └────────────┘          └─────────────┘  └──────────┘
                                                         ...
```

### 7.2 Esquema I²C (Multiplexación)

```
ESP32-S3 (SDA:16, SCL:9)
     │
     ├─ Convertidor Nivel 3.3V → 5V
     │
     ├─ TCA9548A (0x70)
     │   ├─ CH0: INA226 Motor FL (0x40)
     │   ├─ CH1: INA226 Motor FR (0x40)
     │   ├─ CH2: INA226 Motor RL (0x40)
     │   ├─ CH3: INA226 Motor RR (0x40)
     │   ├─ CH4: INA226 Batería (0x40)
     │   └─ CH5: INA226 Motor Dir (0x40)
     │
     ├─ PCA9685 (0x41) - PWM Motor Dirección
     │
     └─ MCP23017 (0x20) - Expansión GPIO → BTS7960
```

### 7.3 Esquema Relés de Potencia

```
                                 ┌─────────────┐
ESP32 GPIO2 ──┬─ R1(1kΩ) ─┬─── BC547 ───────┐ │
              │            │                 │ │
            LED          Base              C │ │  Relé SRD-05VDC
                           │                 │ │  ┌───────┐
                           E                 │ └──┤ Bobina├─ +5V
                           │                 │    │ 70mA  │
                          GND                └────┤       │
                                                  └───┬───┘
                                  Diodo 1N4007        │
                                  (flyback)          GND
                                      ╱│╲
                                     ─ ─
                                    GND

Contacto Relé:
    COM ────┬──── Carga (12V/24V)
            │
    NO ─────┘     (Normalmente Abierto)
```

### 7.4 Esquema Optoacoplador HY-M158

```
Señal 12V ──┬── R(1kΩ) ──┬── LED PC817 ──┬── GND Señal
            │             │               │
          +12V          Ánodo          Cátodo
                          
                     Optoacoplador
                          │
                      Colector ──┬── R_pullup(10kΩ) ── +3.3V
                          │      │
                      Emisor ───┴── GPIO ESP32-S3
                          │
                         GND
```

---

## 8. Checklist Final de Verificación

### 8.1 Verificación de Pines

- [ ] **Todos los GPIOs asignados están disponibles físicamente**
  - ⚠️ GPIOs 23-34 NO disponibles → Usar MCP23017
  - ✅ GPIO 3 asignado correctamente (TOUCH_CS)
  - ✅ GPIO 0, 1 libres para uso futuro

- [ ] **No hay conflictos de dirección I²C**
  - ✅ TCA9548A: 0x70-0x74
  - ✅ INA226 (todos): 0x40 (multiplexados)
  - ✅ PCA9685: 0x41
  - ✅ MCP23017: 0x20

- [ ] **SPI compartido correctamente configurado**
  - ✅ ILI9488: CS en GPIO 8
  - ✅ XPT2046: CS en GPIO 3
  - ✅ MOSI/MISO/SCK compartidos (11/12/10)

### 8.2 Verificación de Protecciones

- [ ] **Fusibles instalados**
  - [ ] 15A en línea 12V principal
  - [ ] 50A en línea 24V motores
  - [ ] 5A en salida Buck 5V
  - [ ] 5A en 12V auxiliares

- [ ] **Diodos anti-retorno instalados**
  - [ ] Schottky 15A en entrada 12V
  - [ ] Schottky 50A en batería 24V

- [ ] **Diodos flyback en relés**
  - [ ] 1N4007 en cada bobina de relé (x4)

- [ ] **Convertidor de nivel I²C**
  - [ ] TXS0108E configurado (3.3V ↔ 5V)

- [ ] **Divisor de tensión pedal Hall**
  - [ ] R1 = 2.7kΩ, R2 = 4.7kΩ
  - [ ] Vout máx = 3.18V ✅

### 8.3 Verificación de Aislamiento

- [ ] **Optoacopladores HY-M158**
  - [ ] Módulo #1: Encoder, Shifter, Botones
  - [ ] Módulo #2: Sensores rueda (x4)
  - [ ] Resistencias 1kΩ en cada entrada LED
  - [ ] Pull-up 10kΩ en cada salida transistor

### 8.4 Verificación de Firmware

- [ ] **Librerías instaladas**
  - [ ] TFT_eSPI 2.5.43
  - [ ] DFRobotDFPlayerMini 1.0.6
  - [ ] DallasTemperature 4.0.5
  - [ ] OneWire 2.3.8
  - [ ] Adafruit PWM Servo Driver
  - [ ] INA226 0.6.4
  - [ ] XPT2046_Touchscreen
  - [ ] FastLED 3.6.0

- [ ] **Configuración platformio.ini correcta**
  - [ ] `TOUCH_CS=3` definido
  - [ ] `upload_port` y `monitor_port` configurados

- [ ] **Archivo pins.h actualizado**
  - [ ] PIN_TOUCH_CS = 3
  - [ ] Todos los pines verificados

### 8.5 Verificación de Alimentación

- [ ] **Fuentes de alimentación**
  - [ ] Buck 12V → 5V (ESP32, DFPlayer, Relés)
  - [ ] 12V directo (Encoder, Sensores, Motor Dir)
  - [ ] 24V directo (Motores tracción BTS7960)

- [ ] **Tierras comunes**
  - [ ] GND 5V, 12V, 24V conectados en punto único
  - [ ] GND señales aisladas separado (optoacopladores)

### 8.6 Verificación de Seguridad

- [ ] **Relés configurados correctamente**
  - [ ] Power Hold en GPIO 2
  - [ ] Secuencia arranque: Hold → 12V Aux → 24V Motores
  - [ ] Secuencia apagado inversa

- [ ] **Watchdog habilitado**
  - [ ] Timeout: 10 segundos
  - [ ] Callback apagado seguro implementado

- [ ] **Límites de corriente configurados**
  - [ ] INA226: Alertas en 50A (motores) / 100A (batería)

---

## 9. Recomendaciones Finales

### 9.1 **CRÍTICO: Remapear GPIOs 23-34**

Los pines asignados a los drivers BTS7960 (GPIOs 23-34) **NO están físicamente disponibles** en la ESP32-S3-DevKitC-1.

**Soluciones:**

**Opción A (Recomendada):** Usar MCP23017 para control BTS7960
```
MCP23017 (0x20) via I²C:
  GPIOA0-2 → FL_PWM, FL_IN1, FL_IN2
  GPIOA3-5 → FR_PWM, FR_IN1, FR_IN2
  GPIOA6-7 + GPIOB0 → RL_PWM, RL_IN1, RL_IN2
  GPIOB1-3 → RR_PWM, RR_IN1, RR_IN2
```

**Opción B:** Remapear a GPIOs libres
- Usar GPIO 0, 1 (únicos libres)
- Buscar otros GPIOs no críticos para reasignar

### 9.2 **Definir GPIO para LEDs WS2812B**

Actualmente no hay GPIO asignado para los LEDs WS2812B.

**Sugerencia:** Usar GPIO 0 o GPIO 1.

### 9.3 **Usar Convertidor de Nivel I²C**

Obligatorio para garantizar compatibilidad 3.3V (ESP32) ↔ 5V (TCA9548A, INA226).

### 9.4 **Instalar Protecciones**

- Fusibles en todas las líneas de potencia
- Diodos flyback en cargas inductivas (relés, motores)
- Diodos anti-retorno en entradas de alimentación

### 9.5 **Verificar Alimentación Antes de Conectar ESP32**

1. Medir tensiones sin ESP32 conectada
2. Verificar que 5V Buck esté estable
3. Verificar que no hay cortocircuitos
4. Conectar ESP32 y medir consumo en reposo (<500mA)

---

## 10. Enlaces de Referencia

### Datasheets
- ESP32-S3: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- ILI9488: https://www.displayfuture.com/Display/datasheet/controller/ILI9488.pdf
- XPT2046: https://datasheetspdf.com/pdf-file/746664/XPTEK/XPT2046/1
- INA226: https://www.ti.com/lit/ds/symlink/ina226.pdf
- BTS7960: https://www.infineon.com/dgdl/bts7960p_datasheet.pdf
- TCA9548A: https://www.ti.com/lit/ds/symlink/tca9548a.pdf
- PCA9685: https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf
- MCP23017: https://ww1.microchip.com/downloads/en/devicedoc/20001952c.pdf

### Librerías
- TFT_eSPI: https://github.com/Bodmer/TFT_eSPI
- DFRobotDFPlayerMini: https://github.com/DFRobot/DFRobotDFPlayerMini
- DallasTemperature: https://github.com/milesburton/Arduino-Temperature-Control-Library
- INA226: https://github.com/RobTillaart/INA226
- FastLED: https://github.com/FastLED/FastLED

---

**Documento generado:** 2025-11-15  
**Versión Firmware:** copilot/refactor-firmware-timing-logic  
**Autor:** Sistema de documentación automática  
**Última actualización:** Commit 990fc95

---

**⚠️ ADVERTENCIAS FINALES:**

1. **NUNCA energizar sin verificar todas las protecciones**
2. **NUNCA conectar 24V sin fusible de 50A**
3. **NUNCA omitir convertidor de nivel I²C**
4. **SIEMPRE verificar polaridad antes de conectar**
5. **SIEMPRE usar optoacopladores para señales 12V/24V**

**✅ Con estas precauciones, el sistema estará listo para operar de forma segura y confiable.**
