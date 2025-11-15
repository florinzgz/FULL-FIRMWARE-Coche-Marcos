# 📖 MANUAL DE CONEXIONES HARDWARE - ESP32-S3-DevKitC-1

**Versión:** 2.0 - Adaptado para ESP32-S3-DevKitC-1 (44 pines)  
**Fecha:** Noviembre 2025  
**Firmware:** Coche Inteligente Marcos - Sistema completo con WiFi, OTA, ABS, TCS, AI Regen

---

## 📌 TABLA DE CONTENIDOS

1. [Información General](#información-general)
2. [Mapa de Pines ESP32-S3-DevKitC-1](#mapa-de-pines-esp32-s3-devkitc-1)
3. [Conexiones Detalladas Módulo por Módulo](#conexiones-detalladas-módulo-por-módulo)
4. [Diagramas de Conexión](#diagramas-de-conexión)
5. [Sistema de Alimentación](#sistema-de-alimentación)
6. [Checklist de Verificación](#checklist-de-verificación)
7. [Notas de Seguridad](#notas-de-seguridad)

---

## 📋 INFORMACIÓN GENERAL

### Hardware Verificado
- **Placa:** ESP32-S3-DevKitC-1 (44 pines físicos, GPIOs 0-48 disponibles)
- **Chip:** ESP32-S3-WROOM-1-N16R8 (16MB Flash, 8MB PSRAM)
- **Sensores Ruedas:** 4x LJ12A3-4-Z/BX (inductivos, 6 pulsos/revolución)
- **Encoder Dirección:** E6B2-CWZ6C 1200PR con señal Z
- **Pantalla:** ILI9488 480x320 + Táctil XPT2046
- **Relés Principales:** 2x HY-M158 (40A estado sólido)
- **Drivers Motores:** 4x BTS7960 (43A por canal)

### Cambios Respecto a Versión Anterior
✅ **24 pines remapeados** para compatibilidad con DevKitC-1  
✅ Motores BTS7960 en GPIOs disponibles (1,3,6-10,24,28,29,47,48)  
✅ I2C remapeado a pines nativos hardware (GPIO 20/21)  
✅ Pantalla MOSI en GPIO 11 (antes GPIO 23)  
✅ Encoder dirección en GPIO 37/38 (antes GPIO 44/45)  
✅ Sensores ruedas en GPIO 30,31,35,36  
✅ Botones en GPIO 40,41,42,46  

---

## 🔌 MAPA DE PINES ESP32-S3-DevKitC-1

### Vista Física de la Placa

```
           ┌─────────────────────────┐
           │    ESP32-S3-DevKitC-1   │
           │  ┌──────────────────┐   │
           │  │    USB Type-C    │   │
           │  └──────────────────┘   │
           │                         │
   IZQUIERDA                    DERECHA
     │                                │
3V3  ●                                ● 5V0
EN   ●                                ● IO46
IO0  ●                                ● IO45
IO1  ●  ← PWM FL                     ● IO48
IO2  ●  ← RELAY MAIN                 ● IO47
IO3  ●  ← IN1 FL                     ● IO21  ← I2C SDA
IO4  ●  ← RELAY TRAC                 ● IO20  ← I2C SCL
IO5  ●  ← RELAY DIR                  ● IO38  ← ENCODER B
IO6  ●  ← IN2 FL                     ● IO37  ← ENCODER A
IO7  ●  ← PWM FR                     ● IO36  ← WHEEL FR
IO8  ●  ← IN1 FR                     ● IO35  ← WHEEL FL
IO9  ●  ← IN2 FR                     ● IO34  ← PEDAL
IO10 ●  ← PWM RL                     ● IO33  ← RELAY MEDIA
IO11 ●  ← TFT MOSI                   ● IO32  ← RELAY LIGHTS
IO12 ●  ← TOUCH CS                   ● IO31  ← WHEEL RR
IO13 ●  ← TOUCH IRQ                  ● IO30  ← WHEEL RL
IO14 ●  ← TFT RST                    ● IO29  ← IN2 RR
IO15 ●  ← TFT CS                     ● IO28  ← IN1 RR
IO16 ●  ← DFPLAYER RX                ● IO27  ← TFT DC
IO17 ●  ← DFPLAYER TX                ● IO26  ← DS18B20
IO18 ●  ← SPI SCK                    ● IO25  ← ENCODER Z
IO19 ●  ← SPI MISO                   ● IO24  ← PWM RR
GND  ●                                ● GND
```

---

## 🔧 CONEXIONES DETALLADAS MÓDULO POR MÓDULO

### 1️⃣ RELÉS HY-M158 (Estado Sólido 40A)

#### HY-M158 #1 - Tracción 24V (Principal)
| Pin HY-M158 | Color Cable | Conectar a ESP32-S3 | Posición Física |
|-------------|-------------|---------------------|-----------------|
| DC+ (Input) | 🔴 Rojo     | **GPIO 4**          | Izquierda pin 7 |
| DC- (Input) | ⚫ Negro    | **GND**             | Cualquier GND   |
| + (Output)  | 🔴 Rojo     | Batería 24V +       | Alimentación    |
| - (Output)  | ⚫ Negro    | GND 24V común       | Alimentación    |
| Load        | 🟡 Amarillo | Controladores BTS7960 | Tracción      |

**Función:** Corta/conecta alimentación 24V a los 4 motores de tracción

#### HY-M158 #2 - Sistema 12V (Auxiliar)
| Pin HY-M158 | Color Cable | Conectar a ESP32-S3 | Posición Física |
|-------------|-------------|---------------------|-----------------|
| DC+ (Input) | 🔴 Rojo     | **GPIO 2**          | Izquierda pin 5 |
| DC- (Input) | ⚫ Negro    | **GND**             | Cualquier GND   |
| + (Output)  | 🔴 Rojo     | Batería 12V +       | Alimentación    |
| - (Output)  | ⚫ Negro    | GND 12V común       | Alimentación    |
| Load        | 🟢 Verde    | Luces/TFT/Audio     | Periféricos     |

**Función:** Corta/conecta alimentación 12V a periféricos

---

### 2️⃣ PANTALLA ILI9488 480x320 + TÁCTIL XPT2046

#### Conexiones Pantalla ILI9488 (SPI)
| Pin Pantalla | Color Cable | ESP32-S3 GPIO | Posición Física | Función |
|--------------|-------------|---------------|-----------------|---------|
| VCC          | 🔴 Rojo     | **3.3V**      | Izquierda pin 1 | Alimentación |
| GND          | ⚫ Negro    | **GND**       | Cualquier GND   | Tierra |
| CS           | 🟠 Naranja  | **GPIO 15**   | Izquierda pin 16| Chip Select |
| RESET        | 🟡 Amarillo | **GPIO 14**   | Izquierda pin 15| Reset |
| DC/RS        | 🟢 Verde    | **GPIO 27**   | Derecha pin 5   | Data/Command |
| MOSI/SDI     | 🔵 Azul     | **GPIO 11**   | Izquierda pin 12| Datos SPI Out |
| SCK/CLK      | 🟣 Violeta  | **GPIO 18**   | Izquierda pin 19| Reloj SPI |
| LED/BLK      | ⚪ Blanco   | **3.3V**      | Izquierda pin 1 | Backlight |
| MISO/SDO     | 🟤 Marrón   | **GPIO 19**   | Izquierda pin 20| Datos SPI In |

#### Conexiones Táctil XPT2046 (SPI compartido)
| Pin Táctil | Color Cable | ESP32-S3 GPIO | Posición Física | Función |
|------------|-------------|---------------|-----------------|---------|
| T_CS       | 🟤 Marrón   | **GPIO 12**   | Izquierda pin 13| Chip Select Touch |
| T_IRQ      | 🟣 Rosa     | **GPIO 13**   | Izquierda pin 14| Interrupción (opcional) |
| T_DIN      | -           | **GPIO 11**   | Compartido MOSI | Datos Touch |
| T_DO       | -           | **GPIO 19**   | Compartido MISO | Datos Touch |
| T_CLK      | -           | **GPIO 18**   | Compartido SCK  | Reloj Touch |

**⚠️ IMPORTANTE:** MOSI, MISO y SCK se comparten entre pantalla y táctil. Solo conecta UNA VEZ cada señal.

---

### 3️⃣ SENSORES DE RUEDAS LJ12A3-4-Z/BX (Inductivos 5V)

**Especificación:** 6 tornillos por rueda distribuidos uniformemente (6 pulsos/revolución)

| Sensor Rueda | Color Cable | ESP32-S3 GPIO | Posición Física | Ubicación Física |
|--------------|-------------|---------------|-----------------|------------------|
| FL (Frontal Izq) | 🔵 Azul | **GPIO 35**   | Derecha pin 10  | Rueda delantera izq |
| FR (Frontal Der) | 🟢 Verde| **GPIO 36**   | Derecha pin 9   | Rueda delantera der |
| RL (Trasera Izq) | 🟡 Amarillo| **GPIO 30**| Derecha pin 16  | Rueda trasera izq |
| RR (Trasera Der) | 🟠 Naranja| **GPIO 31** | Derecha pin 15  | Rueda trasera der |

**Cada sensor tiene 3 cables:**
- 🔴 Rojo → 5V (alimentación)
- ⚫ Negro → GND
- 🟡 Amarillo/Señal → GPIO ESP32-S3 (señal)

**⚠️ IMPORTANTE:** Sensores son 5V pero GPIO ESP32-S3 es 3.3V. Usar convertidor de nivel bidireccional TXS0104E.

---

### 4️⃣ ENCODER DIRECCIÓN E6B2-CWZ6C 1200PR + Sensor Z

**Especificación:** Encoder incremental 1200 pulsos/revolución con índice Z

| Señal | Color Cable | ESP32-S3 GPIO | Posición Física | Función |
|-------|-------------|---------------|-----------------|---------|
| Canal A | 🔵 Azul   | **GPIO 37**   | Derecha pin 8   | Cuadratura A |
| Canal B | 🟢 Verde  | **GPIO 38**   | Derecha pin 7   | Cuadratura B |
| Índice Z | 🟡 Amarillo| **GPIO 25** | Derecha pin 18  | Centrado (sensor LJ12A3) |
| 5V      | 🔴 Rojo   | **5V**        | Derecha pin 1   | Alimentación |
| GND     | ⚫ Negro  | **GND**       | Cualquier GND   | Tierra |

**Señal Z:** Un tornillo en el centro del volante detectado por sensor inductivo LJ12A3-4-Z/BX.

**⚠️ IMPORTANTE:** Encoder es 5V, usar convertidor de nivel para canales A, B y Z.

---

### 5️⃣ MOTORES BTS7960 (4 Canales - 43A cada uno)

#### Motor Frontal Izquierdo (FL)
| Señal | Color Cable | ESP32-S3 GPIO | Posición Física |
|-------|-------------|---------------|-----------------|
| PWM   | 🟣 Violeta  | **GPIO 1**    | Izquierda pin 4 |
| IN1   | 🔵 Azul     | **GPIO 3**    | Izquierda pin 6 |
| IN2   | 🟢 Verde    | **GPIO 6**    | Izquierda pin 9 |

#### Motor Frontal Derecho (FR)
| Señal | Color Cable | ESP32-S3 GPIO | Posición Física |
|-------|-------------|---------------|-----------------|
| PWM   | 🟣 Violeta  | **GPIO 7**    | Izquierda pin 10|
| IN1   | 🔵 Azul     | **GPIO 8**    | Izquierda pin 11|
| IN2   | 🟢 Verde    | **GPIO 9**    | Izquierda pin 12|

#### Motor Trasero Izquierdo (RL)
| Señal | Color Cable | ESP32-S3 GPIO | Posición Física |
|-------|-------------|---------------|-----------------|
| PWM   | 🟣 Violeta  | **GPIO 10**   | Izquierda pin 13|
| IN1   | 🔵 Azul     | **GPIO 47**   | Derecha pin 4   |
| IN2   | 🟢 Verde    | **GPIO 48**   | Derecha pin 3   |

#### Motor Trasero Derecho (RR)
| Señal | Color Cable | ESP32-S3 GPIO | Posición Física |
|-------|-------------|---------------|-----------------|
| PWM   | 🟣 Violeta  | **GPIO 24**   | Derecha pin 19  |
| IN1   | 🔵 Azul     | **GPIO 28**   | Derecha pin 6   |
| IN2   | 🟢 Verde    | **GPIO 29**   | Derecha pin 17  |

**Cada BTS7960 también tiene:**
- VCC → 5V (lógica)
- GND → GND común
- MOTOR+ → Terminal motor +
- MOTOR- → Terminal motor -
- B+ → Batería 24V +
- B- → Batería 24V -

---

### 6️⃣ SENSORES I2C (Bus compartido)

**Bus I2C Hardware Nativo ESP32-S3:**
- **SDA:** GPIO 21 (Derecha pin 6)
- **SCL:** GPIO 20 (Derecha pin 7)

#### Sensores INA226 (Corriente/Voltaje)
| Sensor | Dirección I2C | Función |
|--------|---------------|---------|
| INA226 #1 | 0x40 | Batería principal 24V |
| INA226 #2 | 0x41 | Sistema auxiliar 12V |
| INA226 #3 | 0x44 | Motor FL |
| INA226 #4 | 0x45 | Motor FR |

**Conexión común a todos los INA226:**
- SDA → GPIO 21
- SCL → GPIO 20
- VCC → 3.3V
- GND → GND

**✅ VENTAJA:** INA226 trabajan a 3.3V nativamente, NO necesitan convertidor de nivel.

---

### 7️⃣ DFPLAYER MINI (Reproductor Audio)

| Pin DFPlayer | Color Cable | ESP32-S3 GPIO | Posición Física | Función |
|--------------|-------------|---------------|-----------------|---------|
| RX           | 🔵 Azul     | **GPIO 17**   | Izquierda pin 18| ESP32 TX → DFPlayer RX |
| TX           | 🟢 Verde    | **GPIO 16**   | Izquierda pin 17| DFPlayer TX → ESP32 RX |
| VCC          | 🔴 Rojo     | **5V**        | Derecha pin 1   | Alimentación |
| GND          | ⚫ Negro    | **GND**       | Cualquier GND   | Tierra |
| SPK1         | 🟠 Naranja  | -             | -               | Altavoz + |
| SPK2         | 🟤 Marrón   | -             | -               | Altavoz - |

**Tarjeta SD:** Carpeta `/mp3/` con archivos 0001.mp3 a 0038.mp3

---

### 8️⃣ PEDAL ACELERADOR (Analógico)

| Pin Pedal | Color Cable | ESP32-S3 GPIO | Posición Física | Rango |
|-----------|-------------|---------------|-----------------|-------|
| Señal     | 🟡 Amarillo | **GPIO 34**   | Derecha pin 11  | 0-100% |
| VCC       | 🔴 Rojo     | **3.3V**      | Izquierda pin 1 | Alimentación |
| GND       | ⚫ Negro    | **GND**       | Cualquier GND   | Tierra |

**Tipo:** Potenciómetro lineal 0-3.3V (ADC 12-bit)

---

### 9️⃣ BOTONES FÍSICOS

| Botón | Función | ESP32-S3 GPIO | Posición Física | Color |
|-------|---------|---------------|-----------------|-------|
| BTN1  | Luces   | **GPIO 40**   | NO DISPONIBLE*  | 🔵 Azul |
| BTN2  | Media   | **GPIO 41**   | NO DISPONIBLE*  | 🟢 Verde |
| BTN3  | Modo 4x4| **GPIO 42**   | NO DISPONIBLE*  | 🟡 Amarillo |
| BTN4  | Batería | **GPIO 46**   | Derecha pin 2   | 🔴 Rojo |

**⚠️ IMPORTANTE:** GPIO 40, 41, 42 NO están físicamente disponibles en los headers del DevKitC-1. Opciones:
1. Soldar cables directamente al chip ESP32-S3
2. Reasignar botones a GPIOs libres (GPIO 0, 22, 23, 43)
3. Usar teclado I2C externo

**Configuración típica botón:**
- Un terminal → GPIO
- Otro terminal → GND
- Pull-up interno activado en firmware

---

### 🔟 SENSOR TEMPERATURA DS18B20 (OneWire)

| Pin DS18B20 | Color Cable | ESP32-S3 GPIO | Posición Física |
|-------------|-------------|---------------|-----------------|
| DQ (Datos)  | 🟡 Amarillo | **GPIO 26**   | Derecha pin 4   |
| VCC         | 🔴 Rojo     | **5V**        | Derecha pin 1   |
| GND         | ⚫ Negro    | **GND**       | Cualquier GND   |

**Resistencia pull-up:** 4.7kΩ entre DQ y VCC (puede estar integrada en módulo)  
**⚠️ IMPORTANTE:** Sensor 5V, usar convertidor de nivel si es necesario.

---

### 1️⃣1️⃣ RELÉS ADICIONALES (Módulo 8 Canales 5V)

| Relé | Función | ESP32-S3 GPIO | Posición Física |
|------|---------|---------------|-----------------|
| IN3  | Encoder Dirección | **GPIO 5**  | Izquierda pin 8 |
| IN6  | Luces   | **GPIO 32**   | Derecha pin 12  |
| IN7  | Media   | **GPIO 33**   | Derecha pin 13  |

**Alimentación módulo:**
- VCC → 5V
- GND → GND

**Cada relé tiene:** COM, NC, NO para conmutar cargas hasta 10A

---

## ⚡ SISTEMA DE ALIMENTACIÓN

### Diagrama de Distribución de Potencia

```
┌──────────────────────────────────────────────────────────┐
│                 BATERÍA PRINCIPAL 24V                    │
│                      (40Ah Li-Ion)                       │
└────────────┬──────────────────────────────┬──────────────┘
             │                              │
             │                    ┌─────────▼──────────┐
             │                    │  HY-M158 #1 (40A)  │
             │                    │   Control: GPIO 4   │
             │                    └─────────┬──────────┘
             │                              │
             │                    ┌─────────▼──────────┐
             │                    │  4x BTS7960 (43A)  │
             │                    │  PWM: 1,7,10,24    │
             │                    │  IN1: 3,8,47,28    │
             │                    │  IN2: 6,9,48,29    │
             │                    └────────────────────┘
             │                              │
             │                     ┌────────▼────────┐
             │                     │  MOTORES 24V DC │
             │                     │  (4 unidades)   │
             │                     └─────────────────┘
             │
   ┌─────────▼─────────┐
   │  Buck 24V → 12V   │
   │    (10A)          │
   └─────────┬─────────┘
             │
   ┌─────────▼──────────┐
   │  HY-M158 #2 (40A)  │
   │   Control: GPIO 2   │
   └─────────┬──────────┘
             │
   ┌─────────▼─────────┐
   │  Buck 12V → 5V    │
   │    (5A)           │
   └─────────┬─────────┘
             │
             ├──► ESP32-S3 (5V → 3.3V interno)
             ├──► DFPlayer Mini (5V)
             ├──► Módulo Relés (5V)
             ├──► Sensores 5V (LJ12A3, Encoder)
             └──► LEDs WS2812B (5V)

┌────────────────────────────────────┐
│  BATERÍA AUXILIAR 12V (Opcional)   │
│        (7Ah Li-Ion)                │
└─────────┬──────────────────────────┘
          │
          ├──► Luces 12V
          ├──► Audio 12V
          └──► Pantalla TFT (3.3V)
```

### Especificaciones de Alimentación

| Componente | Voltaje | Corriente Máx | Fuente |
|------------|---------|---------------|--------|
| ESP32-S3   | 5V → 3.3V | 500mA      | Buck 5V |
| Motores BTS7960 | 24V | 4 x 43A = 172A | Batería 24V |
| DFPlayer Mini | 5V | 500mA          | Buck 5V |
| Pantalla ILI9488 | 3.3V | 200mA       | ESP32 3.3V |
| Sensores INA226 | 3.3V | 50mA (4x)   | ESP32 3.3V |
| Sensores LJ12A3 | 5V | 200mA (4x)    | Buck 5V |
| Encoder E6B2 | 5V | 100mA          | Buck 5V |
| DS18B20    | 5V | 50mA            | Buck 5V |
| Relés HY-M158 | 24V/12V | 40A (2x)  | Baterías |
| Módulo Relés 8ch | 5V | 100mA      | Buck 5V |

**⚠️ PROTECCIONES NECESARIAS:**
- Fusible 50A en batería 24V principal
- Fusible 15A en salida 12V
- Fusible 8A en salida 5V
- Diodo flyback en cada relé
- Capacitores 1000µF en cada Buck converter

---

## 📋 DIAGRAMAS DE CONEXIÓN

### Bus SPI Compartido (Pantalla + Táctil)

```
         ESP32-S3
    ┌─────────────┐
    │             │
    │  GPIO 18 ───┼──┬─► ILI9488 SCK
    │  (SCK)      │  └─► XPT2046 T_CLK
    │             │
    │  GPIO 19 ───┼──┬─► ILI9488 MISO
    │  (MISO)     │  └─► XPT2046 T_DO
    │             │
    │  GPIO 11 ───┼──┬─► ILI9488 MOSI
    │  (MOSI)     │  └─► XPT2046 T_DIN
    │             │
    │  GPIO 15 ───┼────► ILI9488 CS
    │             │
    │  GPIO 12 ───┼────► XPT2046 T_CS
    │             │
    └─────────────┘
```

### Bus I2C Compartido (4x INA226)

```
         ESP32-S3
    ┌─────────────┐
    │             │
    │  GPIO 21 ───┼──┬─► INA226 #1 (0x40) SDA
    │  (SDA)      │  ├─► INA226 #2 (0x41) SDA
    │             │  ├─► INA226 #3 (0x44) SDA
    │             │  └─► INA226 #4 (0x45) SDA
    │             │
    │  GPIO 20 ───┼──┬─► INA226 #1 SCL
    │  (SCL)      │  ├─► INA226 #2 SCL
    │             │  ├─► INA226 #3 SCL
    │             │  └─► INA226 #4 SCL
    │             │
    └─────────────┘
    
Resistencias pull-up: 4.7kΩ en SDA y SCL a 3.3V
```

### Convertidor de Nivel 5V → 3.3V

```
    Sensor 5V         TXS0104E         ESP32-S3 3.3V
  ┌──────────┐     ┌────────────┐     ┌──────────┐
  │          │     │            │     │          │
  │  LJ12A3  ├────►│HV1     LV1 ├────►│ GPIO 35  │ FL
  │          │     │            │     │          │
  │          │     │HV2     LV2 ├────►│ GPIO 36  │ FR
  │          │     │            │     │          │
  │          │     │HV3     LV3 ├────►│ GPIO 30  │ RL
  │  Encoder │     │            │     │          │
  │          │     │HV4     LV4 ├────►│ GPIO 31  │ RR
  │          │     │            │     │          │
  └──────────┘     └────────────┘     └──────────┘
       5V              VCC_HV=5V          3.3V
                       VCC_LV=3.3V
                       GND común
```

---

## ✅ CHECKLIST DE VERIFICACIÓN PRE-ENCENDIDO

### Fase 1: Inspección Visual
- [ ] Todos los cables están conectados firmemente
- [ ] No hay cables pelados o expuestos
- [ ] Polaridad correcta en todos los conectores (VCC/GND)
- [ ] Ningún pin está cortocircuitado
- [ ] Disipadores instalados en BTS7960
- [ ] Ventilación adecuada para ESP32-S3 y drivers

### Fase 2: Verificación de Continuidad (Multímetro)
- [ ] Continuidad GND ESP32-S3 ↔ GND batería
- [ ] Continuidad 3.3V ESP32-S3 ↔ VCC sensores 3.3V
- [ ] No hay cortocircuito VCC ↔ GND en ningún punto
- [ ] Resistencia pull-up I2C presente (4.7kΩ)
- [ ] Continuidad bus SPI (SCK, MOSI, MISO)

### Fase 3: Verificación de Voltajes (Sin Carga)
- [ ] Batería 24V: 25.2V (cargada) - 21V (descargada)
- [ ] Salida Buck 12V: 12.0V ± 0.2V
- [ ] Salida Buck 5V: 5.0V ± 0.1V
- [ ] ESP32-S3 3.3V: 3.30V ± 0.05V
- [ ] Sin voltaje en GPIOs (todos en LOW al inicio)

### Fase 4: Prueba Incremental (Con Carga Gradual)
1. [ ] Alimentar solo ESP32-S3 (sin periféricos)
2. [ ] Conectar pantalla TFT (debe mostrar algo)
3. [ ] Conectar sensores I2C (verificar direcciones con escaneo)
4. [ ] Conectar DFPlayer (debe reproducir audio de bienvenida)
5. [ ] Conectar sensores ruedas (verificar interrupciones)
6. [ ] Conectar encoder dirección (verificar cuadratura)
7. [ ] Conectar motores BTS7960 (probar PWM bajo 10%)
8. [ ] Prueba completa con todos los sistemas

### Fase 5: Verificación Software
- [ ] Firmware flasheado correctamente
- [ ] Monitor serial muestra "Sistema iniciado OK"
- [ ] Pantalla TFT muestra interfaz completa
- [ ] Sensores I2C responden (INA226 lectura OK)
- [ ] Encoder dirección detecta movimiento
- [ ] Sensores ruedas detectan pulsos (6/revolución)
- [ ] Audio reproduce mensajes correctamente
- [ ] WiFi conectado (si aplica)
- [ ] OTA funcional (si aplica)

---

## ⚠️ NOTAS DE SEGURIDAD CRÍTICAS

### 🔴 PELIGROS ELÉCTRICOS

1. **NUNCA conectes/desconectes nada con el sistema alimentado**
   - Siempre desconectar baterías antes de modificar conexiones
   - Esperar 30 segundos después de desconectar para descargar capacitores

2. **Corrientes altas en motores (43A por canal)**
   - Usar cables de al menos 10 AWG (5.5mm²) para alimentación motores
   - Conectores con capacidad mínima 50A
   - Fusibles obligatorios en cada línea de potencia

3. **Polaridad CRÍTICA en componentes electrónicos**
   - Invertir VCC/GND destruye instantáneamente: ESP32-S3, INA226, pantalla TFT
   - Marcar con cinta aislante de color TODOS los cables VCC (rojo) y GND (negro)

### 🟡 PROTECCIÓN DE COMPONENTES

4. **Convertidores de nivel OBLIGATORIOS**
   - Sensores 5V sin convertidor dañarán GPIOs del ESP32-S3
   - Usar TXS0104E o similar bidireccional
   - Verificar VCC_HV=5V y VCC_LV=3.3V en el módulo

5. **Disipadores de calor OBLIGATORIOS**
   - BTS7960 se calientan >80°C bajo carga
   - ESP32-S3 necesita disipador si WiFi activo continuamente
   - Buck converters necesitan ventilación forzada

6. **Protección contra inversión de polaridad**
   - Diodo Schottky 50A en serie con batería 24V
   - Fusibles de acción rápida en todas las líneas de potencia

### 🟢 OPERACIÓN SEGURA

7. **Secuencia de encendido OBLIGATORIA**
   - ✅ Conectar batería 12V auxiliar (periféricos)
   - ✅ Esperar 2 segundos
   - ✅ Conectar batería 24V principal (motores)
   - ✅ Verificar audio "Bienvenido" antes de mover
   - ✅ Verificar pantalla TFT muestra SOC > 20%

8. **Secuencia de apagado OBLIGATORIA**
   - ✅ Colocar marcha en P (Parking)
   - ✅ Activar freno de parking
   - ✅ Girar llave a OFF
   - ✅ Esperar audio "Hasta pronto" (5 segundos)
   - ✅ Desconectar batería 24V principal
   - ✅ Desconectar batería 12V auxiliar (opcional)

9. **Monitoreo continuo OBLIGATORIO**
   - Temperatura motores < 60°C (alerta automática)
   - Voltaje batería > 21V (alerta automática)
   - Corriente motores < 40A por canal (protección BTS7960)

### 🔵 MANTENIMIENTO PREVENTIVO

10. **Inspección semanal**
    - [ ] Verificar apriete de terminales (par de 0.5 Nm)
    - [ ] Verificar temperatura en reposo (< 30°C ambiente)
    - [ ] Limpiar polvo de componentes electrónicos
    - [ ] Verificar voltaje baterías en reposo

11. **Inspección mensual**
    - [ ] Revisar aislamiento cables (buscar grietas/pelados)
    - [ ] Verificar continuidad tierra/GND en todos los puntos
    - [ ] Calibrar encoder dirección (centrado mecánico)
    - [ ] Calibrar sensores ruedas (6 pulsos/rev verificados)
    - [ ] Actualizar firmware si hay nuevas versiones

---

## 📥 DESCARGA DEL FIRMWARE

### Enlaces de Descarga

**Firmware completo adaptado para ESP32-S3-DevKitC-1:**

```
https://github.com/florinzgz/coche-2-inteligente/
  archive/refs/heads/copilot/revise-and-fix-functionality.zip
```

**Contenido del paquete:**
- ✅ Firmware compilado `.bin` (listo para flashear)
- ✅ Código fuente completo PlatformIO
- ✅ Documentación completa (`docs/`)
- ✅ Archivos de configuración (`platformio.ini`, `pins.h`)
- ✅ Manual de conexiones (este documento)
- ✅ Mapa de pines DevKitC-1 (`PIN_MAPPING_DEVKITC1.md`)

### Instalación del Firmware

**Opción A: PlatformIO IDE (Recomendado)**
```bash
cd "firmware Coche Marcos"
pio run -t upload
```

**Opción B: esptool (Línea de comandos)**
```bash
esptool.py --chip esp32s3 --port COM3 write_flash 0x0 firmware.bin
```

**Opción C: ESP Flash Download Tool (Windows)**
1. Descargar: https://www.espressif.com/en/support/download/other-tools
2. Seleccionar chip: ESP32-S3
3. Cargar `firmware.bin` en dirección `0x0`
4. Presionar "START"

---

## 📞 SOPORTE Y CONTACTO

**Repositorio GitHub:**
https://github.com/florinzgz/coche-2-inteligente

**Documentación adicional:**
- `README.md` - Información general del proyecto
- `docs/PIN_MAPPING_DEVKITC1.md` - Tabla completa de pines
- `docs/CONEXIONES_HARDWARE_DEVKITC1.md` - Versión resumida

**Versión del firmware:** 2.0 - ESP32-S3-DevKitC-1 Compatible  
**Última actualización:** Noviembre 2025  
**Autor:** GitHub Copilot + @florinzgz

---

## 📜 LICENCIA Y DISCLAIMER

Este firmware y documentación se proporcionan "TAL CUAL" sin garantía de ningún tipo.

**⚠️ ADVERTENCIA:** Este es un proyecto de vehículo eléctrico con componentes de alta potencia (24V, 172A). El uso incorrecto puede causar:
- Daños materiales graves
- Lesiones personales
- Incendio o explosión de baterías

**El usuario es responsable de:**
- Cumplir todas las normativas locales de seguridad eléctrica
- Usar equipo de protección personal adecuado
- Realizar instalación por personal cualificado
- Mantenimiento preventivo regular
- Seguro de responsabilidad civil

**Este proyecto NO está homologado para uso en vía pública.**

---

**FIN DEL MANUAL DE CONEXIONES**
