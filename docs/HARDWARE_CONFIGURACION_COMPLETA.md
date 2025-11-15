# Configuración Hardware Completa - ESP32-S3-DevKitC-1
## Firmware Coche Inteligente Marcos v2.0

---

## 📋 Resumen del Sistema

Este documento detalla la configuración hardware completa del sistema de control del coche eléctrico inteligente basado en **ESP32-S3-DevKitC-1 (44 pines)**.

### Características Principales

- ✅ **Control de 4 motores de tracción** independientes (BTS7960 43A)
- ✅ **Motor de dirección asistida** RS390 12V 6000RPM + reductora 1:50
- ✅ **Monitorización de corriente** en tiempo real (6 canales INA226)
- ✅ **Sistema de dirección preciso** con encoder 1200PPR + señal Z
- ✅ **Detección de velocidad de ruedas** con sensores inductivos
- ✅ **Interfaz táctil** ILI9488 480x320
- ✅ **Sistemas avanzados de seguridad** (ABS, TCS, Regenerativo)
- ✅ **Iluminación LED inteligente** estilo KITT
- ✅ **Conectividad WiFi** y OTA updates

---

## 🔌 Hardware Integrado

### 1. Controlador Principal
- **ESP32-S3-DevKitC-1** (44 pines)
  - GPIOs disponibles: 0-48
  - Flash: 16MB (N16R8)
  - PSRAM: 8MB
  - USB Type-C integrado

### 2. Sensores de Corriente (6x INA226)

Todos los INA226 usan **shunts externos CG FL-2C** de alta precisión (±0.5% Class 0.5).

| Canal | Aplicación | Shunt | I²C Mux | GPIO |
|-------|------------|-------|---------|------|
| 0 | Motor FL (Frontal Izq) | CG FL-2C 50A 75mV | TCA9548A Ch0 | SDA=21, SCL=20 |
| 1 | Motor FR (Frontal Der) | CG FL-2C 50A 75mV | TCA9548A Ch1 | SDA=21, SCL=20 |
| 2 | Motor RL (Trasera Izq) | CG FL-2C 50A 75mV | TCA9548A Ch2 | SDA=21, SCL=20 |
| 3 | Motor RR (Trasera Der) | CG FL-2C 50A 75mV | TCA9548A Ch3 | SDA=21, SCL=20 |
| 4 | **Batería 24V** | **CG FL-2C 100A 75mV** | TCA9548A Ch4 | SDA=21, SCL=20 |
| 5 | Motor Dirección RS390 | CG FL-2C 50A 75mV | TCA9548A Ch5 | SDA=21, SCL=20 |

**Especificaciones Shunts CG FL-2C:**
- Precisión: ±0.5% (Class 0.5)
- Voltaje nominal: 75mV @ corriente nominal
- Corrientes nominales: 100A (batería) / 50A (motores)
- Resistencia shunt batería: 0.00075Ω (75mV/100A)
- Resistencia shunt motores: 0.0015Ω (75mV/50A)

### 3. Multiplexadores I²C (5x TCA9548A)

Permiten conectar múltiples dispositivos I²C con la misma dirección.

| Módulo | Dirección I²C | Canales Usados | Dispositivos |
|--------|---------------|----------------|--------------|
| TCA9548A #1 | 0x70 | 0-5 (6 canales) | 6x INA226 |
| TCA9548A #2 | 0x71 | Reserva | Expansión futura |
| TCA9548A #3 | 0x72 | Reserva | Expansión futura |
| TCA9548A #4 | 0x73 | Reserva | Expansión futura |
| TCA9548A #5 | 0x74 | Reserva | Expansión futura |

### 4. Driver PWM (1x PCA9685)

- **Dirección I²C:** 0x41 (modificada para evitar conflicto con INA226 0x40)
- **Resolución:** 12-bit (0-4095)
- **Frecuencia:** 1000 Hz (motor dirección)
- **Aplicación:** Control PWM motor dirección RS390
- **⚠️ IMPORTANTE:** Soldar puente A0 en placa PCA9685 para cambiar dirección a 0x41

### 5. Expansor GPIO (1x MCP23017)

- **Dirección I²C:** 0x20
- **Pines:** 16 GPIO (GPA0-GPA7, GPB0-GPB7)
- **Aplicación:** Control de relés de potencia
- **Ventaja:** Libera GPIOs del ESP32-S3

### 6. Drivers de Motor

#### Tracción (4x BTS7960)

| Motor | PWM | IN1 | IN2 | Corriente Máx | Aplicación |
|-------|-----|-----|-----|---------------|------------|
| FL (Frontal Izq) | GPIO 1 | GPIO 3 | GPIO 6 | 43A | Tracción |
| FR (Frontal Der) | GPIO 7 | GPIO 8 | GPIO 9 | 43A | Tracción |
| RL (Trasera Izq) | GPIO 10 | GPIO 47 | GPIO 48 | 43A | Tracción |
| RR (Trasera Der) | GPIO 24 | GPIO 28 | GPIO 29 | 43A | Tracción |

#### Dirección (1x BTS7960)

- **Motor:** RS390 12V 6000RPM
- **Reductora:** 1:50
- **Control:** PCA9685 Canal 0 (PWM) + pines dirección
- **Corriente Máx:** 43A

### 7. Sensores de Posición

#### Encoder Dirección (E6B2-CWZ6C 1200PR)

- **Resolución:** 1200 pulsos por revolución
- **Canales:** A, B, Z
- **Ratio:** 1:1 al eje del volante
- **Conexión:**
  - Canal A: GPIO 37
  - Canal B: GPIO 38
  - Señal Z: GPIO 25 (centrado con sensor LJ12A3-4-Z/BX)

#### Sensores de Rueda (5x LJ12A3-4-Z/BX)

Sensores inductivos de proximidad NPN NO 4mm.

| Sensor | GPIO | Aplicación | Pulsos/Rev |
|--------|------|------------|------------|
| FL | 35 | Velocidad rueda frontal izq | 6 (tornillos) |
| FR | 36 | Velocidad rueda frontal der | 6 (tornillos) |
| RL | 30 | Velocidad rueda trasera izq | 6 (tornillos) |
| RR | 31 | Velocidad rueda trasera der | 6 (tornillos) |
| Z | 25 | Señal Z encoder (centrado) | 1 (tornillo central) |

**Configuración ruedas:** 6 tornillos distribuidos uniformemente (360°/6 = 60° entre pulsos)

#### Pedal (A1324LUA-T)

- **Tipo:** Sensor Hall analógico lineal
- **GPIO:** 34 (ADC)
- **Rango:** 0-3.3V
- **Aplicación:** Posición pedal acelerador

### 8. Pantalla y Táctil

#### ILI9488 (480x320 TFT)

| Pin | GPIO | Función |
|-----|------|---------|
| CS | 15 | Chip Select |
| DC | 27 | Data/Command |
| RST | 14 | Reset |
| MOSI | 11 | SPI Data Out |
| MISO | 19 | SPI Data In |
| SCK | 18 | SPI Clock |

#### XPT2046 (Táctil)

| Pin | GPIO | Función |
|-----|------|---------|
| T_CS | 12 | Chip Select táctil |
| T_IRQ | 13 | Interrupción |
| T_DIN | 11 | Compartido MOSI |
| T_DO | 19 | Compartido MISO |
| T_CLK | 18 | Compartido SCK |

### 9. Audio (DFPlayer Mini)

- **TX:** GPIO 17 (ESP32 → DFPlayer RX)
- **RX:** GPIO 16 (DFPlayer TX → ESP32)
- **Comunicación:** UART2
- **Tarjeta SD:** Carpeta /mp3/ con pistas 0001.mp3 - 0038.mp3

### 10. Iluminación (WS2812B)

- **GPIO:** 22
- **Protocolo:** NeoPixel
- **Tipo:** Tira LED RGB direccionable
- **Librería:** FastLED

### 11. Relés de Potencia

#### Optoacopladores (2x HY-M158)

- **Tipo:** PC817 8 canales
- **Total:** 16 canales optoacoplados
- **Control:** MCP23017 GPIO
- **Aislamiento:** 5000V

#### Relés Instalados

| Relé | Tipo | Voltaje | Corriente | Aplicación |
|------|------|---------|-----------|------------|
| Módulo 2Ch | SRD-05VDC-SL-C | 5V | 10A | Control auxiliar |
| SPDT Automotive | 5-pin | 24V | 100A | Batería principal |
| TAXNELE TN606 | RL280-12 | 12V | 100-200A | Motor tracción |
| Contactor | HCH8s-25z 2NO | 12V | 25A | Sistema 12V |
| Automotive WP | 5-pin SPDT | 12V | 100A | Respaldo |

**Pines de control relés principales:**
- RELAY_MAIN: GPIO 2 (Sistema 12V)
- RELAY_TRAC: GPIO 4 (Motores 24V)
- RELAY_DIR: GPIO 5 (Motor dirección)
- RELAY_LIGHTS: GPIO 32 (Luces)
- RELAY_MEDIA: GPIO 33 (Audio/DFPlayer)

### 12. Convertidores de Nivel (3x)

- **Tipo:** Bidireccionales 4 canales IIC I²C
- **Niveles:** 5V ↔ 3.3V
- **Aplicación:** Protección sensores 5V → ESP32-S3 3.3V
- **Canales:** 12 totales (3 módulos × 4 canales)

### 13. Hub I²C

- **Tipo:** Divisor/expansor 8 vías
- **Aplicación:** Distribuir bus I²C a múltiples módulos
- **Protección:** Buffers activos

### 14. Sensores Temperatura (DS18B20)

- **Protocolo:** OneWire
- **GPIO:** 26
- **Cantidad:** Múltiples en paralelo
- **Aplicación:** Monitorización térmica motores

---

## 🔧 Direcciones I²C del Sistema

| Dispositivo | Dirección | Notas |
|-------------|-----------|-------|
| INA226 × 6 | 0x40 | Multiplexado vía TCA9548A |
| PCA9685 | **0x41** | ⚠️ Cambiar de 0x40 (soldar A0) |
| MCP23017 | 0x20 | Expansor GPIO |
| TCA9548A #1 | 0x70 | Mux principal INA226 |
| TCA9548A #2 | 0x71 | Expansión |
| TCA9548A #3 | 0x72 | Expansión |
| TCA9548A #4 | 0x73 | Expansión |
| TCA9548A #5 | 0x74 | Expansión |

**⚠️ CRÍTICO:** El PCA9685 DEBE configurarse en dirección 0x41 para evitar conflicto con INA226 (0x40). Soldar puente A0 en la placa PCA9685.

---

## 📊 Mapeo Completo de Pines ESP32-S3

### Lado Izquierdo

| Pin | GPIO | Función | Módulo |
|-----|------|---------|--------|
| 1 | 3.3V | Alimentación | - |
| 2 | EN | Reset | - |
| 3 | 0 | Strapping | - |
| 4 | 1 | PWM Motor FL | BTS7960 |
| 5 | 2 | Relé Main 12V | HY-M158 |
| 6 | 3 | IN1 Motor FL | BTS7960 |
| 7 | 4 | Relé Tracción 24V | HY-M158 |
| 8 | 5 | Relé Dir | HY-M158 |
| 9 | 6 | IN2 Motor FL | BTS7960 |
| 10 | 7 | PWM Motor FR | BTS7960 |
| 11 | 8 | IN1 Motor FR | BTS7960 |
| 12 | 9 | IN2 Motor FR | BTS7960 |
| 13 | 10 | PWM Motor RL | BTS7960 |
| 14 | 11 | TFT MOSI | ILI9488 |
| 15 | 12 | Touch CS | XPT2046 |
| 16 | 13 | Touch IRQ | XPT2046 |
| 17 | 14 | TFT RST | ILI9488 |
| 18 | 15 | TFT CS | ILI9488 |
| 19 | 16 | DFPlayer RX | UART |
| 20 | 17 | DFPlayer TX | UART |
| 21 | 18 | SPI SCK | Compartido |
| 22 | 19 | SPI MISO | Compartido |
| 23 | GND | Tierra | - |

### Lado Derecho

| Pin | GPIO | Función | Módulo |
|-----|------|---------|--------|
| 1 | 5V | Alimentación | - |
| 2 | 46 | Botón Batería | Input |
| 3 | 45 | Entrada | - |
| 4 | 48 | IN2 Motor RL | BTS7960 |
| 5 | 47 | IN1 Motor RL | BTS7960 |
| 6 | 21 | I²C SDA | Bus I²C |
| 7 | 20 | I²C SCL | Bus I²C |
| 8 | 38 | Encoder B | E6B2-CWZ6C |
| 9 | 37 | Encoder A | E6B2-CWZ6C |
| 10 | 36 | Sensor Rueda FR | LJ12A3 |
| 11 | 35 | Sensor Rueda FL | LJ12A3 |
| 12 | 34 | Pedal ADC | A1324LUA-T |
| 13 | 33 | Relé Media | HY-M158 |
| 14 | 32 | Relé Luces | HY-M158 |
| 15 | 31 | Sensor Rueda RR | LJ12A3 |
| 16 | 30 | Sensor Rueda RL | LJ12A3 |
| 17 | 29 | IN2 Motor RR | BTS7960 |
| 18 | 28 | IN1 Motor RR | BTS7960 |
| 19 | 27 | TFT DC | ILI9488 |
| 20 | 26 | OneWire DS18B20 | Temperatura |
| 21 | 25 | Encoder Z (centro) | LJ12A3 |
| 22 | 24 | PWM Motor RR | BTS7960 |
| 23 | GND | Tierra | - |

---

## ⚡ Alimentación del Sistema

### Requisitos de Potencia

| Subsistema | Voltaje | Corriente | Potencia |
|------------|---------|-----------|----------|
| ESP32-S3 | 3.3V | 500mA | 1.65W |
| Motores tracción (4x) | 24V | 43A c/u | 4.1kW |
| Motor dirección | 12V | 10A | 120W |
| Pantalla ILI9488 | 3.3V | 150mA | 0.5W |
| LEDs WS2812B | 5V | 3A (max) | 15W |
| Relés y lógica | 5V / 12V | 2A | 24W |
| **TOTAL ESTIMADO** | - | - | **~4.3kW** |

### Baterías

- **Principal:** 24V (tracción)
- **Auxiliar:** 12V (dirección, lógica, luces)
- **Shunt batería:** CG FL-2C 100A 75mV
- **Monitorización:** INA226 Canal 4

---

## 🛡️ Sistemas de Seguridad

1. **ABS (Anti-lock Braking System)**
   - Monitorización velocidad 4 ruedas
   - Prevención bloqueo individual
   
2. **TCS (Traction Control System)**
   - Control deslizamiento ruedas
   - Ajuste PWM motor individual

3. **Regenerative Braking con IA**
   - Frenado regenerativo inteligente
   - Recuperación energía a batería

4. **Monitorización Corriente**
   - 6 canales INA226 en tiempo real
   - Alerta sobrecorriente
   - Protección térmica motores

---

## 📝 Notas de Configuración

### Calibración INA226

Los shunts CG FL-2C requieren calibración específica:

```cpp
// Batería (Canal 4): 100A 75mV
float shuntBattery = 0.00075; // Ω (75mV / 100A)
float maxCurrentBattery = 100.0; // A

// Motores (Canales 0-3,5): 50A 75mV
float shuntMotor = 0.0015; // Ω (75mV / 50A)
float maxCurrentMotor = 50.0; // A
```

### Modificación PCA9685

⚠️ **OBLIGATORIO:** Soldar puente A0 en placa PCA9685 para cambiar dirección I²C de 0x40 a 0x41.

### Convertidores de Nivel

Usar convertidores bidireccionales para:
- Sensores LJ12A3-4-Z/BX (5V) → ESP32-S3 (3.3V)
- Encoder E6B2-CWZ6C (5V) → ESP32-S3 (3.3V)

### Pull-up I²C

- Resistencias pull-up internas ESP32-S3: 45kΩ (débiles)
- Recomendado añadir externas: 4.7kΩ en SDA/SCL

---

## 📚 Referencias

- ESP32-S3 Datasheet: https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf
- INA226 Datasheet: https://www.ti.com/lit/ds/symlink/ina226.pdf
- PCA9685 Datasheet: https://www.nxp.com/docs/en/data-sheet/PCA9685.pdf
- TCA9548A Datasheet: https://www.ti.com/lit/ds/symlink/tca9548a.pdf
- BTS7960 Datasheet: https://www.infineon.com/dgdl/bts7960b.pdf
- E6B2-CWZ6C Datasheet: https://www.omron.com/ecb/products/pdf/precautions_for_all_encoders.pdf

---

## ✅ Checklist Pre-Encendido

- [ ] Verificar alimentación 24V / 12V / 5V / 3.3V
- [ ] Confirmar PCA9685 en dirección 0x41 (soldar A0)
- [ ] Instalar convertidores nivel 5V↔3.3V
- [ ] Conectar 6 INA226 via TCA9548A
- [ ] Verificar shunts CG FL-2C (1x100A + 5x50A)
- [ ] Conectar encoder E6B2-CWZ6C (A, B, Z)
- [ ] Instalar sensores LJ12A3-4-Z/BX (5 unidades)
- [ ] Configurar MCP23017 para relés
- [ ] Montar optoacopladores HY-M158 (2x)
- [ ] Conectar 4 BTS7960 tracción
- [ ] Conectar BTS7960 dirección + RS390
- [ ] Instalar pantalla ILI9488 + táctil
- [ ] Preparar SD DFPlayer (/mp3/0001-0038.mp3)
- [ ] Conectar LEDs WS2812B
- [ ] Verificar relés potencia
- [ ] Test continuidad GND común
- [ ] Compilar y flashear firmware
- [ ] Verificar WiFi y OTA
- [ ] Test sistemas seguridad (ABS/TCS)

---

**Documento generado:** 2025-01-06  
**Versión Firmware:** 2.0  
**Autor:** GitHub Copilot  
**Repositorio:** https://github.com/florinzgz/coche-2-inteligente
