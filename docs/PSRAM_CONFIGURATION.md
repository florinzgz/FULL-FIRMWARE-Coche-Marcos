# Configuración y Uso de PSRAM en ESP32-S3

**Fecha:** 2026-01-12  
**Hardware:** ESP32-S3 N16R8 - 16MB Flash QIO + 8MB PSRAM QSPI @ 3.3V  
**Firmware:** v2.17.1+

---

## 📋 Resumen Ejecutivo

Este documento detalla la configuración completa de PSRAM (Pseudo Static RAM) en el ESP32-S3 N16R8 y cómo el firmware la utiliza para mejorar el rendimiento y capacidad del sistema.

---

## 🔍 1. Verificación de Hardware

### Hardware Actual Detectado

- **Modelo:** ESP32-S3-WROOM-2 N16R8
- **Flash:** 16 MB (QIO mode, 4-bit, 3.3V @ 80MHz)
- **PSRAM:** 8 MB (ESPPSRAM32, AP_3v3 - 3.3V)
- **Tipo:** Quad SPI PSRAM (QSPI, 4-bit)
- **Velocidad:** 80 MHz

### Especificaciones del Módulo

El módulo ESP32-S3 N16R8 incluye:
- **Embedded PSRAM:** 8MB (ESPPSRAM32, AP_3v3)
- **Flash externa:** 16MB
- **Voltaje PSRAM:** 3.3V (AP_3v3)
- **Interface PSRAM:** Quad SPI (4 data lines)

---

## ⚙️ 2. Configuración en platformio.ini

### 2.1 Configuración de Board

```ini
[env:esp32-s3-n16r8]
platform = espressif32@6.12.0
board = esp32s3_n16r8
framework = arduino

board_build.mcu = esp32s3
board_build.f_cpu = 240000000L

; ---- MEMORIA ----
board_build.flash_size = 16MB
board_build.flash_mode = qio
board_build.arduino.memory_type = qio_qspi  # QIO Flash + QSPI PSRAM
board_build.partitions = partitions/n16r8_ota.csv
board_build.sdkconfig = sdkconfig/n16r8.defaults
```

### 2.2 SDK Configuration (sdkconfig/n16r8.defaults)

The PSRAM is configured via SDK defaults:

```
CONFIG_ESPTOOLPY_FLASHMODE_QIO=y
CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y
CONFIG_SPIRAM_MODE_QUAD=y      # Quad SPI (4-bit)
CONFIG_SPIRAM_SIZE=8388608     # 8MB
CONFIG_SPIRAM_SPEED_80M=y      # 80MHz
```

### 2.3 Explicación de Configuración

| Parameter | Propósito | Valor |
|-----------|-----------|-------|
| `memory_type` | Flash + PSRAM mode | `qio_qspi` (QIO Flash + QSPI PSRAM) |
| `SPIRAM_MODE_QUAD` | Modo de interfaz SPI | Quad (4-bit @ 3.3V) |
| `SPIRAM_SPEED_80M` | Velocidad del bus | 80MHz (óptimo) |
| `SPIRAM_SIZE` | Tamaño PSRAM | 8388608 (8MB) |

### 2.4 Particiones para 16MB Flash

```csv
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
otadata,  data, ota,     0xf000,  0x2000,
app0,     app,  ota_0,   0x10000, 0x7F0000,    # ~8MB por app
app1,     app,  ota_1,   ,        0x7F0000,    # ~8MB por app
spiffs,   data, spiffs,  ,        0x10000,     # 64KB para datos
```

---

## 💾 3. Uso de PSRAM en el Firmware

### 3.1 Asignación Automática

Cuando `CONFIG_SPIRAM_USE_MALLOC=1` está habilitado, las funciones estándar de memoria usan PSRAM automáticamente:

```cpp
// Asignaciones > 16KB van automáticamente a PSRAM
void* bigBuffer = malloc(100000);  // Usa PSRAM
void* smallBuffer = malloc(1000);  // Usa RAM interna
```

### 3.2 Asignación Explícita en PSRAM

Para forzar uso de PSRAM sin importar el tamaño:

```cpp
#include <esp_heap_caps.h>

// Asignar específicamente en PSRAM
void* psramBuffer = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);

// Verificar si la asignación fue en PSRAM
if (heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > 0) {
    Serial.println("PSRAM disponible");
}

// Liberar memoria PSRAM
heap_caps_free(psramBuffer);
```

### 3.3 Estructuras y Buffers Grandes

```cpp
// Ejemplo: Buffer para display en PSRAM
#ifdef BOARD_HAS_PSRAM
    uint16_t* frameBuffer = (uint16_t*)heap_caps_malloc(
        320 * 480 * 2,  // 320x480 pixels, 16-bit color
        MALLOC_CAP_SPIRAM
    );
#else
    // Fallback a RAM interna (fragmentación)
    uint16_t* frameBuffer = (uint16_t*)malloc(320 * 480 * 2);
#endif
```

### 3.4 Tareas FreeRTOS con Stack en PSRAM

```cpp
#include <esp_task.h>

// Crear tarea con stack en PSRAM (ESP-IDF)
TaskHandle_t taskHandle;
esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
cfg.stack_alloc_caps = MALLOC_CAP_SPIRAM;
esp_pthread_set_cfg(&cfg);

// Ahora las tareas creadas usarán PSRAM para stack
xTaskCreate(myTask, "MyTask", 8192, NULL, 5, &taskHandle);
```

---

## 📊 4. Diagnóstico de PSRAM

### 4.1 Verificación en Boot

El firmware ahora incluye diagnóstico completo de PSRAM en `System::init()`:

```
System init: === DIAGNÓSTICO DE MEMORIA ===
System init: Total Heap: 393216 bytes (384.00 KB)
System init: Free Heap: 351432 bytes (343.20 KB)
System init: ✅ PSRAM DETECTADA Y HABILITADA
System init: PSRAM Total: 16777216 bytes (16.00 MB)
System init: PSRAM Libre: 16777088 bytes (16.00 MB, 100.0%)
System init: PSRAM Usada: 128 bytes (0.12 KB, 0.0%)
System init: ✅ Tamaño de PSRAM coincide con hardware (16MB)
System init: === FIN DIAGNÓSTICO DE MEMORIA ===
```

### 4.2 API de Diagnóstico

```cpp
// Verificar si PSRAM está presente
if (psramFound()) {
    Serial.println("PSRAM detectada");
    
    // Obtener tamaño total
    uint32_t psramSize = ESP.getPsramSize();
    Serial.printf("PSRAM total: %u bytes (%.2f MB)\n", 
                  psramSize, psramSize / 1048576.0f);
    
    // Obtener PSRAM libre
    uint32_t freePsram = ESP.getFreePsram();
    Serial.printf("PSRAM libre: %u bytes (%.2f MB)\n", 
                  freePsram, freePsram / 1048576.0f);
    
    // Mayor bloque contiguo disponible
    size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
    Serial.printf("Mayor bloque PSRAM: %u bytes\n", largestBlock);
}
```

### 4.3 Herramientas de Monitoreo

```cpp
// En cualquier momento durante ejecución
void printMemoryStatus() {
    Serial.println("\n=== ESTADO DE MEMORIA ===");
    
    // RAM interna
    Serial.printf("RAM Interna Libre: %u bytes\n", ESP.getFreeHeap());
    
    // PSRAM
    if (psramFound()) {
        Serial.printf("PSRAM Libre: %u bytes (%.2f MB)\n", 
                      ESP.getFreePsram(), 
                      ESP.getFreePsram() / 1048576.0f);
    }
    
    // Por capacidades
    Serial.printf("DMA-capable: %u bytes\n", 
                  heap_caps_get_free_size(MALLOC_CAP_DMA));
    Serial.printf("32-bit access: %u bytes\n", 
                  heap_caps_get_free_size(MALLOC_CAP_32BIT));
    Serial.printf("SPIRAM: %u bytes\n", 
                  heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
}
```

---

## ✅ 5. Validación de Configuración

### 5.1 Checklist de Verificación

- [x] **platformio.ini**: `board_build.psram = enabled`
- [x] **platformio.ini**: `board_build.psram_size = 16MB` (correcto para QFN56)
- [x] **platformio.ini**: `board_build.flash_size = 32MB` (correcto para Macronix)
- [x] **Build flags**: `-DBOARD_HAS_PSRAM` presente
- [x] **Build flags**: Flags de ESP-IDF para PSRAM AP_1v8 configurados
- [x] **Build flags**: `-DCONFIG_SPIRAM_SIZE=16777216` configurado
- [x] **Código**: Diagnóstico de PSRAM en `System::init()`
- [x] **Código**: Test de memoria incluye estadísticas de PSRAM
- [x] **Hardware**: ESP32-S3 (QFN56) rev 0.2 (16MB PSRAM AP_1v8)

### 5.2 Tests de Validación

```bash
# 1. Compilar firmware
pio run -e esp32-s3-devkitc1

# 2. Flashear
pio run -e esp32-s3-devkitc1 -t upload

# 3. Monitorear salida serial
pio device monitor

# Buscar en salida:
# ✅ "PSRAM DETECTADA Y HABILITADA"
# ✅ "PSRAM Total: 16777216 bytes (16.00 MB)"
```

### 5.3 Señales de Problemas

❌ **PSRAM NO DETECTADA**: 
- Verificar hardware (debe ser QFN56 rev 0.2 con 16MB PSRAM)
- Verificar `board_build.psram = enabled`
- Recompilar completamente (`pio run -t clean`)

❌ **Tamaño incorrecto** (ej: 8MB en vez de 16MB):
- Verificar `board_build.psram_size = 16MB`
- Verificar modelo de chip (debe ser QFN56 con AP_1v8 16MB)

⚠️ **PSRAM detectada pero no usada**:
- Verificar flags `-DCONFIG_SPIRAM_USE_MALLOC=1`
- Usar `heap_caps_malloc()` explícitamente si necesario

---

## 🚀 6. Optimizaciones y Recomendaciones

### 6.1 Qué Colocar en PSRAM

✅ **Recomendado para PSRAM:**
- Frame buffers grandes (TFT, etc.)
- Buffers de audio/video
- Tablas lookup grandes
- Logs y colas de datos
- Arrays de estructuras grandes
- Cache de datos

❌ **NO recomendado para PSRAM:**
- ISR handlers (usar IRAM)
- Código crítico de tiempo
- Buffers DMA (usar MALLOC_CAP_DMA)
- Variables frecuentemente accedidas en bucles críticos

### 6.2 Configuración Óptima para Este Proyecto

```ini
; Configuración actual (óptima para este proyecto)
-DCONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=16384  # 16KB límite
-DCONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=32768 # 32KB reserva

; Esto significa:
; - Objetos < 16KB → RAM interna (rápida)
; - Objetos ≥ 16KB → PSRAM (abundante - 16MB disponibles)
; - 32KB RAM interna siempre disponible (seguridad)
```

### 6.3 Ajustes Futuros según Necesidad

Si se queda sin RAM interna:
```ini
-DCONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=8192   # Bajar límite a 8KB
```

Si necesita más RAM interna para DMA/ISR:
```ini
-DCONFIG_SPIRAM_MALLOC_RESERVE_INTERNAL=65536 # Aumentar reserva a 64KB
```

---

## 🔧 7. Solución de Problemas

### Problema 1: PSRAM no detectada

**Síntomas:**
```
System init: ❌ PSRAM NO DETECTADA
```

**Soluciones:**
1. Verificar hardware tiene PSRAM (debe ser QFN56 rev 0.2 con 16MB PSRAM embebida)
2. Limpiar build: `pio run -t clean`
3. Verificar `platformio.ini`:
   ```ini
   board_build.psram = enabled
   -DBOARD_HAS_PSRAM
   ```
4. Recompilar completamente

### Problema 2: Tamaño de PSRAM incorrecto

**Síntomas:**
```
System init: ⚠️ Tamaño de PSRAM menor al esperado: 8.00 MB < 16 MB
```

**Soluciones:**
1. Verificar modelo chip (etiqueta física - debe ser QFN56 rev 0.2)
2. Ajustar `platformio.ini`:
   ```ini
   board_build.psram_size = 16MB  # Coincidir con hardware
   -DCONFIG_SPIRAM_SIZE=16777216
   ```
3. Recompilar

### Problema 3: Memoria fragmentada

**Síntomas:**
- `malloc()` falla aunque hay memoria libre
- "Largest free block" mucho menor que memoria libre

**Soluciones:**
1. Usar PSRAM para buffers grandes:
   ```cpp
   void* buffer = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
   ```
2. Aumentar límite SPIRAM:
   ```ini
   -DCONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=8192  # Reducir de 16KB
   ```

### Problema 4: Performance issues con PSRAM

**Síntomas:**
- Sistema lento al usar PSRAM
- Latencia alta en accesos

**Soluciones:**
1. Verificar modo Octal habilitado:
   ```ini
   -DCONFIG_SPIRAM_MODE_OCT=1
   ```
2. Verificar velocidad correcta:
   ```ini
   -DCONFIG_SPIRAM_SPEED_80M=1
   ```
3. Datos críticos en RAM interna:
   ```cpp
   void* fastBuffer = heap_caps_malloc(size, MALLOC_CAP_INTERNAL);
   ```

---

## 📈 8. Métricas de Uso

### Estado Actual del Firmware

Basado en el análisis del código:

| Componente | Uso de Memoria | Ubicación Recomendada |
|------------|----------------|------------------------|
| Display frame buffer | ~300 KB | ✅ PSRAM (TFT_eSPI optimizado) |
| Audio buffers | Variable | ✅ PSRAM |
| Sensor data arrays | < 16 KB | ✅ RAM interna |
| Task stacks | 32-128 KB total | ✅ RAM interna (configurado) |
| Logger queues | < 16 KB | ✅ RAM interna |
| Obstacle detection buffers | < 16 KB | ✅ RAM interna |

### Distribución Esperada

Con la configuración actual:
- **RAM Interna:** ~300-350 KB libre después de init
- **PSRAM:** ~16 MB libre (>99% disponible)
- **Uso típico PSRAM:** 100-500 KB (display, audio, buffers grandes)

---

## 📚 9. Referencias

### Documentación Oficial

- [ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf) - Sección PSRAM
- [ESP-IDF PSRAM Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/external-ram.html)
- [Arduino ESP32 Memory Management](https://docs.espressif.com/projects/arduino-esp32/en/latest/guides/memory_management.html)

### Archivos del Proyecto

- `platformio.ini` - Configuración principal de PSRAM
- `src/core/system.cpp` - Diagnóstico de PSRAM en boot
- `src/test/memory_stress_test.cpp` - Tests y estadísticas de memoria
- `docs/REFERENCIA_HARDWARE.md` - Especificaciones de hardware

---

## ✨ 10. Conclusiones

### Estado Actual (Post-Configuración)

✅ **PSRAM Habilitada:** Sí  
✅ **Tamaño Correcto:** 16 MB (QFN56 rev 0.2)  
✅ **Flash Configurada:** 32 MB (Macronix)  
✅ **Modo Operación:** Octal 80MHz (óptimo)  
✅ **Voltaje:** 1.8V (AP_1v8)  
✅ **Uso Automático:** malloc() configurable  
✅ **Diagnóstico:** Completo en boot  
✅ **Documentación:** Actualizada  
✅ **Particiones:** Optimizadas para 32MB flash  

### Próximos Pasos Sugeridos

1. **Monitorear uso real** durante operación normal
2. **Optimizar buffers grandes** para usar PSRAM explícitamente
3. **Ajustar thresholds** si se detecta presión de memoria
4. **Considerar PSRAM para:**
   - Frame buffers dobles (smooth animations)
   - Logs históricos extensos
   - Cache de datos de sensores

### Contacto

Para preguntas sobre configuración de PSRAM o problemas:
- Revisar este documento
- Verificar salida de diagnóstico en boot
- Consultar logs con `Logger::info()` habilitado

---

**Última actualización:** 2026-01-07  
**Versión documento:** 1.0  
**Autor:** Sistema de Análisis de PSRAM
