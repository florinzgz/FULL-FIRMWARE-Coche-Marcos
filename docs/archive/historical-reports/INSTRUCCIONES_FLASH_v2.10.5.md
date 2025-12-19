# Instrucciones de Flasheo - Firmware v2.10.5

## 🎯 Versión: 2.10.5 - Fix Boot Loop Crítico

Esta versión resuelve el problema de **bucle de reinicios infinito** donde el firmware no arranca y la pantalla no se inicializa.

### ✅ Qué Arregla Esta Versión

- ✅ **Boot loop infinito** - El sistema ahora completa la inicialización sin reinicios
- ✅ **Pantalla no arranca** - El display se inicializa correctamente
- ✅ **Watchdog timeout** - Alimentación del watchdog durante todo el setup()
- ✅ **Timeout de WiFi** - El sistema puede tardar >10s en inicializar sin problemas

### 🔧 Antes de Flashear

#### 1. Verificar Puerto COM

**Windows:**
- Abrir Device Manager
- Buscar "Ports (COM & LPT)"
- Anotar el número COM del ESP32 (ej: COM4)

**Linux/Mac:**
```bash
ls /dev/tty*
# Buscar algo como /dev/ttyUSB0 o /dev/ttyACM0
```

#### 2. Actualizar platformio.ini (si es necesario)

Editar las líneas 130-131 en `platformio.ini`:
```ini
upload_port = COM4      ; Cambiar al puerto correcto
monitor_port = COM4     ; Cambiar al puerto correcto
```

#### 3. Cerrar Serial Monitor

Si tienes el serial monitor abierto, **ciérralo** antes de flashear:
```bash
# Presionar Ctrl+C en la terminal donde está corriendo el monitor
```

---

## 📦 Opción 1: Flasheo Normal (Recomendado)

### Paso 1: Limpiar Cache de Compilación

```bash
cd /ruta/al/proyecto
pio run -t clean
```

### Paso 2: Compilar y Flashear

**Entorno Base (Producción - Con todos los sensores):**
```bash
pio run -e esp32-s3-devkitc -t upload --upload-port COM4
```

**Entorno Sin Touch (Si el touch causa problemas):**
```bash
pio run -e esp32-s3-devkitc-no-touch -t upload --upload-port COM4
```

**Entorno Debug Touch (Para diagnosticar touch):**
```bash
pio run -e esp32-s3-devkitc-touch-debug -t upload --upload-port COM4
```

### Paso 3: Monitorear Serial

```bash
pio device monitor --port COM4 --baud 115200
```

---

## 🚨 Opción 2: Flasheo con Borrado Completo (Si persisten problemas)

Si el sistema sigue sin arrancar después del flasheo normal, borra toda la flash:

### Paso 1: Borrar Flash Completa

```bash
pio run -t erase
```

⚠️ **ADVERTENCIA:** Esto borra TODO, incluyendo:
- Firmware actual
- Configuración EEPROM
- Datos de calibración
- Partición NVS (WiFi credentials, etc.)

### Paso 2: Flashear Firmware Limpio

```bash
pio run -e esp32-s3-devkitc -t upload --upload-port COM4
```

### Paso 3: Recalibrar

Después de borrar la flash, necesitarás recalibrar:
1. Pedal (acelerador)
2. Encoder de dirección
3. Touch screen (si aplica)

---

## 📋 Verificación del Flasheo Exitoso

### Output Serial Esperado

Deberías ver algo como esto:

```
========================================
ESP32-S3 Car Control System v2.10.5 (Dec 14 2025 15:23:00)
========================================
CPU Freq: 240 MHz
Free heap: 290000 bytes
PSRAM: 8388608 bytes (Free: 8380000 bytes)
Stack high water mark: 30000 bytes
Configured loop stack: 32768 bytes
Configured main task stack: 20480 bytes
Boot sequence starting...
[BOOT] Enabling TFT backlight...
[BOOT] Backlight enabled on GPIO42
[BOOT] Resetting TFT display...
[BOOT] TFT reset complete
[BOOT] Debug level set to 2
[BOOT] Initializing System...
[STACK] After System::init - Free: 29500 bytes
[BOOT] Initializing Storage...
[STACK] After Storage::init - Free: 29400 bytes
[BOOT] Initializing Watchdog early...
[BOOT] Watchdog initialized and fed
[BOOT] Loading configuration from EEPROM...
[BOOT] Display brightness loaded: 200
[BOOT] Initializing Logger...
[STACK] After Logger::init - Free: 29300 bytes
[BOOT] FULL MODE: Starting hardware initialization...
[BOOT] Initializing I2C Recovery...
[STACK] After I2CRecovery::init - Free: 29200 bytes
[BOOT] Initializing WiFi Manager...
WiFi: Iniciando conexión a [TU_SSID]
WiFi: Conexión iniciada (modo no bloqueante)
[STACK] After WiFiManager::init - Free: 25000 bytes
[BOOT] Initializing Relays...
... (más inicialización) ...
[BOOT] All modules initialized. Starting self-test...
[BOOT] Self-test PASSED!
[BOOT] Setup complete! Entering main loop...
```

### ✅ Señales de Éxito

- ✅ No hay mensajes "Task watchdog timeout"
- ✅ No hay "Guru Meditation Error"
- ✅ Mensaje "[BOOT] Watchdog initialized and fed" aparece TEMPRANO
- ✅ La inicialización completa sin reinicios
- ✅ Mensaje "[BOOT] Setup complete! Entering main loop..."
- ✅ La pantalla enciende y muestra el dashboard

### ❌ Señales de Problema

Si ves estos mensajes, hay un problema:

- ❌ "Task watchdog got triggered" → Reintentar flasheo con borrado completo
- ❌ "Stack canary watchpoint triggered" → Reportar problema (no debería ocurrir)
- ❌ Sistema se reinicia antes de "[BOOT] Setup complete!" → Verificar hardware

---

## 🔧 Solución de Problemas

### Problema: "A fatal error occurred: Could not open port"

**Solución:**
1. Cerrar cualquier serial monitor abierto
2. Desconectar y reconectar el USB
3. Verificar que el puerto COM es correcto
4. En Linux: `sudo chmod 666 /dev/ttyUSB0`

### Problema: "No module named 'serial'"

**Solución:**
```bash
pip install pyserial
```

### Problema: El sistema reinicia constantemente

**Solución:**
1. Flashear con borrado completo (Opción 2)
2. Verificar alimentación USB (usar cable de datos de calidad)
3. Si persiste, probar modo standalone display:

Editar `platformio.ini` y agregar en `build_flags`:
```ini
-DSTANDALONE_DISPLAY
```

Luego flashear:
```bash
pio run -e esp32-s3-devkitc -t upload --upload-port COM4
```

### Problema: Pantalla no enciende

**Verificar:**
1. Conexión del PIN_TFT_BL (GPIO 42)
2. Conexión de alimentación 5V/3.3V
3. Cables de datos SPI (MOSI, MISO, SCLK, CS, DC, RST)

**Probar:**
```bash
# Flashear en modo debug máximo
pio run -e esp32-s3-devkitc-touch-debug -t upload --upload-port COM4
pio device monitor --port COM4 --baud 115200
```

### Problema: Touch no responde

**Solución 1: Modo sin touch**
```bash
pio run -e esp32-s3-devkitc-no-touch -t upload --upload-port COM4
```

**Solución 2: Debug touch**
```bash
pio run -e esp32-s3-devkitc-touch-debug -t upload --upload-port COM4
```

---

## 📊 Entornos Disponibles

| Entorno | Descripción | Cuándo Usar |
|---------|-------------|-------------|
| `esp32-s3-devkitc` | Producción normal | Uso diario, todos los sensores |
| `esp32-s3-devkitc-release` | Optimizado para velocidad | Máximo rendimiento |
| `esp32-s3-devkitc-no-touch` | Sin touch screen | Problemas con touch |
| `esp32-s3-devkitc-touch-debug` | Debug de touch | Diagnosticar touch |
| `esp32-s3-devkitc-predeployment` | Testing completo | Antes de producción |
| `esp32-s3-devkitc-ota` | Over-The-Air update | Actualizar vía WiFi |

---

## 🎓 Comandos Útiles

### Ver puertos disponibles
```bash
pio device list
```

### Compilar sin flashear
```bash
pio run -e esp32-s3-devkitc
```

### Solo flashear (ya compilado)
```bash
pio run -e esp32-s3-devkitc -t upload
```

### Solo monitor serial
```bash
pio device monitor --port COM4 --baud 115200
```

### Compilar, flashear y monitorear (todo en uno)
```bash
pio run -e esp32-s3-devkitc -t upload && pio device monitor --port COM4
```

### Ver información del chip
```bash
pio run -e esp32-s3-devkitc -t monitor
# Luego presionar botón RESET en el ESP32
```

---

## 📚 Documentación Adicional

- **RESUMEN_FIX_BOOT_LOOP_v2.10.5.md** - Análisis técnico completo del fix
- **RESUMEN_FIX_STACK_v2.10.3.md** - Fix de stack overflow (incluido en v2.10.5)
- **platformio.ini** - Configuración de entornos y build flags
- **MANUAL_COMPLETO_CONEXIONES.md** - Mapa de pines y conexiones

---

## 📞 Soporte

Si después de seguir estas instrucciones el sistema sigue sin funcionar:

1. **Captura el output serial completo** con:
   ```bash
   pio device monitor --port COM4 --baud 115200 > boot_log.txt
   ```

2. **Reporta el problema** incluyendo:
   - Output serial completo (boot_log.txt)
   - Versión de firmware intentada
   - Entorno usado (esp32-s3-devkitc, etc.)
   - Pasos de flasheo seguidos
   - Hardware conectado (sensores, etc.)

---

**Versión del documento:** v2.10.5  
**Fecha:** 2025-12-14  
**Autor:** Sistema de desarrollo automático  
**Estado:** ✅ Verificado y testeado
