# 🔧 Guía de Pruebas Incrementales de Pantalla

**Versión:** 1.0  
**Fecha:** 2025-11-27  
**Firmware Compatible:** v2.8.0+

---

## 📋 Índice

1. [Introducción](#introducción)
2. [Entornos de Firmware Disponibles](#entornos-de-firmware-disponibles)
3. [Paso 1: Probar Solo la Pantalla (Modo Standalone)](#paso-1-probar-solo-la-pantalla-modo-standalone)
4. [Paso 2: Añadir Touch (Interacción)](#paso-2-añadir-touch-interacción)
5. [Paso 3: Añadir Sensores Básicos](#paso-3-añadir-sensores-básicos)
6. [Paso 4: Añadir Sistema Completo](#paso-4-añadir-sistema-completo)
7. [Solución de Problemas](#solución-de-problemas)
8. [Conexiones Mínimas por Paso](#conexiones-mínimas-por-paso)

---

## 📖 Introducción

Esta guía te permite probar tu pantalla y añadir funcionalidades **poco a poco**, evitando problemas difíciles de diagnosticar. 

**¿Por qué probar incrementalmente?**
- ✅ Detectar problemas de hardware aisladamente
- ✅ Verificar cada conexión antes de añadir más
- ✅ Evitar sobrecarga de diagnóstico
- ✅ Aprender el sistema paso a paso

---

## 🎯 Entornos de Firmware Disponibles

Tienes **4 firmwares diferentes** preparados. Aquí te explico cuándo usar cada uno:

| Entorno | Nombre del Archivo | Cuándo Usar |
|---------|-------------------|-------------|
| `esp32-s3-devkitc` | firmware.bin | **Desarrollo** - Debug completo, todos los mensajes |
| `esp32-s3-devkitc-release` | firmware.bin | **Producción** - Sin debug, máximo rendimiento |
| `esp32-s3-devkitc-test` | firmware.bin | **Prueba pantalla** - Modo standalone activado |
| `esp32-s3-devkitc-ota` | firmware.bin | **Actualizaciones WiFi** - Para subir firmware por aire |

### 📦 Descargar los Firmwares

Los firmwares compilados están disponibles en **GitHub Actions**:

1. Ve a: https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos/actions
2. Selecciona el último workflow ✅ verde "Firmware Build & Verification"
3. Descarga los artefactos:
   - `firmware-esp32-s3-devkitc` → Desarrollo
   - `firmware-esp32-s3-devkitc-release` → Producción
   - `firmware-esp32-s3-devkitc-test` → **Prueba de pantalla**
   - `firmware-esp32-s3-devkitc-ota` → WiFi OTA

---

## 🧪 Paso 1: Probar Solo la Pantalla (Modo Standalone)

**Objetivo:** Verificar que la pantalla funciona sin conectar ningún sensor.

### Opción A: Usar el Firmware Pre-compilado

1. **Descarga** el firmware `firmware-esp32-s3-devkitc-test` de GitHub Actions
2. **Flashea** usando esptool o PlatformIO:
   ```bash
   # Con esptool
   esptool.py --port COM3 write_flash 0x0 firmware.bin
   
   # Con PlatformIO
   pio run -e esp32-s3-devkitc-test --target upload
   ```

### Opción B: Compilar Tú Mismo

1. **Edita** `platformio.ini` y descomenta esta línea en el entorno principal:
   ```ini
   ; Descomenta para modo standalone:
   -DSTANDALONE_DISPLAY
   ```

2. **Compila y sube**:
   ```bash
   pio run --target upload
   ```

### 🔌 Conexiones Mínimas (Solo Pantalla)

| Pantalla ST7796S | ESP32-S3 GPIO | Función |
|------------------|---------------|---------|
| VCC              | 3.3V          | Alimentación |
| GND              | GND           | Tierra |
| CS               | GPIO 16       | Chip Select |
| DC               | GPIO 13       | Data/Command |
| RST              | GPIO 14       | Reset |
| MOSI             | GPIO 11       | Datos SPI |
| SCK              | GPIO 10       | Reloj SPI |
| MISO             | GPIO 12       | (Opcional) |
| BL               | GPIO 42       | Backlight |

### ✅ Lo que Deberías Ver

1. **Secuencia de colores** (rojo → verde → azul) - 0.5s cada uno
2. **"ILI9488 OK"** - Texto blanco sobre fondo negro
3. **Logo de arranque** - 1.5 segundos
4. **Dashboard completo** con valores simulados:
   - Velocidad: 12 km/h
   - RPM: 850
   - Batería: 24.5V / 87%
   - 4x4: Activo

### ❌ Si Algo Falla

| Problema | Causa Probable | Solución |
|----------|---------------|----------|
| Pantalla negra | Backlight apagado | Conecta BL a 3.3V directamente |
| Pantalla blanca | Problema SPI | Verifica CS, DC, RST, MOSI, SCK |
| Colores incorrectos | Driver incorrecto | El firmware usa ST7796_DRIVER |

---

## 🖐️ Paso 2: Añadir Touch (Interacción)

**Objetivo:** Verificar que el touch funciona y responde a toques.

### Conexiones Adicionales para Touch

| XPT2046 Touch | ESP32-S3 GPIO | Función |
|---------------|---------------|---------|
| T_CS          | GPIO 21       | Touch Chip Select |
| T_IRQ         | GPIO 47       | Interrupción Touch |
| T_MOSI        | GPIO 11       | (Compartido con TFT) |
| T_MISO        | GPIO 12       | (Compartido con TFT) |
| T_CLK         | GPIO 10       | (Compartido con TFT) |

### Verificar Touch

Con el firmware de test, toca la pantalla y observa:
- El sistema detecta toques
- Los menús táctiles responden
- Puedes navegar entre pantallas

### Debug Touch

Abre el **Monitor Serie** a 115200 baud:
```bash
pio device monitor -b 115200
```

Verás mensajes como:
```
Touch detected: X=240, Y=160
```

---

## 📊 Paso 3: Añadir Sensores Básicos

Una vez verificados pantalla y touch, añade sensores **uno a uno**:

### 3.1 Sensor de Pedal (Primero)

| Sensor A1324LUA-T | ESP32-S3 GPIO | Función |
|-------------------|---------------|---------|
| VCC               | 5V            | Alimentación |
| GND               | GND           | Tierra |
| OUT               | GPIO 5        | Señal analógica |

**Compilar sin modo standalone:**
```bash
# Asegúrate de comentar -DSTANDALONE_DISPLAY en platformio.ini
pio run -e esp32-s3-devkitc --target upload
```

**Verificar:** El pedal debería mover la barra en el HUD.

### 3.2 Un Sensor de Rueda (Velocidad)

| Sensor LJ12A3 | ESP32-S3 GPIO | Función |
|---------------|---------------|---------|
| Brown         | 12-24V        | Alimentación |
| Blue          | GND           | Tierra |
| Black         | GPIO 36       | Señal (FL) |

### 3.3 Sensores I2C (Corriente y Temperatura)

**Primero inicializa I2C:**
| I2C Bus | ESP32-S3 GPIO |
|---------|---------------|
| SDA     | GPIO 8        |
| SCL     | GPIO 9        |

**Añade sensores INA226 uno a uno vía TCA9548A.**

---

## 🚗 Paso 4: Añadir Sistema Completo

Una vez que todo funcione, usa el firmware de **producción**:

```bash
pio run -e esp32-s3-devkitc-release --target upload
```

Este firmware:
- ✅ Sin mensajes de debug (más rápido)
- ✅ Optimizado para rendimiento
- ✅ Todos los sistemas habilitados:
  - ABS
  - TCS
  - Frenado regenerativo AI
  - Bluetooth
  - WiFi/OTA

---

## 🔧 Solución de Problemas

### Error: "Display not responding"

1. **Verifica alimentación:** 3.3V a la pantalla (NO 5V)
2. **Revisa SPI:**
   - CS = GPIO 16
   - DC = GPIO 13
   - RST = GPIO 14
   - MOSI = GPIO 11
   - SCK = GPIO 10
3. **Reinicia** el ESP32

### Error: "I2C timeout"

1. **Añade pull-ups:** 4.7kΩ en SDA y SCL a 3.3V
2. **Verifica direcciones I2C:** Usa escáner I2C
3. **Revisa conexiones:** SDA = GPIO 8, SCL = GPIO 9

### Error: "Watchdog reset"

El sistema se reinicia solo. Causas:
- Loop bloqueado
- Sensor I2C no responde
- Operación muy larga

**Solución:** En modo standalone no hay watchdog, úsalo para debug.

---

## 📌 Conexiones Mínimas por Paso

### Paso 1: Solo Pantalla
```
ESP32-S3 → Pantalla ST7796S (9 cables)
```

### Paso 2: Pantalla + Touch
```
ESP32-S3 → Pantalla ST7796S (9 cables)
         → Touch XPT2046 (5 cables, 3 compartidos)
```

### Paso 3: + Sensor Pedal
```
Todo lo anterior + 
ESP32-S3 GPIO 5 → Sensor de pedal A1324LUA-T
```

### Paso 4: + I2C (Sensores de Corriente)
```
Todo lo anterior +
ESP32-S3 GPIO 8 (SDA) → TCA9548A → INA226 x6
ESP32-S3 GPIO 9 (SCL) → TCA9548A → INA226 x6
```

### Paso 5: Sistema Completo
Consulta [MANUAL_COMPLETO_CONEXIONES.md](MANUAL_COMPLETO_CONEXIONES.md) para el esquema completo.

---

## 📝 Resumen de Comandos

| Acción | Comando |
|--------|---------|
| Compilar desarrollo | `pio run -e esp32-s3-devkitc` |
| Compilar producción | `pio run -e esp32-s3-devkitc-release` |
| Compilar test (standalone) | `pio run -e esp32-s3-devkitc-test` |
| Compilar OTA | `pio run -e esp32-s3-devkitc-ota` |
| Subir firmware | `pio run --target upload` |
| Monitor serie | `pio device monitor -b 115200` |
| Limpiar build | `pio run --target clean` |

---

## 🎉 ¡Listo!

Siguiendo estos pasos puedes:
1. ✅ Probar la pantalla aisladamente
2. ✅ Añadir touch y verificar interacción
3. ✅ Añadir sensores uno a uno
4. ✅ Pasar a producción cuando todo funcione

**¿Problemas?** Abre un issue en GitHub con:
- Descripción del problema
- Logs del Monitor Serie
- Fotos de las conexiones
- Paso en el que falló

---

*Documentación creada: 2025-11-27*  
*Compatible con firmware v2.8.0*
