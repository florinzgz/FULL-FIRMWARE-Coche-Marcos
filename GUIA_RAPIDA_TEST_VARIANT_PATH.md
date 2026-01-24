# 🚀 GUÍA RÁPIDA: Test de Solución Bootloop variant_path

## Pasos Obligatorios (EN ORDEN)

### 1️⃣ Borrar Flash Completa
```bash
pio run -t erase
```
⏱️ Tiempo: ~30 segundos

### 2️⃣ Recompilar con Nueva Configuración
```bash
pio run -e esp32-s3-n16r8
```
⏱️ Tiempo: ~2-3 minutos

### 3️⃣ Flashear Firmware
```bash
pio run -e esp32-s3-n16r8 -t upload
```
⏱️ Tiempo: ~1 minuto

### 4️⃣ Monitor Serial
```bash
pio device monitor
```

### ✅ Salida Esperada (SI TODO ESTÁ OK)

```
rst:0x1 (POWERON_RESET),boot:0x8 (SPI_FAST_FLASH_BOOT)
...
=================================
✅ BOOT OK - Sistema Iniciado
=================================
CPU Freq: 240 MHz
PSRAM Size: 8388608 bytes
Free PSRAM: 8322872 bytes
=================================
Loop #0 - Uptime: 2045 ms
Loop #1 - Uptime: 3046 ms
```

### ❌ Si Sigue en Bootloop

Si ves:
```
rst:0x3 (RTC_SW_SYS_RST)
entry 0x403c98b8
```

**Verificar:**
1. ¿Existe `/variants/esp32s3/pins_arduino.h`?
2. ¿Board JSON tiene `"variant_path": "variants/esp32s3"`?
3. ¿Se ejecutó `pio run -t erase` antes de recompilar?

## 🔍 Diagnóstico Rápido

### Comando de Verificación Completa
```bash
# Verificar estructura
ls -la variants/esp32s3/pins_arduino.h

# Verificar board JSON
cat boards/esp32s3_n16r8.json | grep variant_path

# Limpiar todo y recompilar desde cero
pio run -t erase && \
pio run -e esp32-s3-n16r8 -t upload && \
pio device monitor
```

## 📝 Checklist Pre-Flash

- [ ] Archivo `variants/esp32s3/pins_arduino.h` existe
- [ ] Board JSON contiene `"variant_path": "variants/esp32s3"`
- [ ] Board JSON contiene `"arduino.memory_type": "qio_qspi"`
- [ ] Board JSON contiene `"protocol": "esptool"`
- [ ] Flash borrada con `pio run -t erase`
- [ ] Código recompilado completamente
- [ ] Cable USB conectado y puerto correcto

## 🎯 Test Mínimo (Opcional)

Si quieres probar con código mínimo antes del firmware completo:

```cpp
// Crear: src/test_minimal.cpp
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("✅ BOOT OK");
}

void loop() {
  Serial.printf("Uptime: %lu ms\n", millis());
  delay(1000);
}
```

Renombra `src/main.cpp` a `src/main.cpp.bak` temporalmente para probar.

---

**Tiempo Total Estimado:** ~5 minutos  
**Resultado Esperado:** Sin bootloop, sistema arranca normalmente
