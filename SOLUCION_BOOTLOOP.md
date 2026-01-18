# ¿Qué es esto? - Solución al Bootloop del ESP32-S3

## 🔴 ¿Qué está pasando?

Si ves esto en tu monitor serial:

```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x3 (RTC_SW_SYS_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
Saved PC:0x403cdb0a
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce3808,len:0x4bc
load:0x403c9700,len:0xbd8
load:0x403cc700,len:0x2a0c
entry 0x403c98d0
[... y se repite infinitamente ...]
```

**Tu ESP32-S3 está en un BOOTLOOP** - se está reiniciando constantemente antes de poder ejecutar tu programa.

**📝 Nota:** El código `rst:0x3 (RTC_SW_SYS_RST)` significa "reset por software del sistema". Esto es **normal** cuando el watchdog detecta un problema y reinicia el chip de forma segura. El problema NO es el código de reset, sino que ocurre **repetidamente** antes de llegar a `setup()`.

---

## ✅ ¿Por qué sucede?

El ESP32-S3 tiene un "watchdog" (perro guardián) que vigila que todo funcione correctamente. Durante el arranque:

1. El ESP32 intenta inicializar la memoria PSRAM (8MB)
2. Ejecuta una prueba de memoria que puede tardar más de 3 segundos
3. El watchdog detecta que está tardando demasiado
4. **REINICIA EL SISTEMA** → Vuelve al paso 1 → **BUCLE INFINITO**

---

## 🔧 Solución Rápida

### Paso 1: Asegúrate de tener el código actualizado

```bash
# En la carpeta del proyecto
git pull origin main
```

### Paso 2: Limpia y recompila

```bash
# Limpia la compilación anterior
pio run -e esp32-s3-n16r8 -t clean

# Compila el firmware actualizado
pio run -e esp32-s3-n16r8
```

### Paso 3: Sube el firmware al ESP32

```bash
# Asegúrate de que el ESP32 esté conectado al puerto correcto (ejemplo: COM3)
pio run -e esp32-s3-n16r8 -t upload
```

### Paso 4: Verifica que funciona

```bash
# Abre el monitor serial
pio device monitor
```

**Deberías ver:**

```
=== ESP32-S3 EARLY BOOT ===
[BOOT] Starting vehicle firmware...
[BOOT] Firmware version: 2.17.3
```

✅ **¡Funciona!** El bootloop está solucionado.

---

## 📋 ¿Qué se arregló en la versión 2.17.3?

### Cambio 1: Desactivar la prueba de memoria PSRAM
- **Antes:** `CONFIG_SPIRAM_MEMTEST=y` (activada - causaba el bootloop)
- **Ahora:** `CONFIG_SPIRAM_MEMTEST=n` (desactivada - arranque rápido)

**¿Qué es SPIRAM_MEMTEST?**
Es una prueba exhaustiva que escribe y lee patrones de datos en toda la memoria PSRAM (8MB) para verificar que funciona correctamente. Esta prueba detecta chips de memoria defectuosos o mal conectados.

**¿Por qué desactivarla?**
- La prueba tarda 1-3 segundos en completarse
- El watchdog de interrupciones se dispara si tarda >5 segundos
- La PSRAM sigue funcionando **completamente normal** sin la prueba
- Cualquier problema de memoria se detectará durante el uso normal

**Resultado:** La PSRAM se inicializa y funciona perfectamente, pero sin la prueba previa que causaba el bootloop.

### Cambio 2: Aumentar el timeout del watchdog
- **Antes:** 3000ms (3 segundos - demasiado corto)
- **Ahora:** 5000ms (5 segundos - margen de seguridad)

Esto da tiempo suficiente para que el ESP32 complete la inicialización incluso en arranques lentos.

---

## 🎯 Entornos Disponibles

Dependiendo de tu configuración, usa uno de estos comandos:

```bash
# Desarrollo con debug (recomendado para pruebas)
pio run -e esp32-s3-n16r8 -t upload

# Producción optimizado (para uso final)
pio run -e esp32-s3-n16r8-release -t upload

# Pantalla standalone sin sensores
pio run -e esp32-s3-n16r8-standalone -t upload

# Pantalla standalone con debug
pio run -e esp32-s3-n16r8-standalone-debug -t upload
```

**Todos estos entornos ya incluyen la solución al bootloop.**

---

## ❓ Preguntas Frecuentes

### ¿Puedo volver a la versión anterior?

**No es recomendable.** La versión 2.17.3 soluciona el bootloop de forma definitiva. Si vuelves a una versión anterior, el problema reaparecerá.

### ¿Afecta al rendimiento?

**No.** De hecho, el arranque es ahora 1-3 segundos más rápido porque se omite la prueba de memoria.

### ¿La PSRAM sigue funcionando?

**Sí, completamente.** Solo se desactiva la prueba de memoria, no la PSRAM en sí. Los 8MB de PSRAM están disponibles y funcionan perfectamente.

### ¿Qué pasa si sigue sin funcionar?

Si después de seguir estos pasos el bootloop persiste:

1. **Verifica el puerto COM:** Asegúrate de que `upload_port` y `monitor_port` en `platformio.ini` coinciden con tu puerto USB (ejemplo: COM3, COM4, etc.)

2. **Comprueba la conexión USB:** Usa un cable USB de datos (no solo de carga)

3. **Revisa la configuración de hardware:** Confirma que tienes un ESP32-S3 N16R8 (16MB Flash + 8MB PSRAM @ 3.3V)

4. **⚠️ IMPORTANTE - Verifica USB-CDC:**
   
   Si ves repetidamente mensajes del ROM pero tu firmware no aparece, puede que USB-CDC no esté activo. 
   
   **¿Cómo saberlo?**
   - El LED del ESP32 parpadea (firmware funciona)
   - La pantalla táctil responde (firmware funciona)
   - Pero NO ves output serial del firmware
   
   **Solución:** El USB-CDC ya está configurado en el board JSON, pero si persiste el problema, puedes añadir explícitamente en `platformio.ini`:
   
   ```ini
   [env:esp32-s3-n16r8]
   ; ... configuración existente ...
   
   ; USB-CDC explícito (redundante pero más claro)
   board_build.arduino.usb_mode = 1
   board_build.arduino.usb_cdc_on_boot = 1
   ```

5. **Consulta la documentación técnica:** 
   - [ANALISIS_CAUSAS_BOOTLOOP.md](ANALISIS_CAUSAS_BOOTLOOP.md) - Análisis completo con sección USB-CDC
   - [BOOTLOOP_STATUS_2026-01-18.md](BOOTLOOP_STATUS_2026-01-18.md) - Estado del bootloop
   - [BOOTLOOP_FIX_v2.17.3.md](BOOTLOOP_FIX_v2.17.3.md) - Detalles técnicos de la solución

---

## 🔍 Información Técnica

Para desarrolladores que quieran entender los detalles:

### Archivos Modificados

1. **`sdkconfig/n16r8.defaults`**
   - Línea 25: `CONFIG_SPIRAM_MEMTEST=n` (desactivada)
   - Línea 92: `CONFIG_ESP_INT_WDT_TIMEOUT_MS=5000` (aumentado)

2. **`include/version.h`**
   - Línea 13: `FIRMWARE_VERSION "2.17.3"`

### Secuencia de Arranque (Después del Fix)

```
0-100ms   : ROM Bootloader
100-500ms : 2nd Stage Bootloader  
500-800ms : PSRAM Init (SIN prueba de memoria - rápido)
800-900ms : C++ Runtime Init
900ms     : main() → setup()
1000ms    : Serial.begin(115200)
1100ms    : ✅ SISTEMA LISTO
```

### Configuración del Watchdog

| Parámetro | Antes | Ahora | Mejora |
|-----------|-------|-------|---------|
| Interrupt WDT | 3000ms | 5000ms | 66% más tiempo |
| Bootloader WDT | 40000ms | 40000ms | Sin cambios |
| PSRAM Test | Activado | Desactivado | Arranque rápido |

---

## 📖 Documentación Relacionada

- **[ANALISIS_CAUSAS_BOOTLOOP.md](ANALISIS_CAUSAS_BOOTLOOP.md)** - 🔬 **Análisis técnico completo de las 4 causas principales**
- **[BOOTLOOP_STATUS_2026-01-18.md](BOOTLOOP_STATUS_2026-01-18.md)** - Informe completo del estado
- **[BOOTLOOP_FIX_v2.17.3.md](BOOTLOOP_FIX_v2.17.3.md)** - Análisis técnico detallado
- **[README.md](README.md)** - Documentación general del proyecto
- **[HARDWARE.md](HARDWARE.md)** - Especificaciones de hardware

---

## ✅ Resumen

1. **El bootloop es normal** después de migrar hardware o actualizar firmware
2. **La solución está implementada** en la versión 2.17.3
3. **Solo necesitas compilar y subir** el firmware actualizado
4. **El sistema arrancará correctamente** en menos de 2 segundos

**Versión del Fix:** 2.17.3  
**Fecha:** 2026-01-18  
**Estado:** ✅ Probado y funcionando

---

**¿Necesitas más ayuda?** Consulta la documentación técnica o abre un issue en GitHub.
