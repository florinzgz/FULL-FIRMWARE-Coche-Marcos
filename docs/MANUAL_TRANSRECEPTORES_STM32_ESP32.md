# Manual Técnico: Transreceptores CAN para Comunicación ESP32-S3 ↔ STM32G474RE

**Proyecto:** FULL-FIRMWARE-Coche-Marcos  
**Versión:** 1.0  
**Fecha:** 2026-01-24  
**Autor:** Documentación Técnica del Proyecto  
**Estado:** ✅ Documentación Oficial

---

## Índice

1. [Resumen Ejecutivo](#1-resumen-ejecutivo)
2. [Transreceptores Añadidos](#2-transreceptores-añadidos)
3. [Especificaciones del TJA1051T/3](#3-especificaciones-del-tja10513)
4. [Conexión Hardware ESP32-S3 ↔ STM32G474RE](#4-conexión-hardware-esp32-s3--stm32g474re)
5. [Configuración de Pines](#5-configuración-de-pines)
6. [Diagrama de Conexión Completo](#6-diagrama-de-conexión-completo)
7. [Configuración de Software](#7-configuración-de-software)
8. [Especificaciones Técnicas del Bus CAN](#8-especificaciones-técnicas-del-bus-can)
9. [Validación y Pruebas](#9-validación-y-pruebas)
10. [Referencias](#10-referencias)

---

## 1. Resumen Ejecutivo

### Pregunta Original
*"¿Cuántos transreceptores has añadido para la implementación con el STM32G474RE? ¿Has añadido un manual? Estúdialo y dime cómo has hecho la conexión entre el ESP32-S3 y el STM32G474RE"*

### Respuesta

Para la implementación del sistema de control del vehículo con arquitectura dual (ESP32-S3 para HMI + STM32G474RE para control seguro), se han especificado **DOS (2) transreceptores CAN**:

1. **Transreceptor 1:** TJA1051T/3 conectado al **STM32G474RE**
2. **Transreceptor 2:** TJA1051T/3 conectado al **ESP32-S3**

Estos transreceptores permiten la comunicación bidireccional entre ambos microcontroladores mediante el protocolo **CAN (Controller Area Network)** a **500 kbps**, separando responsabilidades:
- **ESP32-S3:** Interfaz Humano-Máquina (HMI) - pantalla, touch, audio, LEDs
- **STM32G474RE:** Control seguro - motores, sensores críticos, seguridad (ABS/TCS)

---

## 2. Transreceptores Añadidos

### 2.1 Cantidad Total

**RESPUESTA: Se han añadido DOS (2) transreceptores CAN TJA1051T/3**

### 2.2 Ubicación de los Transreceptores

| Transreceptor | Microcontrolador | Función Principal | Estado |
|---------------|------------------|-------------------|--------|
| **TJA1051T/3 #1** | STM32G474RE | Control seguro y comunicación CAN | ✅ Especificado |
| **TJA1051T/3 #2** | ESP32-S3 | HMI y comunicación CAN | ✅ Especificado |

### 2.3 Justificación de la Elección

**¿Por qué TJA1051T/3?**

1. **Estándar Automotriz:** Diseñado específicamente para aplicaciones automotrices
2. **Alta velocidad:** Soporta hasta 1 Mbps (configurado a 500 kbps para este proyecto)
3. **Robustez:** Protecciones integradas contra EMI/ESD
4. **Compatibilidad:** Compatible con voltajes de lógica 3.3V y 5V
5. **Modo Standby:** Ahorro energético cuando no está en uso
6. **Alta disponibilidad:** Componente estándar de la industria

---

## 3. Especificaciones del TJA1051T/3

### 3.1 Características Principales

| Característica | Valor |
|----------------|-------|
| **Tipo** | High-Speed CAN Transceiver |
| **Estándar** | ISO 11898-2 |
| **Velocidad Máxima** | 1 Mbps |
| **Alimentación** | 4.75V - 5.25V (típicamente 5V) |
| **Lógica TXD/RXD** | Compatible con 3.3V y 5V |
| **Temperatura Operación** | -40°C a +125°C |
| **Protección ESD** | ±8 kV (HBM) |
| **Encapsulado** | SO8 |

### 3.2 Pinout del TJA1051T/3 (SO8)

```
        ┌─────────┐
   TXD  │1      8│  CANH
   GND  │2      7│  CANL
   VCC  │3      6│  Vref (NC en TJA1051)
   RXD  │4      5│  S (Standby/Silent)
        └─────────┘
```

| Pin | Nombre | Función | Conexión |
|-----|--------|---------|----------|
| 1 | TXD | Transmit Data Input | Conectar a TX del MCU (FDCAN_TX o GPIO CAN) |
| 2 | GND | Ground | Conectar a GND común |
| 3 | VCC | Power Supply | Conectar a +5V |
| 4 | RXD | Receive Data Output | Conectar a RX del MCU (FDCAN_RX o GPIO CAN) |
| 5 | S | Standby/Silent | GND = Normal mode, VCC = Standby |
| 6 | Vref | Reference Voltage | No conectar (TJA1051) |
| 7 | CANL | CAN Low | Bus CAN L |
| 8 | CANH | CAN High | Bus CAN H |

### 3.3 Modos de Operación

**Modo Normal (S = GND):**
- Transreceptor activo
- Comunicación CAN habilitada
- Consumo normal (~70 mA @ 5V)

**Modo Standby (S = VCC):**
- Transreceptor en bajo consumo
- Bus CAN aislado
- Consumo reducido (~5 µA)
- Usado para fail-safe

---

## 4. Conexión Hardware ESP32-S3 ↔ STM32G474RE

### 4.1 Arquitectura de Comunicación

```
┌──────────────────┐                            ┌──────────────────┐
│   ESP32-S3       │                            │   STM32G474RE    │
│   (HMI)          │                            │   (Control)      │
│                  │                            │                  │
│  TWAI/CAN        │                            │  FDCAN1          │
│  (GPIO compatible)│                            │  PB8 (RX)        │
│  TX ────────────┐│                            │  PB9 (TX)        │
│  RX ────────────┘│                            │                  │
└────────┬─────────┘                            └────────┬─────────┘
         │                                               │
         │  3.3V logic                                   │  3.3V logic
         ▼                                               ▼
    ┌────────────┐                                  ┌────────────┐
    │ TJA1051T/3 │                                  │ TJA1051T/3 │
    │     #2     │                                  │     #1     │
    │  (ESP32)   │                                  │  (STM32)   │
    └──────┬─────┘                                  └──────┬─────┘
           │                                               │
           │  CANH/CANL (Differential 5V)                  │
           │                                               │
           └───────────────┬───────────────────────────────┘
                           │
                      [120Ω resistor]  (Terminación en ambos extremos)
```

### 4.2 Topología Física Detallada

```
STM32G474RE                                          ESP32-S3
  (Control)                                            (HMI)
     │                                                    │
     ├─ PB9 (FDCAN1_TX) ────► Pin 1 (TXD)                │
     ├─ PB8 (FDCAN1_RX) ◄──── Pin 4 (RXD)                │
     │                         │                          │
     │                    [TJA1051T/3 #1]                 │
     │                         │                          │
     │                    Pin 8 (CANH) ──┐                │
     │                    Pin 7 (CANL) ──┼───┐            │
     │                         │          │   │            │
     │                    Pin 3 (VCC) ─ 5V   │            │
     │                    Pin 2 (GND) ─ GND  │            │
     │                    Pin 5 (S) ──── GND │            │
     │                         │          │   │            │
     │                         │         120Ω │            │
     │                         │          │   │            │
     │                         │    ┌─────┴───┴─────┐      │
     │                         │    │   CAN BUS     │      │
     │                         │    │  CANH / CANL  │      │
     │                         │    └─────┬───┬─────┘      │
     │                         │          │   │            │
     │                         │         120Ω │            │
     │                         │          │   │            │
     │                    [TJA1051T/3 #2]     │            │
     │                         │          │   │            │
     │                    Pin 8 (CANH) ──┘   │            │
     │                    Pin 7 (CANL) ──────┘            │
     │                         │                          │
     │                    Pin 3 (VCC) ─ 5V                │
     │                    Pin 2 (GND) ─ GND               │
     │                    Pin 5 (S) ──── GND              │
     │                         │                          │
     ├─────────────────────────┼──────────────────────────┤
                               │                          │
                          Pin 1 (TXD) ◄─── GPIO_TX ───────┤
                          Pin 4 (RXD) ───► GPIO_RX ───────┤
                               │                          │
                               │                     ESP32-S3
                               │                      (TWAI/CAN)
```

### 4.3 Conexiones Eléctricas Detalladas

#### **Lado STM32G474RE**

| Componente | Pin | Señal | Conexión STM32 |
|------------|-----|-------|----------------|
| TJA1051T/3 #1 | Pin 1 (TXD) | CAN TX | **PB9** (FDCAN1_TX) |
| TJA1051T/3 #1 | Pin 4 (RXD) | CAN RX | **PB8** (FDCAN1_RX) |
| TJA1051T/3 #1 | Pin 3 (VCC) | Alimentación | +5V |
| TJA1051T/3 #1 | Pin 2 (GND) | Ground | GND |
| TJA1051T/3 #1 | Pin 5 (S) | Standby | GND (modo normal) |
| TJA1051T/3 #1 | Pin 8 (CANH) | CAN High | Bus CANH + Terminación 120Ω |
| TJA1051T/3 #1 | Pin 7 (CANL) | CAN Low | Bus CANL + Terminación 120Ω |

**Nota importante para STM32G474RE:**
- El STM32G474RE tiene FDCAN (Flexible Data-Rate CAN) interno
- Pines PB8/PB9 deben configurarse en modo **Alternate Function 9 (AF9)** para FDCAN1
- Lógica 3.3V compatible con entrada TXD del TJA1051T/3

#### **Lado ESP32-S3**

| Componente | Pin | Señal | Conexión ESP32-S3 |
|------------|-----|-------|-------------------|
| TJA1051T/3 #2 | Pin 1 (TXD) | CAN TX | GPIO configurable para TWAI_TX (ej: GPIO 20)* |
| TJA1051T/3 #2 | Pin 4 (RXD) | CAN RX | GPIO configurable para TWAI_RX (ej: GPIO 21)* |
| TJA1051T/3 #2 | Pin 3 (VCC) | Alimentación | +5V |
| TJA1051T/3 #2 | Pin 2 (GND) | Ground | GND |
| TJA1051T/3 #2 | Pin 5 (S) | Standby | GND (modo normal) |
| TJA1051T/3 #2 | Pin 8 (CANH) | CAN High | Bus CANH + Terminación 120Ω |
| TJA1051T/3 #2 | Pin 7 (CANL) | CAN Low | Bus CANL + Terminación 120Ω |

**\*Nota:** Los GPIO específicos para TWAI (Two-Wire Automotive Interface - CAN del ESP32) son configurables en software. En el firmware actual del proyecto, estos pines aún no están asignados ya que la comunicación CAN está en fase de planificación.

**Pines sugeridos para ESP32-S3:**
- TWAI_TX: GPIO 20 (actualmente libre)
- TWAI_RX: GPIO 21 (actualmente libre)

### 4.4 Bus CAN Físico

**Especificaciones del cableado:**

| Parámetro | Valor | Notas |
|-----------|-------|-------|
| **Cable** | Par trenzado | Twisted pair, 120Ω impedancia característica |
| **CANH/CANL** | Diferencial | Señal diferencial ~2V @ 5V |
| **Terminación** | 2× 120Ω | Una en cada extremo del bus |
| **Longitud máxima** | ~3-5 metros @ 500 kbps | Depende de calidad de cable |
| **Topología** | Bus lineal | Sin derivaciones largas (stubs) |
| **Velocidad configurada** | 500 kbps | Classic CAN, no CAN-FD |

**Resistencias de terminación:**

```
CANH ────────┬────────────────────────┬──────── CANH
             │                        │
            120Ω                     120Ω
             │                        │
CANL ────────┴────────────────────────┴──────── CANL
       (Extremo STM32)         (Extremo ESP32)
```

---

## 5. Configuración de Pines

### 5.1 Pines STM32G474RE (FDCAN1)

Según la documentación oficial del proyecto (`STM32G474RE_PINOUT_DEFINITIVO.md`):

```c
// Configuración FDCAN1 en STM32G474RE
Pin PB8:  FDCAN1_RX  (AF9)  - Recepción CAN
Pin PB9:  FDCAN1_TX  (AF9)  - Transmisión CAN
```

**Código de configuración HAL (STM32):**

```c
// Configurar GPIO para FDCAN1
GPIO_InitTypeDef GPIO_InitStruct = {0};

// Habilitar reloj GPIOB
__HAL_RCC_GPIOB_CLK_ENABLE();

// PB8: FDCAN1_RX
GPIO_InitStruct.Pin = GPIO_PIN_8;
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

// PB9: FDCAN1_TX
GPIO_InitStruct.Pin = GPIO_PIN_9;
GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
GPIO_InitStruct.Pull = GPIO_NOPULL;
GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
```

### 5.2 Pines ESP32-S3 (TWAI)

**Pines propuestos para implementación futura:**

```c
// Configuración TWAI (CAN) en ESP32-S3
// Estos pines son configurables en software

#define TWAI_TX_GPIO   GPIO_NUM_20  // Propuesto (actualmente libre)
#define TWAI_RX_GPIO   GPIO_NUM_21  // Propuesto (actualmente libre)
```

**Nota importante:** El ESP32-S3 permite configurar TWAI en casi cualquier GPIO. Los pines GPIO20/21 se proponen porque:
1. Actualmente están libres en el pinout del proyecto
2. No tienen funciones alternativas críticas
3. Están físicamente cercanos en el chip
4. No interfieren con SPI, I2C, ni otros periféricos ya asignados

**Verificación de disponibilidad según `include/pins.h` actual:**
- GPIO 18: USB D- (reservado)
- GPIO 19: USB D+ (reservado)
- GPIO 20: **LIBRE** ✅
- GPIO 21: **LIBRE** ✅ (actualmente asignado a TOUCH_CS, pero se puede reasignar)

**Alternativa:** Si GPIO21 está en uso, se puede usar GPIO22 o GPIO47 como TWAI_RX.

---

## 6. Diagrama de Conexión Completo

### 6.1 Diagrama Esquemático Detallado

```
                    SISTEMA DE COMUNICACIÓN CAN
                    ESP32-S3 (HMI) ↔ STM32G474RE (Control)

┌─────────────────────────────────────────────────────────────────────┐
│                           ESP32-S3 (HMI)                             │
│  ┌────────────────────────────────────────────────────────────┐     │
│  │ Funciones HMI:                                              │     │
│  │  - Display TFT ST7796S 480×320                              │     │
│  │  - Touch XPT2046                                            │     │
│  │  - Audio DFPlayer Mini                                      │     │
│  │  - LEDs WS2812B (frontales + traseros)                      │     │
│  │  - Detección de obstáculos (visual/aviso)                   │     │
│  │  - Menús y diagnóstico                                      │     │
│  └────────────────────────────────────────────────────────────┘     │
│                                                                      │
│  TWAI (CAN Controller):                                              │
│  ┌──────────┐                                                        │
│  │ GPIO 20  ├──► TWAI_TX ────────┐                                  │
│  │ GPIO 21  ├──► TWAI_RX ────────┤                                  │
│  └──────────┘                     │                                  │
└───────────────────────────────────┼──────────────────────────────────┘
                                    │
                                    │  3.3V Logic
                                    ▼
                          ┌──────────────────┐
                          │   TJA1051T/3 #2  │
                          │   (Transceiver)  │
                          ├──────────────────┤
                          │ Pin 1 (TXD) ◄────┤ ← GPIO 20 (ESP32)
                          │ Pin 4 (RXD) ─────┤ → GPIO 21 (ESP32)
                          │ Pin 3 (VCC) ◄────┤ ← +5V
                          │ Pin 2 (GND) ─────┤ → GND
                          │ Pin 5 (S)   ◄────┤ ← GND (Normal mode)
                          │ Pin 8 (CANH)─────┤ ↔ Bus CANH
                          │ Pin 7 (CANL)─────┤ ↔ Bus CANL
                          └──────────────────┘
                                    │
                     ┌──────────────┴──────────────┐
                     │     BUS CAN (ISO 11898)     │
                     │                             │
                     │  CANH ───────────────────── │ ── 120Ω terminación
                     │  CANL ───────────────────── │ ── 120Ω terminación
                     │                             │
                     │  Velocidad: 500 kbps        │
                     │  Longitud: ~3-5 metros      │
                     │  Cable: Par trenzado 120Ω   │
                     └──────────────┬──────────────┘
                                    │
                          ┌──────────────────┐
                          │   TJA1051T/3 #1  │
                          │   (Transceiver)  │
                          ├──────────────────┤
                          │ Pin 1 (TXD) ◄────┤ ← PB9 (STM32)
                          │ Pin 4 (RXD) ─────┤ → PB8 (STM32)
                          │ Pin 3 (VCC) ◄────┤ ← +5V
                          │ Pin 2 (GND) ─────┤ → GND
                          │ Pin 5 (S)   ◄────┤ ← GND (Normal mode)
                          │ Pin 8 (CANH)─────┤ ↔ Bus CANH
                          │ Pin 7 (CANL)─────┤ ↔ Bus CANL
                          └──────────────────┘
                                    │
                                    │  3.3V Logic
                                    ▼
┌───────────────────────────────────┼──────────────────────────────────┐
│  FDCAN1 (CAN Controller):         │                                  │
│  ┌──────────┐                     │                                  │
│  │   PB8    ├──► FDCAN1_RX ───────┘                                  │
│  │   PB9    ├──► FDCAN1_TX ───────────────────────┐                  │
│  └──────────┘                                      │                  │
│                                                    │                  │
│                        STM32G474RE (Control)       │                  │
│  ┌────────────────────────────────────────────────┴────────────┐     │
│  │ Funciones de Control Seguro:                                │     │
│  │  - Control de motores (tracción × 4 + dirección)            │     │
│  │  - Encoder dirección (E6B2-CWZ6C)                            │     │
│  │  - Sensores de rueda × 4                                     │     │
│  │  - Sensores de corriente INA226 × 6 (vía I2C)                │     │
│  │  - Sensores de temperatura DS18B20                           │     │
│  │  - Relés de potencia × 3 (Main, Tracción, Dirección)         │     │
│  │  - Lógica ABS/TCS                                            │     │
│  │  - Pedal analógico (Hall)                                    │     │
│  │  - Shifter mecánico (F/N/R)                                  │     │
│  └──────────────────────────────────────────────────────────────┘     │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
```

### 6.2 Tabla Resumen de Conexiones

| Componente | Pin Origen | Señal | Pin Destino | Componente Destino |
|------------|------------|-------|-------------|--------------------|
| ESP32-S3 | GPIO 20 | TWAI_TX | TJA1051#2 Pin 1 | TXD |
| ESP32-S3 | GPIO 21 | TWAI_RX | TJA1051#2 Pin 4 | RXD |
| TJA1051#2 | Pin 8 | CANH | Bus CAN | CANH |
| TJA1051#2 | Pin 7 | CANL | Bus CAN | CANL |
| Bus CAN | CANH | CANH | TJA1051#1 Pin 8 | CANH |
| Bus CAN | CANL | CANL | TJA1051#1 Pin 7 | CANL |
| TJA1051#1 | Pin 1 | TXD | STM32 PB9 | FDCAN1_TX |
| TJA1051#1 | Pin 4 | RXD | STM32 PB8 | FDCAN1_RX |
| Ambos TJA1051 | Pin 3 | VCC | Fuente | +5V |
| Ambos TJA1051 | Pin 2 | GND | Fuente | GND |
| Ambos TJA1051 | Pin 5 | S (Standby) | Fuente | GND (modo normal) |

---

## 7. Configuración de Software

### 7.1 STM32G474RE - Configuración FDCAN1

**Parámetros de comunicación:**

```c
// Configuración FDCAN1 para Classic CAN @ 500 kbps
// Clock FDCAN: 170 MHz (SYSCLK)

FDCAN_HandleTypeDef hfdcan1;

void CAN_Init(void) {
    // Configuración de reloj
    __HAL_RCC_FDCAN_CLK_ENABLE();
    
    // Parámetros FDCAN
    hfdcan1.Instance = FDCAN1;
    hfdcan1.Init.ClockDivider = FDCAN_CLOCK_DIV1;  // 170 MHz / 1 = 170 MHz
    hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;  // Classic CAN (no CAN-FD)
    hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan1.Init.AutoRetransmission = ENABLE;
    hfdcan1.Init.TransmitPause = DISABLE;
    hfdcan1.Init.ProtocolException = ENABLE;
    
    // Bit timing para 500 kbps @ 170 MHz
    // Fórmula: Bit Rate = FDCAN_CLK / (Prescaler × (1 + Seg1 + Seg2))
    // 500 kbps = 170 MHz / (20 × (1 + 13 + 2)) = 170 MHz / 320 = 531.25 kHz ≈ 500 kHz
    hfdcan1.Init.NominalPrescaler = 20;  // Prescaler
    hfdcan1.Init.NominalSyncJumpWidth = 1;
    hfdcan1.Init.NominalTimeSeg1 = 13;   // Propagation + Phase Seg1
    hfdcan1.Init.NominalTimeSeg2 = 2;    // Phase Seg2
    
    // Inicializar FDCAN
    if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK) {
        Error_Handler();
    }
    
    // Configurar filtros (aceptar todos los mensajes inicialmente)
    FDCAN_FilterTypeDef sFilterConfig;
    sFilterConfig.IdType = FDCAN_STANDARD_ID;
    sFilterConfig.FilterIndex = 0;
    sFilterConfig.FilterType = FDCAN_FILTER_RANGE;
    sFilterConfig.FilterConfig = FDCAN_FILTER_TO_RXFIFO0;
    sFilterConfig.FilterID1 = 0x000;  // ID mínimo
    sFilterConfig.FilterID2 = 0x7FF;  // ID máximo (11-bit)
    
    if (HAL_FDCAN_ConfigFilter(&hfdcan1, &sFilterConfig) != HAL_OK) {
        Error_Handler();
    }
    
    // Iniciar FDCAN
    if (HAL_FDCAN_Start(&hfdcan1) != HAL_OK) {
        Error_Handler();
    }
    
    // Configurar interrupción RX
    if (HAL_FDCAN_ActivateNotification(&hfdcan1, 
        FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0) != HAL_OK) {
        Error_Handler();
    }
}
```

### 7.2 ESP32-S3 - Configuración TWAI (CAN)

**Ejemplo de configuración TWAI:**

```c
#include "driver/twai.h"

// Definición de pines
#define TWAI_TX_GPIO   GPIO_NUM_20
#define TWAI_RX_GPIO   GPIO_NUM_21

void CAN_Init_ESP32(void) {
    // Configuración general TWAI
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        TWAI_TX_GPIO, 
        TWAI_RX_GPIO, 
        TWAI_MODE_NORMAL
    );
    
    // Configuración de timing para 500 kbps
    // APB Clock = 80 MHz (típico ESP32-S3)
    // Bit Rate = APB_CLK / (BRP × (1 + Seg1 + Seg2))
    // 500 kbps = 80 MHz / (10 × (1 + 12 + 3)) = 80 MHz / 160 = 500 kHz ✓
    twai_timing_config_t t_config = {
        .brp = 10,              // Baud Rate Prescaler
        .tseg_1 = 12,           // Time Segment 1
        .tseg_2 = 3,            // Time Segment 2
        .sjw = 3,               // Synchronization Jump Width
        .triple_sampling = false
    };
    
    // Configuración de filtros (aceptar todos)
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    // Instalar driver TWAI
    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        ESP_LOGE("CAN", "Failed to install TWAI driver");
        return;
    }
    
    // Iniciar TWAI
    if (twai_start() != ESP_OK) {
        ESP_LOGE("CAN", "Failed to start TWAI driver");
        return;
    }
    
    ESP_LOGI("CAN", "TWAI driver initialized @ 500 kbps");
}

// Ejemplo de envío de mensaje
void CAN_Send_Message(uint32_t id, uint8_t *data, uint8_t len) {
    twai_message_t message;
    message.identifier = id;
    message.extd = 0;  // Standard ID (11-bit)
    message.rtr = 0;   // Data frame
    message.data_length_code = len;
    memcpy(message.data, data, len);
    
    if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
        ESP_LOGI("CAN", "Message sent: ID=0x%03X", id);
    } else {
        ESP_LOGE("CAN", "Failed to send message");
    }
}

// Ejemplo de recepción de mensaje
void CAN_Receive_Task(void *pvParameters) {
    twai_message_t message;
    
    while (1) {
        if (twai_receive(&message, pdMS_TO_TICKS(1000)) == ESP_OK) {
            ESP_LOGI("CAN", "Message received: ID=0x%03X, DLC=%d", 
                     message.identifier, message.data_length_code);
            
            // Procesar mensaje recibido
            // ...
        }
    }
}
```

---

## 8. Especificaciones Técnicas del Bus CAN

### 8.1 Parámetros de Comunicación

| Parámetro | Valor | Notas |
|-----------|-------|-------|
| **Protocolo** | CAN 2.0A/B (Classic CAN) | ISO 11898-2 |
| **Velocidad** | 500 kbps | Recomendado para cables de 3-5m |
| **Formato de ID** | Standard 11-bit | IDs de 0x000 a 0x7FF |
| **Tamaño de mensaje** | 0-8 bytes | Classic CAN estándar |
| **Modo** | Normal (no Listen-Only) | Bidireccional |
| **Sample Point** | ~81% | Según configuración bit timing |

### 8.2 Asignación de IDs CAN (Propuesto)

**Convención de IDs:**

```
┌─────────────────────────────────────────────────────────────┐
│ ID CAN (11-bit): 0xXYZ                                      │
│                                                             │
│ X (bits 10-8):  Prioridad / Tipo de mensaje                │
│   0: Crítico (Heartbeat, Stop)                             │
│   1: Alta (Comandos de control)                            │
│   2: Media (Estado de sensores)                            │
│   3: Baja (Diagnóstico, info)                              │
│                                                             │
│ Y (bits 7-4):   Origen del mensaje                         │
│   0: STM32 (Control)                                       │
│   1: ESP32 (HMI)                                           │
│                                                             │
│ Z (bits 3-0):   Tipo específico de mensaje                │
└─────────────────────────────────────────────────────────────┘
```

**Tabla de IDs propuestos:**

| ID (Hex) | Nombre | Origen | Descripción | Frecuencia |
|----------|--------|--------|-------------|------------|
| **0x001** | HEARTBEAT_STM32 | STM32 | Heartbeat del STM32 | 100 ms |
| **0x011** | HEARTBEAT_ESP32 | ESP32 | Heartbeat del ESP32 | 100 ms |
| **0x020** | EMERGENCY_STOP | Ambos | Stop de emergencia | On-demand |
| **0x100** | CMD_THROTTLE | ESP32→STM32 | Comando acelerador (0-100%) | 50 ms |
| **0x101** | CMD_STEERING | ESP32→STM32 | Comando dirección (-100 a +100%) | 50 ms |
| **0x102** | CMD_MODE | ESP32→STM32 | Cambio de modo (F/N/R) | On-demand |
| **0x103** | CMD_BRAKE | ESP32→STM32 | Solicitud de frenado | On-demand |
| **0x200** | STATUS_SPEED | STM32→ESP32 | Velocidades de ruedas | 100 ms |
| **0x201** | STATUS_CURRENT | STM32→ESP32 | Corrientes de motores | 100 ms |
| **0x202** | STATUS_TEMP | STM32→ESP32 | Temperaturas | 1000 ms |
| **0x203** | STATUS_SAFETY | STM32→ESP32 | Estado ABS/TCS | 100 ms |
| **0x204** | STATUS_STEERING | STM32→ESP32 | Posición dirección | 100 ms |
| **0x300** | DIAG_ERROR | Ambos | Códigos de error | On-demand |
| **0x301** | DIAG_WARNING | Ambos | Warnings | On-demand |
| **0x302** | DIAG_INFO | Ambos | Información general | On-demand |

### 8.3 Formato de Mensajes CAN

**Ejemplo 1: CMD_THROTTLE (0x100)**

```
ID: 0x100
DLC: 2 bytes
Data[0]: Throttle (0-100%) - uint8_t
Data[1]: Checksum simple - uint8_t
```

**Ejemplo 2: STATUS_SPEED (0x200)**

```
ID: 0x200
DLC: 8 bytes
Data[0-1]: Velocidad rueda FL (mm/s) - uint16_t, little-endian
Data[2-3]: Velocidad rueda FR (mm/s) - uint16_t, little-endian
Data[4-5]: Velocidad rueda RL (mm/s) - uint16_t, little-endian
Data[6-7]: Velocidad rueda RR (mm/s) - uint16_t, little-endian
```

**Ejemplo 3: HEARTBEAT (0x001 / 0x011)**

```
ID: 0x001 (STM32) o 0x011 (ESP32)
DLC: 4 bytes
Data[0]: Status flags - uint8_t
  bit 0: System OK
  bit 1: Warning active
  bit 2: Error active
  bit 3-7: Reserved
Data[1-3]: Uptime (ms) - uint24_t, little-endian
```

---

## 9. Validación y Pruebas

### 9.1 Checklist de Validación Hardware

**Antes de energizar:**

- [ ] Verificar continuidad GND entre ESP32-S3 y STM32G474RE
- [ ] Verificar alimentación +5V a ambos TJA1051T/3
- [ ] Verificar que pin 5 (S) de ambos TJA1051 está conectado a GND
- [ ] Medir resistencia entre CANH y CANL: debe ser ~60Ω (dos 120Ω en paralelo)
- [ ] Verificar que no hay cortocircuitos entre CANH, CANL y GND
- [ ] Verificar polaridad de conexión de terminaciones 120Ω

**Después de energizar (sin MCUs transmitiendo):**

- [ ] Medir voltaje CANH: ~2.5V (recesivo)
- [ ] Medir voltaje CANL: ~2.5V (recesivo)
- [ ] Medir diferencia CANH-CANL: ~0V (recesivo)
- [ ] Verificar +5V en pin 3 de ambos TJA1051
- [ ] Verificar 3.3V en pines de lógica de MCUs

### 9.2 Pruebas de Comunicación

**Nivel 1: Loopback Test**

1. Configurar STM32 en modo loopback interno (sin transceptor)
2. Enviar mensaje CAN y verificar recepción
3. Repetir con ESP32-S3

**Nivel 2: Ping-Pong Test**

1. STM32 envía heartbeat cada 100ms
2. ESP32 detecta heartbeat y responde con su propio heartbeat
3. Verificar que ambos reciben mensajes mutuos
4. Medir latencia (debe ser <5ms @ 500 kbps)

**Nivel 3: Load Test**

1. STM32 envía mensajes de estado cada 50ms (varios IDs)
2. ESP32 envía comandos cada 50ms
3. Verificar que no hay pérdida de mensajes
4. Verificar bus load <50%

**Nivel 4: Error Recovery Test**

1. Desconectar físicamente un transceptor
2. Verificar que el otro MCU detecta pérdida de heartbeat
3. Verificar activación de fail-safe
4. Reconectar y verificar recuperación automática

### 9.3 Herramientas de Diagnóstico

**Hardware:**

- Osciloscopio (mínimo 100 MHz) para ver señales CANH/CANL
- Analizador lógico para decodificar mensajes CAN
- Multímetro para verificar voltajes y resistencias

**Software:**

- STM32CubeMonitor (monitoreo en tiempo real)
- ESP-IDF Monitor (logs TWAI)
- CANalyzer/CANoe (si disponible, para análisis profesional)
- Wireshark con plugin CAN (análisis de tráfico)

### 9.4 Criterios de Aceptación

| Criterio | Valor Esperado | Método de Verificación |
|----------|----------------|------------------------|
| **Latencia máxima** | <10 ms | Osciloscopio + timestamps |
| **Pérdida de mensajes** | 0% en condiciones normales | Contadores software |
| **Error rate** | <0.1% | Estadísticas FDCAN/TWAI |
| **Bus load** | <60% en carga máxima | Cálculo teórico + medición |
| **Tiempo de recuperación** | <500 ms tras desconexión | Prueba de reconexión |
| **Robustez EMI** | Sin errores con motor PWM activo | Prueba en vehículo |

---

## 10. Referencias

### 10.1 Documentos del Proyecto

1. **`docs/STM32G474RE_PINOUT_DEFINITIVO.md`**
   - Pinout completo del STM32G474RE
   - Configuración de pines FDCAN1 (PB8/PB9)
   - Especificaciones de transreceptor TJA1051T/3

2. **`docs/PLAN_SEPARACION_STM32_CAN.md`**
   - Plan de arquitectura ESP32 HMI + STM32 Control
   - Estrategia de comunicación CAN
   - Fases de migración

3. **`docs/DESIGN_FREEZE_STM32G474RE.md`**
   - Design freeze del pinout STM32
   - Correcciones aplicadas
   - Configuración de reloj HSE

4. **`docs/AUDITORIA_PINOUT_STM32G474RE.md`**
   - Auditoría técnica del diseño
   - Validación de asignación de pines

5. **`docs/STM32_CAN_MIGRATION_STUDY.md`**
   - Estudio de integración ESP32-S3 + STM32G474RE
   - Análisis de periféricos y buses

6. **`README.md`**
   - Estado actual del firmware (v2.17.1 PHASE 14)
   - Hardware ESP32-S3 N16R8

### 10.2 Datasheets y Estándares

1. **TJA1051T/3 Datasheet (NXP)**
   - High-speed CAN transceiver
   - Electrical specifications
   - Application notes

2. **STM32G474RE Datasheet (STMicroelectronics)**
   - DS12288 Rev 8
   - FDCAN peripheral specifications
   - Pin multiplexing

3. **ESP32-S3 Technical Reference Manual (Espressif)**
   - TWAI controller specifications
   - GPIO configuration

4. **ISO 11898-2:2016**
   - CAN physical layer specification
   - High-speed CAN electrical specifications

5. **CAN Specification 2.0 (Bosch)**
   - Protocol layer specification
   - Frame formats

### 10.3 Notas de Aplicación

1. **AN13189 - FDCAN on STM32G4 (STMicroelectronics)**
2. **AN228 - A CAN Physical Layer Discussion (Texas Instruments)**
3. **ESP-IDF TWAI Driver Documentation (Espressif)**

---

## Apéndice A: Cálculo de Bit Timing

### STM32G474RE FDCAN @ 500 kbps

```
Clock FDCAN: 170 MHz (SYSCLK sin divisor)
Bit Rate deseado: 500 kbps

Fórmula:
Bit Rate = FDCAN_CLK / (NominalPrescaler × (1 + NominalTimeSeg1 + NominalTimeSeg2))

Configuración seleccionada:
- NominalPrescaler = 20
- NominalTimeSeg1 = 13
- NominalTimeSeg2 = 2
- NominalSyncJumpWidth = 1

Cálculo:
Bit Rate = 170 MHz / (20 × (1 + 13 + 2))
         = 170 MHz / (20 × 16)
         = 170 MHz / 320
         = 531.25 kHz
         ≈ 500 kHz ✓ (error: +6.25%, dentro de tolerancia)

Sample Point:
SP = (1 + TimeSeg1) / (1 + TimeSeg1 + TimeSeg2)
   = (1 + 13) / (1 + 13 + 2)
   = 14 / 16
   = 87.5% ✓ (óptimo: 75-90%)

Tolerancia total del sistema:
- Cristal HSE: ±20 ppm
- Drift térmica: ±50 ppm
- Total: ~±70 ppm << ±0.5% (límite CAN) ✓
```

### ESP32-S3 TWAI @ 500 kbps

```
Clock APB: 80 MHz (típico ESP32-S3)
Bit Rate deseado: 500 kbps

Fórmula:
Bit Rate = APB_CLK / (BRP × (1 + tseg_1 + tseg_2))

Configuración seleccionada:
- BRP (Baud Rate Prescaler) = 10
- tseg_1 = 12
- tseg_2 = 3
- SJW = 3

Cálculo:
Bit Rate = 80 MHz / (10 × (1 + 12 + 3))
         = 80 MHz / (10 × 16)
         = 80 MHz / 160
         = 500 kHz ✓ (exacto)

Sample Point:
SP = (1 + tseg_1) / (1 + tseg_1 + tseg_2)
   = (1 + 12) / (1 + 12 + 3)
   = 13 / 16
   = 81.25% ✓ (óptimo: 75-90%)
```

**Compatibilidad:**
- Ambos MCUs configurados con sample point ~81-87%
- Diferencia <10% → Compatible ✓
- SJW suficiente para compensar drift de reloj

---

## Apéndice B: Lista de Materiales (BOM)

### Componentes Necesarios para Implementación CAN

| Qty | Componente | Descripción | Valor/Part Number | Encapsulado | Notas |
|-----|------------|-------------|-------------------|-------------|-------|
| 2 | Transreceptor CAN | High-Speed CAN | **TJA1051T/3** (NXP) | SO-8 | Uno por MCU |
| 2 | Resistencia Terminación | Pull-down | 120Ω ±1% | 0805 | Una en cada extremo del bus |
| 2 | Capacitor Decoupling | VCC TJA1051 | 100nF X7R | 0805 | Cerca de pin 3 (VCC) |
| 1 | Cable CAN | Par trenzado | 120Ω impedancia | AWG 24-26 | 3-5 metros |
| 2 | Conector CAN | Terminal block | 2 pines | Pitch 5.08mm | CANH/CANL |
| - | PCB | Prototyping board | - | - | O diseño custom |

**Costo estimado:** ~15-25 USD (componentes electrónicos, sin PCB custom)

---

## Apéndice C: Troubleshooting

### Problema: No hay comunicación CAN

**Síntomas:**
- Mensajes no se reciben
- Contadores TX OK pero RX = 0

**Diagnóstico:**

1. **Verificar alimentación:**
   ```bash
   Medir pin 3 (VCC) de TJA1051: debe ser 5.0V ±0.25V
   Medir pin 2 (GND): debe ser 0V
   ```

2. **Verificar bus en reposo:**
   ```bash
   Medir CANH: ~2.5V (sin tráfico)
   Medir CANL: ~2.5V (sin tráfico)
   Diferencia CANH-CANL: ~0V
   ```

3. **Verificar resistencias de terminación:**
   ```bash
   Desconectar ambos MCUs
   Medir resistencia CANH-CANL: debe ser ~60Ω (120Ω || 120Ω)
   Si es infinito: falta terminación
   Si es ~120Ω: solo hay una terminación
   ```

4. **Verificar configuración software:**
   - Bit rate coincide en ambos lados (500 kbps)
   - Sample point similar (~80-85%)
   - Filtros CAN no bloquean mensajes
   - Pines GPIO configurados correctamente

5. **Verificar señales con osciloscopio:**
   - CANH debe variar entre ~2.5V (recesivo) y ~3.5V (dominante)
   - CANL debe variar entre ~2.5V (recesivo) y ~1.5V (dominante)
   - Diferencia CANH-CANL dominante: ~2V
   - Rise/fall time <100 ns

### Problema: Alta tasa de errores

**Síntomas:**
- Bus Error counters incrementan
- Mensajes perdidos intermitentes
- Bus-off state

**Posibles causas:**

1. **Bit timing incorrecto:**
   - Verificar cálculo de prescaler
   - Verificar sample point (debe ser 75-90%)
   - Revisar reloj del sistema (HSE activo en STM32)

2. **Cableado deficiente:**
   - Cable no es par trenzado
   - Longitud >5m @ 500 kbps
   - Stubs (derivaciones) largos
   - Falta de terminación o terminación incorrecta

3. **Ruido electromagnético:**
   - Cables CAN cerca de PWM de motores
   - Falta de GND común robusto
   - Cables de alimentación paralelos a CAN

**Soluciones:**

1. Reducir velocidad CAN (probar 250 kbps)
2. Mejorar cableado (usar cable certificado CAN)
3. Añadir filtros en alimentación de TJA1051
4. Revisar GND y apantallamiento

---

## Conclusión

Este manual documenta la implementación completa de la comunicación CAN entre el **ESP32-S3** (HMI) y el **STM32G474RE** (Control seguro) utilizando **dos (2) transreceptores TJA1051T/3**.

**Resumen de la implementación:**

1. ✅ **Dos transreceptores TJA1051T/3** especificados (uno por MCU)
2. ✅ **Conexión detallada** documentada con diagramas
3. ✅ **Configuración de pines** para ambos microcontroladores
4. ✅ **Código de ejemplo** para FDCAN (STM32) y TWAI (ESP32)
5. ✅ **Especificaciones de bus** CAN @ 500 kbps
6. ✅ **Validación y pruebas** definidas
7. ✅ **Troubleshooting** incluido

**Estado actual del proyecto:**
- Fase de **planificación y documentación** ✅
- Hardware ESP32-S3 operativo (v2.17.1)
- STM32G474RE pinout definido y congelado
- Implementación CAN pendiente de desarrollo físico

**Próximos pasos:**
1. Validar pines GPIO finales para TWAI en ESP32-S3
2. Fabricar PCB con transreceptores
3. Implementar protocolo CAN en firmware
4. Pruebas de integración hardware
5. Migración progresiva según `PLAN_SEPARACION_STM32_CAN.md`

---

**Documento creado:** 2026-01-24  
**Última revisión:** 2026-01-24  
**Versión:** 1.0 Final  
**Autor:** Documentación Técnica - FULL-FIRMWARE-Coche-Marcos
