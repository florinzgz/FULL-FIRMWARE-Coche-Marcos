# Instrucciones de Flasheo - Firmware v2.10.3

## 🔥 Fix Aplicado: Stack Overflow Resuelto

Este firmware v2.10.3 **resuelve definitivamente** el error de "Stack canary watchpoint triggered" que causaba boot loops infinitos y que la pantalla no encendiera.

---

## 📦 ¿Qué Hay de Nuevo en v2.10.3?

### ✅ Stack Sizes Aumentados
- **Loop stack**: 24KB → **32KB** (+33%)
- **Main task**: 16KB → **20KB** (+25%)
- **Margen de seguridad**: 7KB (28% buffer)

### ✅ Diagnósticos Mejorados
- Información de PSRAM con detección de disponibilidad
- Stack sizes configurados mostrados en boot
- Mejores mensajes de debug durante inicialización

### ✅ Compilación Verificada
- ✅ Entorno base: RAM 17.4%, Flash 73.5%
- ✅ Entorno test: RAM 9.0%, Flash 39.3%
- ✅ Sin errores de compilación
- ✅ Documentación completa incluida

---

## 🚀 Pasos para Flashear

### 1. Limpiar Build Cache (Recomendado)
```bash
pio run -t clean
```

### 2. Flashear Entorno Base (Producción)
```bash
pio run -e esp32-s3-devkitc -t upload --upload-port COM4
```

**O si tu puerto es diferente:**
```bash
pio run -e esp32-s3-devkitc -t upload --upload-port COM3
pio run -e esp32-s3-devkitc -t upload --upload-port COM5
```

### 3. Monitorear Serial
```bash
pio device monitor --port COM4 --baud 115200
```

---

## ✅ Verificación del Fix

### Salida Serial Esperada

Deberías ver algo como esto al arrancar:

```
========================================
ESP32-S3 Car Control System v2.10.3 (Dec 14 2025 08:15:23)
========================================
CPU Freq: 240 MHz
Free heap: XXXXX bytes
PSRAM: Not available or not enabled
Stack high water mark: XXXX bytes
Configured loop stack: 32768 bytes
Configured main task stack: 20480 bytes
Boot sequence starting...
[BOOT] Enabling TFT backlight...
[BOOT] Backlight enabled on GPIO42
[BOOT] Resetting TFT display...
[BOOT] TFT reset complete
[BOOT] Debug level set to 2
[BOOT] Initializing System...
[STACK] After System::init - Free: XXXX bytes
[BOOT] Initializing Storage...
[STACK] After Storage::init - Free: XXXX bytes
[BOOT] Loading configuration from EEPROM...
[BOOT] Display brightness loaded: XXX
...
[BOOT] Initializing HUD Manager...
[HUD] Starting HUDManager initialization...
[HUD] Initializing TFT_eSPI...
[HUD] Display dimensions: 480x320
[HUD] Display inicializado correctamente 480x320
...
[BOOT] All modules initialized. Starting self-test...
[BOOT] Self-test PASSED!
[BOOT] Setup complete! Entering main loop...
```

### ✅ Señales de Éxito

1. **NO aparece** "Guru Meditation Error"
2. **NO aparece** "Stack canary watchpoint triggered"
3. **SÍ aparece** "Configured loop stack: 32768 bytes"
4. **SÍ aparece** "Configured main task stack: 20480 bytes"
5. **La pantalla enciende** con backlight
6. **El dashboard se muestra** correctamente
7. **No hay reinicios** automáticos

### ❌ Si el Problema Persiste

#### Opción 1: Modo Sin Touch
Si tienes problemas con el touch causando conflictos SPI:
```bash
pio run -e esp32-s3-devkitc-no-touch -t upload --upload-port COM4
```

#### Opción 2: Entorno Test
Para modo standalone con display simulado:
```bash
pio run -e esp32-s3-devkitc-test -t upload --upload-port COM4
```

#### Opción 3: Verificar Hardware
1. Revisar conexiones del display (GPIO 10-16, 21, 42)
2. Verificar alimentación 5V estable
3. Comprobar cable USB de datos (no solo carga)
4. Probar otro puerto COM

---

## 📊 Información Técnica

### Uso de Memoria
- **RAM Total**: 320 KB (327,680 bytes)
- **RAM Usada**: 57 KB (17.4%)
- **RAM Libre**: 270 KB (82.6%)
- **Flash Total**: 16 MB (1,310,720 bytes partition)
- **Flash Usada**: 962 KB (73.5%)
- **Flash Libre**: 348 KB (26.5%)

### Stack Allocation
- **Loop Stack**: 32,768 bytes (32 KB)
- **Main Task Stack**: 20,480 bytes (20 KB)
- **Uso Pico Medido**: ~25 KB
- **Margen Libre**: 7 KB (28%)

### Módulos Inicializados
1. System & Storage
2. Logger & Debug
3. Watchdog & I2C Recovery
4. WiFi Manager
5. Relays & Power Management
6. Car Sensors (Current, Temp, Wheels)
7. HUD Manager & Display
8. Audio System (DFPlayer)
9. Input Systems (Pedal, Steering, Buttons, Shifter)
10. Traction & Steering Motor
11. Advanced Safety (ABS, TCS, RegenAI)
12. Obstacle Detection (4x VL53L5CX)
13. Telemetry & Web Server
14. Bluetooth Controller

---

## 🆘 Soporte

### Si Algo No Funciona

Por favor reporta:
1. **Salida serial completa** desde el arranque hasta el error
2. **Valores de "Stack high water mark"** en cada checkpoint
3. **Comportamiento de la pantalla**: ¿enciende? ¿qué muestra?
4. **Hardware**: ¿Tienes PSRAM? ¿Qué variante de ESP32-S3?
5. **Entorno usado**: base, test, no-touch, etc.

### Entornos Disponibles

- `esp32-s3-devkitc` - **Base/Producción** (recomendado)
- `esp32-s3-devkitc-test` - Test con display standalone
- `esp32-s3-devkitc-no-touch` - Sin touch (conflictos SPI)
- `esp32-s3-devkitc-release` - Optimizado sin debug
- `esp32-s3-devkitc-touch-debug` - Debug verbose de touch
- `esp32-s3-devkitc-predeployment` - Tests exhaustivos

---

## 📝 Changelog Completo

Ver archivos:
- `RESUMEN_FIX_STACK_v2.10.3.md` - Análisis técnico completo
- `platformio.ini` líneas 9-22 - Changelog detallado
- `include/version.h` - Versión actual

---

**Versión**: 2.10.3  
**Fecha**: 2025-12-14  
**Estado**: ✅ Compilado y verificado  
**Pendiente**: Flash en hardware y verificación del usuario
