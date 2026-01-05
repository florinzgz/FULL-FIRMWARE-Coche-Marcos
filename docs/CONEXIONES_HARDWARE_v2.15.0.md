# 🔌 GUÍA MAESTRA DE CONEXIONES HARDWARE
## ESP32-S3 Car Control System - Coche Inteligente Marcos

**Versión Firmware:** v2.15.0  
**Placa:** ESP32-S3-DevKitC-1 (N16R8 - 44 pines)  
**Fecha:** 2026-01-05  
**Estado:** ✅ Actualizado tras migración TOFSense-M S 8x8 Matrix

---

## 📋 ÍNDICE RÁPIDO

1. [Diagrama General del Sistema](#-diagrama-general-del-sistema)
2. [Alimentación y Power Control](#-alimentación-y-power-control)
3. [Sensor de Obstáculos TOFSense-M S](#-sensor-de-obstáculos-tofsense-m-s-8x8-lidar)
4. [Audio DFPlayer Mini](#-audio-dfplayer-mini)
5. [Pantalla TFT + Touch](#-pantalla-tft--touch)
6. [Bus I²C](#-bus-ic)
7. [Motores de Tracción](#-motores-de-tracción)
8. [Motor de Dirección](#-motor-de-dirección)
9. [Sensores de Ruedas](#-sensores-de-ruedas)
10. [Encoder de Dirección](#-encoder-de-dirección)
11. [Pedal y Palanca](#-pedal-y-palanca)
12. [LEDs WS2812B](#-leds-ws2812b)
13. [Sensores de Temperatura](#-sensores-de-temperatura)
14. [Relés](#-relés)
15. [GPIOs Libres](#-gpios-libres-para-expansión)
16. [Checklist de Verificación](#-checklist-de-verificación)

---

## 🔋 DIAGRAMA GENERAL DEL SISTEMA

```
┌──────────────────────────────────────────────────────────────────────────────┐
│                    ESP32-S3-N16R8 DevKitC-1 (44 pines)                       │
│                                                                              │
│  UART0 (Native)              UART1                     I²C Bus (400kHz)     │
│  ├─ GPIO 44: RX             ├─ GPIO 18: TX            ├─ GPIO  8: SDA      │
│  └─ GPIO 43: TX             └─ GPIO 17: RX            └─ GPIO  9: SCL      │
│       │                          │                          │               │
│       │ TOFSense-M S             │ DFPlayer Mini            │ TCA9548A Mux  │
│       │ 8x8 LiDAR                │ Audio Module             │ INA226 x6     │
│       │ 921600 baud              │ 9600 baud                │ PCA9685 PWM   │
│       │ 4m range                 │                          │ MCP23017 GPIO │
│       │ 65° FOV                  │                          │               │
│                                                                              │
│  SPI (TFT)                   PWM/Digital              Power Control         │
│  ├─ GPIO 10: SCK            ├─ GPIO 19: LED_FRONT    ├─ GPIO 40: KEY_ON    │
│  ├─ GPIO 11: MOSI           ├─ GPIO 48: LED_REAR     └─ GPIO 41: KEY_OFF   │
│  ├─ GPIO 12: MISO           ├─ GPIO 21: RELAY_MAIN                         │
│  ├─ GPIO 13: DC             ├─ GPIO 47: RELAY_AUX                          │
│  ├─ GPIO 14: RST            ├─ GPIO 38: RELAY_HOLD                         │
│  └─ GPIO 16: CS (shared)    └─ GPIO 39: RELAY_EMER                         │
│                                                                              │
│  OneWire                     Analog                    Wheel Sensors        │
│  └─ GPIO 20: DS18B20        └─ GPIO  1: PEDAL         ├─ GPIO  3: WHEEL_FL │
│       (Temp x4)                  (ADC)                 ├─ GPIO 15: WHEEL_RL │
│                                                        ├─ GPIO  4: WHEEL_FR │
│                                                        └─ GPIO 16: WHEEL_RR │
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## ⚡ ALIMENTACIÓN Y POWER CONTROL

### Esquema de Alimentación

```
                  Batería 24V ───┬─── [FUSIBLE 50A] ───┬─── Motores Tracción (BTS7960 x4)
                                 │                     │
                                 ├─── [FUSIBLE 30A] ───┬─── Motor Dirección (BTS7960)
                                 │                     │
                      Batería 12V ───┬─── RELÉ AUX ────┬─── Sensores 12V (LJ12A3, Encoder)
                                     │                 │
                                     ├─── Convertidor Buck 24V→5V
                                     │           │
                                     │           └─── [FUSIBLE 5A] ───┬─── ESP32-S3 (5V)
                                     │                                ├─── TFT Display
                                     │                                ├─── DFPlayer Mini
                                     │                                ├─── LEDs WS2812B
                                     │                                └─── Lógica 5V
                                     │
                                     └─── ESP32-S3 LDO ────────────────── 3.3V (I²C, GPIOs)
```

### Power Control - Llave de Contacto

#### Conexión con Optoacoplador PC817 (RECOMENDADO)

```
Lado 12V (Aislado):                    Lado 3.3V (ESP32):
                                       
Llave Contacto ──────┬                         
    (+12V ON)        │                         ┌──── GPIO 40 (PIN_KEY_ON)
                     │                         │
              [Resistencia 1kΩ]           Pull-up 10kΩ
                     │                    to 3.3V
                     ├─── LED ────┐            │
                     │            │            │
                    GND          Fototr. ──────┴──── GND
                                  PC817

Estado:
- Llave ON  → LED enciende → Fototransistor conduce → GPIO 40 = LOW  (0V)
- Llave OFF → LED apaga    → Fototransistor abre    → GPIO 40 = HIGH (3.3V)
```

#### Botón Shutdown Opcional (GPIO 41)

```
Lado Botón:                            Lado 3.3V (ESP32):
                                       
Botón Shutdown ──────┬                         
    (Normalmente     │                         ┌──── GPIO 41 (PIN_KEY_OFF)
     abierto)        │                         │
                     │                    Pull-up 10kΩ
              [Resistencia 1kΩ]           to 3.3V
                     │                         │
                     ├─── LED ────┐            │
                     │            │            │
                    GND          Fototr. ──────┴──── GND
                                  PC817

Estado:
- Botón NO presionado → GPIO 41 = HIGH (3.3V) - Normal
- Botón presionado    → GPIO 41 = LOW  (0V)   - Shutdown request
```

### Tabla de Conexiones Power Control

| Pin ESP32 | Función | Conexión | Cable Color | Notas |
|-----------|---------|----------|-------------|-------|
| **GPIO 40** | PIN_KEY_ON | Optoacoplador PC817 (colector) | 🟠 Naranja | INPUT_PULLUP, LOW=ON, HIGH=OFF |
| **GPIO 41** | PIN_KEY_OFF | Optoacoplador PC817 (colector) | 🟡 Amarillo | INPUT_PULLUP, LOW=Shutdown |
| **GPIO 21** | RELAY_MAIN | Relé principal (IN1) | 🔴 Rojo | Control 24V motores |
| **GPIO 47** | RELAY_AUX | Relé auxiliar (IN2) | 🟤 Marrón | Control 12V sensores |
| **GPIO 38** | RELAY_HOLD | Relé power hold (IN3) | 🟢 Verde | Mantiene 5V alimentación |
| **GPIO 39** | RELAY_EMER | Relé emergencia (IN4) | 🟣 Púrpura | Corte emergencia |

**⚠️ IMPORTANTE:**
- GPIO 0 y GPIO 45 ahora **LIBRES** (eran power pins en versiones antiguas)
- GPIO 0 es strapping pin - **NO usar para power control**
- GPIO 40/41 son pines estables, no strapping pins
- Usar optoacopladores para aislar 12V de 3.3V
- Resistencia 1kΩ en serie con LED del optoacoplador

---

## 📡 SENSOR DE OBSTÁCULOS TOFSense-M S 8x8 LiDAR

### Especificaciones

- **Modelo:** TOFSense-M S (Nooploop)
- **Matriz:** 8x8 puntos (64 mediciones simultáneas)
- **Rango:** 4 metros
- **Campo de visión:** 65°
- **Frecuencia:** ~15Hz
- **Protocolo:** UART 921600 baud, 400 bytes/frame
- **Header:** `57 01 FF 00` (4 bytes)

### Conexión UART0 (Nativo ESP32-S3)

```
TOFSense-M S:                ESP32-S3:
┌─────────────────┐         
│  VCC (5V)    ●──┼────────── 5V (Buck converter)
│  GND         ●──┼────────── GND
│  TX (Data)   ●──┼────────── GPIO 44 (RX) ── UART0_RX
│  RX (No usado) ─┼── NC      GPIO 43 (TX) ── UART0_TX (no conectado)
└─────────────────┘
```

### Tabla de Conexiones TOFSense

| Pin Sensor | Pin ESP32 | Cable Color | Función |
|------------|-----------|-------------|---------|
| **VCC** | 5V Buck | 🔴 Rojo | Alimentación 5V |
| **GND** | GND | ⚫ Negro | Tierra común |
| **TX** | GPIO 44 (RX) | 🟢 Verde | Datos UART (921600 baud) |
| **RX** | - | - | **NO CONECTAR** (sensor solo TX) |

### Protocolo de Datos (400 bytes)

```
Byte  0-3:   Header (57 01 FF 00)
Byte  4:     ID
Byte  5-6:   Length (0x0190 = 400 little-endian)
Byte  7-10:  System time (ms, little-endian)
Byte  11-394: Matrix data (64 pixels × 6 bytes)
    Cada pixel (6 bytes):
      Byte 0-2: Distance (3-byte signed, little-endian)
      Byte 3:   Signal strength
      Byte 4:   Status
      Byte 5:   Reserved
Byte  395:   Checksum (sum of all bytes)
Byte  396-399: Reserved

Conversión distancia (mm):
  int32_t temp = (byte[0] | (byte[1] << 8) | (byte[2] << 16));
  if (temp & 0x800000) temp |= 0xFF000000;  // Sign extend
  int32_t distanceMm = temp / 256;
```

**⚠️ MIGRACIÓN DESDE VL53L5X:**
- ❌ Eliminado: VL53L5X I²C (2 sensores)
- ❌ Eliminado: PCA9548A multiplexor I²C @ 0x71
- ❌ Eliminado: GPIO 46 (XSHUT_FRONT), GPIO 19 (XSHUT_REAR)
- ✅ Nuevo: TOFSense-M S UART (1 sensor, mejor cobertura)
- ✅ GPIO 46 ahora **LIBRE**

---

## 🔊 AUDIO DFPlayer Mini

### Conexión UART1

```
DFPlayer Mini:              ESP32-S3:
┌─────────────────┐         
│  VCC          ●──┼────────── 5V
│  GND          ●──┼────────── GND
│  TX           ●──┼────────── GPIO 17 (RX) ── UART1_RX
│  RX           ●──┼────────── GPIO 18 (TX) ── UART1_TX
│  SPK_1        ●──┼────────── Parlante (+)
│  SPK_2        ●──┼────────── Parlante (-)
│  BUSY (opt)  ●──┼── NC
└─────────────────┘
```

### Tabla de Conexiones DFPlayer

| Pin DFPlayer | Pin ESP32 | Cable Color | Función |
|--------------|-----------|-------------|---------|
| **VCC** | 5V Buck | 🔴 Rojo | Alimentación 5V |
| **GND** | GND | ⚫ Negro | Tierra común |
| **TX** | GPIO 17 (RX) | 🟡 Amarillo | Recibe respuestas |
| **RX** | GPIO 18 (TX) | 🟠 Naranja | Envía comandos |
| **SPK_1** | Parlante + | 🔵 Azul | Audio positivo |
| **SPK_2** | Parlante - | 🟢 Verde | Audio negativo |

**Configuración:**
- Baudrate: 9600 bps
- Tarjeta SD con archivos MP3 numerados (0001.mp3, 0002.mp3, etc.)

**⚠️ MIGRACIÓN:**
- ✅ Movido desde UART0 (GPIO 43/44) a UART1 (GPIO 18/17)
- ✅ UART0 liberado para TOFSense-M S (sensor prioritario)

---

## 🖥️ PANTALLA TFT + TOUCH

### Display ST7796S (480x320) - SPI

```
TFT Display:                ESP32-S3:
┌─────────────────┐         
│  VCC          ●──┼────────── 5V
│  GND          ●──┼────────── GND
│  SCK          ●──┼────────── GPIO 10 (SPI_SCK)
│  MOSI (SDA)   ●──┼────────── GPIO 11 (SPI_MOSI)
│  MISO         ●──┼────────── GPIO 12 (SPI_MISO)
│  DC (RS)      ●──┼────────── GPIO 13 (TFT_DC)
│  RST          ●──┼────────── GPIO 14 (TFT_RST)
│  CS           ●──┼────────── GPIO 16 (TFT_CS) ── Compartido
│  LED (BL)     ●──┼────────── 5V (siempre ON)
└─────────────────┘
```

### Touch XPT2046 - SPI (Compartido)

```
Touch XPT2046:              ESP32-S3:
┌─────────────────┐         
│  T_IRQ        ●──┼────────── GPIO  6 (TOUCH_IRQ)
│  T_DO (MISO)  ●──┼────────── GPIO 12 (SPI_MISO, compartido)
│  T_DIN (MOSI) ●──┼────────── GPIO 11 (SPI_MOSI, compartido)
│  T_CS         ●──┼────────── GPIO  5 (TOUCH_CS)
│  T_CLK (SCK)  ●──┼────────── GPIO 10 (SPI_SCK, compartido)
└─────────────────┘
```

### Tabla Completa TFT + Touch

| Pin Display/Touch | Pin ESP32 | Cable Color | Función |
|-------------------|-----------|-------------|---------|
| **VCC** | 5V | 🔴 Rojo | Alimentación |
| **GND** | GND | ⚫ Negro | Tierra |
| **SCK** | GPIO 10 | 🟡 Amarillo | SPI Clock (compartido) |
| **MOSI** | GPIO 11 | 🟢 Verde | SPI MOSI (compartido) |
| **MISO** | GPIO 12 | 🔵 Azul | SPI MISO (compartido) |
| **TFT DC** | GPIO 13 | 🟣 Púrpura | Data/Command select |
| **TFT RST** | GPIO 14 | ⚪ Blanco | Reset display |
| **TFT CS** | GPIO 16 | 🟠 Naranja | Chip select TFT |
| **TOUCH CS** | GPIO 5 | 🟤 Marrón | Chip select touch |
| **TOUCH IRQ** | GPIO 6 | 🔴 Rosa | Touch interrupt |

**⚠️ NOTA:**
- GPIO 16 compartido con WHEEL_RR (modo INPUT cuando no se usa SPI)
- Velocidad SPI: 40 MHz (TFT), 2 MHz (Touch)

---

## 🔌 BUS I²C

### Configuración I²C

- **SDA:** GPIO 8
- **SCL:** GPIO 9
- **Velocidad:** 400 kHz (Fast Mode)
- **Pull-ups:** 4.7kΩ a 3.3V **OBLIGATORIOS** (físicos, externos al ESP32)

### Diagrama I²C Bus

```
        3.3V
         │
    [4.7kΩ]   [4.7kΩ]     ← Pull-ups EXTERNOS obligatorios
         │       │
ESP32 ───┼───────┼─────────────┬──── TCA9548A (Multiplexor I²C @ 0x71)
GPIO 8 ──┘ SDA   │             │
GPIO 9 ──────────┘ SCL         ├──── Canal 0: PCA9685 PWM @ 0x40
                               ├──── Canal 1: INA226 #1 @ 0x40 (Motor FL)
                               ├──── Canal 2: INA226 #2 @ 0x40 (Motor FR)
                               ├──── Canal 3: INA226 #3 @ 0x40 (Motor RL)
                               ├──── Canal 4: INA226 #4 @ 0x40 (Motor RR)
                               ├──── Canal 5: INA226 #5 @ 0x40 (Steering)
                               ├──── Canal 6: INA226 #6 @ 0x40 (Battery)
                               └──── Canal 7: MCP23017 GPIO Expander @ 0x20
```

### Dispositivos I²C

| Dispositivo | Dirección I²C | Canal TCA9548A | Función |
|-------------|---------------|----------------|---------|
| **TCA9548A** | 0x71 | - | Multiplexor I²C |
| **PCA9685** | 0x40 | Canal 0 | PWM para motores (16 canales) |
| **INA226 #1** | 0x40 | Canal 1 | Corriente Motor FL |
| **INA226 #2** | 0x40 | Canal 2 | Corriente Motor FR |
| **INA226 #3** | 0x40 | Canal 3 | Corriente Motor RL |
| **INA226 #4** | 0x40 | Canal 4 | Corriente Motor RR |
| **INA226 #5** | 0x40 | Canal 5 | Corriente Dirección |
| **INA226 #6** | 0x40 | Canal 6 | Corriente Batería |
| **MCP23017** | 0x20 | Canal 7 | Expansor 16 GPIO |

**⚠️ MIGRACIÓN:**
- ❌ Eliminado: VL53L5X @ 0x29 (canales 0 y 1 de versión antigua)
- ✅ PCA9685 ahora en Canal 0 (antes Canal 2)
- ✅ Canales reorganizados para INA226

---

## 🚗 MOTORES DE TRACCIÓN

### BTS7960 Motor Drivers (x4)

Cada motor tiene su propio driver BTS7960 de 43A:

```
Motor FL (Front Left):      BTS7960 FL:               ESP32/PCA9685:
┌────────────────┐         ┌──────────────┐         
│ M+          ●──┼─────────┤ OUT1         │         GPIO/PWM via PCA9685:
│ M-          ●──┼─────────┤ OUT2         │         ├─ RPWM ← Canal PWM 0
└────────────────┘         │              │         ├─ LPWM ← Canal PWM 1
                           │ RPWM      ●──┼─────────┤ R_EN ← GPIO (MCP23017)
                           │ LPWM      ●──┼─────────┤ L_EN ← GPIO (MCP23017)
                           │ R_EN      ●──┼─────────┤ R_IS ← ADC (opcional)
                           │ L_EN      ●──┼─────────┤ L_IS ← ADC (opcional)
                           │ VCC       ●──┼────────── 5V lógica
                           │ GND       ●──┼────────── GND
                           │ B+        ●──┼────────── 24V batería
                           │ B-        ●──┼────────── GND batería
                           └──────────────┘
```

### Tabla de Conexiones Motores Tracción

| Motor | BTS7960 Pin | Destino | Cable | Función |
|-------|-------------|---------|-------|---------|
| **FL** | RPWM | PCA9685 Canal 0 | 🟡 Amarillo | PWM adelante |
| | LPWM | PCA9685 Canal 1 | 🟢 Verde | PWM atrás |
| | R_EN | MCP23017 GPA0 | 🔵 Azul | Enable derecha |
| | L_EN | MCP23017 GPA1 | 🟣 Púrpura | Enable izquierda |
| **FR** | RPWM | PCA9685 Canal 2 | 🟡 Amarillo | PWM adelante |
| | LPWM | PCA9685 Canal 3 | 🟢 Verde | PWM atrás |
| | R_EN | MCP23017 GPA2 | 🔵 Azul | Enable derecha |
| | L_EN | MCP23017 GPA3 | 🟣 Púrpura | Enable izquierda |
| **RL** | RPWM | PCA9685 Canal 4 | 🟡 Amarillo | PWM adelante |
| | LPWM | PCA9685 Canal 5 | 🟢 Verde | PWM atrás |
| | R_EN | MCP23017 GPA4 | 🔵 Azul | Enable derecha |
| | L_EN | MCP23017 GPA5 | 🟣 Púrpura | Enable izquierda |
| **RR** | RPWM | PCA9685 Canal 6 | 🟡 Amarillo | PWM adelante |
| | LPWM | PCA9685 Canal 7 | 🟢 Verde | PWM atrás |
| | R_EN | MCP23017 GPA6 | 🔵 Azul | Enable derecha |
| | L_EN | MCP23017 GPA7 | 🟣 Púrpura | Enable izquierda |

**Alimentación Motores:**
- VCC: 5V (lógica)
- GND: Común con ESP32
- B+: 24V batería (vía fusible 50A)
- B-: GND batería (común)

---

## 🎯 MOTOR DE DIRECCIÓN

### BTS7960 Steering Motor Driver

```
Motor Dirección:            BTS7960 Steering:         ESP32/PCA9685:
┌────────────────┐         ┌──────────────┐         
│ RS390       ●──┼─────────┤ OUT1         │         
│ 12V DC      ●──┼─────────┤ OUT2         │         
└────────────────┘         │              │         
                           │ RPWM      ●──┼─────────── PCA9685 Canal 8
                           │ LPWM      ●──┼─────────── PCA9685 Canal 9
                           │ R_EN      ●──┼─────────── MCP23017 GPB0
                           │ L_EN      ●──┼─────────── MCP23017 GPB1
                           │ VCC       ●──┼─────────── 5V
                           │ GND       ●──┼─────────── GND
                           │ B+        ●──┼─────────── 12V (vía Relé AUX)
                           │ B-        ●──┼─────────── GND
                           └──────────────┘
```

### Tabla de Conexiones Dirección

| Pin | Destino | Cable Color | Función |
|-----|---------|-------------|---------|
| **RPWM** | PCA9685 Canal 8 | 🟡 Amarillo | PWM derecha |
| **LPWM** | PCA9685 Canal 9 | 🟢 Verde | PWM izquierda |
| **R_EN** | MCP23017 GPB0 | 🔵 Azul | Enable derecha |
| **L_EN** | MCP23017 GPB1 | 🟣 Púrpura | Enable izquierda |
| **VCC** | 5V | 🔴 Rojo | Lógica |
| **B+** | 12V (Relé AUX) | 🔴 Rojo grueso | Motor 12V |

---

## 🔍 SENSORES DE RUEDAS

### Sensores Inductivos LJ12A3-4-Z/BX (x4)

```
Sensor LJ12A3 (NPN NO):     ESP32-S3:
┌────────────────┐         
│ Azul (GND)   ●──┼────────── GND
│ Marrón (12V) ●──┼────────── 12V (vía Relé AUX)
│ Negro (OUT)  ●──┼──┬─ [10kΩ pull-down] ─ GND
│                  │  │
│                  │  └────────── GPIO (directo 3.3V tolerante)
└────────────────┘  
```

### Tabla de Conexiones Sensores de Ruedas

| Sensor | Pin ESP32 | Cable Sensor | Función |
|--------|-----------|--------------|---------|
| **WHEEL_FL** | GPIO 3 | Negro (OUT) | Rueda Delantera Izquierda |
| **WHEEL_FR** | GPIO 4 | Negro (OUT) | Rueda Delantera Derecha |
| **WHEEL_RL** | GPIO 15 | Negro (OUT) | Rueda Trasera Izquierda |
| **WHEEL_RR** | GPIO 16 | Negro (OUT) | Rueda Trasera Derecha |

**Conexión común todos los sensores:**
- Cable Marrón: 12V (Relé AUX)
- Cable Azul: GND
- Cable Negro: GPIO (con pull-down 10kΩ a GND)

**⚠️ MIGRACIÓN:**
- ✅ WHEEL_RL: GPIO 17 → GPIO 15 (liberado para UART1)
- ✅ WHEEL_RR: GPIO 15 → GPIO 16 (compartido con TFT_CS)

---

## 📏 ENCODER DE DIRECCIÓN

### Encoder E6B2-CWZ6C (1200 PPR)

```
Encoder Dirección:          ESP32-S3:
┌────────────────┐         
│ VCC (12V)    ●──┼────────── 12V (vía Relé AUX)
│ GND          ●──┼────────── GND
│ A (Phase A)  ●──┼──┬─ [10kΩ pull-down] ─ GND
│                  │  │
│                  │  └────── GPIO 7 (ENCODER_A)
│ B (Phase B)  ●──┼──┬─ [10kΩ pull-down] ─ GND
│                  │  │
│                  │  └────── GPIO 2 (ENCODER_B)
└────────────────┘
```

### Tabla de Conexiones Encoder

| Pin Encoder | Pin ESP32 | Cable Color | Función |
|-------------|-----------|-------------|---------|
| **VCC** | 12V (Relé AUX) | 🟤 Marrón | Alimentación |
| **GND** | GND | 🔵 Azul | Tierra |
| **A** | GPIO 7 | 🟡 Amarillo | Fase A (con pull-down 10kΩ) |
| **B** | GPIO 2 | 🟢 Verde | Fase B (con pull-down 10kΩ) |

**Configuración:**
- Resolución: 1200 pulsos/revolución
- Tipo: Incremental cuadratura
- Interrupciones en GPIO 7 y 2

---

## 🎮 PEDAL Y PALANCA

### Pedal Acelerador (Sensor Hall A1324LUA-T)

```
Sensor Hall:                Divisor de Tensión:       ESP32-S3:
┌────────────────┐         
│ VCC (5V)     ●──┼────────── 5V
│ GND          ●──┼────────── GND
│ OUT (0-5V)   ●──┼──┬─ [R1: 2.7kΩ] ─┬─── GPIO 1 (ADC)
│                  │  │                │
│                  │  └─ [R2: 4.7kΩ] ─┴─── GND
└────────────────┘
                  5V → 3.3V máximo (ADC safe)
```

**Cálculo divisor:**
- Vout = Vin × (R2 / (R1 + R2))
- Vout = 5V × (4.7kΩ / 7.4kΩ) = 3.18V (< 3.3V límite ADC)

### Palanca de Cambios (4 posiciones)

```
Shifter Resistivo:          ESP32-S3:
┌────────────────┐         
│ PIN 1 (señal)●──┼──┬─ [Pull-up 10kΩ a 3.3V]
│                  │  │
│                  │  └────── MCP23017 GPA (via I²C)
│ PIN 2 (GND)  ●──┼────────── GND
└────────────────┘

Posiciones:
- Adelante:  Resistencia 0Ω    → LOW
- Neutral:   Resistencia 10kΩ  → HIGH
- Atrás:     Resistencia 4.7kΩ → MEDIUM
- Parking:   Resistencia 22kΩ  → HIGH (diferente nivel)
```

### Tabla Pedal y Palanca

| Dispositivo | Pin ESP32/I²C | Cable Color | Función |
|-------------|---------------|-------------|---------|
| **Pedal** | GPIO 1 (ADC) | 🟡 Amarillo | 0-3.3V analógico |
| **Shifter** | MCP23017 GPA | 🟢 Verde | 4 posiciones resistivo |

---

## 💡 LEDS WS2812B

### Tiras LED Direccionables

```
LED Frontales:              ESP32-S3:
┌────────────────┐         
│ VCC (5V)     ●──┼────────── 5V (con C=1000µF cerca)
│ GND          ●──┼────────── GND
│ DIN (Data)   ●──┼──── [R=330Ω] ──── GPIO 19 (LED_FRONT)
└────────────────┘

LED Traseros:               ESP32-S3:
┌────────────────┐         
│ VCC (5V)     ●──┼────────── 5V (con C=1000µF cerca)
│ GND          ●──┼────────── GND
│ DIN (Data)   ●──┼──── [R=330Ω] ──── GPIO 48 (LED_REAR)
└────────────────┘
```

### Tabla de Conexiones LEDs

| LED Strip | Pin ESP32 | Componentes | Cable Color |
|-----------|-----------|-------------|-------------|
| **Frontales** | GPIO 19 | Resistencia 330Ω + Capacitor 1000µF | 🟢 Verde |
| **Traseros** | GPIO 48 | Resistencia 330Ω + Capacitor 1000µF | 🔵 Azul |

**⚠️ IMPORTANTE:**
- GPIO 19 anteriormente era XSHUT_REAR (VL53L5X), ahora LED_FRONT
- Capacitor 1000µF/10V cerca de VCC/GND de cada tira (estabilización)
- Resistencia 330Ω en serie con DIN (protección señal)

---

## 🌡️ SENSORES DE TEMPERATURA

### DS18B20 OneWire (x4)

```
DS18B20 (x4 en paralelo):   ESP32-S3:
┌────────────────┐         
│ VCC (5V)     ●──┼────────── 5V
│ DQ (Data)    ●──┼──┬─ [Pull-up 4.7kΩ a 5V] ─ 5V
│                  │  │
│                  │  └────── GPIO 20 (ONEWIRE_PIN)
│ GND          ●──┼────────── GND
└────────────────┘
```

### Configuración OneWire

| Parámetro | Valor |
|-----------|-------|
| **Pin Data** | GPIO 20 |
| **Pull-up** | 4.7kΩ a 5V **OBLIGATORIO** |
| **Sensores** | 4 DS18B20 en paralelo |
| **Direcciones** | Únicas 64-bit ROM |

**Sensores de Temperatura:**
1. Motor FL (Front Left)
2. Motor FR (Front Right)
3. Motor RL (Rear Left)
4. Motor RR (Rear Right)

---

## 🔌 RELÉS

### Módulo 4 Relés (5V Lógica)

```
Relé 1 (Main):              ESP32-S3:
┌────────────────┐         
│ VCC          ●──┼────────── 5V
│ GND          ●──┼────────── GND
│ IN1          ●──┼────────── GPIO 21 (RELAY_MAIN)
│ COM1         ●──┼────────── 24V Batería
│ NO1          ●──┼────────── Motores Tracción (BTS7960 B+)
└────────────────┘
```

### Tabla de Relés

| Relé | GPIO | Función | Carga |
|------|------|---------|-------|
| **RELAY_MAIN** | GPIO 21 | Motor tracción ON/OFF | 24V → Motores (50A fusible) |
| **RELAY_AUX** | GPIO 47 | Sensores 12V ON/OFF | 12V → Sensores (30A fusible) |
| **RELAY_HOLD** | GPIO 38 | Power hold (autoencendido) | 5V Buck → Sistema |
| **RELAY_EMER** | GPIO 39 | Emergencia (corte total) | Corta 24V/12V |

**Secuencia de encendido:**
1. Llave ON → GPIO 40 = LOW
2. ESP32 activa RELAY_HOLD (GPIO 38 = HIGH)
3. ESP32 activa RELAY_AUX (GPIO 47 = HIGH) → Sensores 12V
4. ESP32 activa RELAY_MAIN (GPIO 21 = HIGH) → Motores 24V
5. Sistema operativo

**Secuencia de apagado:**
1. Llave OFF → GPIO 40 = HIGH
2. ESP32 desactiva RELAY_MAIN (GPIO 21 = LOW)
3. ESP32 desactiva RELAY_AUX (GPIO 47 = LOW)
4. ESP32 desactiva RELAY_HOLD (GPIO 38 = LOW)
5. Sistema apagado

---

## 🆓 GPIOS LIBRES PARA EXPANSIÓN

Tras la migración a TOFSense-M S v2.15.0, los siguientes GPIOs están **completamente libres**:

| GPIO | Tipo | Uso Anterior | Disponible Para |
|------|------|--------------|-----------------|
| **GPIO 0** | Strapping | PIN_KEY_SYSTEM | ⚠️ Botón emergencia (con pull-up 10kΩ) |
| **GPIO 45** | Strapping | PIN_KEY_DETECT | ⚠️ Expansión (cuidado VDD_SPI) |
| **GPIO 46** | Strapping | XSHUT_FRONT (VL53L5X) | ⚠️ Expansión (con pull-up 10kΩ) |

**⚠️ ADVERTENCIAS GPIOs Libres:**
- **GPIO 0:** Strapping pin - Requiere pull-up 10kΩ externo. Solo usar para entrada con debounce.
- **GPIO 45:** Strapping pin VDD_SPI - Evitar o usar con pull-down si es necesario.
- **GPIO 46:** Strapping pin - Requiere pull-up 10kΩ externo. Mantener HIGH en boot.

**Recomendaciones de Uso:**
- GPIO 0: Botón de emergencia (pull-up + debounce software)
- GPIO 46: Señal de entrada digital (mantener HIGH en boot)
- GPIO 45: Evitar o consultar datasheet ESP32-S3 primero

---

## ✅ CHECKLIST DE VERIFICACIÓN

### Pre-Conexión
- [ ] Fusible 50A instalado en línea 24V
- [ ] Fusible 30A instalado en línea 12V
- [ ] Fusible 5A instalado en línea 5V
- [ ] Pull-ups 4.7kΩ en SDA/SCL I²C (físicos, externos)
- [ ] Pull-up 4.7kΩ en OneWire GPIO 20
- [ ] Pull-up 10kΩ en GPIO 0 (strapping)
- [ ] Pull-up 10kΩ en GPIO 46 (strapping)
- [ ] Divisor de tensión pedal (2.7kΩ + 4.7kΩ)
- [ ] Resistencias 330Ω en LEDs WS2812B DIN
- [ ] Capacitores 1000µF en LEDs WS2812B VCC

### Conexiones Críticas
- [ ] TOFSense-M S: TX → GPIO 44 (UART0 RX) @ 921600 baud
- [ ] DFPlayer: TX → GPIO 17, RX → GPIO 18 (UART1) @ 9600 baud
- [ ] Power ON: Optoacoplador → GPIO 40 (INPUT_PULLUP)
- [ ] Power OFF: Optoacoplador → GPIO 41 (INPUT_PULLUP)
- [ ] TFT: CS → GPIO 16, DC → GPIO 13, RST → GPIO 14
- [ ] Touch: CS → GPIO 5, IRQ → GPIO 6
- [ ] I²C: SDA → GPIO 8, SCL → GPIO 9 (con pull-ups 4.7kΩ)

### Test de Alimentación
- [ ] 24V batería presente (22-26V rango)
- [ ] 12V batería presente (11-13V rango)
- [ ] Buck 5V salida estable (4.9-5.1V)
- [ ] ESP32 3.3V LDO estable (3.25-3.35V)
- [ ] Relés todos apagados antes de conectar

### Test Funcional
- [ ] TOFSense recibe frames de 400 bytes @ 921600 baud
- [ ] Distancias 8x8 válidas (0-4000mm)
- [ ] DFPlayer reproduce audio correctamente
- [ ] Pantalla TFT muestra imagen
- [ ] Touch detecta toques correctos
- [ ] Motores responden a PWM
- [ ] Sensores de rueda detectan pulsos
- [ ] Encoder dirección cuenta correctamente
- [ ] LEDs WS2812B encienden con colores correctos
- [ ] Temperatura DS18B20 lee valores razonables

### Test de Seguridad
- [ ] Relé emergencia corta todo al activar
- [ ] Freno de emergencia funciona (sensor falla)
- [ ] Power hold mantiene alimentación
- [ ] Shutdown ordenado funciona correctamente
- [ ] GPIO 0/45/46 en estado correcto durante boot

---

## 📚 REFERENCIAS

### Datasheets
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [TOFSense-M S User Manual](https://ftp.nooploop.com/software/products/tofsense_m/doc/TOFSense-M_User_Manual_V1.4_en.pdf)
- [ST7796S Display Datasheet](https://www.displayfuture.com/Display/datasheet/controller/ST7796s.pdf)
- [BTS7960 Motor Driver](https://www.handsontec.com/dataspecs/module/BTS7960%20Motor%20Driver.pdf)
- [DS18B20 Temperature Sensor](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf)

### Documentos Relacionados
- `TOFSENSE_INTEGRATION.md` - Protocolo detallado TOFSense-M S
- `OBSTACLE_SAFETY_FEATURES.md` - Sistema de seguridad anticolisión
- `MIGRATION_SUMMARY_v2.12.0.md` - Resumen migración VL53L5X → TOFSense
- `PIN_MAPPING_DEVKITC1.md` - Mapeo completo de pines ESP32-S3

---

**Versión:** v2.15.0  
**Última Actualización:** 2026-01-05  
**Estado:** ✅ Validado para hardware v2.15.0 con TOFSense-M S 8x8 Matrix

