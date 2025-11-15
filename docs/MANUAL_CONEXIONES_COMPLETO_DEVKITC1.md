# 📋 MANUAL COMPLETO DE CONEXIONES HARDWARE
## ESP32-S3-DevKitC-1 - Coche Inteligente Marcos

**Versión:** 1.0 Adaptada para ESP32-S3-DevKitC-1 (44 pines)  
**Fecha:** 2025-01-05  
**Hardware:** ESP32-S3-DevKitC-1 con GPIOs 0-48

---

## 🔍 ÍNDICE

1. [Pantalla ILI9488 + Táctil XPT2046](#1-pantalla-ili9488--táctil-xpt2046)
2. [Relés HY-M158 (40A)](#2-relés-hy-m158-40a)
3. [Sensores Ruedas LJ12A3-4-Z/BX](#3-sensores-ruedas-lj12a3-4-zbx)
4. [Encoder Dirección E6B2-CWZ6C](#4-encoder-dirección-e6b2-cwz6c)
5. [Motores BTS7960 (4 ruedas)](#5-motores-bts7960-4-ruedas)
6. [DFPlayer Mini Audio](#6-dfplayer-mini-audio)
7. [Sensores INA226 (I2C)](#7-sensores-ina226-i2c)
8. [Pedal Acelerador](#8-pedal-acelerador)
9. [Botones Control](#9-botones-control)
10. [Sensor Temperatura DS18B20](#10-sensor-temperatura-ds18b20)
11. [Alimentación del Sistema](#11-alimentación-del-sistema)
12. [Checklist Pre-Encendido](#12-checklist-pre-encendido)

---

## 1. PANTALLA ILI9488 + TÁCTIL XPT2046

### Especificaciones
- **Pantalla:** ILI9488 480x320 píxeles, TFT LCD
- **Táctil:** XPT2046 resistivo
- **Interfaz:** SPI compartido

### Conexiones Pantalla ILI9488

| Pin Pantalla | Color Cable | Pin ESP32-S3 | GPIO | Función |
|--------------|-------------|--------------|------|---------|
| VCC | Rojo | 3V3 | - | Alimentación 3.3V |
| GND | Negro | GND | - | Tierra |
| CS | Naranja | IO15 | 15 | Chip Select pantalla |
| RESET | Amarillo | IO14 | 14 | Reset pantalla |
| DC/RS | Verde | IO27 | 27 | Data/Command |
| SDI/MOSI | Azul | IO11 | 11 | Datos SPI ⚠️ NUEVO |
| SCK | Violeta | IO18 | 18 | Reloj SPI |
| LED | Blanco | 3V3 | - | Backlight |
| SDO/MISO | Gris | IO19 | 19 | Datos lectura SPI |

### Conexiones Táctil XPT2046

| Pin Táctil | Color Cable | Pin ESP32-S3 | GPIO | Función |
|------------|-------------|--------------|------|---------|
| T_CLK | - | IO18 | 18 | Compartido SCK |
| T_CS | Marrón | IO12 | 12 | Chip Select táctil |
| T_DIN | - | IO11 | 11 | Compartido MOSI |
| T_DO | - | IO19 | 19 | Compartido MISO |
| T_IRQ | Rosa | IO13 | 13 | Interrupción (opcional) |

### ⚠️ IMPORTANTE
- La pantalla y táctil **comparten** los pines SPI: MOSI (GPIO11), MISO (GPIO19), SCK (GPIO18)
- Solo se diferencian por sus Chip Select: GPIO15 (pantalla) y GPIO12 (táctil)
- **GPIO 11 es el nuevo MOSI** (antes era GPIO 23 en versiones anteriores)

---

## 2. RELÉS HY-M158 (40A)

### Especificaciones
- **Modelo:** HY-M158 (Relé Estado Sólido)
- **Entrada:** 3-32V DC (compatible ESP32)
- **Salida:** Hasta 40A DC
- **Cantidad:** 2 unidades + módulo 8 canales

### HY-M158 #1 - Tracción 24V

| Terminal HY-M158 | Cable | Conexión | Descripción |
|------------------|-------|----------|-------------|
| DC+ (Input) | Rojo | GPIO 4 (IO4) | Control desde ESP32 |
| DC- (Input) | Negro | GND | Tierra común |
| + (Output) | Rojo grueso | Batería 24V (+) | Alimentación motores |
| - (Output) | Negro grueso | Controladores BTS7960 | Salida a motores |
| LOAD | - | Motores 24V | Carga 4 motores |

### HY-M158 #2 - Sistema 12V

| Terminal HY-M158 | Cable | Conexión | Descripción |
|------------------|-------|----------|-------------|
| DC+ (Input) | Rojo | GPIO 2 (IO2) | Control desde ESP32 |
| DC- (Input) | Negro | GND | Tierra común |
| + (Output) | Rojo grueso | Batería 12V (+) | Alimentación general |
| - (Output) | Negro grueso | Luces/TFT/Audio | Salida sistema |
| LOAD | - | Sistema 12V | Carga periféricos |

### Relés Adicionales (Módulo 8 Canales 5V)

| Canal | GPIO | Función | Módulo Controlado |
|-------|------|---------|-------------------|
| IN1 | GPIO 5 | Encoder Dirección | Alimentación encoder |
| IN2 | GPIO 32 | Luces | Sistema luces |
| IN3 | GPIO 33 | Audio/DFPlayer | DFPlayer Mini |
| IN4-8 | - | Reserva | Futuras expansiones |

**Alimentación Módulo:**
- VCC → 5V
- GND → GND

---

## 3. SENSORES RUEDAS LJ12A3-4-Z/BX

### Especificaciones
- **Modelo:** LJ12A3-4-Z/BX (Sensor Inductivo NPN NO)
- **Distancia:** 4mm
- **Alimentación:** 6-36V DC
- **Configuración:** 6 tornillos M4 por rueda (equidistantes 60°)
- **Salida:** NPN Normalmente Abierto

### Conexiones por Rueda

#### Rueda Frontal Izquierda (FL)

| Pin Sensor | Cable | Conexión ESP32-S3 | Descripción |
|------------|-------|-------------------|-------------|
| Marrón | Marrón | 12V (Buck 12V) | Alimentación + |
| Azul | Azul | GND | Tierra |
| Negro | Amarillo | GPIO 35 (IO35) | Señal detección |

#### Rueda Frontal Derecha (FR)

| Pin Sensor | Cable | Conexión ESP32-S3 | Descripción |
|------------|-------|-------------------|-------------|
| Marrón | Marrón | 12V (Buck 12V) | Alimentación + |
| Azul | Azul | GND | Tierra |
| Negro | Amarillo | GPIO 36 (IO36) | Señal detección |

#### Rueda Trasera Izquierda (RL)

| Pin Sensor | Cable | Conexión ESP32-S3 | Descripción |
|------------|-------|-------------------|-------------|
| Marrón | Marrón | 12V (Buck 12V) | Alimentación + |
| Azul | Azul | GND | Tierra |
| Negro | Amarillo | **GPIO 30 (IO30)** ⚠️ NUEVO | Señal detección |

#### Rueda Trasera Derecha (RR)

| Pin Sensor | Cable | Conexión ESP32-S3 | Descripción |
|------------|-------|-------------------|-------------|
| Marrón | Marrón | 12V (Buck 12V) | Alimentación + |
| Azul | Azul | GND | Tierra |
| Negro | Amarillo | **GPIO 31 (IO31)** ⚠️ NUEVO | Señal detección |

### ⚠️ Notas Importantes
- Firmware configurado para **6 pulsos por revolución**
- Tornillos M4 espaciados 60° (360°/6 = 60°)
- Sensores detectan tornillos a máximo 4mm
- **GPIOs 30 y 31 son NUEVOS** en esta versión adaptada

---

## 4. ENCODER DIRECCIÓN E6B2-CWZ6C

### Especificaciones
- **Modelo:** Omron E6B2-CWZ6C 1200P/R
- **Resolución:** 1200 pulsos/revolución
- **Canales:** A, B (cuadratura) + Z (índice)
- **Alimentación:** 5-12V DC
- **Salida:** NPN Open Collector

### Conexiones Encoder

| Pin Encoder | Cable | Conexión ESP32-S3 | Descripción |
|-------------|-------|-------------------|-------------|
| Marrón | Marrón | 5V (Buck 5V) | Alimentación + |
| Azul | Azul | GND | Tierra |
| Negro (A) | Amarillo | **GPIO 37 (IO37)** ⚠️ NUEVO | Canal A |
| Blanco (B) | Verde | **GPIO 38 (IO38)** ⚠️ NUEVO | Canal B |
| Naranja (Z) | Naranja | GPIO 25 (IO25) | Índice Z (centrado) |

### Sensor Centrado Z (LJ12A3-4-Z/BX)

| Pin Sensor | Cable | Conexión | Descripción |
|------------|-------|----------|-------------|
| Marrón | Marrón | 12V | Alimentación + |
| Azul | Azul | GND | Tierra |
| Negro | Naranja | GPIO 25 (IO25) | Señal Z (tornillo central) |

### ⚠️ Configuración
- **Tornillo M4 único** en centro del volante (posición 0°)
- Canal A y B detectan rotación y dirección
- Señal Z detecta paso por centro (calibración)
- **GPIOs 37 y 38 son NUEVOS** (antes 44 y 45)

---

## 5. MOTORES BTS7960 (4 RUEDAS)

### Especificaciones
- **Modelo:** BTS7960 43A Motor Driver
- **Alimentación:** 6-27V DC
- **Corriente:** Hasta 43A por canal
- **Control:** PWM + IN1/IN2 (dirección)
- **Cantidad:** 4 módulos (uno por rueda)

### Motor Frontal Izquierda (FL)

| Pin BTS7960 | Cable | Pin ESP32-S3 | GPIO | Función |
|-------------|-------|--------------|------|---------|
| RPWM | Naranja | **IO1** | **1** | PWM ⚠️ NUEVO |
| LPWM | Amarillo | **IO1** | **1** | PWM (mismo pin) |
| R_EN | Rojo | 5V | - | Enable derecha |
| L_EN | Rojo | 5V | - | Enable izquierda |
| R_IS | - | No conectar | - | Sensor corriente |
| L_IS | - | No conectar | - | Sensor corriente |
| VCC | Rojo | 5V | - | Lógica 5V |
| GND | Negro | GND | - | Tierra |
| B+ | Rojo grueso | 24V | - | Motor + |
| B- | Negro grueso | GND 24V | - | Motor - |
| M+ | Rojo motor | Motor FL + | - | Salida motor + |
| M- | Negro motor | Motor FL - | - | Salida motor - |

**Pines Control Dirección FL:**
- **IN1:** GPIO 3 (IO3) ⚠️ NUEVO
- **IN2:** GPIO 6 (IO6) ⚠️ NUEVO

### Motor Frontal Derecha (FR)

| Pin BTS7960 | Cable | Pin ESP32-S3 | GPIO | Función |
|-------------|-------|--------------|------|---------|
| RPWM/LPWM | Naranja | **IO7** | **7** | PWM ⚠️ NUEVO |
| R_EN/L_EN | Rojo | 5V | - | Enable |
| VCC | Rojo | 5V | - | Lógica |
| GND | Negro | GND | - | Tierra |
| B+ | Rojo grueso | 24V | - | Motor + |
| M+/M- | Motor FR | Motor FR | - | Salida motor |

**Pines Control Dirección FR:**
- **IN1:** GPIO 8 (IO8) ⚠️ NUEVO
- **IN2:** GPIO 9 (IO9) ⚠️ NUEVO

### Motor Trasera Izquierda (RL)

| Pin BTS7960 | Cable | Pin ESP32-S3 | GPIO | Función |
|-------------|-------|--------------|------|---------|
| RPWM/LPWM | Naranja | **IO10** | **10** | PWM ⚠️ NUEVO |
| R_EN/L_EN | Rojo | 5V | - | Enable |
| VCC | Rojo | 5V | - | Lógica |
| GND | Negro | GND | - | Tierra |
| B+ | Rojo grueso | 24V | - | Motor + |
| M+/M- | Motor RL | Motor RL | - | Salida motor |

**Pines Control Dirección RL:**
- **IN1:** GPIO 47 (IO47) ⚠️ NUEVO
- **IN2:** GPIO 48 (IO48) ⚠️ NUEVO

### Motor Trasera Derecha (RR)

| Pin BTS7960 | Cable | Pin ESP32-S3 | GPIO | Función |
|-------------|-------|--------------|------|---------|
| RPWM/LPWM | Naranja | **IO24** | **24** | PWM ⚠️ NUEVO |
| R_EN/L_EN | Rojo | 5V | - | Enable |
| VCC | Rojo | 5V | - | Lógica |
| GND | Negro | GND | - | Tierra |
| B+ | Rojo grueso | 24V | - | Motor + |
| M+/M- | Motor RR | Motor RR | - | Salida motor |

**Pines Control Dirección RR:**
- **IN1:** GPIO 28 (IO28) ⚠️ NUEVO
- **IN2:** GPIO 29 (IO29) ⚠️ NUEVO

### ⚠️ Tabla Resumen Motores

| Motor | PWM | IN1 | IN2 |
|-------|-----|-----|-----|
| FL | GPIO 1 | GPIO 3 | GPIO 6 |
| FR | GPIO 7 | GPIO 8 | GPIO 9 |
| RL | GPIO 10 | GPIO 47 | GPIO 48 |
| RR | GPIO 24 | GPIO 28 | GPIO 29 |

**TODOS LOS PINES DE MOTORES HAN CAMBIADO**

---

## 6. DFPLAYER MINI AUDIO

### Especificaciones
- **Modelo:** DFPlayer Mini MP3
- **Alimentación:** 3.2-5V DC
- **Interfaz:** UART (Serial)
- **Almacenamiento:** MicroSD (máx 32GB FAT32)

### Conexiones DFPlayer

| Pin DFPlayer | Cable | Pin ESP32-S3 | GPIO | Función |
|--------------|-------|--------------|------|---------|
| VCC | Rojo | 5V | - | Alimentación |
| GND | Negro | GND | - | Tierra |
| TX | Amarillo | IO16 | 16 | UART TX (a RX ESP32) |
| RX | Verde | IO17 | 17 | UART RX (a TX ESP32) |
| SPK_1 | Azul | Parlante + | - | Altavoz + |
| SPK_2 | Blanco | Parlante - | - | Altavoz - |
| BUSY | - | No conectar | - | Estado (opcional) |
| IO_1 | - | No conectar | - | No usado |

### Estructura Tarjeta SD

```
SD Card/
└── mp3/
    ├── 0001.mp3  (Bienvenido Marcos)
    ├── 0002.mp3  (Hasta pronto)
    ├── 0003.mp3  (Freno parking activado)
    ├── 0004.mp3  (Batería cargándose)
    ├── 0005.mp3  (Modo conducción D1)
    ├── 0006.mp3  (Modo conducción D2)
    ├── 0007.mp3  (Marcha atrás)
    ├── 0008.mp3  (Parking)
    ├── 0009.mp3  (Neutral)
    ├── 0010.mp3  (ABS activado)
    ├── 0011.mp3  (TCS activado)
    ├── 0012.mp3  (Regeneración activada)
    ├── 0013.mp3  (Batería crítica)
    ├── 0014.mp3  (Sobrecalentamiento)
    ├── 0015.mp3  (Sistema OK)
    ├── 0016.mp3  (Error sensor)
    ├── 0017-0038.mp3  (Más mensajes)
```

### ⚠️ Formato Audio
- **Formato:** MP3, 128-320 kbps
- **Nomenclatura:** 0001.mp3 a 0038.mp3 (4 dígitos)
- **Carpeta obligatoria:** /mp3/

---

## 7. SENSORES INA226 (I2C)

### Especificaciones
- **Modelo:** INA226 (Texas Instruments)
- **Función:** Medición corriente/voltaje/potencia
- **Interfaz:** I2C
- **Dirección:** 0x40, 0x41, 0x44, 0x45 (configurable)
- **Cantidad:** 4 sensores

### Conexiones I2C (Bus Compartido)

| Pin INA226 | Cable | Pin ESP32-S3 | GPIO | Función |
|------------|-------|--------------|------|---------|
| VCC | Rojo | 3.3V | - | Alimentación |
| GND | Negro | GND | - | Tierra |
| SDA | Amarillo | **IO21** | **21** | Datos I2C |
| SCL | Verde | **IO20** | **20** | Reloj I2C ⚠️ NUEVO |
| VIN+ | Rojo grueso | Batería + | - | Medición + |
| VIN- | Negro grueso | Carga - | - | Medición - |

### Distribución Sensores

| Sensor | Dirección I2C | Medición | VIN+/VIN- |
|--------|---------------|----------|-----------|
| INA226 #1 | 0x40 | Batería 24V | Batería/Motores |
| INA226 #2 | 0x41 | Batería 12V | Batería/Sistema |
| INA226 #3 | 0x44 | Motor FL | Motor FL +/- |
| INA226 #4 | 0x45 | Motor FR | Motor FR +/- |

### ⚠️ Importante
- **GPIO 20 es el nuevo SCL** (antes era GPIO 22)
- Todos los sensores comparten el mismo bus I2C
- Resistencias pull-up de 4.7kΩ ya integradas en módulos
- NO añadir resistencias pull-up adicionales

---

## 8. PEDAL ACELERADOR

### Especificaciones
- **Tipo:** Potenciómetro analógico 10kΩ
- **Rango:** 0-100% (0-3.3V)
- **Interfaz:** ADC1_CH3

### Conexiones Pedal

| Terminal Potenciómetro | Cable | Conexión ESP32-S3 | Descripción |
|------------------------|-------|-------------------|-------------|
| Terminal 1 (0V) | Negro | GND | Tierra |
| Terminal 2 (Cursor) | Amarillo | GPIO 34 (IO34) | Señal ADC |
| Terminal 3 (3.3V) | Rojo | 3.3V | Referencia |

### ⚠️ Calibración
- **0% (reposo):** 0V → ADC ≈ 0
- **100% (máximo):** 3.3V → ADC ≈ 4095
- Firmware aplica filtro software anti-rebote
- GPIO 34 sin cambios (compatible)

---

## 9. BOTONES CONTROL

### Especificaciones
- **Tipo:** Pulsadores táctiles NO (normalmente abierto)
- **Pull-up:** Interno (activado por firmware)
- **Lógica:** Activo en BAJO (presionado = 0V)

### Conexiones Botones

| Botón | Cable | Pin ESP32-S3 | GPIO | Función |
|-------|-------|--------------|------|---------|
| Luces | Amarillo | **IO40** | **40** | Toggle luces ⚠️ NO DISPONIBLE* |
| Media | Verde | **IO41** | **41** | Control audio ⚠️ NO DISPONIBLE* |
| 4x4 | Azul | **IO42** | **42** | Modo tracción ⚠️ NO DISPONIBLE* |
| Batería | Blanco | IO46 | 46 | Info batería |

**Conexión común:**
- Un terminal del botón → GPIO
- Otro terminal → GND

### ⚠️ ADVERTENCIA IMPORTANTE
**GPIOs 40, 41, 42 NO EXISTEN en ESP32-S3-DevKitC-1**

**SOLUCIÓN TEMPORAL:**
- Usar GPIOs disponibles: 0, 22, 23 (requiere ajuste firmware)
- O desactivar botones Luces/Media/4x4
- Botón Batería (GPIO 46) funciona correctamente

**Propuesta remapeo botones:**
- BTN_LIGHTS: GPIO 0 (disponible)
- BTN_MEDIA: GPIO 22 (disponible)  
- BTN_4X4: GPIO 23 (disponible)
- BTN_BATTERY: GPIO 46 (OK)

⚠️ **REQUIERE MODIFICACIÓN DE FIRMWARE** - Solicitar actualización

---

## 10. SENSOR TEMPERATURA DS18B20

### Especificaciones
- **Modelo:** DS18B20 (Digital OneWire)
- **Rango:** -55°C a +125°C
- **Interfaz:** OneWire (1-Wire)
- **Precisión:** ±0.5°C

### Conexiones DS18B20

| Pin DS18B20 | Cable | Conexión ESP32-S3 | Descripción |
|-------------|-------|-------------------|-------------|
| VDD | Rojo | 3.3V o 5V | Alimentación |
| GND | Negro | GND | Tierra |
| DQ (Data) | Amarillo | GPIO 26 (IO26) | Señal OneWire |

### ⚠️ Resistencia Pull-Up
- Añadir resistencia 4.7kΩ entre DQ (GPIO26) y VDD
- Ubicación: cerca del sensor
- Puede alimentarse a 3.3V o 5V (tolerante)

---

## 11. ALIMENTACIÓN DEL SISTEMA

### Diagrama General

```
Batería 24V (100Ah)
    ├─► HY-M158 #1 (GPIO4) ───► BTS7960 (4 motores) ───► Motores 24V
    │
    ├─► Buck 12V (10A) ───┬─► HY-M158 #2 (GPIO2) ───► Luces/TFT/Relés
    │                     │
    │                     ├─► Sensores LJ12A3 (4 ruedas)
    │                     │
    │                     └─► Encoder E6B2-CWZ6C
    │
    └─► Buck 5V (5A) ─────┬─► ESP32-S3-DevKitC-1
                          │
                          ├─► DFPlayer Mini
                          │
                          ├─► Módulo relés 8ch
                          │
                          └─► BTS7960 (lógica VCC)

Buck 3.3V (1A) ───────────┬─► Pantalla ILI9488
                          │
                          ├─► Sensores INA226 (4)
                          │
                          └─► DS18B20
```

### Módulos Convertidores Necesarios

| Módulo | Entrada | Salida | Corriente | Uso |
|--------|---------|--------|-----------|-----|
| Buck 12V | 24V | 12V | 10A | Sensores/Luces |
| Buck 5V | 12V | 5V | 5A | ESP32/DFPlayer |
| Buck 3.3V | 5V | 3.3V | 1A | Pantalla/INA226 |

### Consumo Estimado

| Componente | Voltaje | Corriente | Potencia |
|------------|---------|-----------|----------|
| 4 Motores | 24V | 4x20A | 1920W |
| ESP32-S3 | 5V | 500mA | 2.5W |
| Pantalla | 3.3V | 150mA | 0.5W |
| Sensores | 12V/5V | 200mA | 2.4W |
| DFPlayer | 5V | 50mA | 0.25W |
| **TOTAL** | - | **≈82A** | **≈1925W** |

---

## 12. CHECKLIST PRE-ENCENDIDO

### ✅ Verificación Visual

- [ ] Todas las conexiones de alimentación correctas (polaridad)
- [ ] GND común entre todos los módulos
- [ ] Cables SPI bien conectados (MOSI en GPIO 11)
- [ ] I2C correctamente cableado (SCL en GPIO 20, SDA en GPIO 21)
- [ ] Relés HY-M158 en GPIO 2 y 4
- [ ] 4 sensores de rueda conectados (GPIO 30, 31, 35, 36)
- [ ] Encoder dirección en GPIO 37, 38, 25
- [ ] Motores BTS7960 alimentados por HY-M158 #1
- [ ] Tarjeta SD con archivos MP3 en DFPlayer

### ✅ Verificación Eléctrica

- [ ] Batería 24V cargada (>22V)
- [ ] Buck 12V funcionando y ajustado
- [ ] Buck 5V funcionando y ajustado  
- [ ] Buck 3.3V funcionando y ajustado
- [ ] Continuidad GND en todos los puntos
- [ ] No cortocircuitos entre VCC y GND

### ✅ Verificación Firmware

- [ ] Firmware compilado sin errores
- [ ] Archivo pins.h con GPIOs correctos (versión DevKitC-1)
- [ ] PlatformIO configurado para ESP32-S3

### ✅ Primera Prueba

1. Conectar solo alimentación 5V a ESP32 (sin 24V motores)
2. Verificar LED power ESP32 encendido
3. Conectar USB y verificar puerto COM
4. Subir firmware y verificar Serial Monitor
5. Si OK, conectar pantalla y verificar imagen
6. Si OK, probar DFPlayer (audio bienvenida)
7. Si OK, conectar sensores y verificar lecturas
8. Si OK, conectar 12V (sin 24V aún)
9. Si OK, finalmente conectar 24V motores

### ⚠️ ADVERTENCIA SEGURIDAD

- **NUNCA** conectar/desconectar con sistema energizado
- **SIEMPRE** verificar polaridad antes de conectar
- **USAR** fusibles en líneas de potencia
- **TENER** interruptor emergencia accesible
- **NO** tocar circuitos con sistema ON

---

## 📥 ENLACES DESCARGA

### Firmware Actualizado
**Repositorio GitHub:**
```
https://github.com/florinzgz/coche-2-inteligente/tree/copilot/revise-and-fix-functionality
```

**Descarga ZIP:**
```
https://github.com/florinzgz/coche-2-inteligente/archive/refs/heads/copilot/revise-and-fix-functionality.zip
```

### Documentación Adicional

- **Mapeo Pines:** `docs/PIN_MAPPING_DEVKITC1.md`
- **Este Manual:** `docs/MANUAL_CONEXIONES_COMPLETO_DEVKITC1.md`
- **README:** `README.md`

---

## 📞 SOPORTE

**Autor:** GitHub Copilot  
**Usuario:** @florinzgz  
**Hardware:** ESP32-S3-DevKitC-1 (44 pines)  
**Versión Firmware:** 1.0 Adaptada  
**Fecha:** 2025-01-05

---

**FIN DEL MANUAL**
