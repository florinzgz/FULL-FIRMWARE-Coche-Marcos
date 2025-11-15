# 🔌 GUÍA COMPLETA DE CONEXIONES HARDWARE - ESP32-S3 Electric Car Control System

**Sistema de Control Eléctrico Completo para ESP32-S3-DevKitC-1 (44 pines)**

Esta guía detalla todas las conexiones físicas necesarias para implementar el sistema de control completo del coche eléctrico.

---

## 📋 ÍNDICE

1. [ESP32-S3-DevKitC-1 Pinout](#1-esp32-s3-devkitc-1-pinout)
2. [Módulo 4 Relés SRD-05VDC-SL-C](#2-módulo-4-relés-srd-05vdc-sl-c)
3. [Sensores de Corriente INA226 + Multiplexor I²C](#3-sensores-de-corriente-ina226--multiplexor-i²c)
4. [Encoder Volante E6B2-CWZ6C](#4-encoder-volante-e6b2-cwz6c)
5. [LEDs WS2812B](#5-leds-ws2812b)
6. [Sensores de Rueda LJ12A3-4-Z/BX](#6-sensores-de-rueda-lj12a3-4-z-bx)
7. [Motor Dirección RS390 + BTS7960](#7-motor-dirección-rs390--bts7960)
8. [Motores Tracción + BTS7960](#8-motores-tracción--bts7960)
9. [Pantalla ILI9488 + Táctil XPT2046](#9-pantalla-ili9488--táctil-xpt2046)
10. [DFPlayer Mini](#10-dfplayer-mini)
11. [Pedal Acelerador Hall A1324LUA-T](#11-pedal-acelerador-hall-a1324lua-t)
12. [Optoacopladores HY-M158](#12-optoacopladores-hy-m158)
13. [Diagrama de Potencia General](#13-diagrama-de-potencia-general)
14. [Troubleshooting](#14-troubleshooting)

---

## 1. ESP32-S3-DevKitC-1 PINOUT

### Rango de GPIOs Válidos
- **GPIOs digitales**: 0-21, 35-48
- **GPIOs analógicos (ADC)**: 1-10
- **NO usar**: 22-34 (reservados/no disponibles en ESP32-S3)

### Asignación de Pines Completa

```
┌─────────────────────────────────────────┐
│      ESP32-S3-DevKitC-1 (44 pines)      │
├─────────────────────────────────────────┤
│                                         │
│  3.3V ──┐                    ┌── 5V    │
│  GND ───┤                    ├── GND   │
│  EN ────┤                    ├── GPIO21│
│  GPIO36 ┤ (Encoder A)        ├── GPIO19│
│  GPIO37 ┤ (Encoder B)        ├── GPIO20│
│  GPIO38 ┤ (Encoder Z)        ├── GPIO18│
│  GPIO39 ┤                    ├── GPIO17 (Shifter bit 4)
│  GPIO40 ┤ (Wheel RR)         ├── GPIO16 (Shifter bit 3)
│  GPIO41 ┤ (Wheel RL)         ├── GPIO15 (Shifter bit 2)
│  GPIO42 ┤                    ├── GPIO14 (Shifter bit 1)
│  GPIO1  ┤                    ├── GPIO13 (Shifter bit 0)
│  GPIO2  ┤ (Relay 1)          ├── GPIO12│
│  GPIO3  ┤ (Pedal ADC)        ├── GPIO11│
│  GPIO4  ┤ (Relay 2)          ├── GPIO10│
│  GPIO5  ┤ (Relay 3)          ├── GPIO9 (I2C SDA)
│  GPIO6  ┤ (Relay 4)          ├── GPIO8 (I2C SCL)
│  GPIO7  ┤                    ├── GPIO46│
│  GPIO35 ┤ (Wheel FL)         ├── GPIO45│
│  GPIO0  ┤ (Boot)             ├── GPIO48│
│  GND ───┤                    ├── GPIO47│
│  GPIO43 ┤ (DF TX)            ├── GPIO44 (DF RX)
│                                         │
└─────────────────────────────────────────┘
```

---

## 2. MÓDULO 4 RELÉS SRD-05VDC-SL-C

### ⚡ Configuración CRÍTICA: VCC vs JD-VCC

**IMPORTANTE:** Quitar el jumper entre VCC y JD-VCC para aislamiento correcto.

```
┌─────────────────────────────────────────────┐
│          Módulo 4 Relés                     │
├─────────────────────────────────────────────┤
│                                             │
│  [Relay 1] [Relay 2] [Relay 3] [Relay 4]  │
│     NO         NO         NO         NO     │
│     COM        COM        COM        COM    │
│     NC         NC         NC         NC     │
│                                             │
│  ┌────────────────────────────────────┐    │
│  │ VCC  IN1  IN2  IN3  IN4  JD-VCC   │    │
│  │ GND                        GND     │    │
│  └────────────────────────────────────┘    │
│         ▲                         ▲         │
│         │                         │         │
│         │ NO JUMPER AQUÍ         │        │
│         └─────────X───────────────┘         │
│                                             │
└─────────────────────────────────────────────┘
```

### Conexiones

| ESP32 Pin | Relé Pin | Función | Voltaje |
|-----------|----------|---------|---------|
| GPIO 2 | IN1 | Relay 1 - Power Hold | 3.3V (señal) |
| GPIO 4 | IN2 | Relay 2 - 12V Aux | 3.3V (señal) |
| GPIO 5 | IN3 | Relay 3 - 24V Traction | 3.3V (señal) |
| GPIO 6 | IN4 | Relay 4 - Spare | 3.3V (señal) |
| 3.3V | VCC | Lógica del módulo | 3.3V |
| 5V Buck | JD-VCC | Bobinas relés | 5V |
| GND | GND (ambos) | Tierra común | 0V |

### Conexión de Relés a Carga

```
Power Supply                Relé               Load
┌──────┐                   ┌────┐            ┌──────┐
│ 12V  ├───────────────────┤ COM├────────────┤ 12V  │
│ Buck │                   │    │            │ Load │
└──────┘                   │ NO │  ┌─────────┤ GND  │
                           └────┘  │         └──────┘
                            ▲      │
                            │      │
                       Activado ───┘
                       (GPIO LOW)
```

### Configuración del Firmware

```cpp
// Modo LOW trigger (relé activa con GPIO = LOW)
pinMode(PIN_RELAY_1, OUTPUT);
digitalWrite(PIN_RELAY_1, HIGH); // Relé OFF (safe state)

// Para activar relé:
digitalWrite(PIN_RELAY_1, LOW);  // Relé ON
```

### ⚠️ IMPORTANTE: Secuencia de Arranque

**Orden correcto:**
1. Relay 1 (Power Hold) - Primero (delay 100-5000ms)
2. Relay 2 (12V Aux) - Segundo (delay 100-3000ms)
3. Relay 3 (24V Traction) - Último (delay 500-5000ms)

---

## 3. SENSORES DE CORRIENTE INA226 + MULTIPLEXOR I²C

### Configuración TCA9548A (Multiplexor I²C)

```
┌─────────────────────────────────────────┐
│        TCA9548A I²C Multiplexer         │
│         (Dirección: 0x70)               │
├─────────────────────────────────────────┤
│                                         │
│  SDA ←───── GPIO 9 (ESP32)             │
│  SCL ←───── GPIO 8 (ESP32)             │
│  VCC ←───── 3.3V                       │
│  GND ←───── GND                        │
│  A0/A1/A2 ← GND (dirección 0x70)      │
│                                         │
│  ┌──────────────────────────────┐     │
│  │  SD0/SC0 → INA226 #0 (FL)   │     │
│  │  SD1/SC1 → INA226 #1 (FR)   │     │
│  │  SD2/SC2 → INA226 #2 (RL)   │     │
│  │  SD3/SC3 → INA226 #3 (RR)   │     │
│  │  SD4/SC4 → INA226 #4 (Batt) │     │
│  │  SD5/SC5 → INA226 #5 (Steer)│     │
│  └──────────────────────────────┘     │
│                                         │
└─────────────────────────────────────────┘
```

### Conexión de Cada INA226

```
       ┌──────────────┐
Motor  │   INA226     │  ESP32
+24V ──┤ VIN+    VCC  ├── 3.3V
  │    │             │
  │    │ Shunt       │
  └────┤ Resistor    │
Motor  │ VIN-    SDA  ├── Canal TCA9548A
GND ───┤        SCL  ├── Canal TCA9548A
       │        GND  ├── GND
       └──────────────┘
```

**IMPORTANTE: Resistencias Shunt Requeridas**

Cada INA226 necesita una resistencia shunt en serie con la línea de corriente:

| Sensor | Canal TCA | Monitoriza | Rango | Shunt Resistor | Potencia Shunt |
|--------|-----------|-----------|-------|----------------|----------------|
| INA226 #0 | 0 | Motor FL | 0-50A | 2mΩ (0.002Ω) | 5W mínimo |
| INA226 #1 | 1 | Motor FR | 0-50A | 2mΩ (0.002Ω) | 5W mínimo |
| INA226 #2 | 2 | Motor RL | 0-50A | 2mΩ (0.002Ω) | 5W mínimo |
| INA226 #3 | 3 | Motor RR | 0-50A | 2mΩ (0.002Ω) | 5W mínimo |
| INA226 #4 | 4 | Battery Main | 0-100A | 1mΩ (0.001Ω) | 10W mínimo |
| INA226 #5 | 5 | Steering Motor | 0-20A | 5mΩ (0.005Ω) | 2W mínimo |

### Diagrama de Montaje Shunt

```
Motor Positivo (+24V)
      │
      ├───────► A INA226 VIN+
      │
   ┌──┴──┐
   │     │ Shunt Resistor (ej: 2mΩ, 5W)
   │     │ (resistencia muy baja, alta potencia)
   └──┬──┘
      │
      ├───────► A INA226 VIN-
      │
   Motor (-) / GND
```

### Cálculo Shunt Resistor

**Fórmula:** R_shunt = V_max / I_max

Donde:
- V_max = 81.92mV (máximo voltaje INA226)
- I_max = Corriente máxima esperada

**Ejemplo para motor 50A:**
- R_shunt = 0.08192V / 50A = 1.6mΩ
- Usar: 2mΩ (valor comercial cercano)
- Potencia: P = I² × R = 50² × 0.002 = 5W
- **Usar shunt de 5W mínimo, recomendado 10W**

### ⚠️ CRÍTICO: Especificaciones Shunt

1. **Tolerancia:** ±1% o mejor
2. **Tipo:** Resistencia de potencia de bajo valor
3. **Montaje:** Con disipador térmico si >3W
4. **Ejemplo de compra:** 
   - "2mΩ 10W Shunt Resistor"
   - FL-2 (2mΩ) common shunt
   - Manganina o aleación baja temperatura

### Configuración INA226 en Firmware

```cpp
// Configurar INA226 con shunt de 2mΩ
#define SHUNT_RESISTOR 0.002  // 2mΩ
#define MAX_CURRENT 50.0      // 50A

ina226.begin();
ina226.configure(INA226_AVERAGES_16, 
                 INA226_BUS_CONV_TIME_1100US,
                 INA226_SHUNT_CONV_TIME_1100US,
                 INA226_MODE_SHUNT_BUS_CONT);
ina226.calibrate(SHUNT_RESISTOR, MAX_CURRENT);
```

### Conexión I²C al ESP32

| ESP32 Pin | Función | Señal |
|-----------|---------|-------|
| GPIO 9 | I2C SDA | Datos bidireccional |
| GPIO 8 | I2C SCL | Clock |

### ⚠️ IMPORTANTE: Resistencias Pull-up

```
        3.3V
         │
        ┌┴┐ 4.7kΩ
        │ │
        └┬┘
         ├─────── SDA (GPIO 9)
         │
        ┌┴┐ 4.7kΩ
        │ │
        └┬┘
         └─────── SCL (GPIO 8)
```

---

## 4. ENCODER VOLANTE E6B2-CWZ6C

### Especificaciones
- Tipo: Incremental rotativo
- Resolución: 1200 pulsos/revolución
- Alimentación: 5-24V DC
- Salidas: A, B, Z (NPN open collector)

### Conexión

```
┌────────────────────────────┐
│   E6B2-CWZ6C Encoder       │
├────────────────────────────┤
│                            │
│  Cable Marrón   → +12V     │
│  Cable Azul     → GND      │
│  Cable Negro (A)→ GPIO 37  │
│  Cable Blanco(B)→ GPIO 38  │
│  Cable Naranja(Z)→ GPIO 46 │
│                            │
└────────────────────────────┘
```

### Diagrama de Señales

```
Encoder 12V                ESP32
┌──────┐                 ┌──────┐
│ +12V ├─────────────────┤ 12V  │
│ GND  ├─────────────────┤ GND  │
│      │                 │      │
│  A   ├─────┬───────────┤ 37   │
│      │     │ 10kΩ      │      │
│  B   ├─────┼───────────┤ 38   │
│      │     │ 10kΩ      │      │
│  Z   ├─────┴───────────┤ 46   │
│      │       10kΩ      │      │
└──────┘                 └──────┘
```

| Pin Encoder | Color | ESP32 GPIO | Función |
|-------------|-------|------------|---------|
| +V | Marrón | 12V | Alimentación |
| 0V | Azul | GND | Tierra |
| A | Negro | GPIO 37 | Fase A |
| B | Blanco | GPIO 38 | Fase B |
| Z | Naranja | GPIO 46 | Índice (1 pulso/rev) |

### ⚠️ IMPORTANTE: Resistencias Pull-up

Añadir resistencias de 10kΩ desde cada señal (A, B, Z) a 3.3V para asegurar niveles lógicos correctos.

---

## 5. LEDS WS2812B

### Conexión Directa (Opción Simple)

```
┌──────────────┐         ┌──────────────┐
│  Buck 5V     │         │  WS2812B     │
│  Output      │         │  LED Strip   │
├──────────────┤         ├──────────────┤
│              │         │              │
│  +5V ────────┼─────────┤ VCC          │
│              │         │              │
│  GND ────────┼─────────┤ GND          │
│              │         │              │
└──────────────┘         │              │
                         │              │
┌──────────────┐         │              │
│  ESP32-S3    │         │              │
├──────────────┤         │              │
│              │         │              │
│  GPIO X ─────┼─────────┤ DIN          │
│              │         │              │
│  GND ────────┼─────────┤ GND          │
│              │         │              │
└──────────────┘         └──────────────┘
```

### Conexión con Level Shifter (Opción Recomendada)

```
┌────────┐    ┌──────────┐    ┌──────────┐
│ESP32-S3│    │74HCT245  │    │ WS2812B  │
│        │    │Level     │    │          │
│GPIO X ─┼────┤A1     B1 ├────┤ DIN      │
│ 3.3V ──┼────┤VCC_A     │    │          │
│        │    │          │    │          │
│        │    │  VCC_B ──┼────┤ VCC (5V) │
│        │    │          │    │          │
│ GND ───┼────┤GND       ├────┤ GND      │
└────────┘    └──────────┘    └──────────┘
```

### Configuración

| Componente | Pin | Conexión |
|------------|-----|----------|
| ESP32 | GPIO (definido en pins.h) | Level shifter input |
| Level Shifter | Output | DIN del primer LED |
| Buck 5V | +5V | VCC de LEDs |
| Buck 5V | GND | GND común |

### ⚠️ IMPORTANTE: Capacitor de desacoplo

Añadir capacitor electrolítico 1000µF/16V entre VCC y GND de los LEDs, cerca del primer LED.

---

## 6. SENSORES DE RUEDA LJ12A3-4-Z/BX

### Especificaciones
- Tipo: Sensor inductivo de proximidad
- Voltaje: 6-36V DC
- Salida: NPN normalmente abierta
- Distancia detección: 4mm

### Conexión de Cada Sensor

```
┌──────────────────┐         ┌────────────┐
│  LJ12A3-4-Z/BX   │         │  ESP32-S3  │
├──────────────────┤         ├────────────┤
│                  │         │            │
│  Marrón (+12V)───┼─────────┤ 12V Aux    │
│                  │         │            │
│  Azul (GND)──────┼─────────┤ GND        │
│                  │         │            │
│  Negro (Signal)──┼────┬────┤ GPIO       │
│                  │    │    │            │
└──────────────────┘    │    └────────────┘
                       ┌┴┐
                       │ │ 10kΩ Pull-up
                       └┬┘
                        │
                       3.3V
```

| Sensor | Cable Color | ESP32 GPIO | Función |
|--------|-------------|------------|---------|
| FL (Front Left) | Marrón | 12V | Alimentación |
| | Azul | GND | Tierra |
| | Negro | GPIO 35 | Señal pulsos |
| FR (Front Right) | Marrón | 12V | Alimentación |
| | Azul | GND | Tierra |
| | Negro | GPIO 36 | Señal pulsos |
| RL (Rear Left) | Marrón | 12V | Alimentación |
| | Azul | GND | Tierra |
| | Negro | GPIO 41 | Señal pulsos |
| RR (Rear Right) | Marrón | 12V | Alimentación |
| | Azul | GND | Tierra |
| | Negro | GPIO 40 | Señal pulsos |

### Montaje Físico

```
     Rueda con Dientes
         ┌──┐
      ┌──┤  ├──┐
   ┌──┤  │  │  ├──┐
   │  └──┤  ├──┘  │
   │     └──┘     │
   └──────┬───────┘
          │
          │ 4mm máx
          ▼
      ┌───────┐
      │Sensor │
      │LJ12A3 │
      └───────┘
```

---

## 7. MOTOR DIRECCIÓN RS390 + BTS7960 + PCA9685

### Arquitectura del Sistema

**IMPORTANTE:** El sistema de dirección utiliza PCA9685 (controlador PWM I²C de 16 canales) para generar señales PWM de precisión para el BTS7960.

### Diagrama Completo de Conexión

```
┌────────────────┐      ┌──────────────┐      ┌──────────────┐      ┌─────────┐
│   ESP32-S3     │      │   PCA9685    │      │   BTS7960    │      │ RS390   │
│                │      │  PWM Driver  │      │   Driver     │      │ Motor   │
├────────────────┤      ├──────────────┤      ├──────────────┤      ├─────────┤
│                │      │              │      │              │      │         │
│  I2C SDA (9)───┼──────┤ SDA          │      │              │      │         │
│  I2C SCL (8)───┼──────┤ SCL    PWM0  ├──────┤ RPWM         │      │         │
│                │      │        PWM1  ├──────┤ LPWM    ROUT ├──────┤ +       │
│  EN_R (GPIO)───┼──────┼──────────────┼──────┤ R_EN    LOUT ├──────┤ -       │
│  EN_L (GPIO)───┼──────┼──────────────┼──────┤ L_EN         │      │         │
│                │      │              │      │              │      └─────────┘
│  3.3V ─────────┼──────┤ VCC          │      │              │
│  GND ──────────┼──────┤ GND          │      │              │
└────────────────┘      │         VCC  ├──────┤ VCC          │
                        │         GND  ├──────┤ GND          │      ┌─────────┐
                        └──────────────┘      │              │      │ 12V PSU │
                                              │  Vin ────────┼──────┤ +12V    │
                                              │  GND ────────┼──────┤ GND     │
                                              └──────────────┘      └─────────┘
```

### PCA9685 Configuración

| PCA9685 Pin | Conexión | Función |
|-------------|----------|---------|
| VCC | 3.3V | Alimentación lógica |
| GND | GND | Tierra |
| SDA | GPIO 9 | I²C Data |
| SCL | GPIO 8 | I²C Clock |
| PWM0 | BTS7960 RPWM | PWM derecha |
| PWM1 | BTS7960 LPWM | PWM izquierda |

**Dirección I²C PCA9685:** 0x40 (por defecto)

### Conexiones BTS7960

| ESP32/PCA9685 | BTS7960 Pin | Función |
|---------------|-------------|---------|
| PCA9685 PWM0 | RPWM | PWM sentido derecha (0-4095) |
| PCA9685 PWM1 | LPWM | PWM sentido izquierda (0-4095) |
| ESP32 GPIO | R_EN | Enable derecha (HIGH=activo) |
| ESP32 GPIO | L_EN | Enable izquierda (HIGH=activo) |
| 3.3V | VCC | Alimentación lógica |
| GND | GND | Tierra común |

| BTS7960 Pin | Conexión | Notas |
|-------------|----------|-------|
| Vin | +12V | Alimentación potencia |
| GND | GND power | Tierra potencia |
| ROUT | Motor + | Salida derecha |
| LOUT | Motor - | Salida izquierda |

### Control del Motor con PCA9685

```cpp
#include <Adafruit_PWMServoDriver.h>

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);

void setup() {
  pwm.begin();
  pwm.setPWMFreq(1000); // 1kHz para BTS7960
}

// Girar derecha
digitalWrite(EN_R, HIGH);
digitalWrite(EN_L, LOW);
pwm.setPWM(0, 0, speed); // Canal 0, valor 0-4095
pwm.setPWM(1, 0, 0);

// Girar izquierda  
digitalWrite(EN_R, LOW);
digitalWrite(EN_L, HIGH);
pwm.setPWM(0, 0, 0);
pwm.setPWM(1, 0, speed); // Canal 1, valor 0-4095

// Parar
digitalWrite(EN_R, LOW);
digitalWrite(EN_L, LOW);
pwm.setPWM(0, 0, 0);
pwm.setPWM(1, 0, 0);
```

### ⚠️ Ventajas PCA9685

- **16 canales PWM independientes** (permite controlar múltiples motores)
- **Resolución 12-bit** (0-4095 vs 0-255 del ESP32)
- **Control vía I²C** (libera GPIOs PWM del ESP32)
- **Frecuencia ajustable** 40Hz-1000Hz

---

## 8. MOTORES TRACCIÓN + BTS7960

### Configuración 4 Motores Independientes

Cada motor tiene su propio BTS7960. Configuración idéntica al motor de dirección pero replicada 4 veces.

```
Motor FL ← BTS7960 #1 ← ESP32 (GPIO_FL_PWM, GPIO_FL_EN)
Motor FR ← BTS7960 #2 ← ESP32 (GPIO_FR_PWM, GPIO_FR_EN)
Motor RL ← BTS7960 #3 ← ESP32 (GPIO_RL_PWM, GPIO_RL_EN)
Motor RR ← BTS7960 #4 ← ESP32 (GPIO_RR_PWM, GPIO_RR_EN)
```

### Alimentación

| Componente | Voltaje | Corriente Máx |
|------------|---------|---------------|
| Motor RS775 | 24V DC | 15A cada uno |
| BTS7960 Vin | 24V DC | 20A capacidad |
| BTS7960 VCC | 3.3V | 50mA |

### Distribución de Potencia

```
┌────────────┐
│  24V PSU   │
│  60A min   │
└─────┬──────┘
      │
      ├───────► BTS7960 #1 (FL) → Motor FL
      │
      ├───────► BTS7960 #2 (FR) → Motor FR
      │
      ├───────► BTS7960 #3 (RL) → Motor RL
      │
      └───────► BTS7960 #4 (RR) → Motor RR
```

### ⚠️ IMPORTANTE: Fusibles

Instalar fusible de 20A en cada línea de motor para protección individual.

---

## 9. PANTALLA ILI9488 + TÁCTIL XPT2046

### Conexión SPI

```
┌─────────────┐         ┌──────────────┐
│  ESP32-S3   │         │  ILI9488     │
├─────────────┤         ├──────────────┤
│             │         │              │
│  GPIO_MOSI ─┼─────────┤ MOSI/SDI     │
│  GPIO_MISO ─┼─────────┤ MISO/SDO     │
│  GPIO_SCK ──┼─────────┤ SCK          │
│  GPIO_CS ───┼─────────┤ CS           │
│  GPIO_DC ───┼─────────┤ DC/RS        │
│  GPIO_RST ──┼─────────┤ RESET        │
│             │         │              │
│  3.3V ──────┼─────────┤ VCC          │
│  3.3V ──────┼─────────┤ LED (backlit)│
│  GND ───────┼─────────┤ GND          │
│             │         │              │
└─────────────┘         └──────────────┘

Touch XPT2046 (mismo bus SPI)
┌─────────────┐         ┌──────────────┐
│  ESP32-S3   │         │  XPT2046     │
├─────────────┤         ├──────────────┤
│             │         │              │
│  GPIO_MOSI ─┼─────────┤ DIN          │
│  GPIO_MISO ─┼─────────┤ DOUT         │
│  GPIO_SCK ──┼─────────┤ CLK          │
│  GPIO_T_CS ─┼─────────┤ CS           │
│  GPIO_T_IRQ ┼─────────┤ IRQ          │
│             │         │              │
└─────────────┘         └──────────────┘
```

### Tabla de Pines

| Función | ESP32 GPIO | ILI9488 Pin | XPT2046 Pin |
|---------|------------|-------------|-------------|
| MOSI | GPIO (SPI) | SDI | DIN |
| MISO | GPIO (SPI) | SDO | DOUT |
| SCK | GPIO (SPI) | SCK | CLK |
| Display CS | GPIO | CS | - |
| Touch CS | GPIO | - | CS |
| DC/RS | GPIO | DC | - |
| Reset | GPIO | RST | - |
| Touch IRQ | GPIO | - | IRQ |
| VCC | 3.3V | VCC | VCC |
| GND | GND | GND | GND |
| Backlight | 3.3V | LED | - |

---

## 10. DFPLAYER MINI

### Conexión UART

```
┌──────────────┐         ┌──────────────┐
│  ESP32-S3    │         │  DFPlayer    │
├──────────────┤         ├──────────────┤
│              │         │              │
│  GPIO 43 ────┼─────────┤ RX           │
│  GPIO 44 ────┼─────────┤ TX           │
│              │         │              │
│  5V ─────────┼─────────┤ VCC          │
│  GND ────────┼─────────┤ GND          │
│              │         │              │
│              │         │ SPK+ ────┐   │
│              │         │ SPK- ────┤   │
│              │         │          │   │
└──────────────┘         └──────────┘   │
                                        │
                                   ┌────┴────┐
                                   │ Speaker │
                                   │  8Ω 3W  │
                                   └─────────┘
```

### Estructura de Tarjeta SD

```
SD Card (FAT32)
├── 01/
│   ├── 001.mp3  (Alert: Bluetooth override active)
│   ├── 002.mp3  (Alert: Bluetooth disconnected)
│   ├── 003.mp3  (Alert: Emergency stop)
│   └── 004.mp3  (Alert: System ready)
└── 02/
    ├── 001.mp3  (Music track 1)
    └── 002.mp3  (Music track 2)
```

| ESP32 Pin | DFPlayer Pin | Función |
|-----------|--------------|---------|
| GPIO 43 | RX | UART TX desde ESP32 |
| GPIO 44 | TX | UART RX al ESP32 |
| 5V | VCC | Alimentación |
| GND | GND | Tierra |

### ⚠️ IMPORTANTE

- Resistencia 1kΩ en serie con RX del DFPlayer
- Tarjeta SD formateada en FAT32
- Archivos MP3 nombrados según carpeta/número

---

## 11. PEDAL ACELERADOR HALL A1324LUA-T

### Sensor Hall Lineal

```
┌──────────────┐         ┌──────────────┐
│  ESP32-S3    │         │  A1324LUA-T  │
├──────────────┤         ├──────────────┤
│              │         │              │
│  GPIO 3 ─────┼─────────┤ VOUT         │
│  (ADC)       │         │              │
│              │         │              │
│  3.3V ───────┼─────────┤ VCC          │
│  GND ────────┼─────────┤ GND          │
│              │         │              │
└──────────────┘         └──────────────┘
```

### Conexión

| A1324 Pin | ESP32 Pin | Función |
|-----------|-----------|---------|
| VCC | 3.3V | Alimentación |
| GND | GND | Tierra |
| VOUT | GPIO 3 (ADC) | Salida analógica |

### Rango de Operación

- **Voltaje salida**: 0.5V - 3.0V (ratiométrico)
- **Posición 0%**: ~0.5V (pedal suelto)
- **Posición 100%**: ~3.0V (pedal pisado)
- **Resolución ADC**: 12 bits (0-4095)

### Calibración

```cpp
// Valores típicos
#define PEDAL_MIN 620   // ADC en 0.5V
#define PEDAL_MAX 3720  // ADC en 3.0V

// Lectura calibrada
int raw = analogRead(GPIO_PEDAL);
int percent = map(raw, PEDAL_MIN, PEDAL_MAX, 0, 100);
percent = constrain(percent, 0, 100);
```

---

## 12. OPTOACOPLADORES HY-M158

### Configuración para Shifter (Selector de Marchas)

El shifter proporciona 5 bits binarios para indicar la marcha:
- P (Park): 00000
- R (Reverse): 00001
- N (Neutral): 00010
- D1 (Drive 1): 00100
- D2 (Drive 2): 01000

### Conexión

```
┌──────────────────┐         ┌──────────────┐
│   Shifter 12V    │         │   HY-M158    │
│   (lado auto)    │         │ Optoacoplador│
├──────────────────┤         ├──────────────┤
│                  │         │              │
│  Bit 0 ──────────┼─────────┤ IN1     OUT1 ├───► GPIO 13
│  Bit 1 ──────────┼─────────┤ IN2     OUT2 ├───► GPIO 14
│  Bit 2 ──────────┼─────────┤ IN3     OUT3 ├───► GPIO 15
│  Bit 3 ──────────┼─────────┤ IN4     OUT4 ├───► GPIO 16
│  Bit 4 ──────────┼─────────┤ IN5     OUT5 ├───► GPIO 17
│                  │         │              │
│  +12V ───────────┼─────────┤ VCC_IN       │
│  GND ────────────┼─────────┤ GND_IN       │
│                  │         │              │
└──────────────────┘         │ VCC_OUT ─────┼───► 3.3V
                             │ GND_OUT ─────┼───► GND
                             │              │
                             └──────────────┘
```

### Tabla de Conexiones

| Shifter | HY-M158 IN | HY-M158 OUT | ESP32 GPIO |
|---------|------------|-------------|------------|
| Bit 0 | IN1 | OUT1 | GPIO 13 |
| Bit 1 | IN2 | OUT2 | GPIO 14 |
| Bit 2 | IN3 | OUT3 | GPIO 15 |
| Bit 3 | IN4 | OUT4 | GPIO 16 |
| Bit 4 | IN5 | OUT5 | GPIO 17 |

### Alimentación

| Pin | Conexión | Voltaje |
|-----|----------|---------|
| VCC_IN | Shifter +12V | 12V |
| GND_IN | Shifter GND | 0V |
| VCC_OUT | ESP32 3.3V | 3.3V |
| GND_OUT | ESP32 GND | 0V |

### Decodificación de Marchas

```cpp
// Lectura de 5 bits
uint8_t gear_bits = 0;
gear_bits |= digitalRead(GPIO_13) << 0;
gear_bits |= digitalRead(GPIO_14) << 1;
gear_bits |= digitalRead(GPIO_15) << 2;
gear_bits |= digitalRead(GPIO_16) << 3;
gear_bits |= digitalRead(GPIO_17) << 4;

// Decodificar marcha
switch(gear_bits) {
    case 0b00000: /* Park */     break;
    case 0b00001: /* Reverse */  break;
    case 0b00010: /* Neutral */  break;
    case 0b00100: /* Drive 1 */  break;
    case 0b01000: /* Drive 2 */  break;
}
```

---

## 13. DIAGRAMA DE POTENCIA GENERAL

### Distribución de Alimentación

```
┌──────────────────────────────────────────────────┐
│          FUENTES DE ALIMENTACIÓN                 │
├──────────────────────────────────────────────────┤
│                                                  │
│  Batería Principal 24V ────┬────────────────┐   │
│                            │                │   │
│                            ├─► Relé 3 ──────┼─► 4x BTS7960 → Motores 24V
│                            │   (Traction)   │   │
│                            │                │   │
│  Buck 24V→12V ─────────────┼────────────────┘   │
│                            │                    │
│                            ├─► Relé 2 ──────┬─► Motor Dirección 12V
│                            │   (12V Aux)    │   │
│                            │                ├─► 5x Sensores Rueda 12V
│                            │                │   │
│                            │                ├─► Encoder 12V
│                            │                │   │
│                            │                └─► Shifter 12V
│                            │                    │
│  Buck 12V→5V ──────────────┼─► Relé 1 ──────┬─► ESP32-S3 (5V → 3.3V reg)
│                            │   (Power Hold) │   │
│                            │                ├─► 6x BTS7960 VCC (lógica)
│                            │                │   │
│                            │                ├─► Relés JD-VCC
│                            │                │   │
│                            │                ├─► DFPlayer Mini
│                            │                │   │
│                            │                └─► LEDs WS2812B
│                            │                    │
│  Regulador 5V→3.3V ────────┴─► ESP32-S3        │
│  (onboard ESP32)                │               │
│                                 ├─► INA226 x6   │
│                                 │               │
│                                 ├─► TCA9548A    │
│                                 │               │
│                                 ├─► ILI9488     │
│                                 │               │
│                                 ├─► XPT2046     │
│                                 │               │
│                                 ├─► Pedal Hall  │
│                                 │               │
│                                 └─► HY-M158 OUT │
│                                                 │
└─────────────────────────────────────────────────┘
```

### Consumos Estimados

| Componente | Voltaje | Corriente | Potencia |
|------------|---------|-----------|----------|
| 4x Motores tracción | 24V | 60A (pico) | 1440W |
| Motor dirección | 12V | 5A (pico) | 60W |
| ESP32-S3 + periféricos | 3.3V | 500mA | 1.65W |
| LEDs WS2812B (100 LEDs) | 5V | 3A (máx) | 15W |
| Sensores + Encoder | 12V | 500mA | 6W |
| Pantalla ILI9488 | 3.3V | 100mA | 0.33W |
| **TOTAL ESTIMADO** | - | - | **~1523W** |

### Requisitos de Fuente de Alimentación

1. **Batería principal**: 24V, 60Ah mínimo (para autonomía)
2. **Buck 24V→12V**: 10A capacidad mínima
3. **Buck 12V→5V**: 5A capacidad mínima
4. **Regulador 5V→3.3V**: 1A capacidad (usualmente onboard ESP32)

---

## 14. TROUBLESHOOTING

### Problemas Comunes y Soluciones

#### 1. ESP32 no arranca
- ✓ Verificar alimentación 5V estable
- ✓ Comprobar que GPIO 0 no está conectado a GND (modo boot)
- ✓ Verificar conexión USB

#### 2. Sensores I²C no responden
- ✓ Verificar resistencias pull-up (4.7kΩ)
- ✓ Comprobar direcciones I²C (escaneo con código test)
- ✓ Verificar alimentación 3.3V a sensores
- ✓ Usar I2C Recovery del firmware

#### 3. Relés no conmutan
- ✓ Verificar modo LOW trigger configurado
- ✓ Comprobar voltaje JD-VCC (debe ser 5V)
- ✓ Medir señal en pines IN1-IN4 (debe ser HIGH=OFF, LOW=ON)
- ✓ Verificar que jumper VCC-JD-VCC está QUITADO

#### 4. Motores no responden
- ✓ Verificar secuencia de relés (Relay 3 debe estar ON)
- ✓ Comprobar alimentación 24V/12V según motor
- ✓ Verificar señales PWM con osciloscopio
- ✓ Medir voltaje en salidas BTS7960

#### 5. Encoder da lecturas erráticas
- ✓ Añadir resistencias pull-up 10kΩ
- ✓ Verificar alimentación 12V estable
- ✓ Comprobar distancia de montaje (debe ser precisa)
- ✓ Añadir capacitores 100nF en señales A, B, Z

#### 6. Pantalla táctil no responde
- ✓ Verificar calibración táctil
- ✓ Comprobar CS separados para display y touch
- ✓ Verificar que IRQ touch está conectado

#### 7. Bluetooth no conecta
- ✓ Verificar pairing con código 0000 o 1234
- ✓ Comprobar que mando está en modo pairing
- ✓ Reiniciar ESP32 y mando
- ✓ Verificar que BluetoothSerial está incluido en firmware

#### 8. LEDs WS2812B no encienden
- ✓ Verificar alimentación 5V estable
- ✓ Añadir capacitor 1000µF
- ✓ Comprobar conexión DIN al primer LED
- ✓ Usar level shifter si señal es 3.3V

#### 9. Pedal acelerador no calibra
- ✓ Verificar voltaje de salida (0.5V-3.0V)
- ✓ Comprobar conexión a pin ADC correcto
- ✓ Re-ejecutar calibración desde menú
- ✓ Verificar alimentación 3.3V estable

#### 10. Watchdog resetea continuamente
- ✓ Aumentar timeout watchdog (actualmente 5s)
- ✓ Verificar que no hay bucles bloqueantes
- ✓ Comprobar que Watchdog::feed() se llama en loop
- ✓ Revisar logs seriales para identificar bloqueo

---

## 🔧 LISTA DE VERIFICACIÓN PRE-ENCENDIDO

Antes de alimentar el sistema por primera vez:

### Alimentación
- [ ] Verificar polaridad de todas las fuentes (+/-)
- [ ] Comprobar voltajes: 24V, 12V, 5V, 3.3V
- [ ] Fusibles instalados en todas las líneas de potencia
- [ ] Jumper VCC-JD-VCC de relés **QUITADO**

### Conexiones
- [ ] Todos los GND conectados a tierra común
- [ ] Resistencias pull-up en I²C (SDA, SCL)
- [ ] Resistencias pull-up en encoder (A, B, Z)
- [ ] Level shifter para WS2812B (opcional pero recomendado)
- [ ] Capacitor desacoplo en LEDs (1000µF)

### Sensores
- [ ] INA226 x6 conectados a TCA9548A
- [ ] Direcciones I²C verificadas (TCA 0x70, INA 0x40)
- [ ] Sensores rueda con pull-up 10kΩ
- [ ] Encoder con pull-up 10kΩ

### Motores
- [ ] BTS7960 x5 alimentados correctamente
- [ ] Polaridad motores correcta
- [ ] Fusibles 20A en cada motor de tracción

### Display y Audio
- [ ] Pantalla ILI9488 en bus SPI
- [ ] Touch XPT2046 con CS separado
- [ ] DFPlayer con tarjeta SD (FAT32)
- [ ] Speaker 8Ω conectado

### Software
- [ ] Firmware compilado sin errores
- [ ] Configuración EEPROM inicializada
- [ ] Calibración pedal ejecutada
- [ ] Calibración encoder ejecutada

---

## 📞 SOPORTE

Si encuentras problemas no listados aquí:

1. Verificar logs serial (115200 baud)
2. Usar menú diagnóstico del firmware
3. Comprobar con multímetro las tensiones
4. Revisar continuidad de conexiones
5. Consultar código fuente en `firmware Coche Marcos/`

**IMPORTANTE**: Siempre desconectar alimentación antes de modificar conexiones.

---

## 📄 LICENCIA

Este documento forma parte del proyecto Electric Car Control System.
© 2024 - Uso educativo y de desarrollo.

