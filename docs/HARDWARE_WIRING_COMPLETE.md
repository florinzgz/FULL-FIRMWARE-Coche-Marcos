# 🔌 Guía Completa de Cableado Hardware - ESP32-S3 Car Control System

## 📋 Índice
1. [Diagrama General del Sistema](#diagrama-general-del-sistema)
2. [Strapping Pins - Restricciones Críticas](#strapping-pins---restricciones-críticas)
3. [Conexiones I²C](#conexiones-i²c)
4. [Conexiones SPI](#conexiones-spi)
5. [GPIO Digitales](#gpio-digitales)
6. [GPIO Analógicos](#gpio-analógicos)
7. [Comunicación UART](#comunicación-uart)
8. [Bus OneWire](#bus-onewire)
9. [Valores de Pull-up/Pull-down](#valores-de-pull-uppull-down)
10. [Secuencia de Alimentación](#secuencia-de-alimentación)
11. [Troubleshooting Común](#troubleshooting-común)

---

## 🔧 Diagrama General del Sistema

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          ESP32-S3-DevKitC-1 (44 pines)                      │
│                                                                             │
│  I²C Bus (400kHz)                 SPI Bus (TFT)           UART/Digital      │
│  ├─ SDA: GPIO 8                   ├─ SCK:  GPIO 10       ├─ UART0_TX: 43   │
│  └─ SCL: GPIO 9                   ├─ MOSI: GPIO 11       ├─ UART0_RX: 44   │
│                                   ├─ MISO: GPIO 12       └─ OneWire:  20   │
│                                   ├─ DC:   GPIO 13                          │
│                                   ├─ RST:  GPIO 14                          │
│                                   └─ CS:   GPIO 16                          │
└─────────────────────────────────────────────────────────────────────────────┘
           │                               │                        │
           │                               │                        │
    ┌──────┴──────┐                  ┌────┴─────┐           ┌──────┴──────┐
    │   I²C Bus   │                  │ SPI Bus  │           │   Digital   │
    └──────┬──────┘                  └────┬─────┘           └──────┬──────┘
           │                               │                        │
    ┌──────┴────────────────────────┐     │                  ┌─────┴─────┐
    │                               │     │                  │           │
┌───▼────┐ ┌──────┐ ┌──────┐  ┌───▼───┐ │            ┌─────▼────┐ ┌───▼────┐
│TCA9548A│ │INA226│ │MCP    │  │ST7796S│ │            │ Relays   │ │Buttons │
│I²C Mux │ │Power │ │23017  │  │ TFT   │ │            │ x4       │ │ x3     │
│0x71    │ │0x40  │ │GPIO   │  │480x320│ │            │          │ │        │
└───┬────┘ └──────┘ │Expand │  └───────┘ │            └──────────┘ └────────┘
    │               │0x24   │             │
    │               └───────┘      ┌──────▼─────┐
    │                              │ XPT2046    │
    │                              │ Touch      │
    │                              │ (SPI)      │
    │                              └────────────┘
    │
    ├─ Canal 0: VL53L5CX FRONT (0x29) - Sensor distancia frontal
    ├─ Canal 1: VL53L5CX REAR  (0x29) - Sensor distancia trasero
    ├─ Canal 2: PCA9685 (0x40)        - PWM servo driver (16 canales)
    └─ Canales 3-7: Disponibles

┌──────────────────────────────────────────────────────────────────────────┐
│  Alimentación                                                            │
│  ├─ 24V DC  → Motores                                                    │
│  ├─ 12V DC  → Sensores/Encoder (vía Relé AUX)                            │
│  ├─ 5V Buck → ESP32-S3, TFT, Lógica (vía Relé Power Hold)               │
│  └─ 3.3V    → Generado por ESP32-S3 LDO                                 │
└──────────────────────────────────────────────────────────────────────────┘
```

---

## ⚠️ ESP32-S3 Strapping Pins - Restricciones Críticas

Los **strapping pins** del ESP32-S3 determinan el modo de arranque y configuración de hardware. **Es crítico respetar estos valores durante el boot**, o el sistema no arrancará correctamente.

### Tabla de Strapping Pins

| GPIO | Función Strapping | Uso en Firmware | Estado Boot Requerido | Hardware Recomendado |
|------|-------------------|-----------------|----------------------|---------------------|
| **0** | Boot Mode | ❌ LIBRE (antes KEY_DETECT) | **HIGH** (SPI Boot) | Pull-up 10kΩ externo **OBLIGATORIO** |
| **3** | JTAG (MTDO) | WHEEL_FL | Flotante/HIGH | Sin pull-up/down |
| **45** | VDD_SPI Voltage | KEY_DETECT | LOW/Flotante | Pull-down 10kΩ si se usa |
| **46** | Boot/ROM Log | XSHUT_FRONT | **HIGH** | Pull-up 10kΩ **OBLIGATORIO** |

### Detalles Críticos por Pin

#### GPIO 0 - Boot Mode
- **Función strapping**: Selecciona entre **SPI Boot** (HIGH) y **Download Boot** (LOW)
- **Uso actual**: ❌ LIBRE (liberado en v2.9.1+, antes usado para KEY_DETECT)
- **Estado boot**: **DEBE estar HIGH** para arranque normal desde flash
- **Hardware**: 
  - ✅ **Pull-up 10kΩ a 3.3V OBLIGATORIO**
  - ⚠️ Si flota o está LOW, el ESP32-S3 entra en modo download y NO ejecuta el firmware
- **Notas**: 
  - Botón de boot físico en DevKitC-1 conecta GPIO 0 a GND
  - Presionar botón durante power-on → Modo download (para programar)
  - Normal operation → Pull-up mantiene HIGH → Arranque desde flash

#### GPIO 3 - JTAG MTDO
- **Función strapping**: Pin de JTAG (configuración de debugging)
- **Uso actual**: WHEEL_FL (entrada de encoder de rueda delantera izquierda)
- **Estado boot**: Flotante o HIGH (sin restricción estricta)
- **Hardware**: Sin pull-up/pull-down necesario
- **Notas**: No afecta al boot normal si no se usa JTAG

#### GPIO 45 - VDD_SPI Voltage Select
- **Función strapping**: Selecciona voltaje de VDD_SPI (1.8V vs 3.3V)
- **Uso actual**: KEY_DETECT (detección de llave de encendido, INPUT_PULLUP)
- **Estado boot**: LOW o Flotante (para modo 3.3V SPI flash)
- **Hardware**: 
  - ✅ Pull-down 10kΩ a GND recomendado si se usa como entrada
  - ⚠️ NO debe estar HIGH durante boot (causaría modo 1.8V incompatible)
- **Notas**: 
  - Firmware configura como INPUT_PULLUP después de boot
  - La resistencia pull-up interna se activa DESPUÉS del strapping
  - Detecta llave OFF cuando el pin va a GND

#### GPIO 46 - Boot Mode / ROM Message Printing
- **Función strapping**: Afecta modo boot y salida de mensajes ROM por UART
- **Uso actual**: XSHUT_FRONT (control de sensor VL53L5CX frontal, OUTPUT)
- **Estado boot**: **DEBE estar HIGH** para evitar modo download
- **Hardware**: 
  - ✅ **Pull-up 10kΩ a 3.3V OBLIGATORIO**
  - ⚠️ Si está LOW durante boot → Modo download no deseado
- **Notas**: 
  - Firmware configura como OUTPUT HIGH después de boot
  - La resistencia pull-up garantiza estado correcto ANTES de que el firmware arranque
  - Mantiene sensor VL53L5CX activo durante operación normal

### Secuencia de Strapping

```
1. Power ON
   ├─ ESP32-S3 lee estado de strapping pins
   │  ├─ GPIO 0:  HIGH → SPI Boot mode ✅
   │  ├─ GPIO 46: HIGH → Normal boot  ✅
   │  └─ GPIO 45: LOW  → 3.3V SPI flash ✅
   │
2. ROM bootloader arranca
   ├─ Carga firmware desde SPI flash
   └─ Salta a main()
   
3. Firmware arranca (platformio/main.cpp)
   ├─ Configura GPIOs como OUTPUT/INPUT según necesidad
   ├─ Activa pull-ups internos donde sea necesario
   └─ Strapping pins ahora funcionan como GPIOs normales
```

### Resumen de Resistencias Externas Necesarias

| GPIO | Resistencia | Valor | Conexión | Prioridad |
|------|-------------|-------|----------|-----------|
| 0 | Pull-up | 10kΩ | 3.3V | 🔴 **CRÍTICO** |
| 45 | Pull-down | 10kΩ | GND | 🟡 Recomendado |
| 46 | Pull-up | 10kΩ | 3.3V | 🔴 **CRÍTICO** |

---

## 🔌 Conexiones I²C

### Configuración del Bus

- **SDA**: GPIO 8
- **SCL**: GPIO 9  
- **Frecuencia**: 400 kHz (Fast Mode)
- **Pull-ups**: 4.7kΩ a 3.3V (externas, **OBLIGATORIAS**)
  - ⚠️ Los pull-ups internos del ESP32-S3 (45kΩ) son insuficientes para I²C a 400kHz
  - ✅ Usar resistencias externas 4.7kΩ en SDA y SCL

### Dispositivos I²C por Dirección

| Dirección | Dispositivo | Descripción | Conexión |
|-----------|-------------|-------------|----------|
| **0x24** | MCP23017 | GPIO Expander (16 pines) | Directo al bus I²C |
| **0x40** | INA226 | Monitor de potencia | Directo al bus I²C |
| **0x71** | TCA9548A | I²C Multiplexer (8 canales) | Directo al bus I²C |

### Dispositivos detrás del Multiplexer TCA9548A

El **TCA9548A** (dirección 0x71) permite conectar múltiples dispositivos con la misma dirección I²C:

#### Canal 0 - VL53L5CX FRONT
- **Dispositivo**: VL53L5CX (Sensor ToF 8x8)
- **Dirección**: 0x29
- **Pin XSHUT**: GPIO 46 (⚠️ strapping pin - pull-up 10kΩ obligatorio)
- **Función**: Detección de obstáculos frontal
- **Conexiones**:
  - VCC → 3.3V (max 200mA)
  - GND → GND
  - SDA → TCA9548A Canal 0 SDA
  - SCL → TCA9548A Canal 0 SCL
  - XSHUT → GPIO 46
  - INT → No conectado (polling mode)

#### Canal 1 - VL53L5CX REAR
- **Dispositivo**: VL53L5CX (Sensor ToF 8x8)
- **Dirección**: 0x29
- **Pin XSHUT**: GPIO 19 (GPIO estándar)
- **Función**: Detección de obstáculos trasero
- **Conexiones**: Idénticas a FRONT, XSHUT → GPIO 19

#### Canal 2 - PCA9685
- **Dispositivo**: PCA9685 (PWM Servo Driver)
- **Dirección**: 0x40
- **Función**: Control PWM para servos/LEDs (16 canales)
- **Conexiones**:
  - VCC → 3.3V (lógica)
  - V+ → 5-12V (alimentación servos, externa)
  - GND → GND común
  - SDA → TCA9548A Canal 2 SDA
  - SCL → TCA9548A Canal 2 SCL
  - OE → GND (siempre habilitado)

#### Canales 3-7 - Disponibles
- Libres para expansión futura
- Pueden alojar sensores adicionales, displays, etc.

### Inicialización I²C Segura

```cpp
// Ejemplo de inicialización con I2CRecovery
#include "i2c_recovery.h"

void setup() {
    Wire.begin(8, 9);  // SDA, SCL
    Wire.setClock(400000);  // 400kHz
    I2CRecovery::init();  // Inicializa sistema de recuperación
    
    // Seleccionar canal del multiplexer
    if (I2CRecovery::tcaSelectSafe(0, 0x71)) {
        // Canal 0 seleccionado, comunicar con VL53L5CX FRONT
    }
}
```

---

## 📟 Conexiones SPI

### SPI Bus - Display TFT ST7796S

| Señal | GPIO | Pin ST7796S | Descripción |
|-------|------|-------------|-------------|
| **SCK** | 10 | SCK | Clock (hasta 40 MHz) |
| **MOSI** | 11 | SDI/MOSI | Master Out Slave In |
| **MISO** | 12 | SDO/MISO | Master In Slave Out |
| **DC** | 13 | DC/RS | Data/Command select |
| **RST** | 14 | RESET | Reset (active LOW) |
| **CS** | 16 | CS | Chip Select (active LOW) |
| **BL** | 42 | LED | Backlight PWM (0-255) |

**Configuración:**
- Frecuencia: 40 MHz
- Modo: SPI_MODE0 (CPOL=0, CPHA=0)
- Bit order: MSB first
- Resolución: 480x320 píxeles
- Interfaz: 16-bit color (RGB565)

**Backlight (GPIO 42):**
- Control PWM (0-100% duty cycle)
- Frecuencia PWM: 5 kHz
- 0 = apagado, 255 = máximo brillo
- Configuración por defecto: 128 (50%)

### SPI Bus - Touch Controller XPT2046

| Señal | GPIO | Pin XPT2046 | Descripción |
|-------|------|-------------|-------------|
| **SCK** | 10 | DCLK | Clock compartido con TFT |
| **MOSI** | 11 | DIN | Master Out Slave In |
| **MISO** | 12 | DOUT | Master In Slave Out |
| **CS** | 21 | CS | Chip Select (active LOW) |
| **IRQ** | 47 | IRQ | Interrupt (active LOW) |

**Configuración:**
- Frecuencia: 2 MHz (más lento que TFT)
- Modo: SPI_MODE0
- Bit order: MSB first
- Resolución: 12-bit ADC (0-4095)
- Calibración almacenada en flash

**Notas de IRQ:**
- GPIO 47 como INPUT_PULLUP
- IRQ va LOW cuando se toca la pantalla
- Firmware usa polling, IRQ como indicador rápido

---

## 🔘 GPIO Digitales

### Relés de Potencia (Salidas)

| GPIO | Señal | Relé | Función | Estado Inicial |
|------|-------|------|---------|----------------|
| **35** | RELAY_MAIN | Relé 1 | Power Hold Buck 5V | HIGH (mantiene alimentación) |
| **5** | RELAY_TRAC | Relé 2 | 12V Auxiliares (sensores/encoder) | LOW |
| **6** | RELAY_DIR | Relé 3 | 24V Motores | LOW |
| **7** | RELAY_SPARE | Relé 4 | Opcional/Seguridad | LOW |

**Características:**
- Tipo: Relé SPST-NO (Normalmente Abierto)
- Control: Transistor NPN (BC547 o similar)
- Pull-down: 10kΩ a GND (evita activación espuria en boot)
- Flyback diode: 1N4007 en paralelo con bobina del relé

**Circuito típico por relé:**
```
GPIO (ESP32-S3) ──┬── 1kΩ ──┬── Base NPN
                  │         │
                 10kΩ      │
                  │        Colector ── Relé Coil (+)
                 GND       │
                          Emisor ── GND
                          
        Relé Coil (+) ────┬──── VCC (5V o 12V)
                          │
                      [1N4007 Diode]
                          │
                         GND
```

### Botones (Entradas con Pull-up)

| GPIO | Señal | Función | Pull-up | Activo |
|------|-------|---------|---------|--------|
| **2** | BTN_LIGHTS | Botón luces | Interno | LOW |
| **40** | BTN_MEDIA | Botón multimedia | Interno | LOW |
| **41** | BTN_4X4 | Botón 4x4/4x2 | Interno | LOW |
| **45** | KEY_DETECT | Detección llave encendido | Interno | LOW |

**Configuración:**
- Modo: INPUT_PULLUP
- Debounce: 100ms en firmware
- Lógica: Botón presionado = LOW, suelto = HIGH

**Circuito típico por botón:**
```
3.3V ──── 10kΩ (pull-up interno ESP32-S3)
           │
           ├──── GPIO ──── Leer estado
           │
         [Botón]
           │
          GND
```

### Encoders de Rueda (Entradas)

| GPIO | Señal | Rueda | Tipo |
|------|-------|-------|------|
| **3** | WHEEL_FL | Front Left | Encoder Hall |
| **36** | WHEEL_FR | Front Right | Encoder Hall |
| **17** | WHEEL_RL | Rear Left | Encoder Hall |
| **15** | WHEEL_RR | Rear Right | Encoder Hall |

**Configuración:**
- Modo: INPUT_PULLUP
- Interrupciones: CHANGE (rising + falling)
- Frecuencia máx: 20 kHz por rueda
- Alimentación encoder: 12V (vía RELAY_AUX)

### Encoder Rotatorio (Entradas)

| GPIO | Señal | Tipo |
|------|-------|------|
| **37** | ENCODER_A | Fase A (CLK) |
| **38** | ENCODER_B | Fase B (DT) |
| **39** | ENCODER_Z | Índice (opcional) |

**Configuración:**
- Modo: INPUT_PULLUP
- Interrupciones: CHANGE en A y B
- Resolución: Software x4 (leer ambos flancos)
- Z index: Detección de posición absoluta

### LEDs WS2812B (Salidas)

| GPIO | Señal | Función | LEDs |
|------|-------|---------|------|
| **1** | LED_FRONT | Tira LED frontal | 16 LEDs |
| **48** | LED_REAR | Tira LED trasera | 16 LEDs |

**Configuración:**
- Protocolo: WS2812B (NeoPixel)
- Librería: FastLED 3.6.0
- Frecuencia: 800 kHz
- Voltaje: 5V (alimentación externa recomendada)
- Corriente: ~60mA por LED (blanco máximo)
- Total: 32 LEDs × 60mA = 1.92A máx

**Notas de alimentación:**
- ⚠️ NO alimentar 32 LEDs desde 3.3V del ESP32-S3
- ✅ Usar fuente externa 5V con GND común
- Añadir condensador 1000µF cerca de las tiras
- Resistencia 330Ω en serie con señal data

---

## 🎛️ GPIO Analógicos

### Pedal de Aceleración (ADC)

| GPIO | Señal | Tipo | Rango ADC | Rango Voltaje |
|------|-------|------|-----------|---------------|
| **4** | PEDAL | ADC1_CH3 | 0-4095 | 0-3.1V |

**Sensor:**
- Tipo: Sensor Hall analógico (SS495A o similar)
- Alimentación: 5V
- Salida: 0.5V (reposo) - 4.5V (máximo)
- Divisor de voltaje: 2:1 para ajustar a rango ESP32-S3 (0-3.1V)

**Divisor de voltaje:**
```
Sensor Hall (0.5-4.5V) ──── 10kΩ ────┬──── GPIO 4 (0-2.25V)
                                      │
                                    10kΩ
                                      │
                                     GND
```

**Calibración:**
- Valor mínimo ADC (pedal suelto): ~200 (0.5V / 2)
- Valor máximo ADC (pedal fondo): ~1850 (4.5V / 2)
- Mapeo: `map(adcValue, 200, 1850, 0, 100)` → % acelerador

---

## 📡 Comunicación UART

### DFPlayer Mini (Reproductor MP3)

| Señal | GPIO | Pin DFPlayer | Descripción |
|-------|------|--------------|-------------|
| **TX** | 43 | RX | UART0_TX (ESP32-S3 → DFPlayer) |
| **RX** | 44 | TX | UART0_RX (DFPlayer → ESP32-S3) |

**Configuración:**
- Puerto: UART0 (nativo)
- Baudrate: 9600 bps
- Bits: 8 data, 1 stop, sin paridad
- Control de flujo: Ninguno

**Conexiones DFPlayer:**
- VCC → 5V (100-200mA)
- GND → GND
- SPK+ / SPK- → Altavoz 3W 8Ω
- RX → GPIO 43 (vía resistencia 1kΩ)
- TX → GPIO 44

**Notas:**
- ⚠️ DFPlayer trabaja a 5V lógico, pero tolera 3.3V en RX
- ✅ Resistencia serie 1kΩ en TX del ESP32-S3 protege el pin
- Librería: DFRobotDFPlayerMini 1.0.6
- Almacenamiento: MicroSD (archivos MP3 en `/mp3/0001.mp3`, `/mp3/0002.mp3`, etc.)

---

## 🌡️ Bus OneWire

### Sensor de Temperatura DS18B20

| Señal | GPIO | Tipo |
|-------|------|------|
| **OneWire** | 20 | Bidireccional (Open-drain) |

**Conexión DS18B20:**
```
                    ┌─── 3.3V
                    │
                  4.7kΩ (pull-up)
                    │
GPIO 20 ────────────┼──── DQ (Data)
                    
DS18B20:            
VCC ─── 3.3V
GND ─── GND
DQ  ─── GPIO 20 + pull-up 4.7kΩ
```

**Configuración:**
- Resistencia pull-up: 4.7kΩ a 3.3V (externa **OBLIGATORIA**)
- Protocolo: 1-Wire (timing crítico)
- Resolución: 12-bit (0.0625°C)
- Rango: -55°C a +125°C
- Tiempo de conversión: 750ms (12-bit)

**Librería:**
- OneWire 2.3.8
- DallasTemperature 4.0.5

---

## 🔧 Valores de Pull-up/Pull-down

### Resistencias Externas Críticas

| Pin/Bus | Tipo | Valor | Conexión | Función |
|---------|------|-------|----------|---------|
| I²C SDA | Pull-up | 4.7kΩ | 3.3V | Bus I²C a 400kHz |
| I²C SCL | Pull-up | 4.7kΩ | 3.3V | Bus I²C a 400kHz |
| GPIO 0 | Pull-up | 10kΩ | 3.3V | **Boot strapping** |
| GPIO 46 | Pull-up | 10kΩ | 3.3V | **Boot strapping** |
| GPIO 45 | Pull-down | 10kΩ | GND | Strapping VDD_SPI |
| OneWire (GPIO 20) | Pull-up | 4.7kΩ | 3.3V | Protocolo OneWire |
| Relés (GPIO 5,6,7,35) | Pull-down | 10kΩ | GND | Evitar activación en boot |

### Pull-ups/Pull-downs Internos (Configurados por Firmware)

| GPIO | Tipo | Función |
|------|------|---------|
| 2, 40, 41, 45 | Pull-up interno | Botones |
| 3, 36, 17, 15 | Pull-up interno | Encoders de rueda |
| 37, 38, 39 | Pull-up interno | Encoder rotatorio |
| 47 | Pull-up interno | Touch IRQ |

**Nota:** Los pull-ups internos del ESP32-S3 son de ~45kΩ, suficientes para entradas digitales pero **insuficientes para I²C a 400kHz**.

---

## ⚡ Secuencia de Alimentación

### Orden Recomendado de Encendido

```
1. Batería 24V conectada
   │
2. Llave de encendido ON (GPIO 45 → GND)
   │
3. RELAY_MAIN activa (GPIO 35 → HIGH)
   ├─ Buck 5V arranca
   └─ ESP32-S3 recibe 5V (USB o VIN)
   
4. ESP32-S3 arranca
   ├─ ROM bootloader lee strapping pins
   ├─ Carga firmware desde flash
   └─ main() inicia
   
5. Firmware configura GPIOs
   ├─ RELAY_AUX (GPIO 5) → HIGH (12V sensores/encoders)
   └─ XSHUT_FRONT/REAR → HIGH (sensores VL53L5CX activos)
   
6. Módulos inicializan
   ├─ I²C (TCA9548A, INA226, MCP23017)
   ├─ SPI (TFT, Touch)
   ├─ UART (DFPlayer)
   └─ Sensores (VL53L5CX, DS18B20)
   
7. Sistema listo para operar
```

### Apagado Controlado

```
1. Llave OFF (GPIO 45 → HIGH)
   │
2. Firmware detecta cambio de estado
   ├─ Reproduce audio de apagado (DFPlayer)
   ├─ Guarda datos en flash (configuración)
   └─ Desactiva motores (RELAY_MOTOR → LOW)
   
3. Espera 5 segundos (SHUTDOWN_DELAY_MS)
   │
4. RELAY_MAIN → LOW (GPIO 35)
   └─ Buck 5V se apaga
      └─ ESP32-S3 pierde alimentación
```

**Notas:**
- Power Hold: RELAY_MAIN se mantiene activo después de que la llave pase a OFF
- Permite apagado graceful con guardado de datos
- Tiempo configurable: `SHUTDOWN_DELAY_MS` (default 5000ms)

---

## 🐛 Troubleshooting Común

### Problema: ESP32-S3 no arranca (stuck en download mode)

**Síntomas:**
- Serial Monitor muestra: `waiting for download`
- No ejecuta firmware
- LED de power encendido pero sin actividad

**Diagnóstico:**
- ⚠️ GPIO 0 o GPIO 46 están LOW durante boot
- Falta resistencia pull-up en GPIO 0 o GPIO 46

**Solución:**
1. Añadir pull-up 10kΩ en GPIO 0 a 3.3V
2. Añadir pull-up 10kΩ en GPIO 46 a 3.3V
3. Verificar que no haya cortocircuitos a GND
4. Presionar botón RESET en DevKitC-1

---

### Problema: Errores I²C (timeout, NACK)

**Síntomas:**
- Serial Monitor: `I2C timeout`, `NACK received`
- Dispositivos I²C no responden
- Bus I²C "congelado"

**Diagnóstico:**
- Falta pull-up en SDA/SCL
- Pull-ups internos insuficientes (45kΩ)
- Cable I²C demasiado largo (>30cm)
- Múltiples dispositivos sin pull-ups adecuados

**Solución:**
1. ✅ Añadir pull-ups **4.7kΩ** externos en SDA y SCL a 3.3V
2. Acortar cables I²C (<20cm recomendado)
3. Verificar alimentación de dispositivos (3.3V estable)
4. Usar `I2CRecovery::recoverBus()` si el bus está atascado
5. Escanear bus I²C para verificar direcciones:

```cpp
#include <Wire.h>

void scanI2C() {
    Serial.println("Escaneando I²C...");
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("Dispositivo: 0x%02X\n", addr);
        }
    }
}
```

---

### Problema: Display TFT blanco/negro o sin imagen

**Síntomas:**
- Pantalla encendida pero sin contenido
- Backlight funciona (GPIO 42)
- Imagen distorsionada o con rayas

**Diagnóstico:**
- Conexión SPI incorrecta (SCK, MOSI, MISO)
- Pin DC/CS/RST mal conectado
- Frecuencia SPI demasiado alta para cables largos
- Alimentación insuficiente (TFT consume ~200mA)

**Solución:**
1. Verificar conexiones SPI:
   - SCK: GPIO 10
   - MOSI: GPIO 11
   - MISO: GPIO 12
   - DC: GPIO 13
   - RST: GPIO 14
   - CS: GPIO 16
2. Reducir frecuencia SPI a 20 MHz (en `platformio.ini` o `User_Setup.h`)
3. Verificar alimentación 5V estable (usar fuente externa si es desde USB)
4. Verificar que backlight está encendido: `ledcWrite(0, 255);`

---

### Problema: Touch no responde o coordenadas erróneas

**Síntomas:**
- Tocar pantalla no genera respuesta
- Coordenadas incorrectas (fuera de rango)
- IRQ pin siempre HIGH o siempre LOW

**Diagnóstico:**
- XPT2046 CS (GPIO 21) en conflicto
- Calibración touch incorrecta
- IRQ pin (GPIO 47) mal configurado
- Frecuencia SPI demasiado alta para touch

**Solución:**
1. Verificar pin CS del touch: GPIO 21
2. Verificar IRQ: GPIO 47 como INPUT_PULLUP
3. Ejecutar calibración touch desde menú del sistema
4. Frecuencia SPI touch: 2 MHz (más lento que TFT)
5. Verificar valores de calibración en `storage.cpp`

---

### Problema: LEDs WS2812B no encienden o colores incorrectos

**Síntomas:**
- Algunos LEDs encendidos, otros apagados
- Colores incorrectos (rojo en vez de azul)
- Parpadeo o flickering

**Diagnóstico:**
- Alimentación insuficiente (32 LEDs × 60mA = 1.92A)
- Resistencia serie falta en data line
- GND no común entre ESP32-S3 y LEDs
- Primer LED dañado (afecta al resto de la cadena)

**Solución:**
1. Alimentar tiras LED desde fuente externa 5V (NO desde ESP32-S3)
2. Añadir condensador 1000µF cerca de las tiras (reduce picos de corriente)
3. Resistencia 330Ω en serie con data line (GPIO 1 y GPIO 48)
4. Verificar GND común entre ESP32-S3 y fuente 5V de LEDs
5. Reemplazar primer LED si está dañado

---

### Problema: Sensores VL53L5CX no detectados

**Síntomas:**
- Serial Monitor: `VL53L5CX FRONT not found`
- Multiplexer TCA9548A responde, pero sensores no
- Lecturas de distancia siempre `INVALID` (8191)

**Diagnóstico:**
- XSHUT pin en LOW (sensor en shutdown)
- Canal del multiplexer incorrecto
- Alimentación 3.3V insuficiente (sensor consume ~200mA)
- Conexiones I²C incorrectas

**Solución:**
1. Verificar XSHUT pins en HIGH:
   - XSHUT_FRONT: GPIO 46 → OUTPUT HIGH
   - XSHUT_REAR: GPIO 19 → OUTPUT HIGH
2. Verificar alimentación 3.3V estable (usar fuente externa si es necesario)
3. Escanear canales del multiplexer:

```cpp
for (uint8_t ch = 0; ch < 8; ch++) {
    I2CRecovery::tcaSelectSafe(ch, 0x71);
    Wire.beginTransmission(0x29);
    if (Wire.endTransmission() == 0) {
        Serial.printf("VL53L5CX en canal %d\n", ch);
    }
}
```

4. Verificar pull-up 10kΩ en GPIO 46 (XSHUT_FRONT)

---

### Problema: DFPlayer no reproduce audio

**Síntomas:**
- DFPlayer no responde a comandos
- Serial Monitor: `DFPlayer timeout`
- Audio no se reproduce

**Diagnóstico:**
- Conexión UART incorrecta (TX/RX invertidos)
- MicroSD no insertada o sin archivos MP3
- Baudrate incorrecto (debe ser 9600)
- Alimentación insuficiente (DFPlayer requiere 5V, 200mA)

**Solución:**
1. Verificar conexiones UART:
   - ESP32-S3 TX (GPIO 43) → DFPlayer RX
   - ESP32-S3 RX (GPIO 44) → DFPlayer TX
2. Verificar MicroSD:
   - Archivos en `/mp3/0001.mp3`, `/mp3/0002.mp3`, etc.
   - Formato FAT32
   - Archivos MP3 válidos (bitrate <128kbps recomendado)
3. Verificar baudrate: 9600 en `DFPlayer.begin(Serial0)`
4. Alimentación 5V estable (usar fuente externa, NO desde 3.3V ESP32-S3)

---

## 📚 Referencias

- **ESP32-S3 Technical Reference Manual**: [Espressif](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf)
- **ESP32-S3 Datasheet**: [Espressif](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf)
- **Strapping Pins Guide**: [ESP32-S3 Bootstrap](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/bootloader.html#strapping-pins)
- **TCA9548A Datasheet**: [Texas Instruments](https://www.ti.com/lit/ds/symlink/tca9548a.pdf)
- **VL53L5CX Datasheet**: [STMicroelectronics](https://www.st.com/resource/en/datasheet/vl53l5cx.pdf)
- **PCA9685 Datasheet**: [NXP](https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf)

---

## 📝 Historial de Cambios

| Versión | Fecha | Cambios |
|---------|-------|---------|
| v1.0 | 2025-12-23 | Documento inicial - Configuración completa de hardware |

---

**Soporte:** Para problemas o preguntas, abrir un issue en [GitHub](https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos/issues)
