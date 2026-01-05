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
9. [Módulos Optoacopladores HY-M158](#-módulos-optoacopladores-hy-m158-x2)
10. [Sensores de Ruedas](#-sensores-de-ruedas)
11. [Encoder de Dirección](#-encoder-de-dirección)
12. [Pedal y Palanca](#-pedal-y-palanca)
13. [LEDs WS2812B](#-leds-ws2812b)
14. [Sensores de Temperatura](#-sensores-de-temperatura)
15. [Relés](#-relés)
16. [GPIOs Libres](#-gpios-libres-para-expansión)
17. [Checklist de Verificación](#-checklist-de-verificación)

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
- **GPIO 0, 45, 46 ahora LIBRES** tras migración v2.15.0
- **GPIO 40/41 ahora usados para Power Control** (eran BTN_MEDIA/BTN_4X4)
- GPIO 0, 45, 46 son strapping pins - **usar con precaución**
- GPIO 40/41 son pines estables, no strapping pins
- Usar optoacopladores PC817 para aislar 12V de 3.3V
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

### 🔌 MÓDULOS OPTOACOPLADORES HY-M158 (x2)

**⚠️ CRÍTICO**: Se usan 2 módulos HY-M158 (8 canales c/u) para aislar señales 5V/12V → 3.3V

#### HY-M158 Módulo #1 - Sensores y Encoder

```
Lado 5V/12V (Entrada):          PC817 Optoacoplador:    Lado 3.3V (Salida):

Sensores 12V/5V                                         ESP32-S3 / MCP23017
┌──────────────┐                                       
│ WHEEL_FL ────┼─► IN1 ───► [LED ─┴─ Foto] ──► OUT1 ──► GPIO 3
│ WHEEL_FR ────┼─► IN2 ───► [LED ─┴─ Foto] ──► OUT2 ──► GPIO 4  
│ WHEEL_RL ────┼─► IN3 ───► [LED ─┴─ Foto] ──► OUT3 ──► GPIO 15
│ WHEEL_RR ────┼─► IN4 ───► [LED ─┴─ Foto] ──► OUT4 ──► GPIO 16
│ ENCODER_A ───┼─► IN5 ───► [LED ─┴─ Foto] ──► OUT5 ──► GPIO 7
│ ENCODER_B ───┼─► IN6 ───► [LED ─┴─ Foto] ──► OUT6 ──► GPIO 2
│ ENCODER_Z ───┼─► IN7 ───► [LED ─┴─ Foto] ──► OUT7 ──► (Reserva)
│ RESERVA ─────┼─► IN8 ───► [LED ─┴─ Foto] ──► OUT8 ──► (Reserva)
│              │
│ VCC: +12V/5V │             Aislamiento Galvánico      VCC: +3.3V
│ GND: Común   │                                        GND: Común
└──────────────┘                                       
```

#### HY-M158 Módulo #2 - Palanca de Cambios (Shifter)

```
Lado 12V (Entrada):             PC817 Optoacoplador:    Lado 3.3V (Salida):

Palanca 12V DC                                          MCP23017 I²C (0x20)
┌──────────────┐                                       
│ P (Park) ────┼─► IN1 ───► [LED ─┴─ Foto] ──► OUT1 ──► GPIOB0 (pin 8)
│ R (Reverse) ─┼─► IN2 ───► [LED ─┴─ Foto] ──► OUT2 ──► GPIOB1 (pin 9)
│ N (Neutral) ─┼─► IN3 ───► [LED ─┴─ Foto] ──► OUT3 ──► GPIOB2 (pin 10)
│ D1 (Drive 1)─┼─► IN4 ───► [LED ─┴─ Foto] ──► OUT4 ──► GPIOB3 (pin 11)
│ D2 (Drive 2)─┼─► IN5 ───► [LED ─┴─ Foto] ──► OUT5 ──► GPIOB4 (pin 12)
│ RESERVA ─────┼─► IN6 ───► [LED ─┴─ Foto] ──► OUT6 ──► (Libre)
│ RESERVA ─────┼─► IN7 ───► [LED ─┴─ Foto] ──► OUT7 ──► (Libre)
│ RESERVA ─────┼─► IN8 ───► [LED ─┴─ Foto] ──► OUT8 ──► (Libre)
│              │                                           │
│ COM: +12V    │             Aislamiento Galvánico      MCP23017:
│ VCC: +12V    │                                        SDA → GPIO 8
│ GND: Común   │                                        SCL → GPIO 9
└──────────────┘                                        VCC → 3.3V
```

**Funcionamiento PC817:**
1. Lado entrada (12V): Señal activa → LED enciende → Resistencia limitadora ~1kΩ
2. Aislamiento óptico: Luz cruza barrera galvánica (sin conexión eléctrica)
3. Lado salida (3.3V): Fototransistor conduce → Salida va a GND (LOW)
4. Pull-up en salida: 10kΩ a 3.3V → Sin señal = HIGH, con señal = LOW

**Tabla Resumen HY-M158:**

| Módulo | Canal | Entrada | Voltaje IN | Salida | Destino | Función |
|--------|-------|---------|------------|--------|---------|---------|
| **#1** | CH1 | WHEEL_FL | 12V | OUT1 | GPIO 3 | Sensor rueda FL |
| **#1** | CH2 | WHEEL_FR | 12V | OUT2 | GPIO 4 | Sensor rueda FR |
| **#1** | CH3 | WHEEL_RL | 12V | OUT3 | GPIO 15 | Sensor rueda RL |
| **#1** | CH4 | WHEEL_RR | 12V | OUT4 | GPIO 16 | Sensor rueda RR |
| **#1** | CH5 | ENCODER_A | 5V | OUT5 | GPIO 7 | Encoder fase A |
| **#1** | CH6 | ENCODER_B | 5V | OUT6 | GPIO 2 | Encoder fase B |
| **#1** | CH7 | ENCODER_Z | 5V | OUT7 | Reserva | Encoder señal Z |
| **#1** | CH8 | — | — | OUT8 | Reserva | Disponible |
| **#2** | CH1 | Shifter P | 12V | OUT1 | MCP GPIOB0 | Park |
| **#2** | CH2 | Shifter R | 12V | OUT2 | MCP GPIOB1 | Reverse |
| **#2** | CH3 | Shifter N | 12V | OUT3 | MCP GPIOB2 | Neutral |
| **#2** | CH4 | Shifter D1 | 12V | OUT4 | MCP GPIOB3 | Drive 1 |
| **#2** | CH5 | Shifter D2 | 12V | OUT5 | MCP GPIOB4 | Drive 2 |
| **#2** | CH6-8 | — | — | OUT6-8 | Reserva | Disponibles |

**Ventajas:**
- ✅ Aislamiento galvánico (protege ESP32/MCP23017)
- ✅ Acepta 5V y 12V sin conversores adicionales
- ✅ Protección contra sobrevoltajes
- ✅ Reduce ruido eléctrico
- ✅ Seguridad: Separa potencia de control

---

### Sensores Inductivos LJ12A3-4-Z/BX (x4)

**⚠️ Conexión via HY-M158 Módulo #1 (CH1-4)**

```
Sensor LJ12A3 (NPN NO):     HY-M158 #1:         ESP32-S3:
┌────────────────┐         
│ Azul (GND)   ●──┼────────── GND
│ Marrón (12V) ●──┼────────── 12V (vía Relé AUX)
│ Negro (OUT)  ●──┼────────── IN1-4 ──► PC817 ──► OUT1-4 ──► GPIO
└────────────────┘           (Módulo HY-M158)
```

### Tabla de Conexiones Sensores de Ruedas

| Sensor | HY-M158 IN | Pin ESP32 | Cable Sensor | Función |
|--------|------------|-----------|--------------|---------|
| **WHEEL_FL** | IN1 → OUT1 | GPIO 3 | Negro (OUT) | Rueda Delantera Izquierda |
| **WHEEL_FR** | IN2 → OUT2 | GPIO 4 | Negro (OUT) | Rueda Delantera Derecha |
| **WHEEL_RL** | IN3 → OUT3 | GPIO 15 | Negro (OUT) | Rueda Trasera Izquierda |
| **WHEEL_RR** | IN4 → OUT4 | GPIO 16 | Negro (OUT) | Rueda Trasera Derecha |

**Conexión común todos los sensores:**
- Cable Marrón: 12V (Relé AUX)
- Cable Azul: GND
- Cable Negro: HY-M158 IN1-4 (entrada 12V)

**⚠️ MIGRACIÓN:**
- ✅ WHEEL_RL: GPIO 17 → GPIO 15 (liberado para UART1)
- ✅ WHEEL_RR: GPIO 15 → GPIO 16 (compartido con TFT_CS)

---

## 📏 ENCODER DE DIRECCIÓN

### Encoder E6B2-CWZ6C (1200 PPR)

**⚠️ Conexión via HY-M158 Módulo #1 (CH5-7)**

```
Encoder Dirección:          HY-M158 #1:         ESP32-S3:
┌────────────────┐         
│ VCC (12V)    ●──┼────────── 12V (vía Relé AUX)
│ GND          ●──┼────────── GND
│ A (Phase A)  ●──┼────────── IN5 ──► PC817 ──► OUT5 ──► GPIO 7
│ B (Phase B)  ●──┼────────── IN6 ──► PC817 ──► OUT6 ──► GPIO 2
│ Z (Index)    ●──┼────────── IN7 ──► PC817 ──► OUT7 ──► (Reserva)
└────────────────┘           (Módulo HY-M158)
```

### Tabla de Conexiones Encoder

| Pin Encoder | HY-M158 IN | Pin ESP32 | Cable Color | Función |
|-------------|------------|-----------|-------------|---------|
| **VCC** | — | 12V (Relé AUX) | 🟤 Marrón | Alimentación |
| **GND** | — | GND | 🔵 Azul | Tierra |
| **A** | IN5 → OUT5 | GPIO 7 | 🟡 Amarillo | Fase A (vía HY-M158) |
| **B** | IN6 → OUT6 | GPIO 2 | 🟢 Verde | Fase B (vía HY-M158) |
| **Z** | IN7 → OUT7 | Reserva | ⚪ Blanco | Índice (opcional, via HY-M158) |

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

### Palanca de Cambios (5 posiciones - 12V via HY-M158)

⚠️ **IMPORTANTE**: La palanca opera a **12V DC** y requiere aislamiento mediante optoacopladores HY-M158.

```
Palanca 12V:                HY-M158 #2:              MCP23017:         ESP32-S3:
┌───────────────┐          
│ P  ●──────────┼── (🔴 Rojo) ───► IN1 ──► PC817 ──► OUT1 ──► GPIOB0 ──┐
│ R  ●──────────┼── (⚪ Blanco) ─► IN2 ──► PC817 ──► OUT2 ──► GPIOB1 ──┤
│ N  ●──────────┼── (🟢 Verde) ──► IN3 ──► PC817 ──► OUT3 ──► GPIOB2 ──┼─ I²C
│ D1 ●──────────┼── (🔵 Azul) ───► IN4 ──► PC817 ──► OUT4 ──► GPIOB3 ──┤ 0x20
│ D2 ●──────────┼── (🟡 Amarillo)► IN5 ──► PC817 ──► OUT5 ──► GPIOB4 ──┘
│               │                                                    │
│ COM ●─────────┼── +12V                                    SDA ──► GPIO 8
└───────────────┘                                           SCL ──► GPIO 9

    12V lado                3.3V lado aislado galvánicamente
    ═══════════             ═══════════════════════════════
```

**Funcionamiento:**
1. Palanca conecta +12V a posición seleccionada (P, R, N, D1, D2)
2. HY-M158 aísla y convierte 12V → 3.3V (optoacoplador PC817)
3. MCP23017 lee con pull-ups internos (LOW = activo)
4. ESP32-S3 lee via I²C con prioridad P > R > N > D1 > D2
5. Debounce 50ms en firmware

### Tabla Pedal y Palanca

| Dispositivo | Pin ESP32/I²C | Cable Color | Voltaje | Función |
|-------------|---------------|-------------|---------|---------|
| **Pedal** | GPIO 1 (ADC) | 🟡 Amarillo | 0-3.3V | Analógico (divisor resistivo) |
| **Shifter P** | MCP23017 GPIOB0 | 🔴 Rojo | 12V | Park (via HY-M158) |
| **Shifter R** | MCP23017 GPIOB1 | ⚪ Blanco | 12V | Reverse (via HY-M158) |
| **Shifter N** | MCP23017 GPIOB2 | 🟢 Verde | 12V | Neutral (via HY-M158) |
| **Shifter D1** | MCP23017 GPIOB3 | 🔵 Azul | 12V | Drive 1 (via HY-M158) |
| **Shifter D2** | MCP23017 GPIOB4 | 🟡 Amarillo | 12V | Drive 2 (via HY-M158) |

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

| GPIO | Tipo | Uso Anterior (v2.8.x-v2.14.x) | Disponible Para |
|------|------|------------------------------|-----------------|
| **GPIO 0** | Strapping | PIN_KEY_SYSTEM (power on) | ⚠️ Entrada digital (requiere pull-up 10kΩ) |
| **GPIO 40** | Normal | BTN_MEDIA (multimedia button) | ✅ **AHORA: PIN_KEY_ON (power control)** |
| **GPIO 41** | Normal | BTN_4X4 (4x4 mode button) | ✅ **AHORA: PIN_KEY_OFF (shutdown)** |
| **GPIO 45** | Strapping | PIN_KEY_DETECT (shutdown) | ⚠️ Expansión (cuidado VDD_SPI) |
| **GPIO 46** | Strapping | XSHUT_FRONT (VL53L5X sensor) | ⚠️ Expansión (requiere pull-up 10kΩ) |

**⚠️ ADVERTENCIAS GPIOs Libres:**
- **GPIO 0:** Strapping pin BOOT - Requiere pull-up 10kΩ externo. Debe estar HIGH en boot para modo normal.
- **GPIO 45:** Strapping pin VDD_SPI - Evitar uso o consultar datasheet ESP32-S3.
- **GPIO 46:** Strapping pin - Requiere pull-up 10kΩ externo. Debe estar HIGH en boot.

**✅ GPIOs REASIGNADOS (v2.15.0):**
- **GPIO 40:** Ahora PIN_KEY_ON (ignition/power ON detection)
- **GPIO 41:** Ahora PIN_KEY_OFF (shutdown request detection)

**Recomendaciones de Uso GPIOs Libres:**
- **GPIO 0:** Solo para entrada (ej: botón emergencia con pull-up + debounce)
- **GPIO 46:** Entrada digital (mantener HIGH en boot)
- **GPIO 45:** Evitar - Usar GPIO 40/41 si necesitas más pines

---

## ✅ CHECKLIST DE VERIFICACIÓN

### Pre-Conexión
- [ ] Fusible 50A instalado en línea 24V
- [ ] Fusible 30A instalado en línea 12V
- [ ] Fusible 5A instalado en línea 5V
- [ ] Pull-ups 4.7kΩ en SDA/SCL I²C (físicos, externos)
- [ ] Pull-up 4.7kΩ en OneWire GPIO 20
- [ ] Pull-up 10kΩ en GPIO 40 (power ON, lado 3.3V optoacoplador)
- [ ] Pull-up 10kΩ en GPIO 41 (shutdown, lado 3.3V optoacoplador)
- [ ] Pull-up 10kΩ en GPIO 0 (solo si se usa - strapping pin)
- [ ] Pull-up 10kΩ en GPIO 46 (solo si se usa - strapping pin)
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
- [ ] GPIO 40 detecta llave ON (LOW activo)
- [ ] GPIO 41 detecta shutdown request (LOW activo)
- [ ] GPIO 0/45/46 libres en estado HIGH durante boot (si no se usan)

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

