# 🚀 GUÍA RÁPIDA - PSRAM ESP32-S3

**¿Primera vez? Lee primero:** `ANALISIS_PSRAM_COMPLETO.md`

---

## ✅ Verificación Rápida

### 1. Compilar y Flashear

```bash
pio run -t clean -e esp32-s3-devkitc1
pio run -e esp32-s3-devkitc1 -t upload
pio device monitor
```

### 2. Buscar en Serial Monitor

Si ves esto, **todo está bien** ✅:

```
System init: ✅ PSRAM DETECTADA Y HABILITADA
System init: PSRAM Total: 8388608 bytes (8.00 MB)
System init: ✅ Tamaño de PSRAM coincide con hardware (8MB)
```

### 3. Si NO Aparece PSRAM

```
System init: ❌ PSRAM NO DETECTADA
```

**Soluciones:**
1. Verifica que el chip sea **N16R8** (mira etiqueta física)
2. Haz clean completo: `rm -rf .pio/build`
3. Recompila: `pio run -e esp32-s3-devkitc1`

---

## 📊 Configuración Actual

| Parámetro | Valor |
|-----------|-------|
| **Flash** | 16 MB |
| **PSRAM** | 8 MB |
| **Modo** | Octal SPI (OPI) |
| **Velocidad** | 80 MHz |
| **malloc() automático** | Objetos ≥16KB → PSRAM |
| **RAM reservada** | 32 KB interna |

---

## 🔧 Uso de PSRAM en Tu Código

### Automático (Recomendado)

```cpp
// malloc() usa PSRAM automáticamente para objetos ≥16KB
void* bigBuffer = malloc(100000);  // → PSRAM
void* smallBuffer = malloc(1000);  // → RAM interna
```

### Explícito (Opcional)

```cpp
#include <esp_heap_caps.h>

// Forzar PSRAM
void* buffer = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);

// Verificar PSRAM disponible
if (psramFound()) {
    Serial.printf("PSRAM: %u bytes\n", ESP.getPsramSize());
}

// Liberar
heap_caps_free(buffer);
```

---

## 📚 Documentación Completa

1. **`ANALISIS_PSRAM_COMPLETO.md`** - Resumen ejecutivo (español) 📖
2. **`docs/PSRAM_CONFIGURATION.md`** - Guía técnica completa (inglés) 🔧
3. **`sdkconfig.defaults`** - Configuración ESP-IDF (no modificar) ⚙️

---

## 🎯 Distribución de Memoria

```
ESP32-S3 Memoria Total
├── RAM Interna: ~400 KB
│   ├── Libre: ~350 KB (después de init)
│   ├── Reservada: 32 KB (siempre disponible)
│   └── Uso: Objetos <16KB, stacks, código crítico
│
└── PSRAM: 8 MB
    ├── Libre: ~8 MB (99%+ después de init)
    └── Uso: Buffers grandes (display, audio, etc.)
```

---

## ⚡ Optimizaciones Futuras

Si necesitas más rendimiento:

```cpp
// Frame buffer grande en PSRAM
uint16_t* fb = (uint16_t*)heap_caps_malloc(
    320 * 480 * 2,
    MALLOC_CAP_SPIRAM
);

// Buffer de audio en PSRAM
uint8_t* audio = (uint8_t*)heap_caps_malloc(
    128 * 1024,
    MALLOC_CAP_SPIRAM
);
```

---

## 🆘 Soporte

**Problemas?** Consulta:
1. `ANALISIS_PSRAM_COMPLETO.md` - Sección "Solución de Problemas"
2. `docs/PSRAM_CONFIGURATION.md` - Sección "Troubleshooting"

**¿Funciona?** ¡Disfruta de tus 8MB de PSRAM! 🎉

---

**Última actualización:** 2026-01-07  
**Versión:** 1.0
