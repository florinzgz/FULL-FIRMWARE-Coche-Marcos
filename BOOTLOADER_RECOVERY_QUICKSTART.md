# ESP32-S3 Bootloader Recovery - QUICK START

**⚡ Solución Rápida para Bootloop y Errores de Core Dump**

---

## 🚨 Síntomas

- Bootloop automático después de flashear
- Puerto COM desaparece y reaparece con otro número  
- Errores: "No core dump partition found", "Core dump flash config is corrupted"
- Watchdog: "Stack canary watchpoint triggered (ipc0)"
- Backtrace: `0xA5A5A5A5:0xA5A5A5A5`

---

## ✅ Solución en 3 Pasos

### PASO 1: Borrar Flash Completa

```bash
python -m esptool --chip esp32s3 --port COM4 erase_flash
```

**Resultado esperado:**
```
Chip erase completed successfully
```

### PASO 2: Flash Completo con Standalone

```bash
pio run -e esp32-s3-n16r8-standalone --target clean
pio run -e esp32-s3-n16r8-standalone --target upload
```

**Por qué standalone:**
- ✅ Configuración más simple
- ✅ Sin OTA, sin core dump
- ✅ Mayor probabilidad de éxito
- ✅ 10MB para firmware vs 5MB

### PASO 3: Verificar Boot

```bash
pio device monitor -b 115200
```

**Output esperado:**
```
=== ESP32-S3 Car Control System v2.17.1 ===
[BOOT] Hardware: N16R8 (16MB Flash + 8MB PSRAM)
[SYSTEM] PSRAM detected: 8388608 bytes (8.00 MB)
[TFT] Initializing display...
[MAIN] Setup complete - entering loop
```

---

## ✅ Señales de Éxito

- ✅ Sin errores de "core dump"
- ✅ PSRAM: 8MB detectados
- ✅ Puerto COM permanece estable
- ✅ Display inicializa
- ✅ Setup() completa
- ✅ Sin reinicios automáticos

---

## 🔧 Si el Problema Persiste

1. **Verificar puerto COM:**
   ```bash
   # Listar puertos disponibles
   pio device list
   ```

2. **Probar con otro cable USB:**
   - Debe ser cable de DATOS, no solo carga
   - Verificar que otros dispositivos lo reconocen

3. **Verificar drivers:**
   - CP210x para ESP32-S3-DevKitC-1
   - Descargar de: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers

4. **Consultar guía completa:**
   - Ver: `docs/ESP32_S3_BOOTLOADER_TROUBLESHOOTING.md`
   - Sección "DEBUGGING AVANZADO"

---

## 📋 Entornos Disponibles

Después de que standalone funcione, puedes usar:

| Entorno | Propósito |
|---------|-----------|
| `esp32-s3-n16r8` | Desarrollo con debug |
| `esp32-s3-n16r8-release` | Producción optimizada |
| `esp32-s3-n16r8-standalone` | **Primero usar este** ✅ |
| `esp32-s3-n16r8-standalone-debug` | Standalone con logs verbosos |

---

## 🛡️ Prevención

**Al cambiar entre entornos:**
```bash
python -m esptool --chip esp32s3 --port COM4 erase_flash
pio run -e <nuevo-entorno> --target upload
```

**Cada 2-3 semanas de desarrollo:**
```bash
# Limpiar flash periódicamente
python -m esptool --chip esp32s3 --port COM4 erase_flash
pio run -e esp32-s3-n16r8-standalone --target upload
```

---

## 📞 Documentación Completa

- **Troubleshooting completo:** `docs/ESP32_S3_BOOTLOADER_TROUBLESHOOTING.md`
- **Certificación hardware:** `PHASE14_N16R8_BOOT_CERTIFICATION.md`
- **Guía rápida:** `PHASE14_QUICK_REFERENCE.md`

---

**Última actualización:** 2026-01-12  
**Estado:** ✅ VERIFICADO - Solución garantizada siguiendo estos pasos
