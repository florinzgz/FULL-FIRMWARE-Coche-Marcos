# 📋 MANUAL DE CONEXIONES DETALLADO - ESP32-S3-DevKitC-1

## 🎯 Guía Completa de Conexiones Cable por Cable

**Hardware:** ESP32-S3-DevKitC-1 (44 pines físicos)  
**Firmware:** Coche Inteligente Marcos v1.0 - Adaptado DevKitC-1  
**Fecha:** Enero 2025

---

## 📌 TABLA DE CONTENIDOS

1. [Pantalla ILI9488 + Táctil XPT2046](#1-pantalla-ili9488--táctil-xpt2046)
2. [Relés HY-M158 (Estado Sólido)](#2-relés-hy-m158-estado-sólido)
3. [Relés Adicionales (Módulo 8 canales)](#3-relés-adicionales-módulo-8-canales)
4. [DFPlayer Mini (Audio)](#4-dfplayer-mini-audio)
5. [Sensores de Ruedas LJ12A3-4-Z/BX](#5-sensores-de-ruedas-lj12a3-4-zbx)
6. [Encoder de Dirección E6B2-CWZ6C](#6-encoder-de-dirección-e6b2-cwz6c)
7. [Motores BTS7960 (4 unidades)](#7-motores-bts7960-4-unidades)
8. [Sensores INA226 (I2C)](#8-sensores-ina226-i2c)
9. [Sensor de Temperatura DS18B20](#9-sensor-de-temperatura-ds18b20)
10. [Pedal Analógico](#10-pedal-analógico)
11. [Botones de Control](#11-botones-de-control)
12. [LEDs WS2812B (Opcional)](#12-leds-ws2812b-opcional)
13. [Alimentación del Sistema](#13-alimentación-del-sistema)
14. [Checklist de Verificación](#14-checklist-de-verificación)

---

## 1. PANTALLA ILI9488 + TÁCTIL XPT2046

### 🖥️ Pantalla TFT ILI9488 (480x320) → ESP32-S3

| Cable Color | Pin Pantalla | Pin ESP32-S3 | Función |
|-------------|--------------|--------------|---------|
| 🔴 Rojo | VCC | **3.3V** (pin lateral izq) | Alimentación 3.3V |
| ⚫ Negro | GND | **GND** (pin lateral izq/der) | Tierra común |
| 🟠 Naranja | CS | **GPIO 15** (IO15 izq) | Chip Select pantalla |
| 🟡 Amarillo | RST / RESET | **GPIO 14** (IO14 izq) | Reset pantalla |
| 🟢 Verde | DC / A0 | **GPIO 27** (IO27 der) | Data/Command |
| 🔵 Azul | MOSI / SDI | **GPIO 11** (IO11 izq) | Datos SPI (Master Out) |
| 🟣 Violeta | SCK / CLK | **GPIO 18** (IO18 izq) | Reloj SPI |
| ⚪ Blanco | LED / BLK | **3.3V** (pin lateral izq) | Backlight (siempre ON) |
| 🟤 Marrón | MISO / SDO | **GPIO 19** (IO19 izq) | Datos SPI (Master In) |

### 👆 Táctil XPT2046 (mismo módulo) → ESP32-S3

| Cable Color | Pin Táctil | Pin ESP32-S3 | Función |
|-------------|------------|--------------|---------|
| - | T_CLK | **GPIO 18** (IO18 izq) | ✅ Compartido con SCK pantalla |
| 🟤 Marrón | T_CS | **GPIO 12** (IO12 izq) | Chip Select táctil |
| - | T_DIN | **GPIO 11** (IO11 izq) | ✅ Compartido con MOSI pantalla |
| - | T_DO | **GPIO 19** (IO19 izq) | ✅ Compartido con MISO pantalla |
| 🔶 Rosa | T_IRQ | **GPIO 13** (IO13 izq) | Interrupción táctil (opcional) |

**📝 Notas:**
- Bus SPI compartido: GPIO 11 (MOSI), 18 (SCK), 19 (MISO)
- Backlight LED siempre conectado a 3.3V (sin PWM)
- CS separados: pantalla GPIO 15, táctil GPIO 12

---

## 2. RELÉS HY-M158 (ESTADO SÓLIDO)

### ⚡ HY-M158 #1 - Tracción Motores 24V

| Pin HY-M158 | Cable | Pin ESP32-S3 | Función |
|-------------|-------|--------------|---------|
| DC+ (Input) | 🔴 Rojo | **GPIO 4** (IO4 izq) | Control ON/OFF tracción |
| DC- (Input) | ⚫ Negro | **GND** (cualquier GND) | Tierra común ESP32 |
| + (Output) | 🔴 Rojo grueso | **Batería 24V (+)** | Entrada desde batería 24V |
| Load (Output) | 🔴 Rojo grueso | **BTS7960 VCC** | Salida a drivers motores |
| - (Output) | ⚫ Negro grueso | **GND común 24V** | GND batería 24V |

### ⚡ HY-M158 #2 - Sistema Principal 12V

| Pin HY-M158 | Cable | Pin ESP32-S3 | Función |
|-------------|-------|--------------|---------|
| DC+ (Input) | 🔴 Rojo | **GPIO 2** (IO2 izq) | Control ON/OFF sistema |
| DC- (Input) | ⚫ Negro | **GND** (cualquier GND) | Tierra común ESP32 |
| + (Output) | 🔴 Rojo grueso | **Batería 12V (+)** | Entrada desde batería 12V |
| Load (Output) | 🔴 Rojo grueso | **Luces/TFT/Audio** | Salida a sistema 12V |
| - (Output) | ⚫ Negro grueso | **GND común 12V** | GND batería 12V |

**📝 Notas:**
- HY-M158 son relés de estado sólido (40A máx c/u)
- Control: GPIO HIGH = relé ON, GPIO LOW = relé OFF
- Optoacoplador integrado (protección ESP32)

---

## 3. RELÉS ADICIONALES (MÓDULO 8 CANALES)

### 🔌 Módulo Relé 8 Canales 5V → ESP32-S3

| Pin Módulo | Cable | Pin ESP32-S3 | Función |
|------------|-------|--------------|---------|
| VCC | 🔴 Rojo | **5V** (pin 5V0 der) | Alimentación módulo |
| GND | ⚫ Negro | **GND** (cualquier GND) | Tierra |
| IN1 | - | - | No usado |
| IN2 | 🟠 Naranja | **GPIO 5** (IO5 izq) | Relé Encoder Dirección |
| IN3 | 🟢 Verde | **GPIO 32** (IO32 der) | Relé Luces |
| IN4 | 🔵 Azul | **GPIO 33** (IO33 der) | Relé Audio/DFPlayer |
| IN5-IN8 | - | - | No usados (reserva) |

**Conexión de Cargas:**

| Relé | Función | Conexión COM | Conexión NO |
|------|---------|--------------|-------------|
| IN2 | Encoder Dirección | +5V | Encoder VCC |
| IN3 | Luces | +12V | Luces VCC |
| IN4 | Audio/DFPlayer | +12V | DFPlayer VCC |

**📝 Notas:**
- Lógica inversa: GPIO LOW = relé ON, GPIO HIGH = relé OFF
- Optoacopladores integrados
- LED indicador por canal

---

## 4. DFPLAYER MINI (AUDIO)

### 🎵 DFPlayer Mini → ESP32-S3

| Pin DFPlayer | Cable | Pin ESP32-S3 | Función |
|--------------|-------|--------------|---------|
| VCC | 🔴 Rojo | **Relé Audio** (via IN4) | Alimentación 5V controlada |
| GND | ⚫ Negro | **GND** | Tierra común |
| TX | 🟢 Verde | **GPIO 16** (IO16 izq) | Transmite → ESP32 RX |
| RX | 🔵 Azul | **GPIO 17** (IO17 izq) | Recibe ← ESP32 TX |
| SPK_1 | 🔴 Rojo | **Altavoz +** | Salida altavoz positivo |
| SPK_2 | ⚫ Negro | **Altavoz -** | Salida altavoz negativo |
| DAC_R | - | - | No usado (usar SPK) |
| DAC_L | - | - | No usado (usar SPK) |

**📝 Estructura Tarjeta SD (obligatorio):**

```
SD Card (FAT32):
└── mp3/
    ├── 0001.mp3  → "Bienvenido Marcos"
    ├── 0002.mp3  → "Hasta pronto"
    ├── 0003.mp3  → "Marcha adelante"
    ├── 0004.mp3  → "Marcha atrás"
    ├── 0005.mp3  → "Punto muerto"
    ├── 0006.mp3  → "Freno de mano activado"
    ├── 0013.mp3  → "Batería crítica"
    ├── 0036.mp3  → "Módulo OK"
    └── ... (hasta 0038.mp3)
```

**📝 Notas:**
- UART1 software: GPIO 16 (RX), 17 (TX)
- Volumen configurado en código: 25/30
- Archivos MP3 en carpeta `mp3/` con nombres 0001-0038

---

## 5. SENSORES DE RUEDAS LJ12A3-4-Z/BX

### 🎯 Sensor Inductivo LJ12A3-4-Z/BX (4 sensores) → ESP32-S3

**Configuración:** 6 tornillos M4 por rueda, distribuidos uniformemente (60° separación)

| Sensor | Cable | Pin ESP32-S3 | Función |
|--------|-------|--------------|---------|
| **FL (Frontal Izq)** | | | |
| VCC (marrón) | 🟤 Marrón | **+5V** (via level shifter) | Alimentación sensor |
| GND (azul) | 🔵 Azul | **GND** | Tierra |
| Signal (negro) | ⚫ Negro | **GPIO 35** (IO35 der) via shifter | Pulsos rueda FL |
| **FR (Frontal Der)** | | | |
| VCC (marrón) | 🟤 Marrón | **+5V** (via level shifter) | Alimentación sensor |
| GND (azul) | 🔵 Azul | **GND** | Tierra |
| Signal (negro) | ⚫ Negro | **GPIO 36** (IO36 der) via shifter | Pulsos rueda FR |
| **RL (Trasera Izq)** | | | |
| VCC (marrón) | 🟤 Marrón | **+5V** (via level shifter) | Alimentación sensor |
| GND (azul) | 🔵 Azul | **GND** | Tierra |
| Signal (negro) | ⚫ Negro | **GPIO 30** (IO30 der) via shifter | Pulsos rueda RL |
| **RR (Trasera Der)** | | | |
| VCC (marrón) | 🟤 Marrón | **+5V** (via level shifter) | Alimentación sensor |
| GND (azul) | 🔵 Azul | **GND** | Tierra |
| Signal (negro) | ⚫ Negro | **GPIO 31** (IO31 der) via shifter | Pulsos rueda RR |

**🔄 Level Shifter 5V → 3.3V (TXS0104E):**

| HV (5V) | LV (3.3V) |
|---------|-----------|
| HV1 ← FL Signal | GPIO 35 → LV1 |
| HV2 ← FR Signal | GPIO 36 → LV2 |
| HV3 ← RL Signal | GPIO 30 → LV3 |
| HV4 ← RR Signal | GPIO 31 → LV4 |
| VCC = 5V | VCC = 3.3V |
| GND común | GND común |

**📝 Notas:**
- **6 pulsos por revolución** (configurado en código)
- Sensores NPN normalmente abiertos (NO)
- Detección por flanco ascendente (RISING)
- Level shifter obligatorio (sensor 5V → ESP32 3.3V)
- Distancia detección: 4mm (ajustar distancia tornillo-sensor)

---

## 6. ENCODER DE DIRECCIÓN E6B2-CWZ6C

### 🎯 Encoder Incremental 1200 P/R → ESP32-S3

| Pin Encoder | Cable | Pin ESP32-S3 | Función |
|-------------|-------|--------------|---------|
| VCC | 🔴 Rojo | **+5V** (via level shifter) | Alimentación encoder |
| GND | ⚫ Negro | **GND** | Tierra |
| A Phase | 🟢 Verde | **GPIO 37** (IO37 der) via shifter | Canal A (cuadratura) |
| B Phase | 🔵 Azul | **GPIO 38** (IO38 der) via shifter | Canal B (cuadratura) |
| Z Index | 🟡 Amarillo | **GPIO 25** (IO25 der) via shifter | Señal Z (centrado) |

**🔄 Level Shifter 5V → 3.3V (TXS0104E):**

| HV (5V) | LV (3.3V) |
|---------|-----------|
| HV1 ← Encoder A | GPIO 37 → LV1 |
| HV2 ← Encoder B | GPIO 38 → LV2 |
| HV3 ← Encoder Z | GPIO 25 → LV3 |
| VCC = 5V | VCC = 3.3V |
| GND común | GND común |

**📝 Notas:**
- E6B2-CWZ6C 1200 P/R (pulsos por revolución)
- Señal Z: sensor LJ12A3-4-Z/BX detecta tornillo central (posición 0°)
- Cuadratura A/B para detectar dirección y posición precisa
- Level shifter obligatorio (encoder 5V → ESP32 3.3V)
- Resolución configurada en código: 1024 ticks

---

## 7. MOTORES BTS7960 (4 UNIDADES)

### 🚗 Drivers BTS7960 43A → ESP32-S3

**Motor FL (Frontal Izquierdo):**

| Pin BTS7960 | Cable | Pin ESP32-S3 | Función |
|-------------|-------|--------------|---------|
| VCC (logic) | 🔴 Rojo | **3.3V** | Alimentación lógica driver |
| GND | ⚫ Negro | **GND** | Tierra común |
| LPWM | 🟠 Naranja | **GPIO 1** (IO1 izq) | PWM velocidad adelante |
| RPWM | 🟡 Amarillo | - | No usado (ver IN1/IN2) |
| L_EN / IN1 | 🟢 Verde | **GPIO 3** (IO3 izq) | Dirección 1 |
| R_EN / IN2 | 🔵 Azul | **GPIO 6** (IO6 izq) | Dirección 2 |
| VCC (power) | 🔴 Rojo grueso | **HY-M158 #1 OUT** | Alimentación 24V |
| GND (power) | ⚫ Negro grueso | **GND 24V** | Tierra potencia |
| M+ | 🔴 Rojo grueso | **Motor FL +** | Conexión motor positivo |
| M- | ⚫ Negro grueso | **Motor FL -** | Conexión motor negativo |

**Motor FR (Frontal Derecho):**

| Pin BTS7960 | Cable | Pin ESP32-S3 | Función |
|-------------|-------|--------------|---------|
| VCC (logic) | 🔴 Rojo | **3.3V** | Alimentación lógica |
| GND | ⚫ Negro | **GND** | Tierra común |
| LPWM | 🟠 Naranja | **GPIO 7** (IO7 izq) | PWM velocidad |
| L_EN / IN1 | 🟢 Verde | **GPIO 8** (IO8 izq) | Dirección 1 |
| R_EN / IN2 | 🔵 Azul | **GPIO 9** (IO9 izq) | Dirección 2 |
| VCC (power) | 🔴 Rojo grueso | **HY-M158 #1 OUT** | 24V |
| M+/M- | - | **Motor FR** | Conexión motor |

**Motor RL (Trasero Izquierdo):**

| Pin BTS7960 | Cable | Pin ESP32-S3 | Función |
|-------------|-------|--------------|---------|
| VCC (logic) | 🔴 Rojo | **3.3V** | Alimentación lógica |
| GND | ⚫ Negro | **GND** | Tierra común |
| LPWM | 🟠 Naranja | **GPIO 10** (IO10 izq) | PWM velocidad |
| L_EN / IN1 | 🟢 Verde | **GPIO 47** (IO47 der) | Dirección 1 |
| R_EN / IN2 | 🔵 Azul | **GPIO 48** (IO48 der) | Dirección 2 |
| VCC (power) | 🔴 Rojo grueso | **HY-M158 #1 OUT** | 24V |
| M+/M- | - | **Motor RL** | Conexión motor |

**Motor RR (Trasero Derecho):**

| Pin BTS7960 | Cable | Pin ESP32-S3 | Función |
|-------------|-------|--------------|---------|
| VCC (logic) | 🔴 Rojo | **3.3V** | Alimentación lógica |
| GND | ⚫ Negro | **GND** | Tierra común |
| LPWM | 🟠 Naranja | **GPIO 24** (IO24 der) | PWM velocidad |
| L_EN / IN1 | 🟢 Verde | **GPIO 28** (IO28 der) | Dirección 1 |
| R_EN / IN2 | 🔵 Azul | **GPIO 29** (IO29 der) | Dirección 2 |
| VCC (power) | 🔴 Rojo grueso | **HY-M158 #1 OUT** | 24V |
| M+/M- | - | **Motor RR** | Conexión motor |

**📝 Notas:**
- Todos los motores alimentados desde HY-M158 #1 (24V controlado)
- PWM frecuencia: 20kHz (configurado en código)
- Lógica: PWM + IN1/IN2 para controlar velocidad y dirección
- Corriente máxima: 43A por driver
- Disipadores obligatorios en drivers BTS7960

---

## 8. SENSORES INA226 (I2C)

### 📊 Sensores de Corriente/Voltaje INA226 → ESP32-S3

**Bus I2C Compartido:**

| Pin INA226 | Cable | Pin ESP32-S3 | Función |
|------------|-------|--------------|---------|
| VCC | 🔴 Rojo | **3.3V** | Alimentación sensor |
| GND | ⚫ Negro | **GND** | Tierra |
| SCL | 🟡 Amarillo | **GPIO 20** (IO20 der) | Clock I2C (todos) |
| SDA | 🟢 Verde | **GPIO 21** (IO21 der) | Data I2C (todos) |

**Direcciones I2C (configurar con jumpers A0/A1):**

| Sensor | Función | Dirección I2C | Jumpers |
|--------|---------|---------------|---------|
| INA226 #1 | Batería 24V | 0x40 | A0=GND, A1=GND |
| INA226 #2 | Batería 12V | 0x41 | A0=VCC, A1=GND |
| INA226 #3 | Motor FL | 0x44 | A0=GND, A1=VCC |
| INA226 #4 | Motor FR | 0x45 | A0=VCC, A1=VCC |
| INA226 #5 | Motor RL (opcional) | 0x48 | A0=SDA, A1=GND |
| INA226 #6 | Motor RR (opcional) | 0x49 | A0=SCL, A1=GND |

**📝 Notas:**
- Bus I2C nativo: GPIO 20 (SCL), 21 (SDA)
- Resistencias pull-up externas 4.7kΩ (SCL/SDA → 3.3V)
- INA226 miden voltaje y corriente en tiempo real
- Shunt resistor: 0.01Ω (configurar según modelo)

---

## 9. SENSOR DE TEMPERATURA DS18B20

### 🌡️ Sensor OneWire DS18B20 → ESP32-S3

| Pin DS18B20 | Cable | Pin ESP32-S3 | Función |
|-------------|-------|--------------|---------|
| VCC | 🔴 Rojo | **+5V** o **3.3V** | Alimentación |
| GND | ⚫ Negro | **GND** | Tierra |
| DQ (Data) | 🟡 Amarillo | **GPIO 26** (IO26 der) | Bus OneWire |

**📝 Notas:**
- Resistencia pull-up 4.7kΩ (DQ → VCC)
- Puede usar alimentación parásita (solo VCC y GND)
- Múltiples sensores en el mismo bus (cada uno con ID único)
- Ubicación recomendada: cerca de motores/controladores

---

## 10. PEDAL ANALÓGICO

### 🦶 Pedal Potenciómetro → ESP32-S3

| Pin Pedal | Cable | Pin ESP32-S3 | Función |
|-----------|-------|--------------|---------|
| VCC | 🔴 Rojo | **3.3V** | Alimentación potenciómetro |
| GND | ⚫ Negro | **GND** | Tierra |
| Wiper (salida) | 🟢 Verde | **GPIO 34** (IO34 der) | Entrada ADC |

**📝 Notas:**
- ADC interno 12-bit (0-4095)
- Rango: 0% (pedal suelto) - 100% (pedal pisado)
- Calibración en código: valores mínimo/máximo
- Filtro software para eliminar ruido

---

## 11. BOTONES DE CONTROL

### 🔘 Botones Push → ESP32-S3

| Botón | Cable | Pin ESP32-S3 | Función | Resistencia |
|-------|-------|--------------|---------|-------------|
| Lights | 🟡 Amarillo | **GPIO 40** * | Luces ON/OFF | Pull-up 10kΩ |
| Media | 🟢 Verde | **GPIO 41** * | Multimedia | Pull-up 10kΩ |
| 4x4 | 🔵 Azul | **GPIO 42** * | Modo 4x4 | Pull-up 10kΩ |
| Battery | 🔴 Rojo | **GPIO 46** (IO46 der) | Info batería | Pull-up 10kΩ |

**⚠️ NOTA IMPORTANTE:**
- GPIO 40, 41, 42 **NO EXISTEN** en ESP32-S3-DevKitC-1
- **USAR GPIOs LIBRES ALTERNATIVOS:** GPIO 0, 45 (solo input), o externos via I2C expander
- Botón Battery funciona OK en GPIO 46 (input-only)

**Conexión típica botón:**
```
        3.3V
         |
        [10kΩ] (pull-up)
         |
         +------ GPIO (ESP32)
         |
        [Botón]
         |
        GND
```

**📝 Notas:**
- Lógica inversa: botón presionado = GPIO LOW
- Debounce software en código (20ms)
- Pull-up interno ESP32 también disponible

---

## 12. LEDS WS2812B (OPCIONAL)

### 💡 Tira LED Inteligente WS2812B → ESP32-S3

| Pin LED | Cable | Conexión | Función |
|---------|-------|----------|---------|
| VCC / +5V | 🔴 Rojo | **Fuente 5V externa** | Alimentación LEDs (⚠️ NO desde ESP32) |
| GND | ⚫ Negro | **GND común** | Tierra (común con ESP32) |
| DIN / Data | 🟢 Verde | **GPIO 22** * | Señal datos LEDs |

**⚠️ NOTA IMPORTANTE:**
- GPIO 22 **NO DISPONIBLE** en el firmware adaptado (I2C_SCL ahora en GPIO 20)
- **REASIGNAR A GPIO LIBRE:** Ejemplo GPIO 0 o GPIO 45

**📝 Notas:**
- Librería FastLED configurada en código
- Fuente externa 5V obligatoria (1A cada 30 LEDs aprox)
- Resistencia 330Ω serie en DIN (protección)
- Condensador 1000µF en VCC/GND (estabilidad)

---

## 13. ALIMENTACIÓN DEL SISTEMA

### 🔋 Esquema de Alimentación Completo

```
BATERÍA 24V (Motores)
    |
    ├─► HY-M158 #1 (GPIO 4 control) ───► BTS7960 (4x) ───► Motores DC
    |
    └─► Buck 24V→12V ───┐
                         |
BATERÍA 12V (Sistema)    |
    |                    |
    ├───────────────────┘
    |
    ├─► HY-M158 #2 (GPIO 2 control) ───┬─► Luces
    |                                   ├─► Pantalla TFT ILI9488
    |                                   └─► DFPlayer Mini (via relé)
    |
    └─► Buck 12V→5V ───┬─► ESP32-S3 (pin 5V0)
                       ├─► Sensores 5V (LJ12A3, Encoder)
                       ├─► Módulo relés 8ch
                       └─► LEDs WS2812B (fuente externa dedicada)
```

### 🔌 Conexiones de Alimentación ESP32-S3-DevKitC-1

| Pin ESP32 | Función | Conexión |
|-----------|---------|----------|
| **3V3** (izq) | Salida 3.3V | Alimentar: TFT, INA226, BTS7960 logic |
| **5V0** (der) | Entrada 5V | Desde Buck 12V→5V |
| **GND** (ambos) | Tierra común | GND universal (12V, 24V, sensores) |
| **EN** (izq) | Enable/Reset | Pull-up 10kΩ a 3.3V (botón reset opcional) |

**📝 Notas Críticas:**
- ⚠️ **NUNCA** alimentar sensores 5V desde 3.3V del ESP32
- ⚠️ **NUNCA** conectar 24V directamente a GPIO (usar HY-M158)
- ⚠️ Buck reguladores con disipador (cargas >500mA)
- ⚠️ GND común obligatorio (todos los subsistemas)
- ⚠️ Fusibles recomendados: 5A (12V), 30A (24V)

---

## 14. CHECKLIST DE VERIFICACIÓN

### ✅ Pre-Encendido (CHECK antes de alimentar)

- [ ] **Continuidad GND:** Verificar GND común en todos los módulos
- [ ] **Voltajes:**
  - [ ] Batería 24V: 22-26V (medir con multímetro)
  - [ ] Batería 12V: 11-13V
  - [ ] Buck 5V: 4.8-5.2V (sin carga)
- [ ] **Relés HY-M158:**
  - [ ] GPIO 2, 4 en LOW (relés OFF al inicio)
  - [ ] Conexiones de potencia correctas (batería → HY-M158 → cargas)
- [ ] **Sensores 5V:**
  - [ ] Level shifter TXS0104E alimentado (5V y 3.3V)
  - [ ] Sensores LJ12A3 correctamente polarizados (marrón=VCC, azul=GND, negro=Signal)
- [ ] **Motores BTS7960:**
  - [ ] Todos los VCC logic a 3.3V (NO a 5V)
  - [ ] Todos los VCC power a HY-M158 #1 OUT
  - [ ] Polaridad motores verificada (M+ / M-)
- [ ] **Pantalla TFT:**
  - [ ] VCC y LED a 3.3V (NO a 5V)
  - [ ] Bus SPI compartido: MOSI/SCK/MISO común con táctil
- [ ] **I2C:**
  - [ ] Resistencias pull-up 4.7kΩ presentes (SCL/SDA → 3.3V)
  - [ ] Direcciones INA226 configuradas correctamente

### ✅ Post-Encendido (CHECK con alimentación)

- [ ] **LED de alimentación ESP32 encendido** (rojo/verde en placa)
- [ ] **Voltaje 3.3V en pin 3V3:** Medir con multímetro (debe ser 3.2-3.4V)
- [ ] **Pantalla TFT se enciende:** Backlight visible
- [ ] **Audio bienvenida:** DFPlayer reproduce "Bienvenido Marcos" (0001.mp3)
- [ ] **Relés inactivos:** HY-M158 LEDs apagados (GPIO 2, 4 en LOW)
- [ ] **Sin humo/olor:** Ningún componente sobrecalentado

### ✅ Pruebas Funcionales

- [ ] **Pantalla táctil responde:** Tocar pantalla, ver respuesta en serial
- [ ] **Sensores ruedas:** Girar rueda a mano, verificar pulsos en serial
- [ ] **Encoder dirección:** Girar volante, verificar ticks A/B/Z
- [ ] **Pedal:** Presionar pedal, ver valor ADC 0-4095 en serial
- [ ] **Botones:** Presionar cada botón, verificar detección
- [ ] **Sensores INA226:** Leer voltajes/corrientes en serial
- [ ] **Temperatura:** Leer DS18B20 en serial

### ⚠️ En Caso de Problemas

| Problema | Causa Probable | Solución |
|----------|----------------|----------|
| ESP32 no enciende | Voltaje 5V incorrecto | Verificar Buck 12V→5V, medir salida |
| Pantalla en blanco | Conexión SPI mal | Revisar GPIO 11,14,15,18,19,27 |
| Sensores 5V no detectan | Sin level shifter | Instalar TXS0104E entre sensor y ESP32 |
| Motores no giran | HY-M158 #1 OFF | Activar GPIO 4 HIGH, verificar relé |
| Audio no suena | DFPlayer sin SD | Insertar SD con carpeta `mp3/0001-0038.mp3` |
| I2C no responde | Sin pull-up | Añadir 4.7kΩ SCL/SDA → 3.3V |

---

## 📥 DESCARGA FIRMWARE

**Firmware adaptado ESP32-S3-DevKitC-1:**

🔗 **GitHub Release:**
```
https://github.com/florinzgz/coche-2-inteligente/archive/refs/heads/copilot/revise-and-fix-functionality.zip
```

**Archivos incluidos:**
- `/firmware Coche Marcos/` - Código fuente completo
- `/docs/PIN_MAPPING_DEVKITC1.md` - Mapeo de pines detallado
- `/docs/MANUAL_CONEXIONES_DETALLADO_DEVKITC1.md` - Este manual
- `platformio.ini` - Configuración de compilación

**Compilar y flashear:**

```bash
cd "firmware Coche Marcos"
pio run -t clean
pio run -t upload
```

---

## 📞 SOPORTE

**Hardware confirmado:**
- ✅ ESP32-S3-DevKitC-1 (GPIOs 0-48)
- ✅ Sensores LJ12A3-4-Z/BX (6 tornillos/rueda)
- ✅ Encoder E6B2-CWZ6C 1200PR con Z
- ✅ Pantalla ILI9488 480x320 + XPT2046
- ✅ Relés HY-M158 40A (2 unidades)
- ✅ Drivers BTS7960 43A (4 unidades)

**Documentación adicional:**
- `ADVANCED_SAFETY_SYSTEMS.md` - Sistemas ABS/TCS/Regen
- `WIFI_OTA_SETUP.md` - Configuración WiFi y OTA
- `CONEXIONES_HARDWARE_DEVKITC1.md` - Guía resumida

---

**Versión Manual:** 1.0  
**Fecha:** Enero 2025  
**Hardware:** ESP32-S3-DevKitC-1 (44 pines)  
**Firmware:** Coche Inteligente Marcos - Adaptado DevKitC-1
