# Inventario de Componentes de Almacén

**Proyecto:** FULL-FIRMWARE-Coche-Marcos  
**Fecha de creación:** 2026-02-01  
**Propósito:** Lista completa de componentes disponibles en almacén para implementación de sistema de control vehicular con ESP32-S3 N16R8 y STM32G474RE

---

## 📋 Índice

1. [Microcontroladores y Placas de Desarrollo](#1-microcontroladores-y-placas-de-desarrollo)
2. [Comunicación CAN-Bus](#2-comunicación-can-bus)
3. [Aislamiento y Protección](#3-aislamiento-y-protección)
4. [Temporizadores y Relés](#4-temporizadores-y-relés)
5. [Control de Motores](#5-control-de-motores)
6. [Sensores](#6-sensores)
7. [Display y Interfaz de Usuario](#7-display-y-interfaz-de-usuario)
8. [Iluminación](#8-iluminación)
9. [Audio](#9-audio)
10. [Componentes Pasivos y Conectores](#10-componentes-pasivos-y-conectores)
11. [Recomendaciones de Implementación](#11-recomendaciones-de-implementación)

---

## 1. Microcontroladores y Placas de Desarrollo

### 1.1 ESP32-S3 (HMI - Interfaz Humano-Máquina)

| Componente | Cantidad | Especificación | Uso en Proyecto |
|------------|----------|----------------|-----------------|
| **ESP32-S3 N16R8** | 1 | - 16MB Flash QIO (4-bit, 3.3V)<br>- 8MB PSRAM OPI (8-bit, 3.3V)<br>- Dual-core Xtensa LX7 @ 240 MHz<br>- Flash @ 80 MHz<br>- PSRAM @ 80 MHz | Controlador principal HMI:<br>- Display TFT + Touch<br>- Audio<br>- LEDs WS2812B<br>- Detección obstáculos<br>- Menús y diagnóstico |
| **Placa de desarrollo ESP32-S3-DevKitC-1** | 1 | - 44 pines<br>- USB nativo<br>- Compatible con N16R8 | Plataforma de desarrollo |

### 1.2 STM32G474RE (Control Seguro)

| Componente | Cantidad | Especificación | Uso en Proyecto |
|------------|----------|----------------|-----------------|
| **STM32G474RE** (NUCLEO-G474RE) | 1 | - ARM Cortex-M4F @ 170 MHz<br>- 512 KB Flash<br>- 128 KB RAM<br>- FDCAN integrado (CAN FD)<br>- 3× ADC 12-bit<br>- FPU, DSP | Controlador de seguridad:<br>- Control de motores<br>- Sensores críticos<br>- ABS/TCS<br>- Relés de potencia<br>- Lógica de seguridad |

**Nota importante:** El STM32G474RE incluye **FDCAN (Controller Area Network con soporte CAN FD)** integrado, eliminando la necesidad de controlador CAN externo.

---

## 2. Comunicación CAN-Bus

### 2.1 Transreceptores CAN

| Componente | Cantidad | Especificación | Uso en Proyecto |
|------------|----------|----------------|-----------------|
| **TJA1051T/3** | **2** | - High-Speed CAN Transceiver<br>- Compatible ISO 11898-2<br>- Hasta 1 Mbps (config. 500 kbps)<br>- Alimentación: 5V<br>- Lógica compatible: 3.3V<br>- Temp: -40°C a +125°C<br>- Encapsulado: SO-8 | **Unidad #1:** STM32 ↔ CAN Bus<br>**Unidad #2:** ESP32 ↔ CAN Bus<br><br>Comunicación entre microcontroladores @ 500 kbps |

### 2.2 Componentes Auxiliares CAN

| Componente | Cantidad | Especificación | Uso |
|------------|----------|----------------|-----|
| **Resistencias terminación CAN** | 2 | 120Ω, 1/4W | Terminación de bus CAN en ambos extremos |
| **Cable par trenzado** | ~2-5m | AWG 24-26<br>Impedancia característica ~120Ω | Bus físico CANH/CANL |

---

## 3. Aislamiento y Protección

### 3.1 Optoacopladores

| Componente | Cantidad | Módulos | Especificación | Uso en Proyecto |
|------------|----------|---------|----------------|-----------------|
| **Optoacopladores** | **2** | **8 módulos cada uno**<br>(Total: 16 canales) | Típicamente PC817 o similar<br>- Aislamiento: 5000V<br>- Corriente forward: 20-50 mA<br>- Velocidad: < 80 kHz | - Aislamiento de señales digitales<br>- Protección entre etapas de potencia/control<br>- Interfaz con relés<br>- Aislamiento de sensores críticos |

**Distribución sugerida:**
- **Módulo #1 (8 canales):** Señales de control de relés y actuadores de potencia
- **Módulo #2 (8 canales):** Señales de sensores críticos y protecciones

---

## 4. Temporizadores y Relés

### 4.1 Temporizadores de Retardo

| Componente | Cantidad | Especificación | Uso en Proyecto |
|------------|----------|----------------|-----------------|
| **Temporizador de retardo 12V** | 1+ | - Tensión bobina: 12V DC<br>- Retardo ajustable (típ. 0.1s-10s)<br>- Contactos: SPDT o DPDT<br>- Corriente: 10A típico | - Retardo en arranque de sistemas<br>- Secuenciación de power-up<br>- Protección de arranque suave |
| **Temporizador de retardo 24V** | 1+ | - Tensión bobina: 24V DC<br>- Retardo ajustable (típ. 0.1s-10s)<br>- Contactos: SPDT o DPDT<br>- Corriente: 10A típico | - Control de etapas de potencia 24V<br>- Retardo en activación de motores<br>- Secuenciación de subsistemas |

### 4.2 Relés de Potencia

| Componente | Cantidad | Especificación | Uso |
|------------|----------|----------------|-----|
| **Relé Main Power** | 1 | 12V/24V, 30-40A | Alimentación principal del sistema |
| **Relé Tracción** | 1 | 12V/24V, 30-40A | Control de alimentación motores tracción |
| **Relé Dirección** | 1 | 12V/24V, 20-30A | Control de alimentación motor dirección |

---

## 5. Control de Motores

### 5.1 Drivers de Motor

| Componente | Cantidad | Especificación | Uso |
|------------|----------|----------------|-----|
| **BTS7960** | 5 | - H-Bridge de alta corriente<br>- 43A continua, 60A pico<br>- PWM hasta 25 kHz<br>- Protección térmica/cortocircuito | **5 unidades total:**<br>- 4× Control motores tracción (1 por rueda)<br>- 1× Control motor dirección |

### 5.2 Controlador PWM

| Componente | Cantidad | Especificación | Uso |
|------------|----------|----------------|-----|
| **PCA9685** | 3 | - 16 canales PWM I2C<br>- 12-bit resolución<br>- Frecuencia: 40-1000 Hz | **3 unidades total:**<br>- PCA9685 #1 (0x40): Motores eje delantero (FL+FR)<br>- PCA9685 #2 (0x41): Motores eje trasero (RL+RR)<br>- PCA9685 #3 (0x42): Motor dirección |

### 5.3 Expansor GPIO

| Componente | Cantidad | Especificación | Uso |
|------------|----------|----------------|-----|
| **MCP23017** | 1 | - Expansor GPIO I2C<br>- 16 pines I/O (GPIOA + GPIOB)<br>- Dirección I2C: 0x20 | Control IN1/IN2 de los 5× BTS7960<br>Lectura del shifter (5 posiciones)<br>13/16 pines utilizados |

---

## 6. Sensores

### 6.1 Sensores de Corriente

| Componente | Cantidad | Especificación | Uso |
|------------|----------|----------------|-----|
| **INA226** | 6 | - Sensor I2C<br>- ±16V bus voltage<br>- Alta precisión<br>- Alertas configurables | Monitoreo de corriente:<br>- 4× motores tracción<br>- 1× motor dirección<br>- 1× sistema general |
| **TCA9548A** | 1 | Multiplexor I2C 1:8 | Gestión de múltiples INA226 con misma dirección |

### 6.2 Sensores de Temperatura

| Componente | Cantidad | Especificación | Uso |
|------------|----------|----------------|-----|
| **DS18B20** | 4+ | - Sensor digital 1-Wire<br>- Rango: -55°C a +125°C<br>- Precisión: ±0.5°C<br>- Direccionable | Monitoreo térmico:<br>- Motores<br>- Drivers<br>- Baterías |

### 6.3 Sensores de Velocidad

| Componente | Cantidad | Especificación | Uso |
|------------|----------|----------------|-----|
| **Sensores de rueda** | 4 | Típicamente Hall o magnéticos<br>Salida digital | Medición velocidad individual de cada rueda (ABS/TCS) |

### 6.4 Encoder de Dirección

| Componente | Cantidad | Especificación | Uso |
|------------|----------|----------------|-----|
| **Encoder E6B2-CWZ6C** | 1 | - 360 PPR (Pulsos Por Revolución)<br>- Salidas A/B/Z<br>- Encoder incremental | Medición precisa de ángulo de dirección<br>Cálculo Ackermann |

### 6.5 Sensores de Obstáculos

| Componente | Cantidad | Especificación | Uso |
|------------|----------|----------------|-----|
| **TOFSense-M S** (LiDAR 8x8 Matrix) | 1 | - Sensor de distancia láser 8x8<br>- Rango: 0.2-4m<br>- Interfaz UART (115200 bps)<br>- FOV: 65°<br>- Update rate: ~100Hz | Detección de obstáculos **frontal únicamente**<br>Conectado a GPIO44 (RX) ESP32-S3 |

### 6.6 Sensores de Entrada de Usuario

| Componente | Cantidad | Especificación | Uso |
|------------|----------|----------------|-----|
| **Pedal analógico Hall** | 1 | - Sensor efecto Hall sin contacto<br>- Salida analógica 0-3.3V<br>- Sin desgaste mecánico | Entrada de aceleración/frenado |
| **Shifter mecánico** | 1 | - 3 posiciones: F/N/R<br>- Contactos o encoder | Selector Forward/Neutral/Reverse |

---

## 7. Display y Interfaz de Usuario

### 7.1 Pantalla TFT

| Componente | Cantidad | Especificación | Uso |
|------------|----------|----------------|-----|
| **ST7796S TFT Display** | 1 | - 480×320 píxeles<br>- 3.5" o 4"<br>- 16-bit color (65K)<br>- Interfaz SPI (HSPI) | Display principal HMI:<br>- HUD<br>- Menús<br>- Diagnóstico<br>- Visualización datos |

### 7.2 Pantalla Táctil

| Componente | Cantidad | Especificación | Uso |
|------------|----------|----------------|-----|
| **XPT2046 Touch Controller** | 1 | - Controlador táctil resistivo<br>- Interfaz SPI<br>- 12-bit ADC | Entrada táctil de usuario |

### 7.3 Pines de Conexión Display

| Señal | Pin ESP32-S3 | Descripción |
|-------|--------------|-------------|
| TFT_CS | GPIO 16 | Chip Select TFT |
| TFT_DC | GPIO 13 | Data/Command |
| TFT_RST | GPIO 14 | Reset |
| TFT_MOSI | GPIO 11 | Master Out Slave In |
| TFT_MISO | GPIO 12 | Master In Slave Out |
| TFT_SCLK | GPIO 10 | Clock |
| TFT_BL | GPIO 42 | Backlight |
| TOUCH_CS | GPIO 21 | Chip Select Touch |

---

## 8. Iluminación

### 8.1 LEDs Direccionables

| Componente | Cantidad | Especificación | Uso |
|------------|----------|----------------|-----|
| **WS2812B Frontal** | 28 LEDs | - LEDs RGB direccionables<br>- Protocolo 1-wire<br>- 5V alimentación<br>- Control digital | Luces delanteras:<br>- Posición<br>- Direccionales<br>- Freno<br>- Efectos |
| **WS2812B Trasero** | 16 LEDs | - LEDs RGB direccionables<br>- Protocolo 1-wire<br>- 5V alimentación | Luces traseras:<br>- Posición<br>- Direccionales<br>- Freno<br>- Reversa |

### 8.2 Pines de Control Iluminación

| Componente | Pin ESP32-S3 | Descripción |
|------------|--------------|-------------|
| LED_FRONT | GPIO 1 | Control WS2812B frontal (28 LEDs) |
| LED_REAR | GPIO 48 | Control WS2812B trasero (16 LEDs) |

---

## 9. Audio

| Componente | Cantidad | Especificación | Uso |
|------------|----------|----------------|-----|
| **DFPlayer Mini** | 1 | - Reproductor MP3/WAV<br>- Control UART<br>- MicroSD hasta 32GB<br>- Amplificador 3W integrado<br>- Salida speaker + jack 3.5mm | Feedback audible:<br>- Alertas<br>- Confirmaciones<br>- Avisos del sistema |

---

## 10. Componentes Pasivos y Conectores

### 10.1 Buses de Comunicación

| Bus | Pines ESP32-S3 | Pines STM32 | Uso |
|-----|----------------|-------------|-----|
| **I2C** | SDA: GPIO 8<br>SCL: GPIO 9 | SDA: PB7<br>SCL: PB6 | - INA226 (via TCA9548A)<br>- PCA9685 (×3: 0x40, 0x41, 0x42)<br>- MCP23017 (0x20)<br>- Sensores I2C varios |
| **SPI HSPI** | MOSI: GPIO 13<br>MISO: -<br>SCLK: GPIO 14 | - | Display ST7796S + Touch XPT2046 |
| **1-Wire** | GPIO configurable | GPIO configurable | Sensores DS18B20 |
| **CAN** | TX: GPIO 20 (propuesto)<br>RX: GPIO 21 (propuesto) | TX: PB9<br>RX: PB8 | Comunicación ESP32 ↔ STM32 @ 500 kbps |
| **UART0** | RX: GPIO 44 | - | TOFSense-M S (sensor obstáculos frontal) |
| **UART1** | TX: GPIO 18<br>RX: GPIO 17 | - | DFPlayer Mini (audio) |

### 10.2 Conectores y Cableado

| Componente | Cantidad Estimada | Especificación |
|------------|-------------------|----------------|
| **Conectores JST** | Variable | 2-8 pines, para sensores y periféricos |
| **Conectores Phoenix** | 10-20 | Bloques de terminales para potencia |
| **Conectores Dupont** | 100+ | Macho/Hembra, 2.54mm pitch |
| **Cable calibre 18-20 AWG** | 10-20m | Para señales de control |
| **Cable calibre 14-16 AWG** | 5-10m | Para potencia motores |
| **Cable par trenzado CAN** | 2-5m | Impedancia ~120Ω |

### 10.3 Componentes Pasivos Esenciales

| Componente | Cantidad | Especificación |
|------------|----------|----------------|
| **Resistencias pull-up 4.7kΩ** | 10+ | Para I2C, 1-Wire |
| **Resistencias 120Ω** | 2 | Terminación CAN bus |
| **Condensadores 100nF** | 20+ | Desacoplo alimentación |
| **Condensadores 10µF-100µF** | 10+ | Filtrado fuente alimentación |
| **Diodos 1N4007** | 10+ | Protección flyback relés |
| **Fusibles** | 10+ | Protección circuitos potencia |

---

## 11. Recomendaciones de Implementación

### 11.1 Mejoras en Conexiones Directas

#### A) Uso de Optoacopladores para Aislamiento

**Recomendación:** Utilizar los 2 módulos de 8 canales para separar galvánicamente:

1. **Módulo Optoacoplador #1 (8 canales) - Control de Potencia:**
   - Canal 1: Comando relé Main Power (STM32 → Relé)
   - Canal 2: Comando relé Tracción (STM32 → Relé)
   - Canal 3: Comando relé Dirección (STM32 → Relé)
   - Canal 4: Señal de emergencia/paro (bidireccional)
   - Canales 5-8: Reserva para futuras expansiones

2. **Módulo Optoacoplador #2 (8 canales) - Sensores Críticos:**
   - Canal 1-4: Señales de sensores de rueda (velocidad)
   - Canal 5: Señal de freno de emergencia
   - Canal 6-8: Reserva para sensores adicionales

**Beneficios:**
- ✅ Aislamiento eléctrico entre etapas de control y potencia
- ✅ Protección contra ruido EMI/RFI
- ✅ Prevención de lazos de tierra
- ✅ Mayor robustez del sistema

#### B) Implementación de Temporizadores de Retardo

**Secuencia de Power-Up Recomendada:**

```
T=0s:     Power ON
          ↓
T=0s:     Temporizador 12V activado
          ↓
T=0.5s:   ESP32-S3 alimentado (HMI inicia)
          ↓
T=1.0s:   STM32G474RE alimentado (Control inicia)
          ↓
T=1.5s:   Temporizador 24V activado
          ↓
T=2.0s:   Drivers de motor alimentados
          ↓
T=2.5s:   Sistema listo para operación
```

**Conexiones sugeridas:**
- **Temporizador 12V:** Control de alimentación de microcontroladores y lógica
- **Temporizador 24V:** Control de alimentación de etapas de potencia (motores)

**Beneficios:**
- ✅ Evita picos de corriente en arranque
- ✅ Permite inicialización ordenada de sistemas
- ✅ Protege componentes sensibles
- ✅ Reduce stress eléctrico

#### C) Bus CAN Optimizado

**Topología recomendada:**

```
TJA1051T/3 #1 (STM32)  ────[120Ω]──── CANH ────[120Ω]──── TJA1051T/3 #2 (ESP32)
                                       CANL
                                        │
                                    GND común
```

**Especificaciones:**
- Velocidad: 500 kbps (equilibrio velocidad/robustez)
- Longitud máxima cable: < 100m @ 500 kbps
- Terminación: 120Ω en **ambos extremos** (ya disponibles)
- Cable: Par trenzado con blindaje (recomendado para ambiente vehicular)

**Beneficios:**
- ✅ Comunicación determinística y robusta
- ✅ Inmunidad a ruido electromagnético
- ✅ Separación clara de responsabilidades HMI/Control
- ✅ Escalabilidad para futuros nodos

#### D) Mejora de Conexiones I2C

**Problema común:** Múltiples INA226 con conflicto de direcciones

**Solución implementada:** TCA9548A (multiplexor I2C 1:8)

```
                    ┌── INA226 #1 (Motor Tracción 1)
                    ├── INA226 #2 (Motor Tracción 2)
MCU ──I2C──TCA9548A─┤── INA226 #3 (Motor Tracción 3)
                    ├── INA226 #4 (Motor Tracción 4)
                    ├── INA226 #5 (Motor Dirección)
                    ├── INA226 #6 (Sistema General)
                    ├── PCA9685 #1 (0x40 - Eje delantero)
                    ├── PCA9685 #2 (0x41 - Eje trasero)
                    ├── PCA9685 #3 (0x42 - Dirección)
                    ├── MCP23017 (0x20 - Expansor GPIO)
                    └── otros sensores I2C
```

**Nota:** TOFSense-M S se conecta por UART0 (GPIO44 RX), no por I2C.

**Beneficios:**
- ✅ Hasta 8 buses I2C independientes
- ✅ Permite usar 6+ INA226 sin conflictos
- ✅ Organización lógica por subsistema
- ✅ Facilita diagnóstico y mantenimiento

### 11.2 Distribución de Componentes ESP32 vs STM32

**Criterio de asignación:** Separar HMI (ESP32) de Control Seguro (STM32)

#### Permanecen en ESP32-S3 (HMI):
- ✅ Display ST7796S + Touch XPT2046
- ✅ Audio DFPlayer Mini
- ✅ LEDs WS2812B (28+16)
- ✅ Detección obstáculos TOFSense-M S (sensor único frontal, percepción visual)
- ✅ Menús, diagnóstico, visualización
- ✅ Relé power-hold mínimo (para mantener ESP32 encendido)

#### Migran a STM32G474RE (Control):
- ✅ Control de motores (5× BTS7960: 4 tracción + 1 dirección)
- ✅ Encoder de dirección E6B2-CWZ6C
- ✅ Sensores de rueda (4×)
- ✅ Sensores de corriente INA226 (6× via TCA9548A)
- ✅ Sensores de temperatura DS18B20
- ✅ Pedal + Shifter
- ✅ Relés de potencia (Main, Tracción, Dirección)
- ✅ Lógica ABS/TCS y seguridad

**Beneficios de la separación:**
- ✅ ESP32 se dedica 100% a HMI (sin carga crítica de tiempo real)
- ✅ STM32 ejecuta control determinístico @ 170 MHz
- ✅ Fallo en HMI no afecta seguridad
- ✅ Firmware más simple y mantenible

### 11.3 Checklist de Implementación Física

#### Fase 1: Preparación
- [ ] Verificar inventario de todos los componentes listados
- [ ] Preparar herramientas: soldador, multímetro, osciloscopio
- [ ] Diseñar/adquirir PCB prototipo o usar protoboard inicial
- [ ] Preparar cables y conectores organizados por subsistema

#### Fase 2: Power Supply y Protección
- [ ] Instalar fuente de alimentación regulada 5V/12V/24V
- [ ] Conectar temporizador 12V para lógica/MCU
- [ ] Conectar temporizador 24V para etapas de potencia
- [ ] Instalar fusibles en todas las líneas de potencia
- [ ] Verificar secuencia de power-up con multímetro

#### Fase 3: Microcontroladores
- [ ] Montar ESP32-S3-DevKitC-1 N16R8
- [ ] Montar STM32G474RE NUCLEO
- [ ] Conectar alimentación 5V regulada a ambos
- [ ] Verificar boot correcto en ambos MCU

#### Fase 4: Bus CAN
- [ ] Soldar/conectar TJA1051T/3 #1 a STM32 (PB8/PB9)
- [ ] Soldar/conectar TJA1051T/3 #2 a ESP32 (GPIO 20/21 propuesto)
- [ ] Tender cable par trenzado para CANH/CANL
- [ ] Instalar resistencias de terminación 120Ω en ambos extremos
- [ ] Verificar continuidad y resistencia total (~60Ω entre CANH-CANL)

#### Fase 5: Optoacopladores
- [ ] Montar módulo optoacoplador #1 (control potencia)
- [ ] Montar módulo optoacoplador #2 (sensores)
- [ ] Conectar comandos de relés a través de optoacopladores
- [ ] Conectar señales críticas de sensores vía optoacopladores
- [ ] Verificar aislamiento con multímetro (resistencia infinita)

#### Fase 6: Display y HMI (ESP32)
- [ ] Conectar ST7796S (CS=16, DC=13, RST=14, SPI en GPIOs 10-12)
- [ ] Conectar XPT2046 Touch (CS=21, SPI compartido)
- [ ] Conectar backlight (GPIO 42)
- [ ] Verificar display con sketch de prueba
- [ ] Calibrar touch

#### Fase 7: Sensores y Control I2C (ESP32/STM32)
- [ ] Conectar INA226 (×6) vía TCA9548A al I2C
- [ ] Conectar PCA9685 (×3) para PWM de motores (0x40, 0x41, 0x42)
- [ ] Conectar MCP23017 expansor GPIO (0x20) para control BTS7960 y shifter
- [ ] Conectar DS18B20 en bus 1-Wire
- [ ] Conectar encoder E6B2-CWZ6C (A/B/Z)
- [ ] Conectar sensores de rueda (×4)
- [ ] Conectar pedal analógico Hall
- [ ] Conectar shifter mecánico al MCP23017 vía optoacopladores
- [ ] Verificar lectura de cada sensor individualmente

#### Fase 8: Control de Motores
- [ ] Conectar drivers BTS7960 (×5: 4 tracción + 1 dirección) con protecciones
- [ ] Configurar control PWM desde PCA9685 a BTS7960
- [ ] Configurar control direccional IN1/IN2 desde MCP23017 a BTS7960
- [ ] Verificar PWM en banco de pruebas (sin carga)
- [ ] Verificar protecciones térmicas y de corriente

#### Fase 9: Relés y Potencia
- [ ] Instalar relé Main Power con diodo flyback
- [ ] Instalar relé Tracción con diodo flyback
- [ ] Instalar relé Dirección con diodo flyback
- [ ] Conectar comandos de relés vía optoacopladores
- [ ] Verificar secuencia de activación segura

#### Fase 10: Audio, Iluminación y Sensores (ESP32)
- [ ] Conectar DFPlayer Mini (UART1, GPIO 18/17)
- [ ] Conectar TOFSense-M S (UART0, GPIO 44 RX)
- [ ] Conectar WS2812B frontal (GPIO 1, 28 LEDs)
- [ ] Conectar WS2812B trasero (GPIO 48, 16 LEDs)
- [ ] Cargar pistas de audio en SD del DFPlayer
- [ ] Verificar patrones de LEDs, audio y detección de obstáculos

#### Fase 11: Validación del Sistema
- [ ] Verificar comunicación CAN bidireccional
- [ ] Verificar heartbeat ESP32 ↔ STM32
- [ ] Probar comandos de HMI → Control
- [ ] Probar feedback de estado Control → HMI
- [ ] Validar modos de fallo seguro (pérdida CAN, timeout)
- [ ] Ejecutar test de integración completo

### 11.4 Herramientas Recomendadas

| Herramienta | Uso |
|-------------|-----|
| **Multímetro digital** | Verificación continuidad, voltajes, resistencias |
| **Osciloscopio** | Análisis señales CAN, PWM, encoders |
| **Analizador lógico** | Debug I2C, SPI, 1-Wire |
| **Analizador CAN** | Monitoreo tráfico CAN bus |
| **Fuente alimentación** | Suministro regulado 5V/12V/24V |
| **Soldador + estaño** | Ensamblaje conexiones permanentes |
| **Crimper JST/Dupont** | Fabricación cables personalizados |

---

## 12. Resumen de Componentes Críticos

### Por Prioridad de Implementación

#### Prioridad 1 - Esencial para Boot y Comunicación
1. ✅ ESP32-S3 N16R8 + Placa DevKitC-1
2. ✅ STM32G474RE NUCLEO
3. ✅ TJA1051T/3 (×2) + resistencias 120Ω (×2)
4. ✅ Fuente alimentación 5V regulada
5. ✅ Cable par trenzado CAN

#### Prioridad 2 - Control y Seguridad
6. ✅ BTS7960 (×5: 4 tracción + 1 dirección)
7. ✅ PCA9685 (×3 para PWM motores) + MCP23017 (×1 expansor GPIO)
8. ✅ INA226 (×6) + TCA9548A
9. ✅ Encoder E6B2-CWZ6C
10. ✅ Sensores de rueda (×4)
11. ✅ Relés de potencia (×3)
12. ✅ Optoacopladores 8 módulos (×2)
13. ✅ Temporizadores 12V y 24V

#### Prioridad 3 - HMI y Usuario
14. ✅ Display ST7796S + Touch XPT2046
15. ✅ DFPlayer Mini
16. ✅ WS2812B LEDs (28+16)
17. ✅ Pedal analógico Hall + Shifter

#### Prioridad 4 - Sensores Adicionales
18. ✅ DS18B20 (×4+)
19. ✅ TOFSense-M S (×1 frontal, UART)
20. (Reserva para futura expansión)

---

## 13. Referencias Técnicas

### Datasheets y Documentación

| Componente | Documento |
|------------|-----------|
| ESP32-S3 | [Espressif ESP32-S3 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf) |
| STM32G474RE | [STM32G474xx Reference Manual](https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) |
| TJA1051T/3 | [NXP TJA1051 Datasheet](https://www.nxp.com/docs/en/data-sheet/TJA1051.pdf) |
| BTS7960 | [Infineon BTS7960 Datasheet](https://www.infineon.com/dgdl/Infineon-BTS7960-DataSheet-v01_00-EN.pdf?fileId=db3a30433fa9412f013fbe32289b7c17) |
| INA226 | [Texas Instruments INA226](https://www.ti.com/lit/ds/symlink/ina226.pdf) |
| ST7796S | [Sitronix ST7796S](https://www.displayfuture.com/Display/datasheet/controller/ST7796s.pdf) |

### Documentación del Proyecto

| Documento | Ubicación |
|-----------|-----------|
| Hardware ESP32-S3 | [HARDWARE.md](HARDWARE.md) |
| Manual Transreceptores CAN | [docs/MANUAL_TRANSRECEPTORES_STM32_ESP32.md](docs/MANUAL_TRANSRECEPTORES_STM32_ESP32.md) |
| Pinout STM32G474RE | [docs/STM32G474RE_PINOUT_DEFINITIVO.md](docs/STM32G474RE_PINOUT_DEFINITIVO.md) |
| Plan Separación HMI/Control | [docs/PLAN_SEPARACION_STM32_CAN.md](docs/PLAN_SEPARACION_STM32_CAN.md) |
| Mapeo de Pines ESP32 | [docs/PIN_MAPPING_DEVKITC1.md](docs/PIN_MAPPING_DEVKITC1.md) |

---

## 14. Notas Adicionales

### Componentes en Almacén vs Componentes Planificados

**Estado actual del proyecto:**
- ✅ **Firmware ESP32-S3:** Totalmente operativo (v2.17.1 PHASE 14)
- ⏳ **Hardware STM32G474RE:** Planificado y documentado
- ⏳ **Integración CAN:** En fase de diseño

### Componentes Confirmados en Almacén (según issue)

| Componente | Cantidad | Estado |
|------------|----------|--------|
| STM32G474RE | 1 | ✅ Disponible |
| TJA1051T/3 | 2 | ✅ Disponible |
| Optoacopladores 8 módulos | 2 | ✅ Disponible |
| Temporizador retardo 12V | 1+ | ✅ Disponible |
| Temporizador retardo 24V | 1+ | ✅ Disponible |

### Próximos Pasos Recomendados

1. **Verificación física:** Confirmar que todos los componentes listados están físicamente en almacén
2. **Organización:** Etiquetar y organizar componentes por subsistema
3. **Adquisiciones:** Identificar componentes faltantes (si los hay) y proceder a compra
4. **Prototipado:** Comenzar con implementación CAN entre ESP32 y STM32
5. **Validación incremental:** Probar cada subsistema antes de integración completa

---

**Documento creado:** 2026-02-01  
**Versión:** 1.0  
**Mantenido por:** Equipo FULL-FIRMWARE-Coche-Marcos  
**Repositorio:** [florinzgz/FULL-FIRMWARE-Coche-Marcos](https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos)

---

**📌 NOTA IMPORTANTE:** Este inventario debe actualizarse cuando se adquieran nuevos componentes o se modifique el diseño del sistema.

---

## Historial de Cambios

### Versión 1.1 - 2026-02-06
**Correcciones implementadas:**
- ✅ **BTS7960:** Corregido de 4 a **5 unidades** (4 para motores de tracción + 1 para motor de dirección)
- ✅ **TOFSense:** Corregido de "2-4+" a **1 unidad** (TOFSense-M S LiDAR 8x8 Matrix, montado únicamente en la parte frontal, conexión UART)
- ✅ **PCA9685:** Corregido de 1 a **3 unidades** (2 para motores de tracción en ejes delantero/trasero + 1 para motor de dirección)
- ✅ **MCP23017:** **Agregado** (1 unidad, expansor GPIO I2C para control IN1/IN2 de BTS7960 y lectura de shifter)
- ✅ **Conexión I2C:** Actualizada para reflejar que TOFSense-M S usa UART0 (GPIO44), no I2C
- ✅ **Buses de comunicación:** Agregadas secciones UART0 (TOFSense) y UART1 (DFPlayer)
- ✅ **Prioridades de implementación:** Actualizadas para reflejar correctamente los componentes
- ✅ **Diagrama I2C:** Actualizado para mostrar las 3 unidades PCA9685 con sus direcciones (0x40, 0x41, 0x42) y MCP23017 (0x20)

### Versión 1.0 - 2026-02-01
**Creación inicial del documento**
