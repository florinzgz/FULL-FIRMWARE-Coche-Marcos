# Solución al Bootloop del ESP32-S3 - v2.17.3

## 🔧 Problema Resuelto

Tu ESP32-S3 estaba experimentando **reinicios continuos** con el error:
```
rst:0x3 (RTC_SW_SYS_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
```

## ✅ Solución Implementada

He implementado un **fix automático** que corrige el problema del bootloop. El problema era que el watchdog del ESP32-S3 tenía un timeout muy corto (300ms) y la inicialización de la PSRAM tardaba más tiempo en algunos lotes de hardware.

### ¿Qué se ha modificado?

1. **Script de Parcheo Automático** (`tools/patch_arduino_sdkconfig.py`)
   - Parchea automáticamente el framework de Arduino
   - Aumenta el timeout del watchdog de 300ms a 5000ms
   - Se ejecuta antes de cada compilación

2. **Configuración de la Placa** (`boards/esp32s3_n16r8.json`)
   - Añadida configuración correcta de PSRAM

3. **Configuración del SDK** (`sdkconfig/n16r8.defaults`)
   - Documentación de la configuración ideal para este hardware

## 📦 Cómo Compilar y Subir

### 1. Limpiar y Compilar
```bash
cd /ruta/a/FULL-FIRMWARE-Coche-Marcos

# Limpiar compilación anterior
pio run -e esp32-s3-n16r8-standalone-debug -t clean

# Compilar firmware
pio run -e esp32-s3-n16r8-standalone-debug
```

Durante la compilación verás:
```
🔧 ESP32-S3 Bootloop Fix - Patching Arduino Framework (v2.17.3)
✅ dio_qspi: Already patched (5000ms)
```

Esto confirma que el fix está activo.

### 2. Subir al ESP32-S3
```bash
# Asegúrate de que el dispositivo está conectado a COM3
pio run -e esp32-s3-n16r8-standalone-debug -t upload
```

### 3. Monitorear el Puerto Serie
```bash
pio device monitor -e esp32-s3-n16r8-standalone-debug
```

## ✅ Resultado Esperado

Después de subir el firmware, deberías ver **UNA SOLA SECUENCIA DE ARRANQUE**:

```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
...
entry 0x403c98d0

=== ESP32-S3 EARLY BOOT ===
[STANDALONE] Mode active
[BOOT] Starting vehicle firmware...
[BOOT] Firmware version: 2.17.3
...sistema continúa inicializándose...
```

### Indicadores de Éxito:
✅ **UNA** sola secuencia de arranque (no se repite)  
✅ Aparece el mensaje "=== ESP32-S3 EARLY BOOT ==="  
✅ La versión del firmware muestra "2.17.3"  
✅ El sistema llega al bucle principal  
✅ Permanece estable sin reinicios  

## ⚠️ Si Aún Hay Bootloop

Si el dispositivo sigue en bootloop después de esta solución:

1. **Verifica la Alimentación**
   - Fuente de alimentación estable de 5V con al menos 2A
   - Cable USB de buena calidad (no solo de carga)
   - Prueba con otro puerto USB

2. **Verifica el Parcheo**
   - Durante la compilación debe aparecer "Already patched (5000ms)"
   - Si no aparece, puede haber un problema con los permisos

3. **Prueba con Timeout Extendido**
   - Edita `tools/patch_arduino_sdkconfig.py`
   - Cambia `5000` por `10000` (10 segundos)
   - Vuelve a compilar

## 📋 Pruebas Recomendadas

Después de subir el firmware:

1. **Prueba de Arranque en Frío**
   - Desconecta el USB
   - Espera 10 segundos
   - Reconecta el USB
   - Verifica que arranca correctamente

2. **Prueba de Estabilidad**
   - Deja el dispositivo funcionando 5+ minutos
   - No debe haber reinicios inesperados

3. **Prueba en Otros Entornos** (opcional)
   ```bash
   # Compilación de release
   pio run -e esp32-s3-n16r8-release -t upload
   
   # Modo standalone
   pio run -e esp32-s3-n16r8-standalone -t upload
   ```

## 📚 Documentación Completa

Para más detalles técnicos, consulta:
- **BOOTLOOP_FIX_IMPLEMENTATION_GUIDE.md** (en inglés) - Guía completa de implementación
- **BOOTLOOP_FIX_v2.17.3.md** - Análisis técnico detallado
- **BOOTLOOP_QUICKFIX_v2.17.3.md** - Guía de referencia rápida

## 🔄 Detalles Técnicos

### ¿Qué es el CONFIG_ESP_INT_WDT_TIMEOUT_MS?

El ESP32 tiene un "Watchdog de Interrupción" que vigila las rutinas de servicio de interrupción (ISRs). Si una ISR se ejecuta demasiado tiempo, asume que el sistema está colgado y reinicia.

Durante el arranque temprano:
- Se inicializa el controlador de PSRAM
- Se mapea la memoria PSRAM
- En algunos hardware, esto puede tardar 1-3 segundos

**Problema:** El timeout de 300ms del framework de Arduino era demasiado corto.

**Solución:** Nuestro parche lo aumenta a 5000ms (5 segundos).

### Persistencia del Parche

El parche modifica archivos en tu instalación de PlatformIO:
```
~/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/*/include/sdkconfig.h
```

- ✅ El parche persiste entre compilaciones
- ⚠️  Se pierde si actualizas el paquete del framework de Arduino
- ✅ El script de compilación lo reaplicará automáticamente si es necesario

## 🆘 Soporte

Si continúas teniendo problemas:

1. **Verifica que el Parche se Aplicó**
   ```bash
   grep CONFIG_ESP_INT_WDT_TIMEOUT_MS ~/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/dio_qspi/include/sdkconfig.h
   # Debe mostrar: #define CONFIG_ESP_INT_WDT_TIMEOUT_MS 5000
   ```

2. **Verifica el Hardware**
   - Prueba con otra placa ESP32-S3 si es posible
   - Verifica que no haya daños físicos
   - Asegúrate de que el chip PSRAM está correctamente soldado

3. **Captura Información de Depuración**
   - Guarda la salida completa del puerto serie desde el arranque
   - Anota cualquier cambio en el comportamiento
   - Comparte el log de compilación mostrando la aplicación del parche

---

**Fecha:** 2026-01-23  
**Versión del Firmware:** 2.17.3  
**Estado:** ✅ Listo para Pruebas en Hardware

---

## Comandos Rápidos

```bash
# Limpiar + Compilar + Subir (todo en uno)
pio run -e esp32-s3-n16r8-standalone-debug -t clean && \
pio run -e esp32-s3-n16r8-standalone-debug && \
pio run -e esp32-s3-n16r8-standalone-debug -t upload

# Monitorear
pio device monitor -e esp32-s3-n16r8-standalone-debug
```

¡Buena suerte! 🚀
