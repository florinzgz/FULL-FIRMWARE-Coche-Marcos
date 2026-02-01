# Manual Técnico Completo del Firmware del Vehículo Eléctrico

**Proyecto:** FULL-FIRMWARE-Coche-Marcos  
**Hardware Base:** ESP32-S3 N16R8 (16MB Flash QIO + 8MB PSRAM OPI)  
**Versión Firmware:** v2.18.0+  
**Fecha de Actualización:** 2026-02-01  
**Autor:** Documentación Técnica del Proyecto  

---

## 📋 Tabla de Contenidos

1. [Introducción y Visión General](#1-introducción-y-visión-general)
2. [Arquitectura General del Sistema](#2-arquitectura-general-del-sistema)
3. [Hardware y Configuración](#3-hardware-y-configuración)
4. [Componentes de Software Core](#4-componentes-de-software-core)
5. [Módulos de Sensores](#5-módulos-de-sensores)
6. [Módulos de Control](#6-módulos-de-control)
7. [Sistema HUD e Interfaz de Usuario](#7-sistema-hud-e-interfaz-de-usuario)
8. [Sistemas de Seguridad](#8-sistemas-de-seguridad)
9. [Sistema de Audio](#9-sistema-de-audio)
10. [Sistema de Entrada](#10-sistema-de-entrada)
11. [Sistema de Iluminación](#11-sistema-de-iluminación)
12. [Gestión de Energía y Relés](#12-gestión-de-energía-y-relés)
13. [Comunicaciones y Protocolos](#13-comunicaciones-y-protocolos)
14. [Sistema de Arranque y Boot](#14-sistema-de-arranque-y-boot)
15. [FreeRTOS y Multitasking](#15-freertos-y-multitasking)
16. [Gestión de Memoria](#16-gestión-de-memoria)
17. [Sistema de Telemetría y Logging](#17-sistema-de-telemetría-y-logging)
18. [Manejo de Errores y Recovery](#18-manejo-de-errores-y-recovery)
19. [Guía para Migración a STM32](#19-guía-para-migración-a-stm32)
20. [Construcción y Deployment](#20-construcción-y-deployment)
21. [Troubleshooting y Diagnóstico](#21-troubleshooting-y-diagnóstico)
22. [Referencias y Documentación Adicional](#22-referencias-y-documentación-adicional)

---

## 1. Introducción y Visión General

### 1.1 Propósito del Firmware

Este firmware implementa el sistema completo de control y HMI (Human-Machine Interface) para un vehículo eléctrico de cuatro ruedas con tracción independiente. El sistema proporciona:

- **Control de tracción con 4 motores independientes** (4x BTS7960 43A)
- **Control de dirección con encoder de alta precisión** (E6B2-CWZ6C 1200 PPR)
- **Sistemas avanzados de seguridad** (ABS, TCS, detección de obstáculos)
- **Interfaz gráfica completa** (Display TFT 480×320 con touch capacitivo)
- **Gestión inteligente de energía** (monitorización de corriente en 6 canales)
- **Sistema de audio y alertas** (DFPlayer Mini)
- **Iluminación LED programable** (WS2812B, 44 LEDs totales)
- **Telemetría en tiempo real** y diagnóstico

### 1.2 Principios de Diseño

El firmware está construido siguiendo estos principios fundamentales:

1. **Modularidad:** Separación clara de responsabilidades mediante Managers y módulos independientes
2. **Tolerancia a fallos:** Degradación gradual ante errores, con modos de operación seguros
3. **Auto-recuperación:** Sistemas watchdog, boot guard, y recuperación I2C automática
4. **Thread-safety:** Uso de FreeRTOS con primitivas de sincronización (mutex, semaphores)
5. **Determinismo:** Loops de control con tiempos predecibles (<20ms por iteración)
6. **Trazabilidad:** Logging completo, telemetría, y códigos de error centralizados
7. **Eficiencia de memoria:** Uso inteligente de PSRAM para grandes buffers (display, sprites)

### 1.3 Capacidades del Sistema

#### Características Principales

- **Velocidad de control:** Loop principal a ~10-20ms (50-100 Hz)
- **Dual-core processing:** Core 0 (protocolo WiFi/BT), Core 1 (aplicación)
- **Renderizado avanzado:** Compositor de capas con shadow rendering y dirty rectangles
- **Gestión térmica:** 4 sensores DS18B20 con alertas de sobrecalentamiento
- **Protección de batería:** Monitorización INA226 con shutoff automático por bajo voltaje
- **Modo Limp:** Modo degradado que permite llegar a lugar seguro

#### Capacidades Operativas

- **Rango de temperatura:** -10°C a +60°C (operación), -20°C a +80°C (almacenamiento)
- **Voltaje de batería:** 20V-29.4V (nominal 24V, 6S Li-Ion)
- **Corriente máxima tracción:** 4×50A = 200A peak (4 motores)
- **Corriente dirección:** 50A peak (motor RS390 + reductora 1:50)
- **Velocidad máxima detectable:** ~60 km/h (según calibración encoder ruedas)
- **Precisión dirección:** 0.3° (1200 PPR encoder / 360° volante)
- **Framerate HUD:** 30-60 FPS (según complejidad escena)

### 1.4 Estructura del Repositorio

```
FULL-FIRMWARE-Coche-Marcos/
├── src/                          # Código fuente principal
│   ├── main.cpp                  # Punto de entrada, setup() y loop()
│   ├── core/                     # Módulos fundamentales del sistema
│   ├── sensors/                  # Drivers de sensores
│   ├── control/                  # Lógica de control de motores
│   ├── hud/                      # Sistema de display y UI
│   ├── safety/                   # Sistemas de seguridad (ABS, TCS, obstáculos)
│   ├── audio/                    # Control de audio (DFPlayer Mini)
│   ├── input/                    # Entrada de usuario (botones, pedal, shifter)
│   ├── lighting/                 # Control LEDs WS2812B
│   ├── menu/                     # Menús de configuración y diagnóstico
│   ├── managers/                 # Managers de alto nivel
│   ├── utils/                    # Utilidades (filtros, math, debug)
│   └── test/                     # Tests funcionales y de hardware
├── include/                      # Headers públicos
│   ├── pins.h                    # Definición completa de GPIOs
│   ├── SystemConfig.h            # Configuración global del sistema
│   ├── version.h                 # Información de versión
│   └── [otros headers]
├── boards/                       # Definiciones de placas PlatformIO
│   └── esp32s3_n16r8.json        # Board custom N16R8
├── sdkconfig/                    # Configuración ESP-IDF
│   └── n16r8.defaults            # Defaults para N16R8
├── partitions/                   # Esquemas de partición
│   └── partitions.csv            # Layout de flash 16MB
├── docs/                         # Documentación técnica
├── audio/                        # Archivos de audio MP3
├── data/                         # Datos para SPIFFS
├── tools/                        # Scripts y herramientas
├── platformio.ini                # Configuración PlatformIO
└── [documentos .md]              # Guías, auditorías, manuales

Total de archivos: ~96 headers (.h) + ~79 implementaciones (.cpp)
```

### 1.5 Stack Tecnológico

| Componente | Tecnología | Versión |
|------------|-----------|---------|
| **Microcontrolador** | ESP32-S3 Dual Xtensa LX7 @ 240MHz | N16R8 variant |
| **Framework** | Arduino-ESP32 | Compatible con ESP-IDF 5.x |
| **Build System** | PlatformIO | 6.12.0+ |
| **RTOS** | FreeRTOS | Incluido en ESP-IDF |
| **Display** | TFT_eSPI | 2.5.43+ |
| **Gráficos** | Compositor custom + TFT_eSPI | - |
| **LEDs** | FastLED | 3.10.3+ |
| **I2C** | Adafruit BusIO | 1.14.5+ |
| **Current Sensing** | INA226 (Robtillaart) | 0.6.6+ |
| **Temperature** | DallasTemperature | 4.0.6+ |
| **Audio** | DFRobotDFPlayerMini | 1.0.6+ |
| **Logging** | Logger custom | - |

---

## 2. Arquitectura General del Sistema

### 2.1 Visión Arquitectónica

El firmware implementa una arquitectura multicapa basada en **Managers** que coordinan módulos especializados. La arquitectura sigue el patrón de separación de responsabilidades con niveles jerárquicos claramente definidos.

```
┌─────────────────────────────────────────────────────────────────┐
│                        MAIN LOOP (Core 1)                        │
│                      src/main.cpp - 10-20ms                      │
└─────────────────────────────────────────────────────────────────┘
                               │
           ┌───────────────────┼────────────────────┐
           │                   │                    │
           ▼                   ▼                    ▼
   ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
   │ PowerManager │    │SensorManager │    │SafetyManager │
   │   (Nivel 0)  │    │   (Nivel 1)  │    │   (Nivel 2)  │
   └──────────────┘    └──────────────┘    └──────────────┘
           │                   │                    │
           └───────────────────┼────────────────────┘
                               │
           ┌───────────────────┼────────────────────┐
           ▼                   ▼                    ▼
   ┌──────────────┐    ┌──────────────┐    ┌──────────────┐
   │ControlManager│    │  HUDManager  │    │  ModeManager │
   │   (Nivel 3)  │    │   (Nivel 3)  │    │   (Nivel 3)  │
   └──────────────┘    └──────────────┘    └──────────────┘
           │                   │                    │
           └───────────────────┼────────────────────┘
                               ▼
                       ┌──────────────┐
                       │  Telemetry   │
                       │   (Nivel 4)  │
                       └──────────────┘
```

### 2.2 Jerarquía de Managers

#### **Nivel 0 - Fundamental (Sin Dependencias)**

**PowerManager** (`managers/PowerManager.h`, `control/relays.cpp`)
- Gestiona relés de alimentación (Main, Tracción, Dirección, Auxiliar)
- Control de Power-Hold (mantener sistema encendido después de KEY_OFF)
- Detección de estado de ignición (KEY_ON, KEY_OFF)
- Protección contra bajo voltaje de batería

#### **Nivel 1 - Sensores (Depende de PowerManager)**

**SensorManager** (`managers/SensorManager.h`, `managers/SensorManagerEnhanced.cpp`)
- Coordina lectura de todos los sensores del vehículo
- Gestión de TCA9548A (multiplexor I2C para 6 INA226)
- Validación de datos de sensores con rangos esperados
- Filtrado de señales (filtros paso-bajo, mediana)

Sensores gestionados:
- **Corriente:** 6× INA226 (batería + 4 motores tracción + motor dirección)
- **Temperatura:** 4× DS18B20 (OneWire) en motores tracción
- **Ruedas:** 4× Sensores inductivos LJ12A3-4-Z/BX (velocidad/RPM)
- **Encoder dirección:** E6B2-CWZ6C 1200 PPR (A/B/Z)
- **Pedal:** Sensor Hall A1324LUA-T (analógico ADC)
- **Shifter:** Encoder mecánico 5 posiciones (via MCP23017)
- **Obstáculos:** TOFSense-M S LiDAR 8×8 (UART)

#### **Nivel 2 - Seguridad (Depende de Sensores)**

**SafetyManager** (`managers/SafetyManager.h`, `managers/SafetyManagerEnhanced.cpp`)
- Coordina todos los sistemas de seguridad
- Evaluación continua de condiciones de operación segura
- Transiciones de modo (Normal → Limp → Emergency Stop)
- Gestión de alertas críticas

Subsistemas:
- **ABS System** (`safety/abs_system.cpp`) - Anti-lock Braking System
- **TCS System** (`safety/tcs_system.cpp`) - Traction Control System
- **Obstacle Safety** (`safety/obstacle_safety.cpp`) - Detección y respuesta a obstáculos
- **Regen AI** (`safety/regen_ai.cpp`) - Frenado regenerativo inteligente

#### **Nivel 3 - Control y UI (Depende de Sensores y Seguridad)**

**ControlManager** (`managers/ControlManager.h`)
- Coordina sistemas de tracción y dirección
- Aplica comandos validados por SafetyManager
- Gestión de modos de conducción (Eco, Normal, Sport)

Subsistemas:
- **Traction** (`control/traction.cpp`) - Control de 4 motores independientes
- **Steering Motor** (`control/steering_motor.cpp`) - Control de dirección asistida
- **Adaptive Cruise** (`control/adaptive_cruise.cpp`) - Control de crucero adaptativo

**HUDManager** (`hud/hud_manager.cpp`)
- Gestión de display TFT 480×320
- Touch input (XPT2046)
- Renderizado de UI (gauges, menús, diagnóstico)
- Compositor de capas con shadow rendering

**ModeManager** (`managers/ModeManager.h`)
- Determina modo operativo del vehículo
- State machine: OFF → STANDBY → READY → RUNNING → LIMP
- Gestión de transiciones de modo seguras

#### **Nivel 4 - Telemetría (Depende de Todos)**

**TelemetryManager** (`managers/TelemetryManager.h`, `core/telemetry.cpp`)
- Recopilación de datos de todos los managers
- Logging de eventos y métricas
- Exportación de datos para análisis

### 2.3 Flujo de Datos Principal

```
┌─────────────────────────────────────────────────────────────┐
│  1. ENTRADA (Input Layer)                                   │
│  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  │
│  • Pedal (ADC)                                              │
│  • Shifter (MCP23017)                                       │
│  • Steering encoder (interrupciones A/B/Z)                  │
│  • Touch (XPT2046 SPI)                                      │
│  • Buttons (GPIOs)                                          │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│  2. SENSORES (Sensor Layer)                                 │
│  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  │
│  • SensorManager lee todos los sensores                    │
│  • Validación de rangos y filtrado                         │
│  • Cálculo de variables derivadas (velocidad, potencia)    │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│  3. SEGURIDAD (Safety Layer)                                │
│  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  │
│  • SafetyManager evalúa condiciones                         │
│  • ABS/TCS calculan límites de tracción                    │
│  • Obstacle Safety verifica zona frontal                   │
│  • Decisión: permitir/limitar/inhibir movimiento           │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│  4. CONTROL (Control Layer)                                 │
│  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  │
│  • ControlManager recibe comandos validados                 │
│  • Traction calcula PWM para 4 motores (PCA9685)           │
│  • Steering calcula ángulo target y control PID            │
│  • Apply actuation via I2C (PCA9685) y GPIO (MCP23017)     │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│  5. ACTUADORES (Output Layer)                               │
│  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  │
│  • 4× BTS7960 (tracción) via PCA9685 PWM + MCP23017 DIR    │
│  • 1× BTS7960 (dirección) via PCA9685 PWM + MCP23017 DIR   │
│  • 4× Relés (Main/Tracción/Dirección/Auxiliar) via GPIO    │
│  • 44× LEDs WS2812B (iluminación) via FastLED              │
│  • DFPlayer Mini (audio) via UART1                         │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│  6. FEEDBACK (UI/Telemetry Layer)                           │
│  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  │
│  • HUDManager actualiza display (gauges, estado)           │
│  • Audio alerts según eventos (SafetyManager)              │
│  • LED Controller actualiza animaciones                    │
│  • TelemetryManager registra eventos                       │
└─────────────────────────────────────────────────────────────┘
```

### 2.4 Organización del Código

#### Directorio `src/`

| Subdirectorio | Propósito | Archivos Clave |
|---------------|-----------|----------------|
| **core/** | Módulos fundamentales del sistema | system.cpp, boot_guard.cpp, watchdog.cpp, i2c_recovery.cpp, rtos_tasks.cpp |
| **sensors/** | Drivers de sensores | current.cpp, temperature.cpp, wheels.cpp, obstacle_detection.cpp |
| **control/** | Lógica de control de motores | traction.cpp, steering_motor.cpp, relays.cpp, tcs_system.cpp |
| **hud/** | Sistema de display y gráficos | hud.cpp, hud_manager.cpp, hud_compositor.cpp, gauges.cpp, icons.cpp |
| **safety/** | Sistemas de seguridad | abs_system.cpp, obstacle_safety.cpp, regen_ai.cpp |
| **audio/** | Control de audio | dfplayer.cpp, alerts.cpp, queue.cpp |
| **input/** | Entrada de usuario | pedal.cpp, shifter.cpp, steering.cpp, buttons.cpp |
| **lighting/** | Control de LEDs | led_controller.cpp |
| **menu/** | Menús de configuración | menu_hidden.cpp, menu_sensor_config.cpp, menu_power_config.cpp |
| **managers/** | Managers de alto nivel | [Headers, implementación distribuida en módulos] |
| **utils/** | Utilidades comunes | filters.cpp, math_utils.cpp, debug.cpp |

#### Directorio `include/`

Contiene todos los headers públicos del proyecto. Los headers clave son:

- **pins.h:** Definición COMPLETA de todos los GPIOs del ESP32-S3
- **SystemConfig.h:** Configuración global (timeouts, constantes)
- **version.h:** Información de versión del firmware
- **addresses.h:** Direcciones I2C de todos los dispositivos
- **constants.h:** Constantes físicas (ratios, límites)
- **error_codes.h:** Códigos de error centralizados

---

## 3. Hardware y Configuración

### 3.1 Especificaciones del Microcontrolador

#### ESP32-S3 N16R8

| Parámetro | Especificación |
|-----------|----------------|
| **Nombre Completo** | ESP32-S3-WROOM-2 N16R8 |
| **CPU** | Dual-core Xtensa LX7 @ 240 MHz |
| **Flash** | 16 MB (QIO mode @ 80 MHz, 3.3V) |
| **PSRAM** | 8 MB (OPI/Octal mode @ 80 MHz, 3.3V) |
| **GPIO** | 36 GPIOs disponibles (44-pin DevKitC-1) |
| **ADC** | 2× SAR ADC (12-bit), 20 canales |
| **DAC** | No disponible en S3 |
| **UART** | 3× UART (0, 1, 2) |
| **SPI** | 4× SPI (2 accesibles, 2 para flash/PSRAM) |
| **I2C** | 2× I2C |
| **I2S** | 2× I2S |
| **PWM** | 8× MCPWM, múltiples LEDC |
| **USB** | USB OTG 1.1 (CDC, MSC) |
| **WiFi** | 802.11 b/g/n @ 2.4GHz |
| **Bluetooth** | Bluetooth 5 (LE) |
| **Temperatura Operación** | -40°C a +85°C |
| **Voltaje** | 3.0V - 3.6V (típico 3.3V) |
| **Consumo Activo** | ~240mA @ 240MHz (sin WiFi/BT) |
| **Consumo Sleep** | <10μA (deep sleep) |

#### Justificación de N16R8

El firmware está optimizado específicamente para esta configuración:

1. **16MB Flash QIO:**
   - Almacena firmware completo (~2MB compilado)
   - OTA updates (2× particiones app de ~8MB c/u)
   - SPIFFS para datos (audio, configuración, logs)
   - QIO mode (4 líneas datos) balance velocidad/estabilidad

2. **8MB PSRAM OPI:**
   - Buffers de display: 2× 480×320×2 bytes = 614 KB
   - Sprites y capas: ~1-2 MB
   - Buffers de audio y comunicaciones
   - Heap dinámico para aplicación
   - OPI mode (8 líneas datos) máxima bandwidth para gráficos

**⚠️ IMPORTANTE:** NO usar otros modelos (N8R8, N32R16V). El firmware espera EXACTAMENTE 16MB Flash + 8MB PSRAM en modo `qio_opi`.

### 3.2 Hardware Externo Integrado

#### 3.2.1 Periféricos I2C (Bus Principal GPIO 8/9)

| Dispositivo | Dirección I2C | Función | Detalles |
|-------------|---------------|---------|----------|
| **TCA9548A** | 0x70 | Multiplexor I2C 1-a-8 | Para 6 INA226 (evita conflicto 0x40) |
| **6× INA226** | 0x40 (via TCA9548A) | Sensores de corriente/voltaje | Shunts externos 50A/100A CG FL-2C |
| **PCA9685 #1** | 0x40 | Driver PWM 16 canales | Motores tracción eje delantero (FL+FR) |
| **PCA9685 #2** | 0x41 | Driver PWM 16 canales | Motores tracción eje trasero (RL+RR) |
| **PCA9685 #3** | 0x42 | Driver PWM 16 canales | Motor dirección |
| **MCP23017** | 0x20 | Expansor GPIO 16 pines | Control IN1/IN2 BTS7960 + Shifter inputs |

##### Configuración TCA9548A Canales:

```
Canal 0 → INA226 Motor FL (Frontal Izquierda, shunt 50A)
Canal 1 → INA226 Motor FR (Frontal Derecha, shunt 50A)
Canal 2 → INA226 Motor RL (Trasera Izquierda, shunt 50A)
Canal 3 → INA226 Motor RR (Trasera Derecha, shunt 50A)
Canal 4 → INA226 Batería 24V (shunt 100A)
Canal 5 → INA226 Motor Dirección RS390 12V (shunt 50A)
```

Todos los INA226 configurados en 0x40, aislados por el multiplexor.

##### Configuración MCP23017:

**Bank A (GPIOA0-A7):** Control de dirección de 4 motores tracción
```
GPIOA0 = FL_IN1  (Motor Front Left bit 0)
GPIOA1 = FL_IN2  (Motor Front Left bit 1)
GPIOA2 = FR_IN1  (Motor Front Right bit 0)
GPIOA3 = FR_IN2  (Motor Front Right bit 1)
GPIOA4 = RL_IN1  (Motor Rear Left bit 0)
GPIOA5 = RL_IN2  (Motor Rear Left bit 1)
GPIOA6 = RR_IN1  (Motor Rear Right bit 0)
GPIOA7 = RR_IN2  (Motor Rear Right bit 1)
```

**Bank B (GPIOB0-B7):** Shifter y expansión
```
GPIOB0 = SHIFTER_D1 (bit 0 posición cambio)
GPIOB1 = SHIFTER_D2 (bit 1)
GPIOB2 = SHIFTER_D3 (bit 2)
GPIOB3 = SHIFTER_D4 (bit 3)
GPIOB4 = SHIFTER_D5 (bit 4)
GPIOB5-B7 = LIBRES (expansión futura)
```

**Configuración I2C Bus:**
- Frecuencia: 400 kHz (Fast Mode)
- Pull-ups: 4.7kΩ en SDA/SCL
- Máx dispositivos: 8 (actual: 7 + multiplexor)
- Recovery automático: `i2c_recovery.cpp` detecta y recupera bus stuck

#### 3.2.2 Drivers de Motor (BTS7960 43A)

**5× BTS7960 43A Dual H-Bridge**

| Motor | BTS7960 # | PWM Source | Direction Control | Corriente Max |
|-------|-----------|------------|-------------------|---------------|
| FL (Front-Left) | #1 | PCA9685 0x40 CH0-1 | MCP23017 GPIOA0-1 | 50A |
| FR (Front-Right) | #2 | PCA9685 0x40 CH2-3 | MCP23017 GPIOA2-3 | 50A |
| RL (Rear-Left) | #3 | PCA9685 0x41 CH0-1 | MCP23017 GPIOA4-5 | 50A |
| RR (Rear-Right) | #4 | PCA9685 0x41 CH2-3 | MCP23017 GPIOA6-7 | 50A |
| Steering | #5 | PCA9685 0x42 CH0-1 | MCP23017 GPIOA* | 50A |

**Conexión típica BTS7960:**
```
ESP32 (via PCA9685) ──> RPWM (Right PWM, forward)
ESP32 (via PCA9685) ──> LPWM (Left PWM, reverse)
ESP32 (via MCP23017) ──> L_EN (Left Enable)
ESP32 (via MCP23017) ──> R_EN (Right Enable)
VCC ──> 5V (lógica)
VCC ──> 24V (potencia motores tracción) o 12V (dirección)
GND ──> GND común
M+ ──> Motor terminal +
M- ──> Motor terminal -
```

**Protecciones BTS7960:**
- Sobrecorriente: 43A continuous, 100A peak (1s)
- Sobretemperatura: Shutdown automático a 150°C
- Cortocircuito: Protección integrada
- ESD: Hasta 4kV

#### 3.2.3 Display y Touch (SPI)

**Display: ST7796S 480×320 TFT**

| Parámetro | Valor |
|-----------|-------|
| Resolución | 480×320 píxeles (landscape) |
| Colores | 262K (RGB565) |
| Interfaz | SPI 4-wire |
| Frecuencia SPI | 40 MHz (lectura: 20 MHz) |
| Backlight | LED PWM (GPIO 42) |
| Driver | TFT_eSPI 2.5.43+ |

**Pines de conexión TFT:**
```
GPIO 14 → TFT_SCK  (SPI Clock)
GPIO 13 → TFT_MOSI (SPI MOSI)
GPIO 15 → TFT_CS   (Chip Select)
GPIO 16 → TFT_DC   (Data/Command)
GPIO 17 → TFT_RST  (Reset)
GPIO 42 → TFT_BL   (Backlight PWM, LEDC)
MISO: No usado (ST7796S no requiere lectura)
```

**⚠️ CRÍTICO - Zona Segura de SPI:**
Los pines 13-17 están en "zona segura", lejos de:
- GPIO 10-12 (SPI Flash interno - NO USAR)
- GPIO 26-37 (PSRAM Octal - RESERVADOS)
- Strapping pins críticos

**Touch: XPT2046 (Resistive)**

| Parámetro | Valor |
|-----------|-------|
| Tecnología | Resistiva 4-wire |
| Precisión | 12-bit (4096×4096) |
| Interfaz | SPI compartido con TFT |
| Frecuencia SPI | 2.5 MHz |
| Interrupción | GPIO 47 (TOUCH_IRQ, falling edge) |

**Pines de conexión Touch:**
```
GPIO 14 → T_SCK  (SPI Clock, compartido)
GPIO 13 → T_MOSI (SPI MOSI, compartido)
GPIO 12 → T_MISO (SPI MISO, compartido)
GPIO 21 → T_CS   (Chip Select, dedicado)
GPIO 47 → T_IRQ  (Interrupt, falling edge)
```

**Calibración Touch:**
El firmware incluye rutina de calibración interactiva en `hud/touch_calibration.cpp`. Matriz de transformación almacenada en EEPROM.

#### 3.2.4 Sensores de Temperatura (OneWire)

**4× DS18B20 Digital Temperature Sensors**

| Parámetro | Valor |
|-----------|-------|
| Rango | -55°C a +125°C |
| Precisión | ±0.5°C (-10°C a +85°C) |
| Resolución | 9 a 12 bits (configurable) |
| Interfaz | 1-Wire (Dallas/Maxim) |
| Tiempo conversión | 750ms @ 12-bit |

**Conexión:**
```
GPIO 20 → OneWire Bus (DQ)
Pull-up 4.7kΩ entre DQ y 3.3V
Motores: FL, FR, RL, RR (cada uno con sensor en carcasa)
```

**Configuración del firmware:**
- Resolución: 12-bit (0.0625°C)
- Polling: Cada 1 segundo
- Alertas: >70°C Warning, >85°C Critical (activación Limp Mode)

**Direccionamiento:**
Cada DS18B20 tiene ROM única de 64-bit. El firmware detecta automáticamente y asocia a motor mediante `temperature.cpp`.

#### 3.2.5 Sensores de Rueda (Hall Effect Inductivos)

**4× LJ12A3-4-Z/BX Inductive Proximity Sensors**

| Parámetro | Valor |
|-----------|-------|
| Tipo | NPN Normally Open (NO) |
| Distancia detección | 4mm |
| Voltaje | 6-36V DC |
| Corriente | <10mA |
| Salida | NPN Open Collector |
| Frecuencia max | ~1kHz |

**Conexión (cada rueda):**
```
Brown → +12V
Blue → GND
Black → GPIO (FL:7, FR:36, RL:15, RR:1) via optoacoplador
```

**Optoacoplador PC817 (8 canales en 2× HY-M158):**
Aislamiento óptico 12V→3.3V para proteger ESP32. Salida conectada directamente a GPIO ESP32 (pull-up interno activado).

**Detección de velocidad:**
- Interrupción por flanco de bajada en cada GPIO
- Cálculo RPM mediante tiempo entre pulsos
- Calibración: Pulsos por revolución (típicamente 1 pulso/revolución con target único)
- Velocidad lineal: `V [km/h] = (RPM × π × D [m] × 60) / 1000`
  donde D = diámetro rueda (ej. 0.3m para rueda pequeña)

#### 3.2.6 Encoder de Dirección

**E6B2-CWZ6C 1200 PPR Incremental Encoder**

| Parámetro | Valor |
|-----------|-------|
| Resolución | 1200 pulsos/revolución (PPR) |
| Canales | A, B (quadrature) + Z (index) |
| Voltaje | 5-24V DC |
| Salida | Voltage output (compatible 3.3V) |
| Frecuencia max | 200 kHz |

**Conexión:**
```
GPIO 37 → ENCODER_A (canal A)
GPIO 38 → ENCODER_B (canal B)
GPIO 39 → ENCODER_Z (index/home)
```

**Decodificación Quadrature:**
Implementada en `input/steering.cpp` con interrupciones en ambos flancos de A y B.

```
   A ╱‾‾╲__╱‾‾╲__
   B __╱‾‾╲__╱‾‾╲
      → → → →     (Clockwise, incrementa contador)
   
   A __╱‾‾╲__╱‾‾╲
   B ╱‾‾╲__╱‾‾╲__
      ← ← ← ←     (Counter-clockwise, decrementa contador)
```

**Cálculo de ángulo:**
```cpp
float angle_deg = (encoder_count % 1200) * (360.0f / 1200.0f);
// Resultado: 0.3° de resolución
```

**Señal Z (Index):**
Un pulso por revolución. Usado para:
- Detectar posición de centrado (home)
- Resetear contador acumulativo
- Validar sincronización

**Ratio dirección:**
- Encoder montado 1:1 con volante
- Rango útil: ±540° (1.5 vueltas cada lado desde centro)
- Centro detectado automáticamente en boot via señal Z

#### 3.2.7 Sensor de Pedal (Analógico)

**A1324LUA-T Linear Hall Effect Sensor**

| Parámetro | Valor |
|-----------|-------|
| Tipo | Ratiometric Linear Hall |
| Rango campo magnético | ±600 Gauss |
| Voltaje salida | 0.5V a 4.5V (ratiométrico con Vcc) |
| Sensibilidad | 5 mV/G típico |
| Voltaje operación | 4.5V a 5.5V |

**Conexión:**
```
GPIO 4 → PEDAL (ADC1_CH3)
Vcc → 5V (via regulador)
GND → GND
Vout → GPIO 4 (divisor resistivo si necesario para 3.3V)
```

**Lectura ADC:**
```cpp
// ADC ESP32-S3: 12-bit (0-4095)
// Atenuación: 11dB (0-3.3V)
int raw = analogRead(PIN_PEDAL);  // 0-4095
float voltage = raw * (3.3 / 4095.0);
float pedal_percent = map(voltage, V_MIN, V_MAX, 0.0, 100.0);
```

**Calibración:**
- Posición reposo: ~0.5V (ADC ~620)
- Posición máxima: ~4.5V (ADC ~3700)
- Umbral de detección: 5% (ruido ADC)
- Filtrado: Media móvil de 5 muestras

**Protecciones:**
- Validación rango: 0-100% estricto
- Timeout: Si pedal >95% por >10s → Error (pedal stuck)
- Comparación: Cross-check con corriente de motores

#### 3.2.8 Sensor LiDAR (UART)

**TOFSense-M S 8×8 LiDAR Matrix**

| Parámetro | Valor |
|-----------|-------|
| Resolución | 8×8 zonas (64 mediciones) |
| Rango | 0.3m - 4.0m |
| FOV | 45° diagonal |
| Frecuencia | 10-15 Hz |
| Interfaz | UART (115200 baud) |
| Protocolo | Binario propietario |

**Conexión:**
```
GPIO 44 → TOFSENSE_RX (UART0 RX, recibe datos del sensor)
GPIO 43 → TOFSENSE_TX (UART0 TX, comandos al sensor, opcional)
```

**Formato de datos:**
Paquetes binarios con header, payload de 64 distancias (uint16_t), y checksum.

**Procesamiento:**
- Parser en `sensors/obstacle_detection.cpp`
- Validación checksum
- Filtro de datos inválidos (0xFFFF = no medición)
- Cálculo de distancia mínima frontal
- Triggers de alerta: <2.0m Warning, <1.0m Critical

#### 3.2.9 Audio (UART1)

**DFPlayer Mini MP3 Module**

| Parámetro | Valor |
|-----------|-------|
| Formato audio | MP3, WAV |
| Almacenamiento | MicroSD (hasta 32GB, FAT32) |
| DAC | 16-bit |
| Salida | Estéreo 3W (amplificador integrado) |
| Interfaz | UART (9600 baud) |
| Voltaje | 3.2V - 5V |

**Conexión:**
```
GPIO 18 → DFPLAYER_TX (ESP32 TX → DFPlayer RX)
GPIO 17 → DFPLAYER_RX (ESP32 RX ← DFPlayer TX)
Resistor 1kΩ en serie con ESP32 TX (protección 5V↔3.3V)
```

**Comandos UART:**
Protocolo propietario: `0x7E FF 06 <CMD> <ARG_H> <ARG_L> <CHECKSUM_H> <CHECKSUM_L> 0xEF`

**Tracks de audio:**
Almacenados en `/mp3/` en la SD:
```
0001.mp3 = Boot sound
0002.mp3 = Warning beep
0003.mp3 = Critical alert
0004.mp3 = ABS activation
0005.mp3 = TCS intervention
0006.mp3 = Obstacle detected
... (ver docs/AUDIO_TRACKS_GUIDE.md para lista completa)
```

**Control:**
- Volumen: 0-30 (ajustable desde menú)
- Modo: Loop, shuffle, single
- EQ: Normal, Pop, Rock, Jazz, Classic, Bass

#### 3.2.10 Iluminación LED (FastLED)

**2× Tiras WS2812B RGB Addressable LEDs**

| Parámetro | Valor |
|-----------|-------|
| LED IC | WS2812B (RGB integrado) |
| Protocolo | 1-wire timing protocol (800kHz) |
| Voltaje | 5V |
| Corriente | ~60mA por LED @ full white |
| Control | FastLED library |

**Distribución:**
```
GPIO 19 → LED_FRONT (28 LEDs frontales)
GPIO 48 → LED_REAR  (16 LEDs traseros)
Total: 44 LEDs
```

**Corriente máxima:**
44 LEDs × 60mA = 2.64A @ full brightness white
Recomendación: Fuente 5V 3A mínimo

**Animaciones implementadas:**
- Solid color (luces diurnas)
- Breathing (espera/standby)
- Running (modo activo)
- Alert flash (warnings)
- Rainbow (demo/test)
- Turn signals (indicadores direccionales)
- Brake lights (freno, LEDs traseros)

**Control:**
`src/lighting/led_controller.cpp` con API de alto nivel:
```cpp
LEDController::setMode(LED_MODE_RUNNING);
LEDController::setBrightness(128);  // 0-255
LEDController::setColor(255, 0, 0); // RGB
LEDController::update();  // Llamar en loop()
```

### 3.3 Esquema de Pines Completo (ESP32-S3)

**Ver archivo:** `GPIO_ASSIGNMENT_LIST.md` para tabla detallada de todos los 36 GPIOs.

#### Resumen por Función

**🔌 Comunicaciones (9 pines)**
- GPIO 8/9: I2C_SDA/SCL (bus principal)
- GPIO 17/18: UART1 RX/TX (DFPlayer audio)
- GPIO 43/44: UART0 TX/RX (TOFSense LiDAR)
- GPIO 14/13/15: SPI SCLK/MOSI/CS (display TFT)
- GPIO 21: TOUCH_CS (SPI touch)

**🎮 Control y Entrada (9 pines)**
- GPIO 4: PEDAL (ADC analógico)
- GPIO 2: BTN_LIGHTS (botón físico)
- GPIO 37/38/39: ENCODER_A/B/Z (dirección)
- GPIO 40/41: KEY_ON/KEY_OFF (ignición)
- GPIO 47: TOUCH_IRQ (interrupción táctil)

**⚡ Relés y Potencia (4 pines)**
- GPIO 35: RELAY_MAIN (relé principal power-hold)
- GPIO 5: RELAY_TRAC (relé tracción 24V)
- GPIO 6: RELAY_DIR (relé dirección 12V)
- GPIO 46: RELAY_SPARE (relé auxiliar) ⚠️ Strapping pin

**📺 Display (6 pines)**
- GPIO 14: TFT_SCK
- GPIO 13: TFT_MOSI
- GPIO 16: TFT_DC
- GPIO 17: TFT_RST
- GPIO 15: TFT_CS
- GPIO 42: TFT_BL (backlight PWM)

**🚗 Sensores de Ruedas (4 pines)**
- GPIO 7: WHEEL_FL (rueda delantera izquierda)
- GPIO 36: WHEEL_FR (rueda delantera derecha)
- GPIO 15: WHEEL_RL (rueda trasera izquierda)
- GPIO 1: WHEEL_RR (rueda trasera derecha)

**🌡️ Sensores Diversos (2 pines)**
- GPIO 20: ONEWIRE (4× DS18B20 temperatura)
- GPIO 12: TFT_MISO (SPI, touch readback)

**💡 LEDs (2 pines)**
- GPIO 19: LED_FRONT (28 LEDs WS2812B frontales)
- GPIO 48: LED_REAR (16 LEDs WS2812B traseros)

**🆓 Pines Libres (3 pines)**
- GPIO 0: ⚠️ Strapping (Boot mode)
- GPIO 3: ⚠️ Strapping (JTAG)
- GPIO 45: ⚠️ Strapping (VDD_SPI)

**Recomendación:** Evitar usar strapping pins para funciones críticas. Si es necesario, configurar como OUTPUT temprano en boot.

### 3.4 Configuración PlatformIO

#### platformio.ini

```ini
[env:esp32-s3-devkitc1-n16r8]
platform = espressif32
board = esp32-s3-devkitc1-n16r8
framework = arduino

; Memoria y particiones
board_build.partitions = partitions/partitions.csv
board_build.sdkconfig = sdkconfig/n16r8.defaults
board_build.arduino.memory_type = qio_opi
board_build.flash_mode = qio
board_build.f_flash = 80000000L
board_build.f_cpu = 240000000L

; Build flags
build_flags = 
    -DCORE_DEBUG_LEVEL=5
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue
    -DARDUINO_USB_CDC_ON_BOOT=0
    
    ; TFT_eSPI configuración inline
    -DUSER_SETUP_LOADED=1
    -DST7796_DRIVER=1
    -DTFT_WIDTH=320
    -DTFT_HEIGHT=480
    -DTFT_MOSI=13
    -DTFT_SCLK=14
    -DTFT_CS=15
    -DTFT_DC=16
    -DTFT_RST=17
    -DTFT_BL=42
    -DSPI_FREQUENCY=40000000
    -DTOUCH_CS=21
    
    ; Fuentes
    -DLOAD_GLCD=1
    -DLOAD_FONT2=1
    -DLOAD_FONT4=1
    -DLOAD_FONT6=1
    -DLOAD_FONT7=1
    -DLOAD_FONT8=1
    -DLOAD_GFXFF=1
    -DSMOOTH_FONT=1

; Librerías
lib_deps = 
    adafruit/Adafruit MCP23017 Arduino Library@^2.3.2
    adafruit/Adafruit BusIO@^1.14.5
    adafruit/Adafruit PWM Servo Driver Library@^3.0.2
    dfrobot/DFRobotDFPlayerMini@^1.0.6
    bodmer/TFT_eSPI@^2.5.43
    fastled/FastLED@^3.10.3
    milesburton/DallasTemperature@^4.0.6
    paulstoffregen/OneWire@^2.3.8
    robtillaart/INA226@^0.6.6

monitor_speed = 115200
upload_speed = 921600
monitor_filters = esp32_exception_decoder
```

#### Particiones (partitions/partitions.csv)

```csv
# Name,   Type, SubType, Offset,  Size,    Flags
nvs,      data, nvs,     0x9000,  0x6000,
otadata,  data, ota,     0xF000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x800000,
app1,     app,  ota_1,   0x810000,0x800000,
spiffs,   data, spiffs,  0x1010000,0xF0000,
```

**Total**: 16MB
- **NVS**: 24KB (configuración persistente)
- **OTA Data**: 8KB (metadatos OTA)
- **App 0**: 8MB (partición aplicación primaria)
- **App 1**: 8MB (partición OTA secundaria)
- **SPIFFS**: 960KB (sistema de archivos para datos)

#### SDKConfig (sdkconfig/n16r8.defaults)

Configuración clave:
```
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_SPIRAM_MODE_OCT=y
CONFIG_SPIRAM_SIZE=8388608
CONFIG_SPIRAM_SPEED_80M=y
CONFIG_ESP32S3_SPIRAM_SUPPORT=y
```

---

## 4. Componentes de Software Core

### 4.1 System (src/core/system.cpp)

**Propósito:** Módulo central que coordina la inicialización del sistema y gestiona el estado global.

#### Funciones Principales

**`System::init()`**
```cpp
void System::init()
```

Inicializa todos los subsistemas en orden de dependencias:

1. **Crear mutex de inicialización** (thread-safe)
2. **Verificar heap disponible** (mín. 50KB)
3. **Detectar hardware** (PSRAM, Flash, Chip ID)
4. **Boot guard** - Verificar contador de boots (bootloop detection)
5. **Storage** - EEPROM y SPIFFS
6. **I2C recovery** - Preparar bus I2C
7. **Sensores** - Inicializar drivers (INA226, DS18B20, etc.)
8. **Actuadores** - PCA9685, MCP23017, relés
9. **HUD** - Display y touch
10. **Audio** - DFPlayer Mini
11. **Safety systems** - ABS, TCS, obstacle detection
12. **Telemetry** - Sistema de logging

Uso de **mutex** garantiza que solo un thread pueda inicializar el sistema (evita race conditions en entornos FreeRTOS).

**`System::update()`**
```cpp
void System::update()
```

Loop principal del sistema. Ejecutado desde `loop()` en main.cpp cada ~10-20ms.

Secuencia:
1. **Feed watchdog** - Resetear timer de watchdog hardware
2. **Update managers** - PowerManager, SensorManager, SafetyManager, ControlManager, HUDManager
3. **Check transitions** - Evaluar cambios de estado (OFF → READY → RUNNING → LIMP)
4. **Log telemetry** - Capturar métricas cada N ciclos

**`System::setState()`**
```cpp
void System::setState(State newState)
```

Cambia el estado del sistema con validación de transiciones permitidas.

**State Machine:**
```
    OFF
     ↓
  STANDBY (KEY_ON detected)
     ↓
   READY (sistemas inicializados, pedal en reposo)
     ↓
  RUNNING (vehículo en movimiento)
     ↓
   LIMP (modo degradado por error no-crítico)
     ↓
 EMERGENCY (parada de emergencia, solo freno)
```

---

## 4.2 Boot Guard (`src/core/boot_guard.cpp`)

### Descripción
Sistema de protección contra bootloops y reinicios continuos. Detecta ciclos anormales de reinicio y activa automáticamente el modo seguro (safe mode) para permitir diagnóstico y recuperación.

### Funcionalidades Clave

#### Contador de Reinicios
```cpp
#define BOOTLOOP_THRESHOLD 3      // Reinicios consecutivos antes de safe mode
#define RESET_COUNTER_TIMEOUT 60000  // 60s sin reinicio resetea contador
```

- Contador persistente en RTC memory
- Se incrementa en cada boot
- Se resetea tras 60s de operación estable
- Límite de 3 reinicios consecutivos

#### Safe Mode Automático
```cpp
if (boot_count >= BOOTLOOP_THRESHOLD) {
    Serial.println("⚠️ BOOTLOOP DETECTED - ACTIVATING SAFE MODE");
    enterSafeMode();
}
```

**Características de Safe Mode:**
- Deshabilita HUD y renderizado
- Desactiva todos los actuadores (motores, relés)
- Solo mantiene comunicación serial
- LED de error parpadeante
- Permite reflash OTA

#### Diagnóstico de Causa de Reinicio
```cpp
esp_reset_reason_t reset_reason = esp_reset_reason();

switch (reset_reason) {
    case ESP_RST_POWERON:    // Power-on reset
    case ESP_RST_SW:         // Software reset
    case ESP_RST_PANIC:      // Exception/panic
    case ESP_RST_INT_WDT:    // Interrupt watchdog
    case ESP_RST_TASK_WDT:   // Task watchdog
    case ESP_RST_WDT:        // Other watchdog
    case ESP_RST_BROWNOUT:   // Brownout reset
    // ...
}
```

### Integración
```cpp
// En setup():
BootGuard::init();
if (BootGuard::isSafeMode()) {
    Serial.println("Running in SAFE MODE");
    // No inicializar periféricos críticos
    return;
}
```

---

## 4.3 Watchdog (`src/core/watchdog.cpp`)

### Descripción
Sistema de vigilancia hardware (TWDT - Task Watchdog Timer) que detecta bloqueos del sistema y fuerza un reinicio seguro si el firmware deja de responder.

### Configuración Hardware

```cpp
#define WDT_TIMEOUT 30  // 30 segundos timeout
esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT * 1000,
    .idle_core_mask = 0,  // No monitorear idle tasks
    .trigger_panic = true  // Forzar panic() en timeout
};
esp_task_wdt_init(&wdt_config);
```

### Feed Mechanism

**Alimentación Regular:**
```cpp
void loop() {
    Watchdog::feed();  // Llamar cada ciclo principal
    
    // Procesar tareas...
    
    if (millis() - lastFeed > 1000) {
        // Advertencia: feed demasiado lento
    }
}
```

**Tasks FreeRTOS:**
```cpp
void criticalTask(void* param) {
    esp_task_wdt_add(NULL);  // Registrar task
    
    while (1) {
        // Trabajo crítico...
        
        esp_task_wdt_reset();  // Feed desde task
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

### Recovery en Caso de Hang

1. **Timeout alcanzado (30s sin feed)**
   - Watchdog dispara interrupt
   - Captura backtrace de panic
   - Guarda estado en RTC memory
   - Ejecuta `esp_restart()`

2. **Post-reset**
   - BootGuard detecta `ESP_RST_TASK_WDT`
   - Incrementa contador de reinicios
   - Log de causa guardado

3. **Análisis post-mortem**
```bash
# Decodificar backtrace:
./decode_backtrace.sh
```

### Puntos de Feed Críticos
- `loop()` principal (cada iteración)
- `SafetyManager::update()` (cada 20ms)
- Tasks de control motor (cada 10ms)
- Renderizado HUD (cada frame)

---

## 4.4 I2C Recovery (`src/core/i2c_recovery.cpp`)

### Descripción
Sistema de recuperación automática del bus I2C cuando detecta condiciones de "bus stuck" (SCL/SDA bloqueados). Implementa el procedimiento estándar de recovery mediante clock toggling.

### Detección de Bus Stuck

```cpp
bool I2CRecovery::isBusStuck() {
    // Intentar comunicación con dispositivo conocido
    Wire.beginTransmission(TCA9548A_ADDR);
    uint8_t error = Wire.endTransmission();
    
    if (error == 2 || error == 5) {  // NACK o Timeout
        stuckCount++;
        if (stuckCount >= 3) {
            return true;  // Bus confirmado como stuck
        }
    }
    return false;
}
```

### Procedimiento de Recovery

#### Paso 1: Clock Toggling
```cpp
void I2CRecovery::clockToggle() {
    pinMode(I2C_SCL_PIN, OUTPUT);
    pinMode(I2C_SDA_PIN, INPUT_PULLUP);
    
    // Generar 9 pulsos de clock para liberar slave
    for (int i = 0; i < 9; i++) {
        digitalWrite(I2C_SCL_PIN, LOW);
        delayMicroseconds(5);
        digitalWrite(I2C_SCL_PIN, HIGH);
        delayMicroseconds(5);
        
        if (digitalRead(I2C_SDA_PIN) == HIGH) {
            break;  // SDA liberado
        }
    }
}
```

#### Paso 2: Stop Condition
```cpp
void I2CRecovery::forceStopCondition() {
    pinMode(I2C_SDA_PIN, OUTPUT);
    digitalWrite(I2C_SDA_PIN, LOW);
    delayMicroseconds(5);
    digitalWrite(I2C_SCL_PIN, HIGH);
    delayMicroseconds(5);
    digitalWrite(I2C_SDA_PIN, HIGH);  // Rising edge = STOP
    delayMicroseconds(5);
}
```

#### Paso 3: Reinicialización
```cpp
void I2CRecovery::reinitBus() {
    Wire.end();
    delay(10);
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ);
    delay(10);
}
```

### Re-scan de Dispositivos

```cpp
bool I2CRecovery::rescanDevices() {
    uint8_t devicesFound = 0;
    
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            devicesFound++;
            Serial.printf("✓ Device found at 0x%02X\n", addr);
        }
    }
    
    return (devicesFound >= expectedDeviceCount);
}
```

### Integración con Sistema

```cpp
void SafetyManager::update() {
    if (I2CRecovery::isBusStuck()) {
        Serial.println("⚠️ I2C BUS STUCK - ATTEMPTING RECOVERY");
        
        I2CRecovery::clockToggle();
        I2CRecovery::forceStopCondition();
        I2CRecovery::reinitBus();
        
        if (I2CRecovery::rescanDevices()) {
            Serial.println("✓ I2C Recovery successful");
        } else {
            System::enterLimpMode("I2C_BUS_FAILURE");
        }
    }
}
```

---

## 4.5 Storage (`src/core/storage.cpp`)

### Descripción
Sistema de almacenamiento persistente que maneja EEPROM virtual y sistema de archivos SPIFFS para configuración, calibración y datos de telemetría.

### EEPROM Persistence

#### Estructura de Datos
```cpp
#define EEPROM_SIZE 512

struct PersistentConfig {
    uint32_t magic;              // 0xCAFEBABE (validación)
    uint16_t version;            // Versión de estructura
    
    // Calibración
    float steeringCenterPWM;
    float steeringLeftMaxPWM;
    float steeringRightMaxPWM;
    
    float currentShuntResistor;  // INA226 calibration
    float voltageOffset;
    
    // Configuración
    bool absEnabled;
    bool tcsEnabled;
    uint8_t displayBrightness;
    
    // Estadísticas
    uint32_t totalRuntime;       // Segundos totales de operación
    uint32_t bootCount;
    
    uint16_t crc16;              // Checksum
};
```

#### Operaciones
```cpp
void Storage::save() {
    PersistentConfig cfg;
    cfg.magic = 0xCAFEBABE;
    cfg.version = CONFIG_VERSION;
    // ... llenar campos ...
    cfg.crc16 = calculateCRC16(&cfg, sizeof(cfg) - 2);
    
    EEPROM.put(0, cfg);
    EEPROM.commit();
}

bool Storage::load() {
    PersistentConfig cfg;
    EEPROM.get(0, cfg);
    
    if (cfg.magic != 0xCAFEBABE) {
        Serial.println("⚠️ Invalid EEPROM magic - loading defaults");
        return false;
    }
    
    uint16_t crc = calculateCRC16(&cfg, sizeof(cfg) - 2);
    if (crc != cfg.crc16) {
        Serial.println("⚠️ EEPROM CRC mismatch - corrupted data");
        return false;
    }
    
    // Aplicar configuración cargada
    applyConfig(&cfg);
    return true;
}
```

### SPIFFS File System

#### Inicialización
```cpp
void Storage::initSPIFFS() {
    if (!SPIFFS.begin(true)) {  // Format on fail
        Serial.println("⚠️ SPIFFS Mount Failed");
        return;
    }
    
    size_t total = SPIFFS.totalBytes();
    size_t used = SPIFFS.usedBytes();
    Serial.printf("SPIFFS: %d/%d bytes used\n", used, total);
}
```

#### Gestión de Archivos

**Logs de Telemetría:**
```cpp
void Storage::saveTelemetryLog() {
    File file = SPIFFS.open("/logs/session.csv", FILE_APPEND);
    if (file) {
        file.printf("%lu,%.2f,%.2f,%d,%d\n",
            millis(),
            currentSpeed,
            batteryVoltage,
            motorPWM,
            steeringAngle
        );
        file.close();
    }
}
```

**Perfiles de Conducción:**
```cpp
void Storage::saveProfile(const char* name) {
    char path[64];
    snprintf(path, sizeof(path), "/profiles/%s.json", name);
    
    File file = SPIFFS.open(path, FILE_WRITE);
    if (file) {
        file.print("{");
        file.printf("\"absThreshold\":%.2f,", absThreshold);
        file.printf("\"tcsSlipLimit\":%.2f,", tcsSlipLimit);
        file.printf("\"maxSpeed\":%.1f", maxSpeed);
        file.print("}");
        file.close();
    }
}
```

**Audio Assets:**
```
/data/audio/
  ├── startup.mp3
  ├── warning.mp3
  └── error.mp3
```

#### Mantenimiento
```cpp
void Storage::cleanupOldLogs() {
    File root = SPIFFS.open("/logs");
    File file = root.openNextFile();
    
    while (file) {
        if (getFileAge(file) > 7 * 86400) {  // >7 días
            SPIFFS.remove(file.name());
        }
        file = root.openNextFile();
    }
}
```

---

# 5. Módulos de Sensores

## 5.1 Current Sensor (`src/sensors/current.cpp`)

### Hardware
- **IC:** INA226 (I2C, 0x40-0x43)
- **Multiplexor:** TCA9548A (4 canales)
- **Rango:** ±81.92A (configurable)
- **Precisión:** ±0.1A

### Funcionalidad
```cpp
class CurrentSensor {
    float readCurrent(uint8_t channel);
    float readVoltage(uint8_t channel);
    float readPower(uint8_t channel);
};
```

Monitorea corriente de:
1. Motor tracción delantera izquierda
2. Motor tracción delantera derecha
3. Motor tracción trasera izquierda
4. Motor tracción trasera derecha

**Integración con TCS:**
Detecta diferencias de corriente entre ruedas → indica deslizamiento.

---

## 5.2 Temperature Sensor (`src/sensors/temperature.cpp`)

### Hardware
- **IC:** DS18B20 (OneWire, GPIO configurable)
- **Rango:** -55°C a +125°C
- **Resolución:** 0.0625°C (12-bit)

### Funcionalidad
```cpp
class TemperatureSensor {
    float readTemperature(uint8_t sensorIndex);
    uint8_t getDeviceCount();
};
```

Monitorea temperatura de:
- Controladores de motor (4x)
- Batería
- Habitáculo

**Protecciones:**
- >80°C: Reducción de potencia
- >95°C: Limp mode
- >105°C: Emergency shutdown

---

## 5.3 Wheel Sensors (`src/sensors/wheels.cpp`)

### Hardware
- **Tipo:** Sensores inductivos (NPN, hall-effect)
- **GPIOs:** 4x inputs con pull-up
- **Pulsos/revolución:** 20 (configurable)

### Funcionalidad
```cpp
class WheelSensor {
    float getRPM(uint8_t wheel);
    float getSpeed(uint8_t wheel);  // km/h
    uint32_t getPulseCount(uint8_t wheel);
};
```

**Cálculo de Velocidad:**
```cpp
float speed_kmh = (rpm * wheelCircumference * 60) / 1000;
```

**Integración con ABS/TCS:**
- Detecta bloqueo de rueda (RPM = 0 con acelerador activo)
- Detecta deslizamiento (RPM desbalanceado entre ruedas)

---

## 5.4 Obstacle Detection (`src/sensors/obstacle_detection.cpp`)

### Hardware
- **Sensor:** TOFSense LiDAR (UART, 115200 baud)
- **Rango:** 0.1m - 12m
- **Actualización:** 100Hz

### Funcionalidad
```cpp
class ObstacleDetection {
    float getDistance();           // Metros
    bool isObstacleDetected();
    uint8_t getSignalQuality();
};
```

**Protecciones:**
- <2.0m: Advertencia visual
- <1.0m: Advertencia sonora
- <0.5m: Reducción de velocidad (no implementado en ESP32)

⚠️ **Nota:** Detección solo informativa en HMI. Control activo se moverá a STM32.

---

# 6. Módulos de Control

## 6.1 Traction Control (`src/control/traction.cpp`)

### Hardware
- **PWM:** PCA9685 (16 canales, I2C 0x40)
- **GPIO Expansion:** MCP23017 (I2C 0x20)
- **Motores:** 4x DC brushed (PWM + DIR)

### Funcionalidad
```cpp
class TractionControl {
    void setMotorSpeed(uint8_t motor, int16_t speed);  // -255 a +255
    void brake(uint8_t motorMask);
    void emergencyStop();
};
```

**Distribución de Potencia:**
```cpp
// Tracción AWD (4x4)
setMotorSpeed(FRONT_LEFT,  baseSpeed + yawCorrection);
setMotorSpeed(FRONT_RIGHT, baseSpeed - yawCorrection);
setMotorSpeed(REAR_LEFT,   baseSpeed + yawCorrection);
setMotorSpeed(REAR_RIGHT,  baseSpeed - yawCorrection);
```

**Modos de Operación:**
- `MODE_2WD_FRONT`: Solo motores delanteros
- `MODE_2WD_REAR`: Solo motores traseros
- `MODE_AWD`: Tracción integral
- `MODE_TORQUE_VECTORING`: Distribución activa por rueda

---

## 6.2 Steering Motor Control (`src/control/steering_motor.cpp`)

### Hardware
- **Motor:** Servo de dirección (PWM 50Hz)
- **Sensor:** Potenciómetro de posición (ADC)

### Control PID
```cpp
class SteeringControl {
    float kp = 2.0;
    float ki = 0.1;
    float kd = 0.5;
    
    void setTargetAngle(float degrees);  // -45° a +45°
    void updatePID();
};
```

**Calibración:**
- Centro: 1500µs PWM
- Izquierda máxima: 1000µs (-45°)
- Derecha máxima: 2000µs (+45°)

**Límites de Seguridad:**
- Rate limiter: Máx. 90°/s cambio
- Angle limiter: ±45° absoluto
- Current limiter: <3A continua

---

## 6.3 TCS System (`src/control/tcs_system.cpp`)

### Descripción
Control de tracción anti-deslizamiento basado en comparación de velocidades de ruedas.

### Algoritmo
```cpp
void TCS::update() {
    float avgSpeed = (wheel[0] + wheel[1] + wheel[2] + wheel[3]) / 4.0;
    
    for (int i = 0; i < 4; i++) {
        float slip = (wheel[i] - avgSpeed) / avgSpeed;
        
        if (slip > TCS_THRESHOLD) {  // >10% deslizamiento
            reducePower(i, slip * TCS_GAIN);
        }
    }
}
```

**Parámetros:**
- `TCS_THRESHOLD`: 0.10 (10% slip)
- `TCS_GAIN`: 0.5 (reducción proporcional)
- `TCS_MIN_SPEED`: 5 km/h (no actuar a baja velocidad)

---

## 6.4 ABS System (`src/control/abs_system.cpp`)

### Descripción
Sistema antibloqueo de frenos basado en detección de parada súbita de rueda.

### Algoritmo
```cpp
void ABS::update() {
    for (int i = 0; i < 4; i++) {
        float deceleration = (lastSpeed[i] - wheel[i]) / dt;
        
        if (deceleration > ABS_THRESHOLD) {  // >5 m/s²
            releaseBrake(i);
            vTaskDelay(pdMS_TO_TICKS(50));  // 50ms release
            applyBrake(i);
        }
    }
}
```

**Parámetros:**
- `ABS_THRESHOLD`: 5.0 m/s² (deceleración límite)
- `ABS_CYCLE_TIME`: 50ms (ciclo de liberación/aplicación)
- `ABS_MIN_SPEED`: 10 km/h (no actuar a baja velocidad)

---

# 7. Sistema HUD

## 7.1 HUD Manager (`src/ui/hud_manager.cpp`)

### Descripción
Orquestador central del HMI. Gestiona actualización de datos, renderizado y compositor.

### Arquitectura
```cpp
class HUDManager {
    void init();
    void update();         // Actualizar datos (llamar cada loop)
    void render();         // Renderizar frame (llamar cada 16ms)
    
private:
    HUDCompositor compositor;
    GaugeManager gauges;
    IconManager icons;
};
```

**Ciclo de Actualización:**
```
1. Leer sensores (speed, rpm, temp, current)
2. Actualizar gauges (needles, bars)
3. Actualizar icons (warnings, indicators)
4. Compositor → layers → framebuffer
5. Display → push framebuffer (DMA)
```

---

## 7.2 HUD Core (`src/ui/hud.cpp`)

### Funcionalidad
Renderizado primitivo: líneas, círculos, texto, sprites.

```cpp
class HUD {
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawCircle(int16_t x, int16_t y, int16_t r, uint16_t color);
    void drawText(int16_t x, int16_t y, const char* text, uint16_t color);
    void drawBitmap(int16_t x, int16_t y, const uint16_t* bitmap, int16_t w, int16_t h);
};
```

**Optimizaciones:**
- DMA para transferencias SPI
- Double buffering (previene tearing)
- Clipping rectangle (solo renderizar área visible)

---

## 7.3 HUD Compositor (`src/ui/hud_compositor.cpp`)

### Descripción
Sistema de capas (layers) con soporte para transparencia y dirty rectangles.

### Layers
```cpp
enum Layer {
    LAYER_BACKGROUND,    // Fondo estático
    LAYER_GAUGES,        // Velocímetro, tacómetro
    LAYER_INDICATORS,    // ABS, TCS, warnings
    LAYER_OVERLAY,       // Menús, diagnóstico
    LAYER_COUNT
};
```

**Dirty Rectangle Optimization:**
Solo re-renderizar áreas que cambiaron desde último frame.

```cpp
void Compositor::markDirty(int16_t x, int16_t y, int16_t w, int16_t h) {
    dirtyRects.push({x, y, w, h});
}

void Compositor::compose() {
    for (const auto& rect : dirtyRects) {
        // Solo renderizar layers dentro de rect
        composeLayers(rect.x, rect.y, rect.w, rect.h);
    }
    dirtyRects.clear();
}
```

---

## 7.4 Gauges (`src/ui/gauges.cpp`)

### Tipos de Gauges
1. **Analog Gauge:** Velocímetro/tacómetro con aguja
2. **Bar Gauge:** Barra horizontal/vertical (batería, temp)
3. **Digital Gauge:** Valor numérico (odómetro, voltaje)

```cpp
class AnalogGauge {
    void setValue(float value);      // 0.0 - 1.0 (normalizado)
    void setRange(float min, float max);
    void setPosition(int16_t x, int16_t y, uint16_t radius);
};
```

**Animación de Aguja:**
```cpp
float smoothValue = lastValue + (targetValue - lastValue) * DAMPING;
float angle = map(smoothValue, minValue, maxValue, startAngle, endAngle);
drawNeedle(centerX, centerY, radius, angle);
```

---

## 7.5 Icons (`src/ui/icons.cpp`)

### Iconos Disponibles
- ABS (amarillo)
- TCS (amarillo)
- Battery Low (rojo)
- Temperature High (rojo)
- Check Engine (amarillo)
- Brake Warning (rojo)
- Limp Mode (ámbar)

```cpp
class IconManager {
    void showIcon(IconType type, bool state);
    void blinkIcon(IconType type, uint16_t period_ms);
};
```

**Formato de Iconos:**
- 32x32 píxeles
- RGB565 format
- Almacenados en PROGMEM

---

# 8. Seguridad

## 8.1 Safety Manager Orchestration

### Descripción
Módulo central que coordina todos los sistemas de seguridad y protección.

```cpp
class SafetyManager {
    void init();
    void update();  // Llamar cada 20ms (50Hz)
    
private:
    void checkEmergencyConditions();
    void checkWarningConditions();
    void monitorCriticalSensors();
};
```

### Jerarquía de Protecciones

**NIVEL 1 - Advertencias (Warning):**
- Batería <20%
- Temperatura >80°C
- Obstáculo <2.0m
→ Icono en HUD, advertencia sonora

**NIVEL 2 - Modo Degradado (Limp Mode):**
- Batería <10%
- Temperatura >95°C
- Sensor crítico desconectado
- Fallo en I2C
→ Reducción de velocidad a 30%, sin TCS/ABS

**NIVEL 3 - Emergencia (Emergency Stop):**
- Temperatura >105°C
- Cortocircuito detectado (corriente >50A)
- Watchdog timeout
- Fallo en dirección
→ Parada inmediata, solo freno disponible

---

## 8.2 ABS/TCS Integration

### Coordinación
```cpp
void SafetyManager::update() {
    // Actualizar sensores
    wheelsSpeed = WheelSensor::readAll();
    motorCurrent = CurrentSensor::readAll();
    
    // Ejecutar sistemas de seguridad
    if (absEnabled && brakeActive) {
        ABS::update(wheelsSpeed);
    }
    
    if (tcsEnabled && throttleActive) {
        TCS::update(wheelsSpeed, motorCurrent);
    }
}
```

### Estados Mutuamente Excluyentes
- ABS activo → TCS deshabilitado
- TCS activo → ABS deshabilitado
- Emergency stop → Ambos deshabilitados

---

## 8.3 Limp Mode Triggers

### Condiciones de Activación
```cpp
void SafetyManager::checkLimpMode() {
    if (batteryVoltage < LIMP_BATTERY_THRESHOLD) {
        enterLimpMode("LOW_BATTERY");
    }
    
    if (maxTemperature > LIMP_TEMP_THRESHOLD) {
        enterLimpMode("OVERHEAT");
    }
    
    if (!i2cHealthy) {
        enterLimpMode("I2C_FAILURE");
    }
    
    if (!steeringHealthy) {
        enterLimpMode("STEERING_FAULT");
    }
}
```

### Características de Limp Mode
- Velocidad máxima: 30 km/h
- Sin aceleración rápida (rate limiting)
- Solo tracción 2WD frontal
- ABS/TCS deshabilitados
- HUD muestra "LIMP MODE" parpadeante

**Salida de Limp Mode:**
- Manual (botón reset)
- Automática cuando condición se resuelve por >30s

---

# 19. GUÍA PARA MIGRACIÓN A STM32 ⭐

## 19.1 Visión de Arquitectura Dual

### Filosofía de Diseño
```
┌─────────────────────────────────────────────────────────────┐
│  ESP32-S3                         STM32G474RE               │
│  ═══════════                      ════════════               │
│  • HMI exclusivo                  • Control seguro          │
│  • Display + Touch                • Motores + Sensores      │
│  • Audio + LEDs                   • ABS/TCS real-time       │
│  • Menús + Diagnóstico            • Relés + Protecciones    │
│  • NO crítico                     • CRÍTICO                 │
└─────────────────────────────────────────────────────────────┘
         ↓                                    ↓
    UART/CAN (115200 baud)          High-speed I/O
    Telemetría + Comandos           PWM 20kHz + ADC 12-bit
```

### Separación de Responsabilidades

| Funcionalidad | ESP32-S3 | STM32G474RE | Razón |
|---------------|----------|-------------|-------|
| Display TFT   | ✅       | ❌          | SPI acoplado a HMI |
| Touch panel   | ✅       | ❌          | IRQ + I2C ligero |
| Audio DFPlayer| ✅       | ❌          | No crítico, UART simple |
| LEDs WS2812B  | ✅       | ❌          | RMT hardware en ESP32 |
| Menús UI      | ✅       | ❌          | Lógica de usuario |
| TOFSense LiDAR| ✅       | ❌          | Solo avisos visuales |
| Motores tracción | ❌    | ✅          | Control crítico |
| Dirección     | ❌       | ✅          | Seguridad vital |
| ABS/TCS       | ❌       | ✅          | Tiempo real <1ms |
| Frenos        | ❌       | ✅          | Seguridad vital |
| Sensores RPM  | ❌       | ✅          | Interrupciones rápidas |
| INA226 corriente | ❌   | ✅          | Protección sobrecorriente |
| Relés         | ❌       | ✅          | Encendido/apagado seguro |

---

## 19.2 Componentes que Quedan en ESP32

### Display TFT (ST7789V - 240x320 SPI)

**Razón para quedarse:**
- Bus SPI dedicado (VSPI)
- DMA hardware optimizado en ESP32
- No requiere tiempo real
- Framerate suficiente: 30-60 FPS

```cpp
// ESP32 mantiene:
#define TFT_CS    GPIO5
#define TFT_DC    GPIO16
#define TFT_RST   GPIO17
#define TFT_BL    GPIO4   // PWM brightness control

TFT_eSPI tft = TFT_eSPI();  // Hardware SPI
```

**Comunicación con STM32:**
```cpp
// ESP32 recibe telemetría vía UART
struct TelemetryPacket {
    float speed;        // km/h
    float rpm;          // RPM
    float batteryV;     // Voltaje
    float current[4];   // Corriente motores
    uint16_t status;    // Flags: ABS, TCS, warnings
};
```

---

### Touch Panel (XPT2046 - SPI)

**Razón para quedarse:**
- Mismo bus SPI que display
- Interrupt-driven (no polling)
- Latencia de UI no crítica (>50ms aceptable)

```cpp
// ESP32 mantiene:
#define TOUCH_CS   GPIO21
#define TOUCH_IRQ  GPIO22

XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);
```

**Integración con STM32:**
```cpp
// ESP32 envía comandos vía UART
void sendCommand(uint8_t cmd, uint16_t value) {
    uint8_t packet[4] = {0xAA, cmd, value >> 8, value & 0xFF};
    Serial2.write(packet, 4);
}

// Ejemplo: Usuario toca "ABS ON"
if (touchDetected(absButtonRect)) {
    sendCommand(CMD_ABS_ENABLE, 1);
}
```

---

### Audio DFPlayer (UART)

**Razón para quedarse:**
- Función de feedback de usuario, no crítica
- UART simple (9600 baud)
- No afecta seguridad

```cpp
// ESP32 mantiene:
#define DFPLAYER_TX GPIO26
#define DFPLAYER_RX GPIO27

DFPlayer player(Serial1);

// Reproducir advertencias basadas en telemetría STM32
if (telemetry.status & STATUS_OVERHEAT) {
    player.play(SOUND_WARNING_OVERHEAT);
}
```

---

### LEDs WS2812B (Iluminación)

**Razón para quedarse:**
- RMT peripheral hardware en ESP32 (perfecto para WS2812B)
- Efectos de iluminación no críticos
- Reduce carga en STM32

```cpp
// ESP32 mantiene:
#define LED_PIN    GPIO25
#define NUM_LEDS   12

Adafruit_NeoPixel leds(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Efectos basados en estado del vehículo
if (telemetry.status & STATUS_LIMP_MODE) {
    blinkLEDs(COLOR_AMBER, 500);  // Ámbar parpadeante
}
```

---

### Menús y Diagnóstico

**Razón para quedarse:**
- Lógica de UI compleja (árboles de menú, scroll)
- Acceso a SPIFFS para logs
- WiFi para OTA updates (solo en modo parado)

```cpp
// ESP32 mantiene:
Menu mainMenu[] = {
    {"Configuración", &configMenu},
    {"Diagnóstico", &diagMenu},
    {"Calibración", &calibMenu},
    {"Información", &infoMenu}
};

// Diagnóstico puede leer logs de STM32 vía UART
void showDiagnostics() {
    sendCommand(CMD_REQUEST_ERROR_LOG, 0);
    // STM32 responde con historial de errores
}
```

---

### TOFSense LiDAR (Detección Obstáculos)

**Razón para quedarse:**
- Función **informativa** únicamente
- No afecta control de motores
- Avisos visuales en HUD

```cpp
// ESP32 mantiene:
#define LIDAR_TX GPIO32
#define LIDAR_RX GPIO33

TOFSense lidar(Serial2);

// Solo mostrar advertencia visual
void updateLiDAR() {
    float distance = lidar.readDistance();
    
    if (distance < 2.0) {
        hud.showWarning("OBSTACLE AHEAD", COLOR_RED);
        player.play(SOUND_PROXIMITY_WARNING);
    }
}
```

⚠️ **IMPORTANTE:** Si en el futuro se desea frenado automático, el LiDAR debe moverse a STM32.

---

## 19.3 Componentes que se Mueven a STM32

### Motores de Tracción (PWM + GPIO)

**Razón para moverse:**
- Control en tiempo real (<1ms latency)
- Safety-critical (fallo = pérdida de control)
- PWM de alta frecuencia (20kHz+)

```cpp
// STM32 tendrá:
#define MOTOR_FL_PWM  TIM1_CH1   // Timer 1 Channel 1
#define MOTOR_FR_PWM  TIM1_CH2
#define MOTOR_RL_PWM  TIM1_CH3
#define MOTOR_RR_PWM  TIM1_CH4

// GPIO para dirección (H-bridge)
#define MOTOR_FL_DIR  GPIO_PIN_0
// ...

// Configuración: 20kHz PWM, 12-bit resolution
TIM_HandleTypeDef htim1;
htim1.Init.Prescaler = 0;
htim1.Init.Period = 4095;  // 12-bit
```

---

### Motor de Dirección + PID

**Razón para moverse:**
- Safety-critical (fallo = pérdida de dirección)
- PID de alta frecuencia (1kHz+)
- Feedback de potenciómetro (ADC)

```cpp
// STM32 tendrá:
#define STEERING_PWM    TIM2_CH1
#define STEERING_ADC    ADC1_IN1
#define STEERING_DIR    GPIO_PIN_1

// PID loop a 1kHz
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim == &htim3) {  // 1kHz timer
        float error = targetAngle - currentAngle;
        float output = pidController.compute(error);
        setSteeringPWM(output);
    }
}
```

---

### ABS/TCS (Control Tiempo Real)

**Razón para moverse:**
- Requiere latencia <1ms (vs ESP32 ~10ms)
- FreeRTOS en STM32 más determinista
- Acceso directo a sensores RPM (interrupciones)

```cpp
// STM32 tendrá:
void ABS_Task(void *argument) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        // Ejecutar cada 1ms (1kHz)
        abs_update();
        tcs_update();
        
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1));
    }
}
```

---

### Sensores RPM (Interrupciones)

**Razón para moverse:**
- Interrupciones de alta frecuencia
- Cálculo de velocidad preciso
- Entrada directa para ABS/TCS

```cpp
// STM32 tendrá:
#define WHEEL_FL_PIN  GPIO_PIN_2
#define WHEEL_FR_PIN  GPIO_PIN_3
#define WHEEL_RL_PIN  GPIO_PIN_4
#define WHEEL_RR_PIN  GPIO_PIN_5

// External interrupt
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    uint32_t now = micros();
    
    switch (GPIO_Pin) {
        case WHEEL_FL_PIN:
            wheelPulses[0]++;
            wheelPeriod[0] = now - wheelLastTime[0];
            wheelLastTime[0] = now;
            break;
        // ...
    }
}
```

---

### INA226 (Sensores de Corriente)

**Razón para moverse:**
- Protección contra sobrecorriente (safety-critical)
- Detección de cortocircuitos <10ms
- Entrada para TCS (detección de deslizamiento)

```cpp
// STM32 tendrá:
void currentMonitorTask(void *argument) {
    while (1) {
        for (int i = 0; i < 4; i++) {
            float current = ina226_readCurrent(i);
            
            if (current > CURRENT_LIMIT) {
                // Apagar motor inmediatamente
                motorDisable(i);
                setErrorFlag(ERROR_OVERCURRENT);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));  // 100Hz
    }
}
```

---

### Relés (Encendido/Apagado)

**Razón para moverse:**
- Control de energía crítico
- Secuencia de encendido segura
- Protección de hardware

```cpp
// STM32 tendrá:
#define RELAY_MAIN      GPIO_PIN_10
#define RELAY_MOTORS    GPIO_PIN_11
#define RELAY_PERIPH    GPIO_PIN_12

void powerSequence() {
    HAL_GPIO_WritePin(GPIOA, RELAY_MAIN, GPIO_PIN_SET);
    HAL_Delay(100);  // Wait for caps to charge
    
    HAL_GPIO_WritePin(GPIOA, RELAY_PERIPH, GPIO_PIN_SET);
    HAL_Delay(50);
    
    HAL_GPIO_WritePin(GPIOA, RELAY_MOTORS, GPIO_PIN_SET);
}
```

---

## 19.4 Protocolo de Comunicación ESP32 ↔ STM32

### Hardware
```
ESP32 UART2 ←→ STM32 USART1
   TX (GPIO17) ─── RX (PA10)
   RX (GPIO16) ─── TX (PA9)
   GND ────────────── GND
```

**Parámetros:**
- Baudrate: 115200 baud
- 8N1 (8 data bits, no parity, 1 stop bit)
- Flow control: None

### Formato de Paquetes

#### ESP32 → STM32 (Comandos)
```cpp
struct CommandPacket {
    uint8_t header;      // 0xAA
    uint8_t cmd;         // Comando (ver enum)
    uint16_t value;      // Parámetro
    uint8_t checksum;    // XOR de bytes
};

enum Command {
    CMD_ABS_ENABLE      = 0x01,
    CMD_TCS_ENABLE      = 0x02,
    CMD_SET_MAX_SPEED   = 0x03,
    CMD_SET_STEERING    = 0x04,
    CMD_SET_THROTTLE    = 0x05,
    CMD_EMERGENCY_STOP  = 0xFF
};
```

#### STM32 → ESP32 (Telemetría)
```cpp
struct TelemetryPacket {
    uint8_t header;           // 0xBB
    float speed;              // km/h
    float rpm;                // RPM medio
    float batteryVoltage;     // V
    float current[4];         // A (FL, FR, RL, RR)
    float temperature;        // °C
    uint16_t statusFlags;     // Ver enum StatusFlags
    uint8_t checksum;
};

enum StatusFlags {
    STATUS_ABS_ACTIVE     = 0x0001,
    STATUS_TCS_ACTIVE     = 0x0002,
    STATUS_OVERHEAT       = 0x0004,
    STATUS_LOW_BATTERY    = 0x0008,
    STATUS_LIMP_MODE      = 0x0010,
    STATUS_EMERGENCY      = 0x0020
};
```

### Frecuencia de Actualización
- **Telemetría:** 50Hz (cada 20ms)
- **Comandos:** On-demand (cuando usuario interactúa)

---

## 19.5 Cronograma de Migración

### Fase 1: Preparación (2 semanas)
- [ ] Adquisición de placa STM32G474RE Nucleo
- [ ] Setup de STM32CubeIDE + HAL
- [ ] Prototipo básico de UART ESP32↔STM32

### Fase 2: Migración de Sensores (3 semanas)
- [ ] Portar lectura de sensores RPM (EXTI)
- [ ] Portar INA226 (I2C)
- [ ] Portar DS18B20 (OneWire vía GPIO bitbang)
- [ ] Validar telemetría en serial

### Fase 3: Migración de Actuadores (3 semanas)
- [ ] Configurar PWM de motores (TIM1)
- [ ] Configurar PWM de dirección (TIM2)
- [ ] Implementar control de relés
- [ ] Validar comandos desde ESP32

### Fase 4: Migración de ABS/TCS (4 semanas)
- [ ] Portar algoritmo de ABS
- [ ] Portar algoritmo de TCS
- [ ] Tuning de parámetros PID
- [ ] Testing exhaustivo de seguridad

### Fase 5: Integración Final (2 semanas)
- [ ] Testing completo de sistema dual
- [ ] Validación de latencias
- [ ] Documentación final
- [ ] Certificación de seguridad

**TOTAL:** ~14 semanas (3.5 meses)

---

## 19.6 Checklist de Validación Post-Migración

### Funcionalidad
- [ ] Display muestra telemetría correcta (±5% error)
- [ ] Touch responde a comandos (<100ms latency)
- [ ] Audio reproduce advertencias correctamente
- [ ] LEDs reflejan estado del vehículo
- [ ] Motores responden a acelerador (<10ms latency)
- [ ] Dirección responde a volante (<5ms latency)
- [ ] ABS previene bloqueo de ruedas (test en banco)
- [ ] TCS previene deslizamiento (test en banco)

### Seguridad
- [ ] Emergency stop funciona en <50ms
- [ ] Watchdog STM32 reinicia en caso de hang
- [ ] Protección de sobrecorriente dispara en <10ms
- [ ] Protección de sobretemperatura activa limp mode
- [ ] Fallo de UART no causa pérdida de control (STM32 autonomous)

### Performance
- [ ] Telemetría actualiza a 50Hz constante
- [ ] PID de dirección loop a 1kHz
- [ ] ABS/TCS loop a 1kHz
- [ ] Latencia de comando ESP32→STM32 <20ms

---

## 20. Construcción y Deployment

### 20.1 Requisitos de Construcción

**Software necesario:**
- PlatformIO Core 6.1.0+ o PlatformIO IDE
- Python 3.7+ (para scripts de validación)
- Git (para clonar repositorio)

**Sistema operativo:**
- Linux (Ubuntu 20.04+ recomendado)
- macOS 11+
- Windows 10+ (con WSL2 recomendado)

### 20.2 Compilación

```bash
# Clonar repositorio
git clone https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos.git
cd FULL-FIRMWARE-Coche-Marcos

# Instalar dependencias Python
pip install -r requirements.txt

# Compilar firmware
pio run -e esp32-s3-devkitc1-n16r8

# Compilar con información de debug
pio run -e esp32-s3-devkitc1-n16r8 -v
```

### 20.3 Upload

**Via USB (método recomendado):**
```bash
# Conectar ESP32-S3 via USB
# Presionar BOOT mientras presionas RESET para entrar en modo download
pio run -e esp32-s3-devkitc1-n16r8 -t upload

# Monitor serial después de upload
pio device monitor -e esp32-s3-devkitc1-n16r8
```

**Upload + Monitor en un comando:**
```bash
pio run -e esp32-s3-devkitc1-n16r8 -t upload && pio device monitor
```

**Borrado completo de flash (troubleshooting):**
```bash
pio run -e esp32-s3-devkitc1-n16r8 -t erase
```

### 20.4 OTA (Over-The-Air)

**Preparación:**
1. Compilar firmware: `pio run -e esp32-s3-devkitc1-n16r8`
2. Binario generado en: `.pio/build/esp32-s3-devkitc1-n16r8/firmware.bin`

**OTA via Web UI** (futuro):
- Configurar WiFi en ESP32-S3
- Acceder a `http://<IP_ESP32>/update`
- Subir `firmware.bin`

**⚠️ IMPORTANTE:** OTA requiere ~8MB libres en partición app alterna.

### 20.5 Validación Post-Upload

**Script de validación automática:**
```bash
./tools/validate_boot.sh
```

**Checklist manual:**
1. ✅ Boot exitoso (no bootloop)
2. ✅ PSRAM detectado (8MB)
3. ✅ Flash detectado (16MB)
4. ✅ Display inicializado
5. ✅ I2C bus operativo (7 dispositivos)
6. ✅ UART audio/LiDAR funcional
7. ✅ Sensores leyendo datos válidos

### 20.6 Logs y Diagnóstico

**Niveles de log:**
```cpp
#define CORE_DEBUG_LEVEL 5  // 0=None, 5=Verbose
```

**Decodificar stacktrace:**
```bash
# Si hay crash, copiar backtrace del serial monitor
./decode_backtrace.sh "backtrace_aqui"
```

---

## 21. Troubleshooting y Diagnóstico

### 21.1 Boot Failures

#### Problema: Bootloop (reinicio continuo)

**Síntomas:**
```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0xc (RTC_SW_CPU_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
...
rst:0xc (RTC_SW_CPU_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
```

**Causas comunes:**
1. Configuración PSRAM incorrecta (`memory_type != qio_opi`)
2. Conflicto GPIO con Flash/PSRAM (GPIOs 10-12, 26-37)
3. Stack overflow en inicialización
4. Watchdog timeout en boot

**Soluciones:**
```bash
# 1. Verificar configuración
cat platformio.ini | grep memory_type
# Debe ser: board_build.arduino.memory_type = qio_opi

# 2. Borrar flash completamente
pio run -t erase

# 3. Re-flash con monitor
pio run -t upload && pio device monitor

# 4. Verificar boot guard counter
# En serial monitor, buscar: [BOOT_GUARD] Boot count: X
# Si X > 5, sistema entra en safe mode
```

**Boot Guard Safe Mode:**
Si el contador de boots consecutivos > 5 en <60 segundos, el firmware entra en **Safe Mode**:
- Deshabilita TFT rendering (solo logs serial)
- Deshabilita audio
- Deshabilita sensores no críticos
- Permite diagnóstico via UART

Para resetear contador:
```cpp
// Presionar botón hidden en menu
// O via código:
BootGuard::resetBootCount();
```

#### Problema: PSRAM No Detectado

**Síntomas:**
```
[BOOT] ❌ PSRAM INIT FAILED
[BOOT] PSRAM size: 0 bytes
```

**Verificación:**
1. Hardware: Confirmar que es N16R8 (no N8R8 o N16)
2. SDKConfig: Verificar `sdkconfig/n16r8.defaults`
3. Voltage: PSRAM requiere 3.3V estable (medir con multímetro)

**Fix:**
```bash
# Re-generar sdkconfig
rm -rf .pio/build/*/sdkconfig*
pio run -e esp32-s3-devkitc1-n16r8 -t clean
pio run -e esp32-s3-devkitc1-n16r8
```

### 21.2 Display Issues

#### Problema: Pantalla Blanca/Negra

**Checklist:**
1. Backlight ON: `digitalWrite(PIN_TFT_BL, HIGH)`
2. Conexiones SPI: Verificar cables GPIO 13-17
3. Voltaje TFT: 3.3V o 5V según modelo
4. Reset TFT: Secuencia LOW→delay(100ms)→HIGH

**Test manual:**
```cpp
// En setup(), después de tft.init()
tft.fillScreen(TFT_RED);  // Pantalla roja
delay(1000);
tft.fillScreen(TFT_GREEN);  // Pantalla verde
```

#### Problema: Touch No Responde

**Verificar:**
1. Interrupción: `pinMode(PIN_TOUCH_IRQ, INPUT_PULLUP)`
2. Calibración: Ejecutar `TouchCalibration::calibrate()`
3. SPI Conflict: Touch y TFT comparten SPI, usar CS correctamente

**Test de touch:**
```cpp
uint16_t x, y;
if (touch.getTouch(&x, &y)) {
  Serial.printf("Touch: x=%d, y=%d\n", x, y);
}
```

### 21.3 I2C Communication Errors

#### Problema: Dispositivo I2C No Responde

**Diagnóstico:**
```cpp
// Scan I2C bus
Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 400000);
for (uint8_t addr = 1; addr < 127; addr++) {
  Wire.beginTransmission(addr);
  if (Wire.endTransmission() == 0) {
    Serial.printf("Found device at 0x%02X\n", addr);
  }
}
```

**Dispositivos esperados:**
- 0x20: MCP23017
- 0x40: PCA9685 #1 (Front motors)
- 0x41: PCA9685 #2 (Rear motors)
- 0x42: PCA9685 #3 (Steering)
- 0x70: TCA9548A

**Bus Stuck Recovery:**
Si el bus I2C está "stuck" (SDA LOW permanente):
```cpp
I2CRecovery::recoverBus();  // Automático en system.cpp
```

Procedimiento manual:
1. Configurar SCL como OUTPUT
2. Generar 9 pulsos de clock
3. Verificar que SDA vuelve a HIGH
4. Re-inicializar bus I2C

### 21.4 Motor Control Issues

#### Problema: Motores No Responden

**Checklist:**
1. **Relés energizados:** Verificar GPIO 5 (RELAY_TRAC) = HIGH
2. **PWM funcionando:** Usar osciloscopio en salidas PCA9685
3. **Dirección configurada:** MCP23017 GPIOA0-7 configurados
4. **Voltaje potencia:** 24V presente en BTS7960
5. **BTS7960 Enable:** L_EN y R_EN = HIGH

**Test manual de motor FL:**
```cpp
// En loop() temporal
PCA9685_Front.setPWM(0, 0, 2048);  // 50% duty, forward
MCP23017.digitalWrite(GPIOA0, HIGH);  // IN1 = HIGH
MCP23017.digitalWrite(GPIOA1, LOW);   // IN2 = LOW
delay(2000);
PCA9685_Front.setPWM(0, 0, 0);  // Stop
```

#### Problema: Motor Gira en Dirección Incorrecta

**Solución:**
Invertir cables M+ y M- del motor, O invertir IN1/IN2 en código:
```cpp
// Antes:
MCP23017.digitalWrite(IN1, HIGH);
MCP23017.digitalWrite(IN2, LOW);

// Después:
MCP23017.digitalWrite(IN1, LOW);
MCP23017.digitalWrite(IN2, HIGH);
```

### 21.5 Sensor Failures

#### Problema: INA226 Lee 0A/0V

**Verificación:**
1. TCA9548A canal correcto seleccionado
2. INA226 dirección 0x40 (todas usan misma dirección)
3. Shunt conectado en serie con carga
4. Calibración INA226 correcta para shunt usado

**Test:**
```cpp
TCA9548A.selectChannel(4);  // Canal batería
INA226 ina;
ina.begin(0x40);
ina.setShunt(0.00075, 100.0);  // 75mV, 100A shunt
float current = ina.getCurrent_mA();
Serial.printf("Current: %.2f mA\n", current);
```

#### Problema: DS18B20 No Detectado

**Diagnóstico:**
```cpp
OneWire oneWire(PIN_ONEWIRE);
DallasTemperature sensors(&oneWire);
sensors.begin();
int deviceCount = sensors.getDeviceCount();
Serial.printf("Found %d DS18B20 sensors\n", deviceCount);
```

**Causas:**
- Pull-up 4.7kΩ faltante en GPIO 20
- Sensor dañado o mal contacto
- Conflicto de ROM address (poco común)

### 21.6 Memory Issues

#### Problema: Heap Agotado

**Síntomas:**
```
[ERROR] Failed to allocate 307200 bytes
[ERROR] Heap low: 15234 bytes free
```

**Diagnóstico:**
```cpp
Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
Serial.printf("Free PSRAM: %u bytes\n", ESP.getFreePsram());
Serial.printf("Largest free block: %u bytes\n", heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
```

**Soluciones:**
1. Mover buffers grandes a PSRAM: `ps_malloc()` en lugar de `malloc()`
2. Liberar sprites no usados
3. Reducir tamaño de buffers (ej. display shadow buffer)
4. Deshabilitar features no esenciales (menús, animaciones)

#### Problema: Stack Overflow

**Síntomas:**
```
***ERROR*** A stack overflow in task Core1 has been detected.
Backtrace: 0x...
```

**Solución:**
Aumentar stack size en FreeRTOS tasks:
```cpp
// En rtos_tasks.cpp
xTaskCreatePinnedToCore(
    taskSensors,
    "Sensors",
    8192,  // Stack size (aumentar si hay overflow)
    NULL,
    1,     // Priority
    &taskHandleSensors,
    1      // Core
);
```

### 21.7 Comandos de Diagnóstico Serial

**Durante ejecución, vía Serial Monitor (115200 baud):**

```
help          - Mostrar comandos disponibles
status        - Estado del sistema
sensors       - Lectura de todos los sensores
i2c_scan      - Escanear bus I2C
mem           - Estado de memoria (heap/PSRAM)
reset         - Reiniciar ESP32
safe_mode     - Entrar en Safe Mode
boot_count    - Ver contador de boots
clear_eeprom  - Borrar configuración EEPROM
calibrate     - Iniciar calibración (touch/pedal/encoder)
test_motor <FL|FR|RL|RR> <speed> - Test motor individual
test_audio <track>               - Reproducir track de audio
test_leds <mode>                 - Test LEDs (solid/rainbow/flash)
```

---

## 22. Referencias y Documentación Adicional

### 22.1 Documentos del Repositorio

#### Arquitectura y Diseño
- **docs/ARCHITECTURE.md** - Arquitectura general del firmware
- **docs/FREERTOS_ARCHITECTURE_v2.18.0.md** - Estructura multitarea FreeRTOS
- **docs/MEMORY_OPTIMIZATION_REPORT_v2.18.0.md** - Optimización de memoria

#### Hardware
- **HARDWARE.md** - Especificación oficial ESP32-S3 N16R8
- **GPIO_ASSIGNMENT_LIST.md** - Asignación completa de pines
- **docs/CONEXIONES_HARDWARE_v2.15.0.md** - Esquemas de conexión detallados
- **docs/PIN_MAPPING_DEVKITC1.md** - Mapeo de pines DevKitC-1

#### Migración STM32
- **RESPUESTA_TRANSRECEPTORES.md** - Guía de transreceptores CAN
- **docs/MANUAL_TRANSRECEPTORES_STM32_ESP32.md** - Manual completo TJA1051T/3
- **docs/PLAN_SEPARACION_STM32_CAN.md** - Plan de arquitectura dual
- **docs/STM32G474RE_PINOUT_DEFINITIVO.md** - Pinout completo STM32
- **docs/DESIGN_FREEZE_STM32G474RE.md** - Design freeze hardware
- **docs/STM32_CAN_MIGRATION_STUDY.md** - Estudio de migración

#### Boot y Diagnóstico
- **PHASE14_N16R8_BOOT_CERTIFICATION.md** - Certificación boot N16R8
- **BOOTLOOP_FIX_FINAL_v2.17.3.md** - Fix de bootloop
- **docs/BOOT_INITIALIZATION_AUDIT.md** - Auditoría secuencia boot

#### Display y UI
- **docs/RENDERING_FORENSIC_AUDIT_2026-01-10.md** - Auditoría rendering
- **PHASE8_DIRTY_RECT_IMPLEMENTATION.md** - Dirty rectangles optimization
- **docs/DISPLAY_DRIVER_EXPLANATION.md** - Explicación driver display

#### Audio
- **docs/AUDIO_TRACKS_GUIDE.md** - Guía de tracks de audio
- **docs/AUDIO_IMPLEMENTATION_SUMMARY.md** - Implementación sistema audio

#### Seguridad
- **COMPREHENSIVE_SECURITY_AUDIT_2026-01-08.md** - Auditoría de seguridad
- **docs/COORDINATE_SAFETY_QUICK_REFERENCE.md** - Referencia seguridad

#### Construcción
- **BUILD_INSTRUCTIONS_v2.11.0.md** - Instrucciones de compilación
- **INICIO_RAPIDO.md** - Guía de inicio rápido
- **GUIA_RAPIDA.md** - Guía rápida de usuario

### 22.2 Datasheets Externos

#### Microcontrolador
- [ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)

#### Drivers de Motor
- [BTS7960 Datasheet](https://www.infineon.com/dgdl/Infineon-BTS7960-DS-v01_00-EN.pdf?fileId=db3a30433fa9412f013fbe32289b7c17)
- [PCA9685 16-Channel PWM Driver](https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf)

#### Expansores I/O
- [MCP23017 16-bit I/O Expander](https://ww1.microchip.com/downloads/en/devicedoc/20001952c.pdf)
- [TCA9548A I2C Multiplexer](https://www.ti.com/lit/ds/symlink/tca9548a.pdf)

#### Sensores
- [INA226 Current/Voltage Monitor](https://www.ti.com/lit/ds/symlink/ina226.pdf)
- [DS18B20 Temperature Sensor](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf)
- [LJ12A3-4-Z/BX Inductive Sensor](https://www.fotek.com.hk/sensor/LJ.htm)
- [E6B2-CWZ6C Encoder](https://www.ia.omron.com/products/family/487/)

#### Display
- [ST7796S Display Controller](https://www.displayfuture.com/Display/datasheet/controller/ST7796s.pdf)
- [XPT2046 Touch Controller](http://www.vlsitechnology.org/pharosc/download.php?page_id=21)

#### Audio
- [DFPlayer Mini Manual](https://wiki.dfrobot.com/DFPlayer_Mini_SKU_DFR0299)

#### LEDs
- [WS2812B Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf)

#### CAN Transceiver (Migración STM32)
- [TJA1051T/3 High-Speed CAN Transceiver](https://www.nxp.com/docs/en/data-sheet/TJA1051.pdf)

### 22.3 Librerías Utilizadas

| Librería | Repositorio | Licencia |
|----------|-------------|----------|
| Arduino-ESP32 | [espressif/arduino-esp32](https://github.com/espressif/arduino-esp32) | LGPL 2.1 |
| TFT_eSPI | [Bodmer/TFT_eSPI](https://github.com/Bodmer/TFT_eSPI) | FreeBSD |
| FastLED | [FastLED/FastLED](https://github.com/FastLED/FastLED) | MIT |
| Adafruit MCP23017 | [adafruit/Adafruit-MCP23017-Arduino-Library](https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library) | BSD |
| Adafruit PWM Servo | [adafruit/Adafruit-PWM-Servo-Driver-Library](https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library) | BSD |
| DFRobotDFPlayerMini | [DFRobot/DFRobotDFPlayerMini](https://github.com/DFRobot/DFRobotDFPlayerMini) | MIT |
| DallasTemperature | [milesburton/Arduino-Temperature-Control-Library](https://github.com/milesburton/Arduino-Temperature-Control-Library) | LGPL |
| OneWire | [PaulStoffregen/OneWire](https://github.com/PaulStoffregen/OneWire) | MIT |
| INA226 | [RobTillaart/INA226](https://github.com/RobTillaart/INA226) | MIT |

### 22.4 Herramientas de Desarrollo

- **PlatformIO:** [platformio.org](https://platformio.org/)
- **ESP-IDF:** [docs.espressif.com/projects/esp-idf](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
- **esptool.py:** [github.com/espressif/esptool](https://github.com/espressif/esptool)
- **Exception Decoder:** [github.com/me-no-dev/EspExceptionDecoder](https://github.com/me-no-dev/EspExceptionDecoder)

### 22.5 Comunidad y Soporte

- **GitHub Issues:** [FULL-FIRMWARE-Coche-Marcos/issues](https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos/issues)
- **ESP32 Forum:** [esp32.com](https://esp32.com/)
- **PlatformIO Community:** [community.platformio.org](https://community.platformio.org/)

### 22.6 Versiones del Documento

| Versión | Fecha | Cambios |
|---------|-------|---------|
| 1.0 | 2026-02-01 | Versión inicial completa del manual técnico |

---

## Apéndice A: Glosario de Términos

| Término | Definición |
|---------|------------|
| **ABS** | Anti-lock Braking System - Sistema que previene bloqueo de ruedas en frenado |
| **TCS** | Traction Control System - Sistema que previene deslizamiento de ruedas en aceleración |
| **HMI** | Human-Machine Interface - Interfaz entre operador humano y máquina |
| **OPI** | Octal Peripheral Interface - Interfaz PSRAM de 8 bits de datos |
| **QIO** | Quad I/O - Modo SPI con 4 líneas de datos |
| **PWM** | Pulse Width Modulation - Modulación por ancho de pulso |
| **PPR** | Pulses Per Revolution - Pulsos por revolución (encoder) |
| **PSRAM** | Pseudo Static RAM - RAM externa de alta velocidad |
| **SPI** | Serial Peripheral Interface - Bus serial sincrónico |
| **I2C** | Inter-Integrated Circuit - Bus serial de 2 cables |
| **UART** | Universal Asynchronous Receiver-Transmitter - Comunicación serial asíncrona |
| **ADC** | Analog-to-Digital Converter - Conversor analógico-digital |
| **GPIO** | General Purpose Input/Output - Pin de entrada/salida de propósito general |
| **CAN** | Controller Area Network - Bus de comunicación automotriz |
| **OTA** | Over-The-Air - Actualización remota de firmware |
| **SPIFFS** | SPI Flash File System - Sistema de archivos en flash |
| **NVS** | Non-Volatile Storage - Almacenamiento no volátil key-value |
| **FreeRTOS** | Real-Time Operating System - Sistema operativo de tiempo real |
| **Watchdog** | Timer que resetea sistema si no se alimenta periódicamente |
| **Bootloop** | Reinicio continuo del sistema sin completar boot |
| **Limp Mode** | Modo degradado de operación con funcionalidad limitada |
| **Shadow Rendering** | Renderizado en buffer secundario antes de mostrar en pantalla |

## Apéndice B: Códigos de Error

Ver archivo completo en: `docs/CODIGOS_ERROR.md`

### Categorías de Errores

| Código | Categoría | Severidad |
|--------|-----------|-----------|
| E001-E099 | Sistema | Crítico |
| E100-E199 | Sensores | Medio |
| E200-E299 | Actuadores | Alto |
| E300-E399 | Comunicación | Medio |
| E400-E499 | HMI | Bajo |
| E500-E599 | Seguridad | Crítico |

### Errores Más Comunes

| Código | Descripción | Acción Inmediata |
|--------|-------------|------------------|
| E001 | Fallo inicialización sistema | Reiniciar ESP32 |
| E002 | Watchdog timeout | Verificar loop principal |
| E003 | Stack overflow | Aumentar stack size |
| E101 | INA226 no responde | Verificar I2C bus |
| E102 | DS18B20 no detectado | Verificar pull-up OneWire |
| E103 | Encoder sin señal | Verificar conexiones encoder |
| E201 | Motor no responde | Verificar relés y BTS7960 |
| E202 | Dirección bloqueada | Verificar encoder y motor dirección |
| E301 | I2C bus stuck | Ejecutar I2C recovery |
| E302 | UART timeout | Verificar conexión DFPlayer/LiDAR |
| E501 | ABS fault | Detener vehículo, diagnóstico |
| E502 | TCS fault | Limitar potencia, diagnóstico |
| E503 | Obstacle critical | Detención automática |

---

**FIN DEL MANUAL TÉCNICO COMPLETO DEL FIRMWARE**

**Versión:** 1.0  
**Fecha:** 2026-02-01  
**Firmware:** v2.18.3+  
**Hardware:** ESP32-S3 N16R8  

*Este documento es una guía técnica completa para desarrollo, mantenimiento y migración del firmware del vehículo eléctrico. Para actualizaciones, consultar el repositorio GitHub.*

**Repositorio:** [github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos](https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos)

---

© 2026 - Proyecto FULL-FIRMWARE-Coche-Marcos - Documentación Técnica
*Para consultas técnicas: consultar repositorio GitHub o documentación adicional.*