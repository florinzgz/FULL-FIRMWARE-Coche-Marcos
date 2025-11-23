# 📋 GUÍA COMPLETA DE CONEXIONES HARDWARE
## ESP32-S3-DevKitC-1 (44 pines) - Firmware Coche Inteligente Marcos

**Versión del firmware:** Compatible con ESP32-S3-DevKitC-1  
**Última actualización:** 2025-11-05  
**Documento:** Conexiones pin a pin detalladas

---

## 📌 ÍNDICE

1. [Referencia Rápida de Pines](#referencia-rápida-de-pines)
2. [Pantalla ILI9488 + Táctil XPT2046](#1-pantalla-ili9488--táctil-xpt2046)
3. [Relés HY-M158](#2-relés-hy-m158-estado-sólido)
4. [Relés Secundarios (Módulo 8 canales)](#3-relés-secundarios-módulo-8-canales-5v)
5. [Motores BTS7960](#4-motores-bts7960-43a)
6. [DFPlayer Mini](#5-dfplayer-mini-audio)
7. [Sensores de Rueda LJ12A3-4-Z/BX](#6-sensores-de-rueda-lj12a3-4-zbx)
8. [Encoder Dirección E6B2-CWZ6C](#7-encoder-dirección-e6b2-cwz6c-1200pr)
9. [Sensores INA226 (I2C)](#8-sensores-ina226-bus-i2c)
10. [LEDs WS2812B](#9-leds-ws2812b-inteligentes)
11. [Pedal Acelerador](#10-pedal-acelerador)
12. [Botones de Control](#11-botones-de-control)
13. [Sensor Temperatura DS18B20](#12-sensor-temperatura-ds18b20)
14. [Diagrama de Alimentación](#13-sistema-de-alimentación)
15. [Checklist Pre-Encendido](#14-checklist-pre-encendido)

---

## 🎯 REFERENCIA RÁPIDA DE PINES

### Pinout ESP32-S3-DevKitC-1

```
Lado Izquierdo:                  Lado Derecho:
┌─────────────────┐             ┌─────────────────┐
│ 3V3    → 3.3V   │             │ 5V0    → 5V     │
│ EN     → Reset  │             │ IO46   → GPIO46 │
│ IO0    → GPIO0  │             │ IO45   → GPIO45 │
│ IO1    → GPIO1  │  [USB-C]    │ IO48   → GPIO48 │
│ IO2    → GPIO2  │             │ IO47   → GPIO47 │
│ IO3    → GPIO3  │             │ IO21   → GPIO21 │
│ IO4    → GPIO4  │             │ IO20   → GPIO20 │
│ IO5    → GPIO5  │             │ IO38   → GPIO38 │
│ IO6    → GPIO6  │   ESP32-    │ IO37   → GPIO37 │
│ IO7    → GPIO7  │     S3      │ IO36   → GPIO36 │
│ IO8    → GPIO8  │             │ IO35   → GPIO35 │
│ IO9    → GPIO9  │  DevKitC-1  │ IO34   → GPIO34 │
│ IO10   → GPIO10 │             │ IO33   → GPIO33 │
│ IO11   → GPIO11 │             │ IO32   → GPIO32 │
│ IO12   → GPIO12 │             │ IO31   → GPIO31 │
│ IO13   → GPIO13 │             │ IO30   → GPIO30 │
│ IO14   → GPIO14 │             │ IO29   → GPIO29 │
│ IO15   → GPIO15 │             │ IO28   → GPIO28 │
│ IO16   → GPIO16 │             │ IO27   → GPIO27 │
│ IO17   → GPIO17 │             │ IO26   → GPIO26 │
│ IO18   → GPIO18 │             │ IO25   → GPIO25 │
│ IO19   → GPIO19 │             │ IO24   → GPIO24 │
│ GND    → Tierra │             │ GND    → Tierra │
└─────────────────┘             └─────────────────┘
```

### Tabla de Asignación de Pines

| Función | GPIO | Lado | Dispositivo |
|---------|------|------|-------------|
| **Relés Principales** |
| RELAY_MAIN | 2 | Izq | HY-M158 #2 (Sistema 12V) |
| RELAY_TRAC | 4 | Izq | HY-M158 #1 (Tracción 24V) |
| RELAY_DIR | 5 | Izq | Módulo 8ch - Encoder |
| RELAY_SPARE | 6 | Izq | Módulo 8ch - Reserva (consolidado) |
| **LEDs Inteligentes** |
| LED_FRONT | 0 | Izq | WS2812B Frontales (28 LEDs) |
| LED_REAR | 1 | Izq | WS2812B Traseros (16 LEDs) |
| **Pantalla TFT** |
| TFT_MOSI | 11 | Izq | ILI9488 SDI |
| TFT_CS | 15 | Izq | ILI9488 CS |
| TFT_RST | 14 | Izq | ILI9488 RESET |
| TFT_DC | 27 | Der | ILI9488 DC |
| TFT_SCK | 18 | Izq | ILI9488 CLK (compartido) |
| TFT_MISO | 19 | Izq | ILI9488 SDO (compartido) |
| **Táctil XPT2046** |
| TOUCH_CS | 12 | Izq | XPT2046 T_CS |
| TOUCH_IRQ | 13 | Izq | XPT2046 T_IRQ |
| **Encoder Dirección** |
| ENCODER_A | 37 | Der | E6B2-CWZ6C Canal A |
| ENCODER_B | 38 | Der | E6B2-CWZ6C Canal B |
| ENCODER_Z | 25 | Der | LJ12A3 Índice Z |
| **DFPlayer Mini** |
| DFPLAYER_RX | 16 | Izq | DFPlayer TX |
| DFPLAYER_TX | 17 | Izq | DFPlayer RX |
| **Sensores Rueda** |
| WHEEL_FL | 35 | Der | LJ12A3 Frontal Izq |
| WHEEL_FR | 36 | Der | LJ12A3 Frontal Der |
| WHEEL_RL | 30 | Der | LJ12A3 Trasera Izq |
| WHEEL_RR | 31 | Der | LJ12A3 Trasera Der |
| **Bus I2C** |
| I2C_SDA | 21 | Der | INA226 Datos |
| I2C_SCL | 20 | Der | INA226 Reloj |
| **Motores BTS7960** |
| FL_PWM | 1 | Izq | Motor FL PWM |
| FL_IN1 | 3 | Izq | Motor FL IN1 |
| FL_IN2 | 6 | Izq | Motor FL IN2 |
| FR_PWM | 7 | Izq | Motor FR PWM |
| FR_IN1 | 8 | Izq | Motor FR IN1 |
| FR_IN2 | 9 | Izq | Motor FR IN2 |
| RL_PWM | 10 | Izq | Motor RL PWM |
| RL_IN1 | 47 | Der | Motor RL IN1 |
| RL_IN2 | 48 | Der | Motor RL IN2 |
| RR_PWM | 24 | Der | Motor RR PWM |
| RR_IN1 | 28 | Der | Motor RR IN1 |
| RR_IN2 | 29 | Der | Motor RR IN2 |
| **Botones** |
| BTN_LIGHTS | 40 | - | Botón Luces |
| BTN_MEDIA | 41 | - | Botón Media |
| BTN_4X4 | 42 | - | Botón 4x4 |
| BTN_BATTERY | 46 | Der | Botón Batería |
| **Otros** |
| PEDAL | 34 | Der | Pedal Acelerador ADC |
| ONEWIRE | 26 | Der | DS18B20 Temperatura |

---

## 1. 📟 PANTALLA ILI9488 + TÁCTIL XPT2046

### Conexiones Pantalla ILI9488

| Pin Pantalla | Nombre | Cable | ESP32-S3 GPIO | Lado | Función |
|--------------|--------|-------|---------------|------|---------|
| 1 | VCC | Rojo | 3.3V | Izq | Alimentación 3.3V |
| 2 | GND | Negro | GND | Ambos | Tierra común |
| 3 | CS | Naranja | **GPIO 15** | Izq | Chip Select pantalla |
| 4 | RESET | Amarillo | **GPIO 14** | Izq | Reset pantalla |
| 5 | DC (A0) | Verde | **GPIO 27** | Der | Data/Command |
| 6 | SDI (MOSI) | Azul | **GPIO 11** | Izq | Datos SPI salida |
| 7 | SCK (CLK) | Violeta | **GPIO 18** | Izq | Reloj SPI |
| 8 | LED (BLK) | Blanco | 3.3V | Izq | Backlight ON |
| 9 | SDO (MISO) | Gris | **GPIO 19** | Izq | Datos SPI entrada |

### Conexiones Táctil XPT2046 (mismo módulo)

| Pin Táctil | Nombre | Cable | ESP32-S3 GPIO | Compartido con |
|------------|--------|-------|---------------|----------------|
| T_CLK | Clock | - | **GPIO 18** | SCK Pantalla |
| T_CS | Chip Sel | Marrón | **GPIO 12** | - |
| T_DIN | Data In | - | **GPIO 11** | MOSI Pantalla |
| T_DO | Data Out | - | **GPIO 19** | MISO Pantalla |
| T_IRQ | Interrupt | Rosa | **GPIO 13** | - (opcional) |

**⚠️ IMPORTANTE:**
- La pantalla y el táctil comparten el bus SPI (GPIO 11, 18, 19)
- Usa cables cortos (< 20cm) para evitar ruido
- Conecta LED a 3.3V para backlight siempre encendido
- T_IRQ es opcional (se puede dejar sin conectar)

---

## 2. 🔌 RELÉS HY-M158 (ESTADO SÓLIDO)

### HY-M158 #1 - Tracción Motores 24V (40A)

| Pin HY-M158 #1 | Cable | ESP32-S3 | Lado | Función |
|----------------|-------|----------|------|---------|
| DC+ (Input) | Rojo | **GPIO 4** | Izq | Control ON/OFF |
| DC- (Input) | Negro | GND | Ambos | Tierra común ESP32 |
| **Carga** |
| + Output | Rojo grueso | Batería 24V+ | - | Entrada alimentación |
| - Load | Negro grueso | BTS7960 VCC | - | Salida a motores |
| - Output | Negro grueso | Batería 24V- | - | GND alimentación |

### HY-M158 #2 - Sistema General 12V (40A)

| Pin HY-M158 #2 | Cable | ESP32-S3 | Lado | Función |
|----------------|-------|----------|------|---------|
| DC+ (Input) | Rojo | **GPIO 2** | Izq | Control ON/OFF |
| DC- (Input) | Negro | GND | Ambos | Tierra común ESP32 |
| **Carga** |
| + Output | Rojo grueso | Batería 12V+ | - | Entrada alimentación |
| - Load | Negro grueso | TFT/Audio/Luces | - | Salida a sistema |
| - Output | Negro grueso | Batería 12V- | - | GND alimentación |

**Esquema conexión:**

```
Batería 12V (+) ──────► [HY-M158 #2] ──────► TFT / Audio / Luces
                            ▲
                            │
                       GPIO 2 (Control)
                            
Batería 24V (+) ──────► [HY-M158 #1] ──────► 4x BTS7960 (motores)
                            ▲
                            │
                       GPIO 4 (Control)

ESP32-S3 GND ◄──────────┴────────────────► Baterías GND (común)
```

---

## 3. ⚡ RELÉS SECUNDARIOS (MÓDULO 4 CANALES 5V SRD-05VDC)

| Canal | GPIO | Lado | Dispositivo | Función |
|-------|------|------|-------------|---------|
| VCC | - | - | 5V | Alimentación módulo |
| GND | - | - | GND | Tierra común |
| IN1 | **2** | Izq | Power Hold | Mantiene buck 5V activo |
| IN2 | **4** | Izq | 12V Aux | Alimenta sensores/encoder |
| IN3 | **5** | Izq | 24V Motores | Alimenta motores tracción |
| IN4 | **6** | Izq | Reserva | Función configurable |

**Conexión módulo:**

```
ESP32-S3                    Módulo 4 Relés             Carga
────────                    ──────────────             ─────
5V      ────────────────► VCC
GND     ────────────────► GND
GPIO 2  ────────────────► IN1  ──► COM/NO ──► Power Hold
GPIO 4  ────────────────► IN2  ──► COM/NO ──► 12V Auxiliares
GPIO 5  ────────────────► IN3  ──► COM/NO ──► 24V Motores
GPIO 6  ────────────────► IN4  ──► COM/NO ──► Reserva
```

**NOTA:** GPIO 32 y 33 NO disponibles físicamente - funciones consolidadas en GPIO 6 (RELAY_SPARE)
```

---

## 4. 🚗 MOTORES BTS7960 (43A)

Cada rueda tiene un controlador BTS7960 dedicado.

### Motor Frontal Izquierdo (FL)

| BTS7960 FL Pin | ESP32-S3 GPIO | Lado | Función |
|----------------|---------------|------|---------|
| RPWM | **1** | Izq | PWM Adelante |
| LPWM | - | - | GND (no usar marcha atrás) |
| R_EN | **3** | Izq | Enable Derecha |
| L_EN | **6** | Izq | Enable Izquierda |
| R_IS | - | - | No conectar |
| L_IS | - | - | No conectar |
| VCC | 5V | Izq | Lógica 5V |
| GND | GND | Ambos | Tierra común |
| **Potencia** |
| B+ | Motor FL+ | - | Salida al motor |
| B- | Motor FL- | - | Salida al motor |
| VCC Motor | 24V Relay #1 | - | Alimentación desde HY-M158 #1 |
| GND Motor | GND 24V | - | Tierra potencia |

### Motor Frontal Derecho (FR)

| BTS7960 FR Pin | ESP32-S3 GPIO | Lado | Función |
|----------------|---------------|------|---------|
| RPWM | **7** | Izq | PWM Adelante |
| LPWM | - | - | GND |
| R_EN | **8** | Izq | Enable Derecha |
| L_EN | **9** | Izq | Enable Izquierda |
| VCC | 5V | Izq | Lógica 5V |
| GND | GND | Ambos | Tierra común |

### Motor Trasero Izquierdo (RL)

| BTS7960 RL Pin | ESP32-S3 GPIO | Lado | Función |
|----------------|---------------|------|---------|
| RPWM | **10** | Izq | PWM Adelante |
| LPWM | - | - | GND |
| R_EN | **47** | Der | Enable Derecha |
| L_EN | **48** | Der | Enable Izquierda |
| VCC | 5V | Der | Lógica 5V |
| GND | GND | Ambos | Tierra común |

### Motor Trasero Derecho (RR)

| BTS7960 RR Pin | ESP32-S3 GPIO | Lado | Función |
|----------------|---------------|------|---------|
| RPWM | **24** | Der | PWM Adelante |
| LPWM | - | - | GND |
| R_EN | **28** | Der | Enable Derecha |
| L_EN | **29** | Der | Enable Izquierda |
| VCC | 5V | Der | Lógica 5V |
| GND | GND | Ambos | Tierra común |

**Diagrama conexión potencia:**

```
HY-M158 #1 (24V) ──────┬─► BTS7960 FL VCC Motor
                       ├─► BTS7960 FR VCC Motor
                       ├─► BTS7960 RL VCC Motor
                       └─► BTS7960 RR VCC Motor

ESP32 5V ──────────────┬─► BTS7960 FL VCC Logic
                       ├─► BTS7960 FR VCC Logic
                       ├─► BTS7960 RL VCC Logic
                       └─► BTS7960 RR VCC Logic

ESP32 GND (común) ◄────┴─► Todos los BTS7960 GND
```

---

## 5. 🎵 DFPLAYER MINI (AUDIO)

| DFPlayer Pin | Cable | ESP32-S3 GPIO | Lado | Función |
|--------------|-------|---------------|------|---------|
| VCC | Rojo | 5V (directo) | - | 5V desde buck |
| GND | Negro | GND | Ambos | Tierra común |
| TX | Azul | **GPIO 43** | Izq | DFPlayer → ESP32 (RX) |
| RX | Verde | **GPIO 44** | Izq | ESP32 → DFPlayer (TX) |
| SPK1 | - | Altavoz+ | - | Salida audio + |
| SPK2 | - | Altavoz- | - | Salida audio - |
| BUSY | - | - | - | No conectar |
| IO1, IO2 | - | - | - | No conectar |

**Tarjeta microSD:**
```
SD Card/
└── mp3/
    ├── 0001.mp3  (Bienvenido Marcos)
    ├── 0002.mp3  (Hasta pronto)
    ├── 0013.mp3  (Batería crítica)
    ├── 0036.mp3  (Módulo OK)
    └── ... (hasta 0038.mp3)
```

**⚠️ IMPORTANTE:**
- Conecta resistencia 1kΩ en serie con RX del DFPlayer
- Formatea la SD en FAT32
- Archivos deben llamarse 0001.mp3, 0002.mp3, etc.
- Máximo 255 archivos

---

## 6. 🎯 SENSORES DE RUEDA LJ12A3-4-Z/BX

Sensores inductivos para detectar velocidad (6 tornillos/rueda).

### Especificaciones LJ12A3-4-Z/BX
- Tipo: NPN salida abierta
- Voltaje: 6-36V DC
- Distancia detección: 4mm
- Corriente: 300mA
- Salida: Activa en LOW cuando detecta metal

### Conexiones

| Rueda | Sensor | Cable | ESP32-S3 GPIO | Lado | Tornillos |
|-------|--------|-------|---------------|------|-----------|
| FL | LJ12A3 | Marrón | 5V | - | Alimentación + |
| FL | LJ12A3 | Azul | GND | - | Tierra |
| FL | LJ12A3 | Negro | **GPIO 35** | Der | Señal (6 pulsos/rev) |
| FR | LJ12A3 | Marrón | 5V | - | Alimentación + |
| FR | LJ12A3 | Azul | GND | - | Tierra |
| FR | LJ12A3 | Negro | **GPIO 36** | Der | Señal (6 pulsos/rev) |
| RL | LJ12A3 | Marrón | 5V | - | Alimentación + |
| RL | LJ12A3 | Azul | GND | - | Tierra |
| RL | LJ12A3 | Negro | **GPIO 30** | Der | Señal (6 pulsos/rev) |
| RR | LJ12A3 | Marrón | 5V | - | Alimentación + |
| RR | LJ12A3 | Azul | GND | - | Tierra |
| RR | LJ12A3 | Negro | **GPIO 31** | Der | Señal (6 pulsos/rev) |

**Configuración tornillos en rueda:**

```
        Rueda (vista frontal)
        
           12 o'clock
              ●
              
    9h  ●           ● 3h
    
    
    6h  ●           ● 9h
    
          ●
        6 o'clock

6 tornillos equidistantes (60° entre ellos)
Sensor detecta cada tornillo → 6 pulsos por revolución completa
```

**⚠️ CONVERTIDOR DE NIVEL REQUERIDO:**

Los sensores LJ12A3 funcionan a 5V, el ESP32-S3 trabaja a 3.3V:

```
Sensor LJ12A3 (5V) ──► [Level Shifter HV] ──► [LV] ──► ESP32-S3 (3.3V)
                       TXS0104E / 74LVC245
                       VCC HV = 5V
                       VCC LV = 3.3V
                       GND común
```

---

## 7. 🎯 ENCODER DIRECCIÓN E6B2-CWZ6C (1200PR)

Encoder incremental rotativo con señal de índice Z.

### Especificaciones E6B2-CWZ6C
- Tipo: Incremental cuadratura
- Resolución: 1200 pulsos/revolución
- Salida: Push-pull 5V
- Canales: A, B, Z
- Alimentación: 5-24V DC

### Conexiones Encoder

| Pin Encoder | Cable | Señal | ESP32-S3 GPIO | Lado | Función |
|-------------|-------|-------|---------------|------|---------|
| 1 | Marrón | +V | 5V (Relé GPIO 5) | - | Alimentación |
| 2 | Negro | 0V | GND | Ambos | Tierra |
| 3 | Naranja | A | **GPIO 37** | Der | Canal A (cuadratura) |
| 4 | Amarillo | B | **GPIO 38** | Der | Canal B (cuadratura) |
| 5 | Blanco | Z | **GPIO 25** | Der | Índice Z (1 pulso/rev) |
| Carcasa | Verde | GND | GND | - | Blindaje |

### Sensor de Centrado (Índice Z)

Un sensor LJ12A3-4-Z/BX detecta un tornillo en el centro del volante:

| Sensor Z | Cable | Señal | ESP32-S3 GPIO | Lado | Función |
|----------|-------|-------|---------------|------|---------|
| LJ12A3 | Marrón | +V | 5V | - | Alimentación |
| LJ12A3 | Azul | 0V | GND | Ambos | Tierra |
| LJ12A3 | Negro | OUT | **GPIO 25** | Der | Señal Z centrado |

**Montaje del tornillo de centrado:**

```
        Volante (vista frontal)
        
              TOP
               │
               │
        ───────┼───────  Eje de rotación
               │
               │
              ● ← Tornillo de centrado
              │   (detectado por LJ12A3 
              │    cuando volante está recto)
              
    El sensor LJ12A3 genera un pulso cuando detecta
    el tornillo → Señal Z → GPIO 25
```

**⚠️ CONVERTIDOR DE NIVEL REQUERIDO:**

El encoder E6B2 funciona a 5V:

```
Encoder (5V) ──► [Level Shifter HV] ──► [LV] ──► ESP32-S3 (3.3V)
Canal A (5V) ──►     GPIO 37
Canal B (5V) ──►     GPIO 38
Canal Z (5V) ──►     GPIO 25 (o LJ12A3 si usas sensor)
```

**Módulo convertidor recomendado:**
- TXS0104E o 74LVC245 (4 canales bidireccional)
- Conexión: HV=5V, LV=3.3V, GND común

---

## 8. 📊 SENSORES INA226 (BUS I2C)

Sensores de corriente/voltaje para monitorización de baterías.

### Bus I2C

| Pin I2C | Cable | ESP32-S3 GPIO | Lado | Función |
|---------|-------|---------------|------|---------|
| SDA | Verde | **GPIO 21** | Der | Datos I2C |
| SCL | Amarillo | **GPIO 20** | Der | Reloj I2C |
| VCC | Rojo | 3.3V | Izq | Alimentación |
| GND | Negro | GND | Ambos | Tierra |

### Conexión Múltiples INA226

Todos los sensores comparten el bus I2C. Cada uno tiene una dirección única:

```
ESP32-S3                    Bus I2C Compartido
────────                    ──────────────────
GPIO 21 (SDA) ─────┬──────► INA226 #1 (0x40) Batería 12V
                   │
                   ├──────► INA226 #2 (0x41) Batería 24V
                   │
                   ├──────► INA226 #3 (0x44) Motor FL
                   │
                   └──────► INA226 #4 (0x45) Motor FR

GPIO 20 (SCL) ─────┴──────► SCL (todos)

3.3V ──────────────────────► VCC (todos)
GND ───────────────────────► GND (todos)
```

**Configuración direcciones I2C:**

| Sensor | Dirección | A1 | A0 | Monitoriza |
|--------|-----------|----|----|------------|
| INA226 #1 | 0x40 | GND | GND | Batería 12V |
| INA226 #2 | 0x41 | GND | VS+ | Batería 24V |
| INA226 #3 | 0x44 | VS+ | GND | Motor FL |
| INA226 #4 | 0x45 | VS+ | VS+ | Motor FR |

**⚠️ NO necesitan resistencias pull-up externas** (ya incluidas en ESP32-S3).

---

## 9. 💡 LEDS WS2812B (INTELIGENTES)

**⚠️ PENDIENTE DE DEFINIR GPIO EN FIRMWARE**

El firmware incluye soporte para tiras LED WS2812B (estilo KITT), pero el GPIO aún no está definido en `pins.h`.

**Sugerencia:** Usar GPIO 23 (disponible) o GPIO 22 (no usado actualmente).

### Conexión LED Strip

| LED Strip | Cable | Conexión | Función |
|-----------|-------|----------|---------|
| VCC (5V) | Rojo | 5V alimentación externa | Alimentación LED |
| GND | Negro | GND común | Tierra |
| DIN | Verde | GPIO 23 (sugerido) | Señal de datos |

**⚠️ IMPORTANTE:**
- **NO alimentar** LEDs desde ESP32 (consume demasiado)
- Usar fuente externa 5V (mínimo 2A por cada 30 LEDs)
- Conectar GND de fuente LED con GND de ESP32
- Añadir capacitor 1000µF entre VCC/GND del strip
- Resistencia 330Ω en serie con DIN

**Esquema alimentación:**

```
Fuente 5V 10A ──────┬─► LED Strip VCC
                    │
ESP32-S3 GND ───────┼─► LED Strip GND (común)
                    │
ESP32 GPIO 23 ──────┴─► LED Strip DIN (con R=330Ω)
```

---

## 10. 🦶 PEDAL ACELERADOR

Entrada analógica ADC para leer posición del pedal.

| Pedal Pin | Cable | ESP32-S3 GPIO | Lado | Función |
|-----------|-------|---------------|------|---------|
| VCC | Rojo | 3.3V | Izq | Alimentación |
| GND | Negro | GND | Ambos | Tierra |
| SIG | Naranja | **GPIO 34** | Der | Señal analógica 0-3.3V |

**Calibración:**
- Pedal suelto: 0V (ADC = 0)
- Pedal a fondo: 3.3V (ADC = 4095)
- El firmware mapea 0-4095 → 0-100% aceleración

---

## 11. 🔘 BOTONES DE CONTROL

Botones con pull-up interno (activos en LOW).

| Botón | GPIO | Lado | Función | Pin Botón | ESP32 |
|-------|------|------|---------|-----------|-------|
| Luces | **40** | - | Toggle luces | Terminal 1 | GPIO 40 |
| | | | | Terminal 2 | GND |
| Media | **41** | - | Control audio | Terminal 1 | GPIO 41 |
| | | | | Terminal 2 | GND |
| 4x4 | **42** | - | Modo tracción | Terminal 1 | GPIO 42 |
| | | | | Terminal 2 | GND |
| Batería | **46** | Der | Info batería | Terminal 1 | GPIO 46 |
| | | | | Terminal 2 | GND |

**⚠️ NOTA:** Los GPIOs 40, 41, 42 **NO están en el pinout estándar** de ESP32-S3-DevKitC-1. Verifica si tu placa tiene pads de soldadura para estos pines o usa otros GPIOs disponibles.

**Alternativas si no están disponibles:**
- Usar GPIO 0, 1, 3 (ten cuidado con strapping pins)
- O reasignar en `pins.h`

---

## 12. 🌡️ SENSOR TEMPERATURA DS18B20

Sensor OneWire para temperatura de motores.

| DS18B20 | Cable | ESP32-S3 GPIO | Lado | Función |
|---------|-------|---------------|------|---------|
| VCC | Rojo | 3.3V | Izq | Alimentación |
| GND | Negro | GND | Ambos | Tierra |
| DATA | Amarillo | **GPIO 26** | Der | Señal OneWire |

**Resistencia pull-up:** 4.7kΩ entre DATA y VCC.

**Conexión múltiples sensores:**

```
3.3V ─────┬───── 4.7kΩ ─────┬─────► DS18B20 #1 DATA
          │                  │
ESP32     │                  ├─────► DS18B20 #2 DATA
GPIO 26 ──┴──────────────────┴─────► DS18B20 #n DATA

GND ──────────────────────────────► GND (todos)
```

---

## 13. 🔋 SISTEMA DE ALIMENTACIÓN

### Diagrama General

```
                    SISTEMA DE ALIMENTACIÓN
                    ═══════════════════════

┌──────────────┐                 ┌──────────────┐
│ Batería 12V  │                 │ Batería 24V  │
│  (Sistema)   │                 │  (Motores)   │
└──────┬───────┘                 └──────┬───────┘
       │                                │
       │                                │
       ▼                                ▼
  ┌────────────┐                  ┌────────────┐
  │ HY-M158 #2 │◄── GPIO 2        │ HY-M158 #1 │◄── GPIO 4
  │  (40A)     │                  │  (40A)     │
  └─────┬──────┘                  └─────┬──────┘
        │                               │
        ├──► Pantalla ILI9488            ├──► BTS7960 FL (24V)
        │                               ├──► BTS7960 FR (24V)
        ├──► DFPlayer Mini              ├──► BTS7960 RL (24V)
        │                               └──► BTS7960 RR (24V)
        ├──► LEDs WS2812B (GPIO 0, 1)
        │
        └──► Relés (GPIO 2,4,5,6)
        
┌──────────────┐
│ Buck 5V 10A  │◄── Batería 12V
└──────┬───────┘
       │
       ├──► ESP32-S3 (5V → 3.3V interno)
       ├──► BTS7960 Logic (todos)
       ├──► Sensores LJ12A3 (5V)
       ├──► Encoder E6B2 (5V)
       └──► Módulo Relés 8ch

[Todos los GND conectados en común]
```

### Lista de Componentes de Alimentación

| Componente | Voltaje | Corriente | Fuente |
|------------|---------|-----------|--------|
| ESP32-S3 | 5V → 3.3V | 500mA | Buck converter 12V→5V |
| Pantalla ILI9488 | 3.3V | 200mA | ESP32 3.3V pin |
| DFPlayer Mini | 5V | 200mA | 5V directo desde buck |
| LEDs WS2812B | 5V | 60mA/LED | Fuente externa 5V 10A |
| Sensores LJ12A3 (x4) | 5V | 300mA c/u | Buck 12V→5V |
| Encoder E6B2 | 5V | 100mA | Relé GPIO 5 → 12V→5V |
| BTS7960 Logic (x4) | 5V | 50mA c/u | Buck 12V→5V |
| BTS7960 Motor (x4) | 24V | 43A c/u | Relé HY-M158 #1 |
| INA226 (x4) | 3.3V | 10mA c/u | ESP32 3.3V pin |
| DS18B20 | 3.3V | 5mA | ESP32 3.3V pin |

### Cálculo de Corrientes

**Bus 5V (desde Buck):**
- ESP32-S3: 500mA
- BTS7960 Logic (4x): 200mA
- Sensores LJ12A3 (4x): 1200mA
- Encoder: 100mA
- **TOTAL: ~2A** → Buck converter 5V/3A mínimo

**Bus 12V (Batería → HY-M158 #2):**
- Pantalla: 200mA
- DFPlayer: 200mA
- Relés secundarios: 100mA
- **TOTAL: ~500mA** → HY-M158 #2 (40A) suficiente

**Bus 24V (Batería → HY-M158 #1):**
- 4x Motores a máxima potencia: 172A
- **En uso normal: 20-40A** → HY-M158 #1 (40A) suficiente

---

## 14. ✅ CHECKLIST PRE-ENCENDIDO

### 1️⃣ Verificación de Alimentación

- [ ] Buck converter 12V→5V conectado y funcionando
- [ ] HY-M158 #1 conectado a Batería 24V
- [ ] HY-M158 #2 conectado a Batería 12V
- [ ] Todos los GND conectados en común (⚠️ CRÍTICO)
- [ ] Fusibles instalados:
  - [ ] Batería 12V: Fusible 10A
  - [ ] Batería 24V: Fusible 50A
  - [ ] Bus 5V: Fusible 5A

### 2️⃣ Verificación de Conexiones

- [ ] Pantalla ILI9488 conectada (GPIO 11,14,15,18,19,27)
- [ ] Táctil XPT2046 conectado (GPIO 3,46 - SPI compartido con TFT)
- [ ] Relés HY-M158 conectados (GPIO 2,4)
- [ ] Relés SRD-05VDC conectados (GPIO 2,4,5,6)
- [ ] DFPlayer con tarjeta SD formateada (FAT32, GPIO 43,44)
- [ ] 4x Sensores LJ12A3 conectados (GPIO 30,31,35,36)
- [ ] Encoder E6B2 conectado (GPIO 25,37,38)
- [ ] 4x BTS7960 conectados (ver tabla motores)
- [ ] Sensores INA226 en bus I2C (GPIO 20,21)
- [ ] Pedal conectado (GPIO 34)
- [ ] DS18B20 conectado con pull-up 4.7kΩ (GPIO 26)

### 3️⃣ Convertidores de Nivel

- [ ] Level shifters instalados para:
  - [ ] Sensores LJ12A3 (5V → 3.3V)
  - [ ] Encoder E6B2 (5V → 3.3V)
- [ ] Configuración: HV=5V, LV=3.3V, GND común

### 4️⃣ Verificación de Software

- [ ] Firmware compilado sin errores
- [ ] `platformio.ini` configurado para `esp32-s3-devkitc`
- [ ] Archivos de audio (0001-0038.mp3) en SD del DFPlayer
- [ ] (Opcional) Imágenes/iconos subidos con `pio run -t uploadfs`

### 5️⃣ Test de Continuidad

Con multímetro, verificar:
- [ ] Continuidad GND (ESP32 ↔ Baterías ↔ BTS7960)
- [ ] Voltajes correctos:
  - [ ] ESP32 5V pin: 4.8-5.2V
  - [ ] ESP32 3.3V pin: 3.2-3.4V
  - [ ] Batería 12V: 11-14V
  - [ ] Batería 24V: 22-28V
- [ ] No hay cortocircuitos entre:
  - [ ] VCC ↔ GND
  - [ ] GPIO ↔ GND
  - [ ] 5V ↔ 3.3V

### 6️⃣ Secuencia de Primer Encendido

1. **Desconectar motores** de BTS7960 (solo en primer test)
2. Conectar batería 12V → Relé HY-M158 #2 OFF (GPIO 2 = LOW)
3. Conectar batería 24V → Relé HY-M158 #1 OFF (GPIO 4 = LOW)
4. Conectar ESP32-S3 por USB y flashear firmware:
   ```bash
   cd "firmware Coche Marcos"
   pio run -t upload
   ```
5. Abrir monitor serial: `pio device monitor -b 115200`
6. Verificar boot correcto:
   ```
   [INFO] ESP32-S3 Coche Inteligente v1.0
   [INFO] Inicializando sensores...
   [INFO] INA226 detectado: 0x40, 0x41
   [INFO] Pantalla ILI9488 OK
   [INFO] DFPlayer OK
   [OK] Sistema listo
   ```
7. Verificar audio "Bienvenido Marcos" (0001.mp3)
8. Verificar pantalla muestra interfaz
9. **Si todo OK:** Conectar motores y hacer test de movimiento

### 7️⃣ Test Funcional

- [ ] Audio de bienvenida suena
- [ ] Pantalla muestra interfaz
- [ ] Táctil responde correctamente
- [ ] Pedal lee valores 0-100%
- [ ] Botones funcionan
- [ ] Sensores de rueda detectan pulsos
- [ ] Encoder dirección lee posición
- [ ] INA226 lee voltajes/corrientes
- [ ] Temperatura DS18B20 lee valores

### 8️⃣ Test de Seguridad

- [ ] Relé HY-M158 #1 se activa solo con:
  - [ ] Batería > 20%
  - [ ] Pedal = 0%
  - [ ] Marcha = P
  - [ ] Freno parking = ON
- [ ] Relé se desactiva inmediatamente al cortar llave
- [ ] Audio "Hasta pronto" suena antes de apagar
- [ ] No hay calentamiento excesivo en:
  - [ ] BTS7960
  - [ ] HY-M158
  - [ ] Buck converter
  - [ ] Motores

---

## 📥 DESCARGA DEL FIRMWARE

### Opción 1: Desde GitHub (Recomendado)

```bash
# Descargar ZIP del repositorio
https://github.com/florinzgz/coche-2-inteligente/archive/refs/heads/copilot/revise-and-fix-functionality.zip

# O clonar con Git
git clone -b copilot/revise-and-fix-functionality https://github.com/florinzgz/coche-2-inteligente.git
cd coche-2-inteligente
cd "firmware Coche Marcos"
```

### Opción 2: Compilar y Flashear

```bash
# Instalar PlatformIO (si no lo tienes)
pip install platformio

# Ir al directorio del firmware
cd "firmware Coche Marcos"

# Compilar
pio run

# Flashear (conecta ESP32-S3 por USB)
pio run -t upload

# Monitor serial (opcional)
pio device monitor -b 115200
```

### Archivos Necesarios

| Archivo/Carpeta | Descripción |
|----------------|-------------|
| `platformio.ini` | Configuración del proyecto |
| `include/pins.h` | **Asignación de pines DevKitC-1** |
| `src/main.cpp` | Código principal |
| `src/**/*.cpp` | Módulos del sistema |
| `audio/mp3/*.mp3` | **38 archivos de audio** (copiar a SD) |
| `docs/CONEXIONES_HARDWARE_DEVKITC1.md` | **Este documento** |
| `docs/PIN_MAPPING_DEVKITC1.md` | Mapeo técnico de pines |

---

## 🆘 SOLUCIÓN DE PROBLEMAS

### Pantalla no enciende

- Verificar alimentación 3.3V en pin VCC
- Verificar GPIO 27 (DC) y GPIO 15 (CS) conectados
- Revisar continuidad en cables MOSI/SCK/MISO
- Probar conectar LED (BLK) directamente a 3.3V

### Táctil no responde

- Verificar GPIO 12 (T_CS) conectado
- T_IRQ (GPIO 13) es opcional, puede estar desconectado
- Calibrar táctil desde menú del firmware
- Verificar que comparte correctamente bus SPI con pantalla

### Audio no suena

- Verificar tarjeta SD formateada en FAT32
- Archivos deben llamarse 0001.mp3, 0002.mp3, etc.
- Verificar conexión RX/TX (están cruzadas: ESP-RX → DF-TX)
- Añadir resistencia 1kΩ en serie con RX de DFPlayer
- Alimentación directa desde buck 5V

### Motores no giran

- **PELIGRO:** Desconectar baterías antes de revisar
- Verificar relé HY-M158 #1 (GPIO 4) se activa
- Verificar condiciones de seguridad:
  - Batería > 20%
  - Pedal = 0% al arrancar
  - Marcha en P al arrancar
- Verificar PWM en GPIOs 1,7,10,24
- Verificar alimentación 24V en BTS7960
- Medir voltaje en salidas B+/B- de BTS7960

### Sensores de rueda no detectan

- Verificar alimentación 5V en sensores LJ12A3
- Verificar distancia sensor-tornillo: 2-4mm
- Usar convertidor de nivel 5V→3.3V
- Verificar con multímetro: salida sensor debe alternar 0-5V
- Conectar pull-up 10kΩ a 5V si es necesario

### Encoder dirección no lee

- Verificar alimentación 5V en encoder E6B2
- Usar convertidor de nivel 5V→3.3V obligatorio
- Verificar canales A (GPIO 37) y B (GPIO 38)
- Señal Z (GPIO 25) puede ser de E6B2 o LJ12A3 separado
- Verificar con osciloscopio: debe haber señales cuadradas

### I2C no detecta INA226

- Verificar direcciones I2C configuradas (A0, A1)
- Verificar GPIOs correctos: SDA=21, SCL=20
- **NO añadir** resistencias pull-up (ESP32 ya las tiene)
- Probar scanner I2C:
  ```cpp
  #include <Wire.h>
  Wire.begin(21, 20);  // SDA, SCL
  ```
- Verificar alimentación 3.3V en INA226

---

## 📞 SOPORTE

Para problemas o dudas:

1. **Documentación adicional:**
   - `docs/PIN_MAPPING_DEVKITC1.md` - Mapeo técnico completo
   - `docs/CONEXIONES_HARDWARE.md` - Guía original (desactualizada)
   - `ADVANCED_SAFETY_SYSTEMS.md` - Sistemas ABS/TCS/Regen
   - `WIFI_OTA_SETUP.md` - Configuración WiFi y OTA

2. **Repositorio GitHub:**
   - https://github.com/florinzgz/coche-2-inteligente

3. **Monitor Serial:**
   ```bash
   pio device monitor -b 115200
   ```
   Buscar mensajes `[ERROR]` o `[WARNING]`

---

## ✅ RESUMEN RÁPIDO

### Pines Críticos para Empezar

| Función | GPIOs | Prioridad |
|---------|-------|-----------|
| **Relés HY-M158** | 2, 4 | 🔴 CRÍTICO |
| **Pantalla** | 11,14,15,18,19,27 | 🔴 CRÍTICO |
| **Táctil** | 12,13 | 🟡 IMPORTANTE |
| **DFPlayer** | 16,17 | 🟡 IMPORTANTE |
| **Motores BTS7960** | 1,3,6,7,8,9,10,24,28,29,47,48 | 🔴 CRÍTICO |
| **Sensores Rueda** | 30,31,35,36 | 🔴 CRÍTICO |
| **Encoder** | 25,37,38 | 🔴 CRÍTICO |
| **I2C** | 20,21 | 🟡 IMPORTANTE |
| **Pedal** | 34 | 🔴 CRÍTICO |

### Convertidores de Nivel Necesarios

- **2x TXS0104E** (4 canales c/u):
  - Convertidor #1: Sensores rueda (4 canales)
  - Convertidor #2: Encoder A,B,Z + reserva (4 canales)

### Alimentación Mínima

- Buck 12V→5V: 3A
- Batería 12V: 10Ah mínimo
- Batería 24V: 20Ah mínimo
- Fuente LED externa: 5V 10A (si usas LEDs)

---

**🎉 ¡Firmware adaptado y listo para ESP32-S3-DevKitC-1!**

**Autor:** GitHub Copilot  
**Fecha:** 2025-11-05  
**Versión:** 1.0 - DevKitC-1 Compatible
