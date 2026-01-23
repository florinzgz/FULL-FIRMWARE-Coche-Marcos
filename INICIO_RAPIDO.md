# 🚀 Quick Start - ESP32-S3 Bootloop Fix

**Tu ESP32-S3 está listo para funcionar! Sigue estos pasos:**

---

## ⚡ Pasos Rápidos (5 minutos)

### 1️⃣ Compilar
```bash
pio run -e esp32-s3-n16r8-standalone-debug
```

Verás esto:
```
🔧 ESP32-S3 Bootloop Fix - Patching Arduino Framework (v2.17.3)
✅ dio_qspi: Already patched (5000ms)
```
✅ **Esto confirma que el fix está activo!**

### 2️⃣ Subir al ESP32-S3
```bash
pio run -e esp32-s3-n16r8-standalone-debug -t upload
```

### 3️⃣ Verificar
```bash
pio device monitor -e esp32-s3-n16r8-standalone-debug
```

**Deberías ver:**
```
=== ESP32-S3 EARLY BOOT ===
[BOOT] Firmware version: 2.17.3
...sistema arranca normalmente...
```

---

## ✅ Señales de Éxito

- ✅ UNA sola secuencia de arranque (no se repite)
- ✅ Aparece "=== ESP32-S3 EARLY BOOT ==="
- ✅ Versión muestra "2.17.3"
- ✅ No hay reinicios

---

## ❌ ¿Sigue en Bootloop?

### Solución Rápida 1: Alimentación
- Usa fuente de 5V @ 2A mínimo
- Cable USB de buena calidad
- Prueba otro puerto USB

### Solución Rápida 2: Verificar Parche
```bash
grep CONFIG_ESP_INT_WDT_TIMEOUT_MS ~/.platformio/packages/framework-arduinoespressif32/tools/sdk/esp32s3/dio_qspi/include/sdkconfig.h
```
Debe mostrar: `#define CONFIG_ESP_INT_WDT_TIMEOUT_MS 5000`

### Solución Rápida 3: Timeout Extendido
Si aún no funciona, edita `tools/patch_arduino_sdkconfig.py`:
- Cambia `5000` por `10000`
- Recompila

---

## 📚 Documentación Completa

- **Español:** `SOLUCION_BOOTLOOP_ESP32S3.md`
- **English:** `BOOTLOOP_FIX_IMPLEMENTATION_GUIDE.md`
- **Resumen:** `BOOTLOOP_FIX_FINAL_SUMMARY.md`

---

## 💡 ¿Qué se Arregló?

El timeout del watchdog era 300ms (demasiado corto) → ahora es 5000ms ✅

---

## 🎯 Comando Todo-en-Uno

```bash
pio run -e esp32-s3-n16r8-standalone-debug -t clean && \
pio run -e esp32-s3-n16r8-standalone-debug && \
pio run -e esp32-s3-n16r8-standalone-debug -t upload && \
pio device monitor -e esp32-s3-n16r8-standalone-debug
```

---

**¡Eso es todo! Tu ESP32-S3 ahora debería arrancar correctamente! 🎉**

Si tienes problemas, consulta la documentación completa arriba.
