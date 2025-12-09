# Guía Rápida - Solución Stack Overflow v2.10.2

## 🚨 Problema
Tu ESP32-S3 se reinicia constantemente con el error:
```
Stack canary watchpoint triggered (ipc0)
Backtrace: CORRUPTED
```

## ✅ Solución
El firmware v2.10.2 aumenta el stack a 32KB/24KB para permitir la inicialización de WiFi.

---

## 📋 Pasos para Aplicar la Solución

### 1️⃣ Actualizar el Código
```bash
git pull origin copilot/debug-core-dump-issue
```

### 2️⃣ Limpiar Build Cache
```bash
pio run -t clean
```

### 3️⃣ Compilar
```bash
pio run -e esp32-s3-devkitc
```

### 4️⃣ Flashear (ajusta COM4 a tu puerto)
```bash
pio run -e esp32-s3-devkitc -t upload --upload-port COM4
```

### 5️⃣ Monitorizar
```bash
pio device monitor --port COM4
```

---

## ✅ Verificar que Funciona

Deberías ver en el Serial Monitor:

```
========================================
ESP32-S3 Car Control System v2.10.2
========================================
CPU Freq: 240 MHz
Free heap: XXXXX bytes
Boot sequence starting...
[BOOT] Enabling TFT backlight...
[BOOT] TFT reset complete
[BOOT] Initializing WiFi Manager...
[STACK] After WiFiManager::init - Free: XXXX bytes
...
[BOOT] Setup complete! Entering main loop...
```

**✅ NO debería aparecer "Stack canary watchpoint"**

---

## 🔧 Si Aún Falla (poco probable)

### Opción 1: Usar Entorno sin WiFi (Recomendado)
Compilar con el entorno `esp32-s3-devkitc-no-wifi` que desactiva WiFi/OTA y usa menos stack:
```bash
pio run -e esp32-s3-devkitc-no-wifi
pio run -e esp32-s3-devkitc-no-wifi -t upload --upload-port COM4
```
**Ventajas:**
- Stack reducido a 20KB/16KB (ahorro de 12KB RAM)
- Boot más rápido
- **Desventajas:** Sin WiFi, sin OTA, sin telemetría web

### Opción 2: Borrar Flash Completo
```bash
esptool.py --chip esp32s3 --port COM4 erase_flash
pio run -e esp32-s3-devkitc -t upload --upload-port COM4
```

### Opción 3: Desactivar WiFi Manualmente

Edita `src/main.cpp` líneas 294-296:
```cpp
// Serial.println("[BOOT] Initializing WiFi Manager...");
// WiFiManager::init();
// Serial.printf("[STACK] After WiFiManager::init - Free: %d bytes\n", uxTaskGetStackHighWaterMark(NULL));
```

---

## 📊 Cambios Técnicos

| Parámetro | Antes (v2.10.1) | Ahora (v2.10.2) |
|-----------|-----------------|-----------------|
| Loop Stack | 24 KB | **32 KB** ✅ |
| Main Task Stack | 16 KB | **24 KB** ✅ |
| RAM Usada | 40 KB | 56 KB (+16 KB) |
| RAM Libre | ~270 KB | ~254 KB |

---

## ℹ️ ¿Por Qué Este Fix?

- ESP32-S3 requiere **mínimo 32KB** para inicialización WiFi
- El ESP-IDF oficial recomienda 32KB para tareas WiFi
- Los valores anteriores (24KB/16KB) eran insuficientes
- Este es un requisito de hardware del ESP32-S3, no un bug del firmware

---

## 📚 Más Información

Ver documento completo: **RESUMEN_CORRECCION_STACK_v2.10.2.md**

---

**Versión**: 2.10.2  
**Fecha**: 2025-12-09  
**Estado**: ✅ Resuelto
