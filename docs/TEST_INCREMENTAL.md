# Guía de Test Incremental de Hardware

**Versión:** 1.0  
**Fecha:** 2025-12-21  
**Firmware Compatible:** v2.11.0+

---

## 📋 Índice

1. [Objetivo](#objetivo)
2. [Prerrequisitos](#prerrequisitos)
3. [Pasos de Verificación](#pasos-de-verificación)
   - [PASO 1: Verificar Pantalla TFT](#paso-1-verificar-pantalla-tft-)
   - [PASO 2: Añadir Sensores de Corriente INA226](#paso-2-añadir-sensores-de-corriente-ina226-)
   - [PASO 3: Añadir Sensores de Temperatura DS18B20](#paso-3-añadir-sensores-de-temperatura-ds18b20-)
   - [PASO 4: Añadir Encoders de Ruedas](#paso-4-añadir-encoders-de-ruedas-)
   - [PASO 5: Añadir Detección de Obstáculos](#paso-5-añadir-detección-de-obstáculos-)
   - [PASO 6: Añadir Bluetooth](#paso-6-añadir-bluetooth-)
   - [PASO 7: Sistema Completo](#paso-7-sistema-completo-)
4. [Troubleshooting](#troubleshooting)
5. [Beneficios del Test Incremental](#beneficios-del-test-incremental)

---

## 🎯 Objetivo

Verificar el funcionamiento del sistema añadiendo módulos uno a uno, facilitando la detección de problemas de hardware.

**Problema que resuelve:**

El firmware en modo FULL intenta inicializar TODOS los sensores y módulos. Si solo tienes la pantalla conectada, el sistema intentará:
- Buscar sensores I2C (INA226) → timeouts
- Leer sensores 1-Wire (DS18B20) → timeouts
- Detectar encoders de ruedas → fallos
- Inicializar sensores VL53L0X → timeouts
- Conectar Bluetooth → posibles fallos

**Consecuencia:** Boot loops, pantallas en blanco, y frustración al no saber qué falla.

**Solución:** El environment `esp32-s3-test-incremental` permite habilitar hardware progresivamente mediante flags, verificando cada componente antes de añadir el siguiente.

---

## 📦 Prerrequisitos

- **Hardware:**
  - ESP32-S3-DevKitC-1 (44 pines)
  - Pantalla TFT ST7796S 480x320 con touch XPT2046
  - Cable USB para programación

- **Software:**
  - [PlatformIO](https://platformio.org/) instalado
  - Monitor serial configurado a **115200 baud**

- **Conocimientos:**
  - Editar archivos `.ini`
  - Compilar y subir firmware con PlatformIO
  - Leer output del monitor serial

---

## 🔬 Pasos de Verificación

### PASO 1: Verificar Pantalla TFT ✅

**Objetivo:** Verificar que la pantalla TFT funciona correctamente sin ningún sensor conectado.

#### Hardware necesario:
- ✅ Solo la pantalla TFT ST7796S conectada
- ❌ No se requiere ningún sensor

#### Configuración en platformio.ini:

El environment `esp32-s3-test-incremental` viene configurado por defecto en modo STANDALONE (solo pantalla):

```ini
[env:esp32-s3-test-incremental]
build_flags =
    -DSTANDALONE_DISPLAY    ; ✅ ACTIVADO
    -DSTANDALONE_TIMEOUT=30000
    ; Todo lo demás comentado
```

#### Compilar y subir:

```bash
# Compilar el firmware
pio run -e esp32-s3-test-incremental

# Subir el firmware
pio run -e esp32-s3-test-incremental -t upload

# Abrir monitor serial
pio device monitor
```

#### Resultado esperado:

**Pantalla:**
1. **Backlight** enciende inmediatamente
2. **Logo del sistema** por 1.5 segundos
3. **Dashboard completo** con datos simulados ANIMADOS:
   - **Velocidad:** 5-50 km/h (oscilante con onda sinusoidal)
   - **RPM:** 600-3000 (oscilante proporcional a velocidad)
   - **Temperatura:** 40-55°C (oscilante)
   - **Dirección:** Ruedas girando ±15°
   - **Luces:** Encienden/apagan cada 30s
   - **Batería:** 24-25V con consumo variable
   - **Cambio de marcha:** P ↔ D1 según velocidad simulada

**Serial output esperado:**

```
[BOOT] STANDALONE_DISPLAY MODE: Skipping sensor initialization
[BOOT] Initializing HUD Manager (display only)...
[BOOT] STANDALONE MODE: Dashboard active with simulated values
[BOOT] SKIPPING Current Sensors (not enabled in test config)
[BOOT] SKIPPING Temperature Sensors (not enabled in test config)
[BOOT] SKIPPING Wheel Sensors (not enabled in test config)
[BOOT] SKIPPING Obstacle Detection (not enabled in test config)
[BOOT] SKIPPING Bluetooth (not enabled in test config)
[BOOT] STANDALONE MODE: Setup complete!
```

#### ✅ Si funciona:
- La pantalla muestra el dashboard con animaciones suaves
- Los valores cambian de forma realista
- No hay reinicios del sistema

→ **Pasar al PASO 2**

#### ❌ Si NO funciona:

| Síntoma | Causa Probable | Solución |
|---------|---------------|----------|
| Pantalla negra | Backlight no enciende | Verificar GPIO42 está conectado y HIGH |
| Pantalla blanca | SPI no funciona | Verificar conexiones MOSI, MISO, SCK, CS, DC, RST |
| Colores incorrectos | Driver incorrecto | Verificar que el firmware usa ST7796_DRIVER |
| Boot loops | Watchdog timeout | Verificar alimentación 3.3V estable |

**Verificar conexiones TFT:**

| Pantalla Pin | ESP32-S3 GPIO | Función |
|--------------|---------------|---------|
| VCC          | 3.3V          | Alimentación |
| GND          | GND           | Tierra |
| CS           | GPIO 16       | Chip Select |
| DC           | GPIO 13       | Data/Command |
| RST          | GPIO 14       | Reset |
| MOSI         | GPIO 11       | Datos SPI |
| MISO         | GPIO 12       | Lectura SPI (opcional) |
| SCK          | GPIO 10       | Reloj SPI |
| BL           | GPIO 42       | Backlight |

Ver `include/pins.h` para el mapeo completo.

---

### PASO 2: Añadir Sensores de Corriente INA226 🔌

**Objetivo:** Verificar comunicación I2C y lecturas de corriente reales.

#### Hardware adicional:
- ✅ 6x INA226 conectados al bus I2C (direcciones 0x40-0x45)
- ✅ Resistencias pull-up 4.7kΩ en SDA/SCL a 3.3V

#### Configuración en platformio.ini:

Editar el archivo `platformio.ini` y modificar el environment:

```ini
[env:esp32-s3-test-incremental]
build_flags =
    ${env:esp32-s3-devkitc.build_flags}
    
    ; PASO 2: Habilitar sensores I2C
    ; -DSTANDALONE_DISPLAY    ; ❌ COMENTAR (desactivar standalone)
    -DENABLE_I2C_SENSORS      ; ✅ ACTIVAR sensores I2C
    
    ; Resto comentado
    ; -DENABLE_TEMP_SENSORS
    ; -DENABLE_WHEEL_SENSORS
    ; -DENABLE_OBSTACLE_DETECTION
    ; -DENABLE_BLUETOOTH
```

**Nota importante:** Al comentar `STANDALONE_DISPLAY`, el sistema intentará inicializar SOLO los sensores I2C (porque `ENABLE_I2C_SENSORS` está activo), pero NO los demás sensores.

#### Compilar y subir:

```bash
pio run -e esp32-s3-test-incremental -t upload
pio device monitor
```

#### Resultado esperado:

**Pantalla:**
- Dashboard muestra **corrientes REALES** de los INA226
- Los valores cambian según el consumo real del sistema
- No hay valores simulados

**Serial output esperado:**

```
[BOOT] FULL MODE: Starting hardware initialization...
[BOOT] Initializing Current Sensors (INA226)...
[BOOT] INA226: 6 sensors detected at addresses 0x40-0x45
[STACK] After Current Sensors - Free: XXXX bytes
[BOOT] SKIPPING Temperature Sensors (not enabled in test config)
[BOOT] SKIPPING Wheel Sensors (not enabled in test config)
[BOOT] SKIPPING Obstacle Detection (not enabled in test config)
[BOOT] SKIPPING Bluetooth (not enabled in test config)
```

#### Verificar:

1. **Bus I2C:**
   - SDA = GPIO 8
   - SCL = GPIO 9
   - Pull-ups 4.7kΩ a 3.3V

2. **Direcciones I2C esperadas:**
   - INA226 #1: 0x40 (Batería)
   - INA226 #2: 0x41 (Motor Frontal Izquierdo)
   - INA226 #3: 0x42 (Motor Frontal Derecho)
   - INA226 #4: 0x43 (Motor Trasero Izquierdo)
   - INA226 #5: 0x44 (Motor Trasero Derecho)
   - INA226 #6: 0x45 (Motor Dirección)

#### ✅ Si funciona:
- Los 6 sensores INA226 son detectados
- Las corrientes se muestran en pantalla
- No hay timeouts I2C

→ **Pasar al PASO 3**

#### ❌ Si NO funciona:

| Problema | Solución |
|----------|----------|
| I2C timeout | Añadir pull-ups 4.7kΩ en SDA/SCL |
| Sensores no detectados | Verificar direcciones I2C con escáner I2C |
| Boot loops | Verificar alimentación 3.3V estable |

---

### PASO 3: Añadir Sensores de Temperatura DS18B20 🌡️

**Objetivo:** Verificar bus 1-Wire y lecturas de temperatura.

#### Hardware adicional:
- ✅ 4x DS18B20 en bus 1-Wire
- ✅ Resistencia pull-up 4.7kΩ en la línea de datos a 3.3V

#### Configuración en platformio.ini:

```ini
[env:esp32-s3-test-incremental]
build_flags =
    ${env:esp32-s3-devkitc.build_flags}
    
    -DENABLE_I2C_SENSORS      ; ✅ Mantener I2C
    -DENABLE_TEMP_SENSORS     ; ✅ ACTIVAR temperatura
    
    ; Resto comentado
    ; -DENABLE_WHEEL_SENSORS
    ; -DENABLE_OBSTACLE_DETECTION
    ; -DENABLE_BLUETOOTH
```

#### Compilar y subir:

```bash
pio run -e esp32-s3-test-incremental -t upload
pio device monitor
```

#### Resultado esperado:

**Pantalla:**
- Temperaturas **REALES** de los motores en pantalla
- Cambio de color según umbrales (verde < 50°C, amarillo 50-70°C, rojo > 70°C)

**Serial output esperado:**

```
[BOOT] Initializing Current Sensors (INA226)...
[BOOT] INA226: 6 sensors detected
[BOOT] Initializing Temperature Sensors (DS18B20)...
[BOOT] DS18B20: 4 sensors detected
[BOOT] SKIPPING Wheel Sensors (not enabled in test config)
[BOOT] SKIPPING Obstacle Detection (not enabled in test config)
[BOOT] SKIPPING Bluetooth (not enabled in test config)
```

#### Verificar:

1. **Bus 1-Wire:**
   - Línea de datos con pull-up 4.7kΩ a 3.3V
   - 4 sensores DS18B20 conectados en paralelo

2. **Sensores esperados:**
   - Motor Frontal Izquierdo
   - Motor Frontal Derecho
   - Motor Trasero Izquierdo
   - Motor Trasero Derecho

#### ✅ Si funciona:
- Los 4 sensores DS18B20 son detectados
- Las temperaturas se muestran en pantalla
- No hay timeouts 1-Wire

→ **Pasar al PASO 4**

---

### PASO 4: Añadir Encoders de Ruedas 🎡

**Objetivo:** Verificar encoders Hall y cálculo de velocidad/odómetro.

#### Hardware adicional:
- ✅ 4x Encoders Hall en GPIOs configurados (ver `pins.h`)

#### Configuración en platformio.ini:

```ini
[env:esp32-s3-test-incremental]
build_flags =
    ${env:esp32-s3-devkitc.build_flags}
    
    -DENABLE_I2C_SENSORS
    -DENABLE_TEMP_SENSORS
    -DENABLE_WHEEL_SENSORS    ; ✅ ACTIVAR encoders
    
    ; Resto comentado
    ; -DENABLE_OBSTACLE_DETECTION
    ; -DENABLE_BLUETOOTH
```

#### Compilar y subir:

```bash
pio run -e esp32-s3-test-incremental -t upload
pio device monitor
```

#### Resultado esperado:

**Pantalla:**
- **Velocidad real** calculada de encoders (km/h)
- **RPM** proporcionales a la velocidad
- **Odómetro** funcional (km total y trip)

**Serial output esperado:**

```
[BOOT] Initializing Current Sensors (INA226)...
[BOOT] Initializing Temperature Sensors (DS18B20)...
[BOOT] Initializing Wheel Sensors...
[BOOT] Wheel encoders: 4 sensors initialized
[BOOT] SKIPPING Obstacle Detection (not enabled in test config)
[BOOT] SKIPPING Bluetooth (not enabled in test config)
```

#### Verificar:

1. **Encoders Hall:** Verificar GPIOs de encoders en `pins.h`
2. **Calibración:** Los encoders deben estar calibrados (ver `CHECKLIST.md`)

#### ✅ Si funciona:
- Los 4 encoders son detectados
- La velocidad se calcula correctamente
- El odómetro incrementa cuando las ruedas giran

→ **Pasar al PASO 5**

---

### PASO 5: Añadir Detección de Obstáculos 📡

**Objetivo:** Verificar sensores ToF VL53L0X y sistema de seguridad.

#### Hardware adicional:
- ✅ 4x VL53L0X conectados al bus I2C (diferentes direcciones)

#### Configuración en platformio.ini:

```ini
[env:esp32-s3-test-incremental]
build_flags =
    ${env:esp32-s3-devkitc.build_flags}
    
    -DENABLE_I2C_SENSORS
    -DENABLE_TEMP_SENSORS
    -DENABLE_WHEEL_SENSORS
    -DENABLE_OBSTACLE_DETECTION  ; ✅ ACTIVAR obstáculos
    
    ; Resto comentado
    ; -DENABLE_BLUETOOTH
```

#### Compilar y subir:

```bash
pio run -e esp32-s3-test-incremental -t upload
pio device monitor
```

#### Resultado esperado:

**Pantalla:**
- **Distancias** de obstáculos en pantalla
- **Alertas visuales/sonoras** cuando hay obstáculos cercanos
- Sistema de seguridad activo (frenado automático si está configurado)

**Serial output esperado:**

```
[BOOT] Initializing Current Sensors (INA226)...
[BOOT] Initializing Temperature Sensors (DS18B20)...
[BOOT] Initializing Wheel Sensors...
[BOOT] Initializing Obstacle Detection...
[BOOT] VL53L0X: 4 sensors detected
[BOOT] Initializing Obstacle Safety...
[BOOT] SKIPPING Bluetooth (not enabled in test config)
```

#### ✅ Si funciona:
- Los 4 sensores VL53L0X son detectados
- Las distancias se muestran en pantalla
- Las alertas funcionan correctamente

→ **Pasar al PASO 6**

---

### PASO 6: Añadir Bluetooth 📶

**Objetivo:** Verificar control remoto Bluetooth.

#### Configuración en platformio.ini:

```ini
[env:esp32-s3-test-incremental]
build_flags =
    ${env:esp32-s3-devkitc.build_flags}
    
    -DENABLE_I2C_SENSORS
    -DENABLE_TEMP_SENSORS
    -DENABLE_WHEEL_SENSORS
    -DENABLE_OBSTACLE_DETECTION
    -DENABLE_BLUETOOTH           ; ✅ ACTIVAR Bluetooth
```

#### Compilar y subir:

```bash
pio run -e esp32-s3-test-incremental -t upload
pio device monitor
```

#### Resultado esperado:

**Pantalla:**
- **Icono BT** en dashboard (estado conectado/desconectado)
- Control remoto funcional desde app Bluetooth

**Serial output esperado:**

```
[BOOT] Initializing Current Sensors (INA226)...
[BOOT] Initializing Temperature Sensors (DS18B20)...
[BOOT] Initializing Wheel Sensors...
[BOOT] Initializing Obstacle Detection...
[BOOT] Initializing Bluetooth Controller...
[BOOT] Bluetooth: Initialized successfully
```

#### ✅ Si funciona:
- Bluetooth se inicializa correctamente
- El dispositivo es visible desde el teléfono
- El control remoto funciona

→ **Pasar al PASO 7**

---

### PASO 7: Sistema Completo 🚀

**Objetivo:** Habilitar todos los módulos y verificar sistema completo.

#### Opción A: Todos los módulos en test incremental

Comentar TODOS los flags condicionales para que el sistema use el modo FULL por defecto:

```ini
[env:esp32-s3-test-incremental]
build_flags =
    ${env:esp32-s3-devkitc.build_flags}
    
    ; Comentar TODOS los flags
    ; -DENABLE_I2C_SENSORS
    ; -DENABLE_TEMP_SENSORS
    ; -DENABLE_WHEEL_SENSORS
    ; -DENABLE_OBSTACLE_DETECTION
    ; -DENABLE_BLUETOOTH
```

#### Opción B: Cambiar a environment de producción

Usar el environment optimizado para producción:

```bash
pio run -e esp32-s3-devkitc-release -t upload
```

Este firmware:
- ✅ Todos los módulos activos por defecto
- ✅ Sin debug (más rápido)
- ✅ Optimizado para rendimiento (-O3)
- ✅ Todos los sistemas de seguridad habilitados (ABS, TCS, Regen AI)

#### Resultado esperado:

**Sistema funcionando al 100%:**
- ✅ Pantalla TFT con dashboard completo
- ✅ Sensores de corriente (INA226)
- ✅ Sensores de temperatura (DS18B20)
- ✅ Encoders de ruedas (velocidad/odómetro)
- ✅ Detección de obstáculos (VL53L0X)
- ✅ Bluetooth (control remoto)
- ✅ ABS, TCS, Regen AI
- ✅ Audio (DFPlayer Mini)
- ✅ LEDs (WS2812B)

---

## 🔧 Troubleshooting

### Pantalla en blanco

**Causas:**
- GPIO42 (backlight) no está HIGH
- Problema en conexión SPI (MOSI, MISO, SCK, CS)
- Alimentación insuficiente

**Soluciones:**
1. Verificar GPIO42 conectado y con señal
2. Verificar todas las conexiones SPI
3. Probar con `esp32-s3-devkitc-no-touch` si hay conflictos con touch

### Sensores I2C no detectados

**Causas:**
- Faltan pull-ups en SDA/SCL
- Direcciones I2C incorrectas
- Alimentación 3.3V inestable

**Soluciones:**
1. Añadir resistencias pull-up 4.7kΩ en SDA/SCL a 3.3V
2. Escanear bus I2C con herramienta de diagnóstico
3. Verificar alimentación con multímetro

### Boot loops (reinicios constantes)

**Causas:**
- Watchdog no se alimenta (timeout > 30s)
- Stack overflow
- Sensor I2C bloqueado

**Soluciones:**
1. Verificar que watchdog se alimenta cada 30s max
2. Revisar stack size en platformio.ini (ya configurado en 32KB/20KB)
3. Usar `esp32-s3-devkitc-debug` para más logs
4. Volver a modo STANDALONE para aislar el problema

### Valores de sensores incorrectos

**Causas:**
- Sensor no calibrado
- Conexiones sueltas
- Interferencia electromagnética

**Soluciones:**
1. Ejecutar calibración de sensores (ver `CHECKLIST.md`)
2. Verificar todas las conexiones
3. Alejar cables de fuentes de ruido (motores, relés)

---

## 🎉 Beneficios del Test Incremental

1. ✅ **Verificación paso a paso** del hardware
   - Sabes exactamente qué módulo funciona y cuál no
   
2. ✅ **Diagnóstico fácil** de problemas
   - Si el PASO 3 falla, sabes que el problema está en los sensores de temperatura
   
3. ✅ **No requiere hardware completo** para empezar a probar
   - Puedes comenzar solo con la pantalla
   
4. ✅ **Documentación clara** del proceso de integración
   - Ideal para aprendizaje y mantenimiento
   
5. ✅ **Reduce frustración** al añadir hardware nuevo
   - Evita el "todo a la vez y nada funciona"

6. ✅ **Compatible con el sistema completo**
   - Al final, el sistema funciona igual que el environment `esp32-s3-devkitc`

---

## 📝 Resumen de Comandos

| Acción | Comando |
|--------|---------|
| Compilar test incremental | `pio run -e esp32-s3-test-incremental` |
| Subir firmware test | `pio run -e esp32-s3-test-incremental -t upload` |
| Monitor serie | `pio device monitor -b 115200` |
| Compilar producción | `pio run -e esp32-s3-devkitc-release` |
| Limpiar build | `pio run --target clean` |

---

## 📚 Documentación Relacionada

- **[GUIA_PRUEBAS_INCREMENTALES.md](GUIA_PRUEBAS_INCREMENTALES.md)** - Guía general de pruebas incrementales (versión anterior)
- **[CHECKLIST.md](../CHECKLIST.md)** - Checklist de verificación del sistema
- **[PIN_MAPPING_DEVKITC1.md](PIN_MAPPING_DEVKITC1.md)** - Mapeo completo de pines
- **[CODIGOS_ERROR.md](CODIGOS_ERROR.md)** - Códigos de error y soluciones

---

**¿Problemas?** Abre un issue en GitHub con:
- Descripción del problema
- Paso en el que falló
- Logs del Monitor Serie completos
- Fotos de las conexiones (si es problema de hardware)

---

*Documentación creada: 2025-12-21*  
*Compatible con firmware v2.11.0+*
