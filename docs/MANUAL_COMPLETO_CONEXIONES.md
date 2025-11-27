# 📘 MANUAL COMPLETO DE CONEXIONES
## ESP32-S3 Car Control System - Coche Inteligente Marcos

**Versión Firmware:** 2.8.0  
**Placa:** ESP32-S3-DevKitC-1 (44 pines)  
**Fecha:** 2025-11-27

---

## 📋 ÍNDICE

1. [Introducción y Lista de Materiales](#1-introducción-y-lista-de-materiales)
2. [Módulo 1: ESP32-S3-DevKitC-1](#2-módulo-1-esp32-s3-devkitc-1-placa-principal)
3. [Módulo 2: Bus I²C](#3-módulo-2-bus-i2c---comunicaciones)
4. [Módulo 3: Pantalla ST7796S + Táctil](#4-módulo-3-pantalla-st7796s--táctil-xpt2046)
5. [Módulo 4: Motores de Tracción](#5-módulo-4-motores-de-tracción-4x-bts7960)
6. [Módulo 5: Motor de Dirección](#6-módulo-5-motor-de-dirección-rs390--bts7960)
7. [Módulo 6: Sensores de Ruedas](#7-módulo-6-sensores-de-ruedas-4x-lj12a3-4-zbx)
8. [Módulo 7: Encoder de Dirección](#8-módulo-7-encoder-de-dirección-e6b2-cwz6c)
9. [Módulo 8: Sensores de Corriente](#9-módulo-8-sensores-de-corriente-6x-ina226)
10. [Módulo 9: Sensores de Temperatura](#10-módulo-9-sensores-de-temperatura-4x-ds18b20)
11. [Módulo 10: Pedal Acelerador](#11-módulo-10-pedal-acelerador-a1324lua-t)
12. [Módulo 11: Palanca de Cambios](#12-módulo-11-palanca-de-cambios-shifter)
13. [Módulo 12: Iluminación LED](#13-módulo-12-iluminación-led-ws2812b)
14. [Módulo 13: Audio DFPlayer](#14-módulo-13-audio-dfplayer-mini)
15. [Módulo 14: Relés de Potencia](#15-módulo-14-relés-de-potencia)
16. [Módulo 15: Botones de Control](#16-módulo-15-botones-de-control)
17. [Módulo 16: Optoacopladores HY-M158](#17-módulo-16-optoacopladores-hy-m158)
18. [Alimentación del Sistema](#18-alimentación-del-sistema)
19. [Checklist de Verificación](#19-checklist-de-verificación)

---

## 1. INTRODUCCIÓN Y LISTA DE MATERIALES

### 1.1 Descripción del Sistema

Este manual detalla **cada conexión cable por cable** del sistema de control del coche eléctrico inteligente Marcos. Cada módulo está explicado con:
- ✅ Cables necesarios (color, calibre)
- ✅ Pines de origen y destino
- ✅ Diagrama de conexión visual
- ✅ Notas importantes de seguridad

### 1.2 Lista de Componentes

| Cantidad | Componente | Modelo | Función |
|----------|------------|--------|---------|
| 1 | Placa principal | ESP32-S3-DevKitC-1 (44 pines) | Controlador central |
| 1 | Pantalla | ST7796S 480x320 + XPT2046 | Display táctil |
| 4 | Driver motor | BTS7960 43A | Tracción ruedas |
| 1 | Driver motor | BTS7960 43A | Motor dirección |
| 4 | Sensor inductivo | LJ12A3-4-Z/BX | Velocidad ruedas |
| 1 | Encoder | E6B2-CWZ6C 1200PR | Posición dirección |
| 6 | Sensor corriente | INA226 | Monitorización |
| 1 | Multiplexor I²C | TCA9548A | Mux para INA226 |
| 3 | Driver PWM | PCA9685 | Control motores |
| 1 | Expansor GPIO | MCP23017 | GPIOs adicionales |
| 4 | Sensor temperatura | DS18B20 | Temp motores |
| 1 | Sensor Hall | A1324LUA-T | Pedal acelerador |
| 1 | Reproductor audio | DFPlayer Mini | Efectos sonido |
| 2 | Tira LED | WS2812B | Iluminación |
| 2 | Optoacoplador | HY-M158 (8 canales) | Aislamiento |
| 4 | Relé | SRD-05VDC-SL-C | Control potencia |

---

## 2. MÓDULO 1: ESP32-S3-DevKitC-1 (PLACA PRINCIPAL)

### 2.1 Identificación de Pines

La placa tiene **44 pines** en dos filas. Mirando desde arriba con el USB hacia ti:

```
              ┌─────────────────────────┐
              │       USB TYPE-C        │
              └─────────────────────────┘
              
   LADO 1 (Izquierda)              LADO 2 (Derecha)
   ─────────────────               ─────────────────
   GND  ●                                    ● GND
   GND  ●                                    ● 5V
   19   ●                                    ● 14
   20   ●                                    ● 13
   21   ●                                    ● 12
   47   ●                                    ● 11
   48   ●                                    ● 10
   45   ●                                    ● 9
   0    ●                                    ● 46
   35   ●                                    ● 3
   36   ●                                    ● 8
   37   ●                                    ● 18
   38   ●                                    ● 17
   39   ●                                    ● 16
   40   ●                                    ● 15
   41   ●                                    ● 7
   42   ●                                    ● 6
   2    ●                                    ● 5
   1    ●                                    ● 4
   44   ● (RX)                               ● RST
   43   ● (TX)                               ● 3V3
   GND  ●                                    ● 3V3
```

### 2.2 Resumen de Asignación de Pines

| GPIO | Función | Tipo | Cable Color Sugerido |
|------|---------|------|----------------------|
| 0 | KEY_SYSTEM | Input | Blanco |
| 1 | LED_FRONT | Output | Verde |
| 2 | BTN_LIGHTS | Input | Amarillo |
| 3 | WHEEL_FL | Input | Azul |
| 4 | RELAY_MAIN | Output | Rojo |
| 5 | RELAY_TRAC | Output | Naranja |
| 6 | RELAY_DIR | Output | Marrón |
| 7 | RELAY_SPARE | Output | Rosa |
| 8 | I2C_SDA | I/O | Azul (par trenzado) |
| 9 | I2C_SCL | I/O | Verde (par trenzado) |
| 10 | TFT_SCK | Output | Blanco |
| 11 | TFT_MOSI | Output | Gris |
| 12 | TFT_MISO | Input | Violeta |
| 13 | TFT_DC | Output | Amarillo |
| 14 | TFT_RST | Output | Naranja |
| 15 | WHEEL_RR | Input | Azul |
| 16 | TFT_CS | Output | Marrón |
| 17 | WHEEL_RL | Input | Verde |
| 20 | ONEWIRE | I/O | Amarillo |
| 21 | TOUCH_CS | Output | Rojo |
| 35 | PEDAL | Analog | Blanco |
| 36 | WHEEL_FR | Input | Azul |
| 37 | ENCODER_A | Input | Verde |
| 38 | ENCODER_B | Input | Azul |
| 39 | ENCODER_Z | Input | Amarillo |
| 40 | BTN_MEDIA | Input | Naranja |
| 41 | BTN_4X4 | Input | Rojo |
| 42 | TFT_BL | Output | Blanco |
| 43 | DFPLAYER_TX | Output | Verde |
| 44 | DFPLAYER_RX | Input | Azul |
| 47 | TOUCH_IRQ | Input | Violeta |
| 48 | LED_REAR | Output | Verde |

---

## 3. MÓDULO 2: BUS I²C - COMUNICACIONES

### 3.1 Descripción

El bus I²C conecta todos los dispositivos inteligentes. Usa **2 cables** (SDA y SCL) con resistencias pull-up.

### 3.2 Cables Necesarios

| Cable | Color | Calibre | Descripción |
|-------|-------|---------|-------------|
| SDA | Azul | 22 AWG | Datos I²C |
| SCL | Verde | 22 AWG | Reloj I²C |
| VCC | Rojo | 22 AWG | 3.3V |
| GND | Negro | 22 AWG | Tierra |

### 3.3 Conexiones Detalladas

```
ESP32-S3                    Dispositivos I²C
─────────                   ─────────────────
GPIO 8 (SDA) ──────┬────────► TCA9548A (SDA) @ 0x70
                   ├────────► PCA9685 #1 (SDA) @ 0x40
                   ├────────► PCA9685 #2 (SDA) @ 0x41
                   ├────────► PCA9685 #3 (SDA) @ 0x42
                   └────────► MCP23017 (SDA) @ 0x20

GPIO 9 (SCL) ──────┬────────► TCA9548A (SCL)
                   ├────────► PCA9685 #1 (SCL)
                   ├────────► PCA9685 #2 (SCL)
                   ├────────► PCA9685 #3 (SCL)
                   └────────► MCP23017 (SCL)

3.3V ──────────────┬────────► VCC (todos los dispositivos)
                   │
                  [4.7kΩ]──── SDA (Pull-up OBLIGATORIO)
                   │
                  [4.7kΩ]──── SCL (Pull-up OBLIGATORIO)

GND ───────────────┴────────► GND (todos los dispositivos)
```

### 3.4 Direcciones I²C del Sistema

| Dispositivo | Dirección | Función |
|-------------|-----------|---------|
| TCA9548A | 0x70 | Multiplexor para 6x INA226 |
| PCA9685 #1 | 0x40 | PWM motores eje delantero |
| PCA9685 #2 | 0x41 | PWM motores eje trasero |
| PCA9685 #3 | 0x42 | PWM motor dirección |
| MCP23017 | 0x20 | Expansor GPIO (motores + shifter) |

### 3.5 ⚠️ Notas Importantes

- **Pull-ups OBLIGATORIOS**: Añadir resistencias de **4.7kΩ** entre SDA-3.3V y SCL-3.3V
- **Longitud máxima**: Bus I²C < 1 metro para evitar interferencias
- **Par trenzado**: Usar cables SDA/SCL trenzados juntos para reducir ruido

---

## 4. MÓDULO 3: PANTALLA ST7796S + TÁCTIL XPT2046

### 4.1 Descripción

Pantalla TFT de 480x320 píxeles con controlador táctil resistivo integrado. Comunicación SPI.

### 4.2 Conexiones Pantalla ST7796S

| Cable # | Color | Pin Pantalla | Pin ESP32 | GPIO | Función |
|---------|-------|--------------|-----------|------|---------|
| 1 | Rojo | VCC | 3.3V | - | Alimentación |
| 2 | Negro | GND | GND | - | Tierra |
| 3 | Marrón | CS | GPIO 16 | 16 | Chip Select |
| 4 | Naranja | RST | GPIO 14 | 14 | Reset |
| 5 | Amarillo | DC | GPIO 13 | 13 | Data/Command |
| 6 | Gris | MOSI/SDI | GPIO 11 | 11 | Datos SPI out |
| 7 | Blanco | SCK | GPIO 10 | 10 | Reloj SPI |
| 8 | Blanco | LED | GPIO 42 | 42 | Backlight PWM |
| 9 | Violeta | MISO/SDO | GPIO 12 | 12 | Datos SPI in |

### 4.3 Conexiones Táctil XPT2046

| Cable # | Color | Pin Táctil | Pin ESP32 | GPIO | Función |
|---------|-------|------------|-----------|------|---------|
| 1 | - | T_CLK | GPIO 10 | 10 | Compartido con SCK |
| 2 | Rojo | T_CS | GPIO 21 | 21 | Chip Select Touch |
| 3 | - | T_DIN | GPIO 11 | 11 | Compartido con MOSI |
| 4 | - | T_DO | GPIO 12 | 12 | Compartido con MISO |
| 5 | Violeta | T_IRQ | GPIO 47 | 47 | Interrupción táctil |

### 4.4 Diagrama Visual de Conexión

```
       PANTALLA ST7796S                    ESP32-S3
┌─────────────────────────┐          ┌──────────────────┐
│  VCC  ●─────── Rojo ────┼──────────┤ 3.3V             │
│  GND  ●─────── Negro ───┼──────────┤ GND              │
│  CS   ●─────── Marrón ──┼──────────┤ GPIO 16          │
│  RST  ●─────── Naranja ─┼──────────┤ GPIO 14          │
│  DC   ●─────── Amarillo ┼──────────┤ GPIO 13          │
│  MOSI ●─────── Gris ────┼──────────┤ GPIO 11          │
│  SCK  ●─────── Blanco ──┼──────────┤ GPIO 10          │
│  LED  ●─────── Blanco ──┼──────────┤ GPIO 42          │
│  MISO ●─────── Violeta ─┼──────────┤ GPIO 12          │
│                         │          │                  │
│  T_CS ●─────── Rojo ────┼──────────┤ GPIO 21          │
│ T_IRQ ●─────── Violeta ─┼──────────┤ GPIO 47          │
└─────────────────────────┘          └──────────────────┘
```

### 4.5 ⚠️ Notas Importantes

- **Voltaje**: La pantalla funciona a **3.3V**. ¡NO conectar a 5V!
- **Backlight**: GPIO 42 controla brillo por PWM (0-255)
- **SPI compartido**: SCK, MOSI, MISO se comparten entre pantalla y táctil

---

## 5. MÓDULO 4: MOTORES DE TRACCIÓN (4x BTS7960)

### 5.1 Descripción

Cuatro drivers BTS7960 (43A) controlan los motores de tracción de las 4 ruedas independientemente.

### 5.2 Arquitectura del Control

Los motores se controlan vía **I²C** usando:
- **PCA9685**: Genera señales PWM para velocidad
- **MCP23017**: Controla señales IN1/IN2 para dirección

### 5.3 Conexiones PCA9685 #1 → BTS7960 (Eje Delantero)

**PCA9685 #1** @ dirección I²C **0x40**

| Canal | Color Cable | Pin BTS7960 | Motor | Función |
|-------|-------------|-------------|-------|---------|
| CH0 | Azul | RPWM | FL (Frontal Izq) | Forward PWM |
| CH1 | Verde | LPWM | FL | Reverse PWM |
| CH2 | Azul | RPWM | FR (Frontal Der) | Forward PWM |
| CH3 | Verde | LPWM | FR | Reverse PWM |

```
       PCA9685 #1 (0x40)                BTS7960 FL
┌─────────────────────────┐      ┌──────────────────┐
│  CH0  ●─────── Azul ────┼──────┤ RPWM (Forward)   │
│  CH1  ●─────── Verde ───┼──────┤ LPWM (Reverse)   │
│  VCC  ●─────── Rojo ────┼──────┤ VCC (5V)         │
│  GND  ●─────── Negro ───┼──────┤ GND              │
└─────────────────────────┘      └──────────────────┘

                                       BTS7960 FR
                                 ┌──────────────────┐
│  CH2  ●─────── Azul ────┼──────┤ RPWM (Forward)   │
│  CH3  ●─────── Verde ───┼──────┤ LPWM (Reverse)   │
└─────────────────────────┘      └──────────────────┘
```

### 5.4 Conexiones PCA9685 #2 → BTS7960 (Eje Trasero)

**PCA9685 #2** @ dirección I²C **0x41**

| Canal | Color Cable | Pin BTS7960 | Motor | Función |
|-------|-------------|-------------|-------|---------|
| CH0 | Azul | RPWM | RL (Trasera Izq) | Forward PWM |
| CH1 | Verde | LPWM | RL | Reverse PWM |
| CH2 | Azul | RPWM | RR (Trasera Der) | Forward PWM |
| CH3 | Verde | LPWM | RR | Reverse PWM |

### 5.5 Conexiones MCP23017 → BTS7960 (Control Dirección)

**MCP23017** @ dirección I²C **0x20** - Banco GPIOA

| Pin MCP | Color | Pin BTS7960 | Motor | Función |
|---------|-------|-------------|-------|---------|
| GPIOA0 | Amarillo | R_EN | FL | Enable derecha |
| GPIOA1 | Naranja | L_EN | FL | Enable izquierda |
| GPIOA2 | Amarillo | R_EN | FR | Enable derecha |
| GPIOA3 | Naranja | L_EN | FR | Enable izquierda |
| GPIOA4 | Amarillo | R_EN | RL | Enable derecha |
| GPIOA5 | Naranja | L_EN | RL | Enable izquierda |
| GPIOA6 | Amarillo | R_EN | RR | Enable derecha |
| GPIOA7 | Naranja | L_EN | RR | Enable izquierda |

### 5.6 Conexiones de Potencia BTS7960

Cada BTS7960 necesita alimentación de **24V** para los motores:

```
       BATERÍA 24V                    BTS7960
┌─────────────────────────┐    ┌──────────────────┐
│  +24V ●─── Rojo grueso ─┼────┤ B+ (Potencia +)  │
│  GND  ●─── Negro grueso ┼────┤ B- (Potencia -)  │
└─────────────────────────┘    └──────────────────┘

       BTS7960                        MOTOR
┌─────────────────────────┐    ┌──────────────────┐
│  M+   ●─── Rojo ────────┼────┤ Terminal +       │
│  M-   ●─── Negro ───────┼────┤ Terminal -       │
└─────────────────────────┘    └──────────────────┘
```

### 5.7 ⚠️ Notas Importantes

- **Calibre cables potencia**: Usar **12-14 AWG** para cables de batería
- **Disipador**: Cada BTS7960 **DEBE** tener disipador de calor
- **Fusibles**: Instalar fusible **50A** en línea +24V

---

## 6. MÓDULO 5: MOTOR DE DIRECCIÓN (RS390 + BTS7960)

### 6.1 Descripción

Motor RS390 12V con reductora 1:50 controlado por un BTS7960 dedicado.

### 6.2 Conexiones PCA9685 #3 (0x42) → BTS7960 Dirección

| Canal | Color Cable | Pin BTS7960 | Función |
|-------|-------------|-------------|---------|
| CH0 | Azul | RPWM | Girar Derecha |
| CH1 | Verde | LPWM | Girar Izquierda |

### 6.3 Diagrama de Conexión Completo

```
       PCA9685 #3 (0x42)           BTS7960 Dirección
┌─────────────────────────┐    ┌──────────────────┐
│  CH0  ●─── Azul ────────┼────┤ RPWM             │
│  CH1  ●─── Verde ───────┼────┤ LPWM             │
│  VCC  ●─── Rojo ────────┼────┤ VCC (5V)         │
│  GND  ●─── Negro ───────┼────┤ GND              │
└─────────────────────────┘    └──────────────────┘

       BATERÍA 12V             BTS7960 Dirección
┌─────────────────────────┐    ┌──────────────────┐
│  +12V ●─── Rojo ────────┼────┤ B+ (Potencia)    │
│  GND  ●─── Negro ───────┼────┤ B- (Potencia)    │
└─────────────────────────┘    └──────────────────┘

       BTS7960 Dirección          MOTOR RS390
┌─────────────────────────┐    ┌──────────────────┐
│  M+   ●─── Rojo ────────┼────┤ Motor +          │
│  M-   ●─── Negro ───────┼────┤ Motor -          │
└─────────────────────────┘    └──────────────────┘
```

### 6.4 ⚠️ Notas Importantes

- **Voltaje**: Motor de dirección usa **12V**, ¡NO 24V!
- **Reductora**: Ratio 1:50 amplifica torque significativamente

---

## 7. MÓDULO 6: SENSORES DE RUEDAS (4x LJ12A3-4-Z/BX)

### 7.1 Descripción

Sensores inductivos de proximidad que detectan 6 tornillos por rueda para calcular velocidad.

### 7.2 Cables del Sensor LJ12A3-4-Z/BX

| Cable | Color Original | Función |
|-------|----------------|---------|
| 1 | Marrón | VCC (+5V a +12V) |
| 2 | Azul | GND |
| 3 | Negro | Señal (NPN NO) |

### 7.3 Conexiones (vía HY-M158 Optoacoplador)

Los sensores funcionan a 5-12V. Se conectan vía optoacoplador **HY-M158** para proteger ESP32.

| Sensor | Entrada HY-M158 | Salida ESP32 | GPIO |
|--------|-----------------|--------------|------|
| WHEEL_FL | #1-CH1 | OUT1 | GPIO 3 |
| WHEEL_FR | #1-CH2 | OUT2 | GPIO 36 |
| WHEEL_RL | #1-CH3 | OUT3 | GPIO 17 |
| WHEEL_RR | #1-CH4 | OUT4 | GPIO 15 |

### 7.4 Diagrama de Conexión Detallado

```
      SENSOR FL                  HY-M158 #1              ESP32-S3
┌─────────────────┐         ┌──────────────┐        ┌──────────────┐
│ Marrón ●─ +5V ──┼─────────┤ VCC          │        │              │
│ Azul   ●─ GND ──┼─────────┤ GND ─────────┼────────┤ GND          │
│ Negro  ●────────┼─────────┤ IN1 → OUT1 ──┼────────┤ GPIO 3       │
└─────────────────┘         │              │        │              │
                            │              │        │              │
      SENSOR FR             │              │        │              │
┌─────────────────┐         │              │        │              │
│ Marrón ●─ +5V ──┼─────────┤              │        │              │
│ Azul   ●─ GND ──┼─────────┤              │        │              │
│ Negro  ●────────┼─────────┤ IN2 → OUT2 ──┼────────┤ GPIO 36      │
└─────────────────┘         │              │        │              │
                            │              │        │              │
      SENSOR RL             │              │        │              │
┌─────────────────┐         │              │        │              │
│ Marrón ●─ +5V ──┼─────────┤              │        │              │
│ Azul   ●─ GND ──┼─────────┤              │        │              │
│ Negro  ●────────┼─────────┤ IN3 → OUT3 ──┼────────┤ GPIO 17      │
└─────────────────┘         │              │        │              │
                            │              │        │              │
      SENSOR RR             │              │        │              │
┌─────────────────┐         │              │        │              │
│ Marrón ●─ +5V ──┼─────────┤              │        │              │
│ Azul   ●─ GND ──┼─────────┤              │        │              │
│ Negro  ●────────┼─────────┤ IN4 → OUT4 ──┼────────┤ GPIO 15      │
└─────────────────┘         └──────────────┘        └──────────────┘
```

### 7.5 ⚠️ Notas Importantes

- **Distancia detección**: Máximo **4mm** del tornillo
- **6 tornillos por rueda**: Distribuidos uniformemente (60° entre ellos)
- **Optoacoplador OBLIGATORIO**: Protege ESP32 de voltajes > 3.3V

---

## 8. MÓDULO 7: ENCODER DE DIRECCIÓN (E6B2-CWZ6C)

### 8.1 Descripción

Encoder incremental de **1200 pulsos/revolución** con 3 señales: A, B (cuadratura) y Z (centrado).

### 8.2 Cables del Encoder

| Cable # | Color Original | Función | Destino |
|---------|----------------|---------|---------|
| 1 | Marrón | +5V | Alimentación |
| 2 | Azul | GND | Tierra |
| 3 | Negro | Canal A | HY-M158 → GPIO 37 |
| 4 | Blanco | Canal B | HY-M158 → GPIO 38 |
| 5 | Naranja | Señal Z | HY-M158 → GPIO 39 |
| 6 | Blindaje | GND | Tierra (ruido) |

### 8.3 Diagrama de Conexión (vía HY-M158)

```
      ENCODER E6B2-CWZ6C          HY-M158 #1            ESP32-S3
┌─────────────────────────┐    ┌──────────────┐    ┌──────────────┐
│ Marrón  ●─── +5V ───────┼────┤ VCC          │    │              │
│ Azul    ●─── GND ───────┼────┤ GND ─────────┼────┤ GND          │
│ Negro   ●───────────────┼────┤ IN5 → OUT5 ──┼────┤ GPIO 37      │
│ Blanco  ●───────────────┼────┤ IN6 → OUT6 ──┼────┤ GPIO 38      │
│ Naranja ●───────────────┼────┤ IN7 → OUT7 ──┼────┤ GPIO 39      │
│ Blindaje●─── GND ───────┼────┤ GND          │    │              │
└─────────────────────────┘    └──────────────┘    └──────────────┘
```

### 8.4 ⚠️ Notas Importantes

- **Ratio 1:1**: El encoder está acoplado directamente al volante
- **Señal Z**: Detecta posición central (volante recto)
- **Cuadratura**: Canales A y B permiten detectar dirección de giro

---

## 9. MÓDULO 8: SENSORES DE CORRIENTE (6x INA226)

### 9.1 Descripción

6 sensores INA226 monitorizan corriente y voltaje en motores y batería. Todos conectados vía multiplexor TCA9548A.

### 9.2 Conexiones TCA9548A

```
       ESP32-S3                    TCA9548A (0x70)
┌─────────────────────────┐    ┌──────────────────┐
│ GPIO 8 (SDA) ●──────────┼────┤ SDA              │
│ GPIO 9 (SCL) ●──────────┼────┤ SCL              │
│ 3.3V ●──────────────────┼────┤ VCC              │
│ GND ●───────────────────┼────┤ GND              │
└─────────────────────────┘    └──────────────────┘
```

### 9.3 Asignación de Canales TCA9548A → INA226

| Canal TCA | INA226 | Aplicación | Shunt | Conexión |
|-----------|--------|------------|-------|----------|
| CH0 | #1 @ 0x40 | Motor FL | 50A 75mV | En serie con motor |
| CH1 | #2 @ 0x40 | Motor FR | 50A 75mV | En serie con motor |
| CH2 | #3 @ 0x40 | Motor RL | 50A 75mV | En serie con motor |
| CH3 | #4 @ 0x40 | Motor RR | 50A 75mV | En serie con motor |
| CH4 | #5 @ 0x40 | Batería 24V | 100A 75mV | En serie con batería |
| CH5 | #6 @ 0x40 | Motor Dir | 50A 75mV | En serie con motor |

### 9.4 Conexión por cada INA226

```
       TCA9548A CHx               INA226
┌─────────────────────────┐    ┌──────────────────┐
│ SDx (Canal X) ●─────────┼────┤ SDA              │
│ SCx (Canal X) ●─────────┼────┤ SCL              │
│ VCC ●───────────────────┼────┤ VCC (3.3V)       │
│ GND ●───────────────────┼────┤ GND              │
└─────────────────────────┘    └──────────────────┘

       SHUNT CG FL-2C              INA226
┌─────────────────────────┐    ┌──────────────────┐
│ IN+ (desde +24V) ●──────┼────┤ VIN+             │
│ IN- (hacia carga) ●─────┼────┤ VIN-             │
└─────────────────────────┘    └──────────────────┘
```

### 9.5 ⚠️ Notas Importantes

- **Shunt en SERIE**: El shunt se conecta EN SERIE con la carga
- **TCA9548A**: Seleccionar canal antes de leer cada INA226

---

## 10. MÓDULO 9: SENSORES DE TEMPERATURA (4x DS18B20)

### 10.1 Descripción

4 sensores DS18B20 miden temperatura de los motores de tracción, conectados en bus OneWire paralelo.

### 10.2 Cables por Sensor

| Cable | Color | Función |
|-------|-------|---------|
| 1 | Rojo | VCC (3.3V-5V) |
| 2 | Negro | GND |
| 3 | Amarillo | Data (OneWire) |

### 10.3 Diagrama de Conexión

```
       ESP32-S3                     DS18B20 (x4)
┌─────────────────────────┐    ┌──────────────────┐
│                         │    │ Sensor 1 (FL)    │
│ GPIO 20 ●───┬── Amarillo┼────┤ Data             │
│             │           │    └──────────────────┘
│             │           │    ┌──────────────────┐
│             ├── Amarillo┼────┤ Sensor 2 (FR)    │
│             │           │    │ Data             │
│             │           │    └──────────────────┘
│             │           │    ┌──────────────────┐
│             ├── Amarillo┼────┤ Sensor 3 (RL)    │
│             │           │    │ Data             │
│             │           │    └──────────────────┘
│             │           │    ┌──────────────────┐
│             └── Amarillo┼────┤ Sensor 4 (RR)    │
│                         │    │ Data             │
│                         │    └──────────────────┘
│                         │
│ 3.3V ●──────┬── Rojo ───┼────► VCC (todos)
│             │           │
│            [4.7kΩ]      │    ← Pull-up OBLIGATORIO
│             │           │
│             └───────────┼────► GPIO 20
│                         │
│ GND ●─────── Negro ─────┼────► GND (todos)
└─────────────────────────┘
```

### 10.4 ⚠️ Notas Importantes

- **Pull-up OBLIGATORIO**: Resistencia **4.7kΩ** entre Data y VCC
- **Bus paralelo**: Todos los sensores comparten la línea de datos
- **Ubicación**: Montar cerca de cada motor para medición precisa

---

## 11. MÓDULO 10: PEDAL ACELERADOR (A1324LUA-T)

### 11.1 Descripción

Sensor Hall analógico que detecta posición del pedal acelerador.

### 11.2 Cables

| Cable | Color | Función | Destino |
|-------|-------|---------|---------|
| 1 | Rojo | VCC | +5V |
| 2 | Negro | GND | GND |
| 3 | Blanco | Señal | GPIO 35 (vía divisor) |

### 11.3 Conexión con Divisor de Tensión

El sensor entrega 0-5V, pero ESP32 acepta máximo 3.3V. Se necesita **divisor resistivo**.

```
      SENSOR A1324LUA-T         DIVISOR TENSIÓN         ESP32-S3
┌─────────────────────────┐  ┌───────────────────┐  ┌──────────────┐
│ VCC (Rojo) ●────────────┼──┤ → +5V             │  │              │
│ GND (Negro) ●───────────┼──┤ → GND ────────────┼──┤ GND          │
│                         │  │                   │  │              │
│ Señal (Blanco) ●────────┼──┤ → R1 (2.7kΩ) ──┬──┼──┤ GPIO 35      │
└─────────────────────────┘  │                │  │  │              │
                             │             [4.7kΩ] R2│              │
                             │                │  │  │              │
                             │              GND  │  │              │
                             └───────────────────┘  └──────────────┘

Cálculo: Vout = 5V × (4.7 / (2.7 + 4.7)) = 3.18V máximo ✅
```

### 11.4 ⚠️ Notas Importantes

- **Divisor OBLIGATORIO**: Sin él, se daña GPIO 35
- **Calibración software**: Ajustar valores min/max en código

---

## 12. MÓDULO 11: PALANCA DE CAMBIOS (SHIFTER)

### 12.1 Descripción

Palanca con 5 posiciones (P, R, N, D1, D2) conectada vía MCP23017.

### 12.2 Asignación de Posiciones

| Posición | Función | Pin MCP23017 | Color Cable |
|----------|---------|--------------|-------------|
| P | Park | GPIOB0 (pin 8) | Rojo |
| R | Reverse | GPIOB1 (pin 9) | Blanco |
| N | Neutral | GPIOB2 (pin 10) | Verde |
| D1 | Drive 1 (lento) | GPIOB3 (pin 11) | Azul |
| D2 | Drive 2 (rápido) | GPIOB4 (pin 12) | Amarillo |

### 12.3 Diagrama de Conexión (vía HY-M158 #2)

```
     PALANCA SHIFTER (12V)       HY-M158 #2            MCP23017
┌─────────────────────────┐  ┌──────────────┐    ┌──────────────┐
│ P  ●──── Rojo ──────────┼──┤ IN1 → OUT1 ──┼────┤ GPIOB0       │
│ R  ●──── Blanco ────────┼──┤ IN2 → OUT2 ──┼────┤ GPIOB1       │
│ N  ●──── Verde ─────────┼──┤ IN3 → OUT3 ──┼────┤ GPIOB2       │
│ D1 ●──── Azul ──────────┼──┤ IN4 → OUT4 ──┼────┤ GPIOB3       │
│ D2 ●──── Amarillo ──────┼──┤ IN5 → OUT5 ──┼────┤ GPIOB4       │
│                         │  │              │    │              │
│ Común ●──── +12V ───────┼──┤ VCC          │    │              │
└─────────────────────────┘  └──────────────┘    └──────────────┘
```

### 12.4 ⚠️ Notas Importantes

- **Lógica invertida**: Posición activa = LOW (por optoacoplador)
- **Pull-up**: MCP23017 tiene pull-up interno activado
- **Exclusiva**: Solo una posición activa a la vez

---

## 13. MÓDULO 12: ILUMINACIÓN LED (WS2812B)

### 13.1 Descripción

Dos tiras de LEDs WS2812B direccionables:
- **Frontales**: 28 LEDs (GPIO 1)
- **Traseros**: 16 LEDs (GPIO 48)

### 13.2 Cables por Tira

| Cable | Color | Función |
|-------|-------|---------|
| 1 | Rojo | VCC (+5V) |
| 2 | Negro | GND |
| 3 | Verde | Data (DIN) |

### 13.3 Diagrama de Conexión

```
      FUENTE 5V                  LEDs FRONTALES (28)      ESP32-S3
┌─────────────────────────┐    ┌──────────────────┐    ┌──────────────┐
│ +5V ●──── Rojo ─────────┼────┤ VCC              │    │              │
│ GND ●──── Negro ────────┼────┤ GND ─────────────┼────┤ GND          │
└─────────────────────────┘    │                  │    │              │
                               │ DIN ●── Verde ───┼────┤ GPIO 1       │
                               └──────────────────┘    │              │
                                                       │              │
      FUENTE 5V                  LEDs TRASEROS (16)     │              │
┌─────────────────────────┐    ┌──────────────────┐    │              │
│ +5V ●──── Rojo ─────────┼────┤ VCC              │    │              │
│ GND ●──── Negro ────────┼────┤ GND ─────────────┼────┤ GND          │
└─────────────────────────┘    │                  │    │              │
                               │ DIN ●── Verde ───┼────┤ GPIO 48      │
                               └──────────────────┘    └──────────────┘
```

### 13.4 ⚠️ Notas Importantes

- **Alimentación EXTERNA**: ¡NO alimentar LEDs desde ESP32! (usa mucha corriente)
- **Capacitor**: Añadir **1000µF** cerca de los LEDs para estabilidad
- **Resistencia**: **330Ω** en serie con DIN para protección

---

## 14. MÓDULO 13: AUDIO (DFPlayer Mini)

### 14.1 Descripción

Reproductor MP3 para alertas y efectos de sonido.

### 14.2 Conexiones

| Pin DFPlayer | Color Cable | Pin ESP32 | GPIO | Función |
|--------------|-------------|-----------|------|---------|
| VCC | Rojo | 5V | - | Alimentación |
| GND | Negro | GND | - | Tierra |
| RX | Azul | GPIO 43 | 43 | Recibe de ESP32 TX |
| TX | Verde | GPIO 44 | 44 | Transmite a ESP32 RX |
| SPK1 | Rojo | Altavoz | - | Altavoz + |
| SPK2 | Negro | Altavoz | - | Altavoz - |

### 14.3 Diagrama de Conexión

```
       ESP32-S3                    DFPlayer Mini          ALTAVOZ
┌─────────────────────────┐    ┌──────────────────┐    ┌──────────┐
│ 5V ●──── Rojo ──────────┼────┤ VCC              │    │          │
│ GND ●──── Negro ────────┼────┤ GND              │    │          │
│ GPIO 43 (TX) ●── Azul ──┼────┤ RX               │    │          │
│ GPIO 44 (RX) ●── Verde ─┼────┤ TX               │    │          │
└─────────────────────────┘    │                  │    │          │
                               │ SPK1 ●── Rojo ───┼────┤ +        │
                               │ SPK2 ●── Negro ──┼────┤ -        │
                               └──────────────────┘    └──────────┘
```

### 14.4 Estructura Tarjeta SD

```
SD Card (FAT32)/
└── mp3/
    ├── 0001.mp3  (Arranque sistema)
    ├── 0002.mp3  (Apagado sistema)
    ├── 0003.mp3  (Marcha D1)
    ├── 0004.mp3  (Marcha D2)
    ├── 0005.mp3  (Reversa)
    ├── 0006.mp3  (Neutral)
    ├── 0007.mp3  (Park)
    └── ... hasta 0038.mp3
```

### 14.5 ⚠️ Notas Importantes

- **Formato SD**: FAT32
- **TX/RX cruzados**: TX ESP32 → RX DFPlayer

---

## 15. MÓDULO 14: RELÉS DE POTENCIA

### 15.1 Descripción

4 relés SRD-05VDC-SL-C controlan potencia de diferentes subsistemas.

### 15.2 Asignación de Relés

| Relé | GPIO | Color Cable | Función | Carga |
|------|------|-------------|---------|-------|
| MAIN | 4 | Rojo | Power Hold | Sistema 5V |
| TRAC | 5 | Naranja | Tracción | Motores 24V |
| DIR | 6 | Marrón | Dirección | Motor 12V |
| SPARE | 7 | Rosa | Auxiliar | Luces/Audio |

### 15.3 Diagrama de Conexión

```
       ESP32-S3                    MÓDULO RELÉS (4ch)
┌─────────────────────────┐    ┌──────────────────┐
│ GPIO 4 ●──── Rojo ──────┼────┤ IN1 (MAIN)       │
│ GPIO 5 ●──── Naranja ───┼────┤ IN2 (TRAC)       │
│ GPIO 6 ●──── Marrón ────┼────┤ IN3 (DIR)        │
│ GPIO 7 ●──── Rosa ──────┼────┤ IN4 (SPARE)      │
│ 5V ●──────── Rojo ──────┼────┤ VCC              │
│ GND ●─────── Negro ─────┼────┤ GND              │
└─────────────────────────┘    └──────────────────┘
```

### 15.4 ⚠️ Notas Importantes

- **Lógica inversa**: LOW activa el relé
- **Diodo flyback**: Integrado en módulo
- **Corriente**: Máximo 10A por canal

---

## 16. MÓDULO 15: BOTONES DE CONTROL

### 16.1 Descripción

3 botones físicos para funciones de control.

### 16.2 Conexiones

| Botón | GPIO | Color Cable | Función |
|-------|------|-------------|---------|
| LIGHTS | 2 | Amarillo | Luces ON/OFF |
| MEDIA | 40 | Naranja | Multimedia |
| 4X4 | 41 | Rojo | Switch 4x4/4x2 |

### 16.3 Diagrama de Conexión

```
       BOTÓN                       ESP32-S3
┌─────────────────────────┐    ┌──────────────┐
│ Pin 1 ●─────────────────┼────┤ GPIO X       │
│ Pin 2 ●─────────────────┼────┤ GND          │
└─────────────────────────┘    └──────────────┘

(Pull-up interno activado en firmware)
```

---

## 17. MÓDULO 16: OPTOACOPLADORES HY-M158

### 17.1 Descripción

2 módulos HY-M158 (8 canales c/u) aíslan señales 5V/12V del ESP32 (3.3V).

### 17.2 HY-M158 Módulo #1 - Sensores y Encoder

| Canal | Entrada (5V/12V) | GPIO Salida | Función |
|-------|------------------|-------------|---------|
| CH1 | Sensor FL | GPIO 3 | Rueda frontal izq |
| CH2 | Sensor FR | GPIO 36 | Rueda frontal der |
| CH3 | Sensor RL | GPIO 17 | Rueda trasera izq |
| CH4 | Sensor RR | GPIO 15 | Rueda trasera der |
| CH5 | Encoder A | GPIO 37 | Canal A encoder |
| CH6 | Encoder B | GPIO 38 | Canal B encoder |
| CH7 | Encoder Z | GPIO 39 | Señal Z encoder |
| CH8 | - | - | Reserva |

### 17.3 HY-M158 Módulo #2 - Shifter

| Canal | Entrada (12V) | MCP23017 Pin | Función |
|-------|---------------|--------------|---------|
| CH1 | Shifter P | GPIOB0 | Park |
| CH2 | Shifter R | GPIOB1 | Reverse |
| CH3 | Shifter N | GPIOB2 | Neutral |
| CH4 | Shifter D1 | GPIOB3 | Drive 1 |
| CH5 | Shifter D2 | GPIOB4 | Drive 2 |
| CH6-8 | - | - | Reserva |

### 17.4 Conexiones HY-M158

```
     LADO ENTRADA (12V/5V)         HY-M158              LADO SALIDA (3.3V)
┌─────────────────────────┐    ┌──────────────┐    ┌──────────────────┐
│ VCC ●─── +12V o +5V ────┼────┤ VCC          │    │ VCC ●─── +3.3V   │
│ GND ●─── GND común ─────┼────┤ GND ─────────┼────┤ GND ●─── GND ESP │
│ IN1 ●─── Señal sensor ──┼────┤ Canal 1 ─────┼────┤ OUT1 ●── GPIO    │
│ IN2 ●─── Señal sensor ──┼────┤ Canal 2 ─────┼────┤ OUT2 ●── GPIO    │
│ ... │                   │    │ ...          │    │ ...              │
│ IN8 ●─── Señal sensor ──┼────┤ Canal 8 ─────┼────┤ OUT8 ●── GPIO    │
└─────────────────────────┘    └──────────────┘    └──────────────────┘
```

---

## 18. ALIMENTACIÓN DEL SISTEMA

### 18.1 Diagrama General

```
┌─────────────────────────────────────────────────────────────────────┐
│                        BATERÍA 24V                                  │
│                    (Motores Tracción)                               │
└────────────────────────────┬────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      RELÉ TRAC (GPIO 5)                             │
└────────────────────────────┬────────────────────────────────────────┘
                             │
            ┌────────────────┴────────────────┐
            ▼                                 ▼
    ┌───────────────┐                 ┌───────────────┐
    │   BTS7960 FL  │                 │   BTS7960 RL  │
    │   BTS7960 FR  │                 │   BTS7960 RR  │
    └───────────────┘                 └───────────────┘


┌─────────────────────────────────────────────────────────────────────┐
│                        BATERÍA 12V                                  │
│                    (Sistema y Dirección)                            │
└────────────────────────────┬────────────────────────────────────────┘
                             │
            ┌────────────────┼────────────────┐
            ▼                ▼                ▼
    ┌───────────────┐ ┌───────────────┐ ┌───────────────┐
    │ Convertidor   │ │  BTS7960 DIR  │ │   HY-M158     │
    │  12V → 5V     │ │    (12V)      │ │   (12V)       │
    └───────┬───────┘ └───────────────┘ └───────────────┘
            │
            ▼
    ┌───────────────────────────────────────────────────┐
    │                      5V                           │
    │  ESP32 | DFPlayer | WS2812B | PCA9685 | Sensores  │
    └───────────────────────────────────────────────────┘
```

### 18.2 Consumos Estimados

| Componente | Voltaje | Corriente | Potencia |
|------------|---------|-----------|----------|
| ESP32-S3 | 3.3V | 500mA | 1.65W |
| Pantalla | 3.3V | 150mA | 0.5W |
| 4x Motor Tracción | 24V | 43A c/u | 4.1kW |
| Motor Dirección | 12V | 10A | 120W |
| LEDs WS2812B (44) | 5V | 3A | 15W |
| DFPlayer | 5V | 500mA | 2.5W |
| Relés y lógica | 5V | 500mA | 2.5W |

---

## 19. CHECKLIST DE VERIFICACIÓN

### 19.1 Antes de Encender

#### Alimentación
- [ ] Batería 24V conectada correctamente (+/-)
- [ ] Batería 12V conectada correctamente (+/-)
- [ ] Convertidor 12V→5V funcionando
- [ ] Fusibles instalados (50A batería 24V, 30A batería 12V)

#### Bus I²C
- [ ] SDA (GPIO 8) conectado a todos los dispositivos
- [ ] SCL (GPIO 9) conectado a todos los dispositivos
- [ ] Resistencias pull-up 4.7kΩ instaladas

#### Pantalla
- [ ] Todos los pines SPI conectados
- [ ] VCC conectado a 3.3V (NO 5V)

#### Motores
- [ ] 4x BTS7960 tracción conectados a PCA9685 #1 y #2
- [ ] BTS7960 dirección conectado a PCA9685 #3
- [ ] MCP23017 conectado para control IN1/IN2

#### Sensores
- [ ] 4x Sensores ruedas conectados vía HY-M158
- [ ] Encoder E6B2-CWZ6C conectado vía HY-M158
- [ ] 4x DS18B20 en bus OneWire (GPIO 20)
- [ ] Pedal A1324LUA-T con divisor de tensión

#### Otros
- [ ] DFPlayer Mini conectado (GPIO 43/44)
- [ ] LEDs WS2812B con alimentación externa
- [ ] Relés conectados (GPIO 4-7)
- [ ] Shifter conectado vía HY-M158 → MCP23017

### 19.2 Primera Prueba

1. [ ] Conectar solo 5V (sin baterías de potencia)
2. [ ] Verificar LED de ESP32 encendido
3. [ ] Subir firmware: `pio run --target upload`
4. [ ] Abrir monitor: `pio device monitor`
5. [ ] Verificar mensajes de inicialización
6. [ ] Si todo OK, conectar baterías de potencia

---

## 📞 SOPORTE

**Documentos relacionados:**
- `REFERENCIA_HARDWARE.md` - Referencia técnica completa
- `PIN_MAPPING_DEVKITC1.md` - Mapeo oficial de GPIOs

**Código fuente:**
- `include/pins.h` - Definición de pines

---

**Versión:** 1.0  
**Fecha:** 2025-11-27  
**Firmware:** v2.8.0  
**Placa:** ESP32-S3-DevKitC-1 (44 pines)
