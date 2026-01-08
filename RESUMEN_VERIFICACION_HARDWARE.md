# Resumen de Verificación de Hardware ESP32-S3

**Fecha:** 2026-01-08  
**Tarea:** Verificación de configuración según datasheet ESP32-S3-WROOM-1/1U  
**Estado:** ✅ **VERIFICACIÓN COMPLETADA**

---

## 🎯 Hallazgos Principales

### Discrepancia de Datasheet Detectada

**Problema identificado:** El enlace proporcionado corresponde al datasheet de **ESP32-S3-WROOM-1/1U**, pero el hardware real del proyecto es **ESP32-S3-WROOM-2 N32R16V**.

### Hardware Real del Proyecto

| Componente | Especificación |
|-----------|---------------|
| **Módulo** | ESP32-S3-WROOM-2 N32R16V |
| **Placa de desarrollo** | ESP32-S3-DevKitC-1 (44 pines) |
| **Flash** | 32MB (Quad I/O mode) |
| **PSRAM** | 16MB (Octal SPI mode) |
| **Configuración SDK** | qio_opi (CORRECTO) |

---

## ⚠️ Diferencias Críticas WROOM-1 vs WROOM-2

### ESP32-S3-WROOM-1/1U
- **Flash máximo:** 16MB (típicamente Quad SPI)
- **PSRAM máximo:** 16MB (Quad u Octal SPI)
- **Configuración máxima:** N16R16V (16MB + 16MB)
- **Datasheet:** https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf

### ESP32-S3-WROOM-2/2U (HARDWARE ACTUAL)
- **Flash máximo:** 32MB (Octal SPI capable)
- **PSRAM máximo:** 16MB (Octal SPI)
- **Configuración actual:** N32R16V (32MB + 16MB) ✅
- **Datasheet CORRECTO:** https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-2_datasheet_en.pdf

**⚠️ CRÍTICO:** La configuración N32R16V (32MB Flash + 16MB PSRAM) **SOLO** está disponible en WROOM-2, **NO** en WROOM-1.

---

## ✅ Verificación de Configuración Actual

### 1. Board Configuration (boards/esp32-s3-wroom-2-n32r16v.json)

```json
{
  "name": "ESP32-S3-WROOM-2 N32R16V (32MB QIO Flash, 16MB OPI PSRAM)",
  "build": {
    "arduino": {
      "memory_type": "qio_opi"  // ✅ CORRECTO
    },
    "flash_mode": "qio",         // ✅ CORRECTO (eFuses no quemados)
    "psram_type": "opi",         // ✅ CORRECTO
    "f_flash": "80000000L",      // ✅ 80MHz
    "f_cpu": "240000000L"        // ✅ 240MHz
  },
  "upload": {
    "flash_size": "32MB"         // ✅ CORRECTO
  }
}
```

**Estado:** ✅ **CONFIGURACIÓN CORRECTA**

### 2. PlatformIO Configuration (platformio.ini)

```ini
[env:esp32-s3-n32r16v]
platform = espressif32@6.12.0
board = esp32-s3-wroom-2-n32r16v    // ✅ Board correcto
```

**Comentarios en archivo:**
```ini
; Hardware: ESP32-S3-WROOM-2 N32R16V
; Flash: 32MB QIO (OPI-capable hardware, but eFuses NOT burned)
; PSRAM: 16MB OPI
; SDK: qio_opi (correct for this hardware configuration)
```

**Estado:** ✅ **CONFIGURACIÓN CORRECTA**

### 3. PSRAM Configuration (sdkconfig.defaults)

```
CONFIG_ESP32S3_SPIRAM_SUPPORT=y
CONFIG_SPIRAM=y
CONFIG_SPIRAM_MODE_OCT=y           // ✅ Octal mode para PSRAM
CONFIG_SPIRAM_SIZE=16777216        // ✅ 16MB (16777216 bytes)
CONFIG_SPIRAM_SPEED_80M=y          // ✅ 80MHz
```

**Estado:** ✅ **CONFIGURACIÓN CORRECTA**

---

## 🔍 Explicación Técnica: ¿Por qué QIO Flash y no OPI?

### Configuración de eFuses

El hardware ESP32-S3-WROOM-2 N32R16V tiene:
- **Flash:** Chip OPI-capable (32MB), pero **eFuses NO quemados** por el fabricante
- **PSRAM:** eFuses **SÍ quemados** para modo OPI

| Memoria | Capacidad HW | eFuse Status | Modo Operacional |
|---------|--------------|--------------|------------------|
| Flash   | OPI-capable  | ❌ NO quemado | **QIO** (Quad I/O) |
| PSRAM   | OPI          | ✅ Quemado    | **OPI** (Octal)    |

**Nota:** Los eFuses son **ONE-TIME programmable** y no se pueden cambiar. Por lo tanto, el Flash DEBE usar modo QIO.

### SDK Variant Selection

PlatformIO selecciona el variant del SDK basándose en `memory_type`:
- `qio_opi` → usa SDK en `packages/.../esp32s3/qio_opi/` ✅ **CORRECTO**
- `opi_opi` → usaría SDK en `packages/.../esp32s3/opi_opi/` ❌ **CAUSARÍA BOOT CRASH**

---

## 📋 Cambios Realizados

### Nuevos Archivos
1. **HARDWARE_VERIFICATION.md**
   - Documentación completa de verificación de hardware
   - Comparación WROOM-1 vs WROOM-2
   - Referencias a datasheets oficiales
   - Instrucciones de verificación

### Archivos Actualizados
1. **README.md**
   - Clarificación del hardware: ESP32-S3-WROOM-2 N32R16V
   - Actualización de especificaciones de memoria
   - Advertencia sobre incompatibilidad con WROOM-1
   - Corrección de nombres de entornos de compilación
   - Enlace a documentación de verificación

2. **docs/REFERENCIA_HARDWARE.md**
   - Actualización de especificaciones del módulo
   - Corrección de memoria: 32MB Flash + 16MB PSRAM
   - Enlace al datasheet correcto de WROOM-2

---

## 🎓 Conclusión y Recomendaciones

### Estado Actual
✅ **La configuración del firmware es CORRECTA** para el hardware ESP32-S3-WROOM-2 N32R16V.

### Datasheets Correctos
- **Usar:** ESP32-S3-WROOM-2 Datasheet
- **NO usar:** ESP32-S3-WROOM-1 Datasheet (incompatible con este hardware)

### Recomendaciones
1. ✅ **Mantener la configuración actual** - está optimizada para el hardware
2. ✅ **Usar el datasheet de WROOM-2** para referencia técnica
3. ✅ **Verificar el hardware físico** si hay dudas sobre el módulo instalado
4. ⚠️ **NO intentar cambiar a modo OPI Flash** - los eFuses no están quemados

### Enlaces de Referencia
- **Datasheet WROOM-2:** https://www.espressif.com/sites/default/files/documentation/esp32-s3-wroom-2_datasheet_en.pdf
- **ESP32-S3 Technical Reference:** https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf
- **Verificación de Hardware:** [HARDWARE_VERIFICATION.md](HARDWARE_VERIFICATION.md)

---

## 📝 Notas Adicionales

### ¿Cómo Verificar el Módulo Físico?

1. **Revisar el boot log:**
   ```
   ESP-ROM:esp32s3-20210327
   chip revision: v0.2
   Flash: 32MB        ← Si ves 32MB, es WROOM-2
   PSRAM: 16MB
   ```

2. **Leer la etiqueta del módulo:** Debe decir "ESP32-S3-WROOM-2-N32R16V"

3. **Verificar con esptool:**
   ```bash
   esptool.py --port COM4 flash_id
   ```

### Entornos de Compilación Disponibles

```bash
# Desarrollo
pio run -e esp32-s3-n32r16v

# Producción (optimizado)
pio run -e esp32-s3-n32r16v-release

# Debug de touch
pio run -e esp32-s3-n32r16v-touch-debug

# Sin touch (diagnóstico)
pio run -e esp32-s3-n32r16v-no-touch

# Standalone display
pio run -e esp32-s3-n32r16v-standalone

# Standalone con debug
pio run -e esp32-s3-n32r16v-standalone-debug
```

---

**Verificado por:** Copilot Agent  
**Fecha de verificación:** 2026-01-08  
**Estado final:** ✅ **CONFIGURACIÓN CORRECTA Y VERIFICADA**
