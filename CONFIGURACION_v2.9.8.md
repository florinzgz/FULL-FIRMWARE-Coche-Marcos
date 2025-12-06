# Configuración v2.9.8 - Revertido a Defaults ESP32

**Fecha**: 2025-12-06  
**Versión**: 2.9.8  
**Estado**: ✅ Configuración restaurada a v2.8.9 funcional

---

## 🔄 Cambio Realizado

Se ha **revertido** la configuración de stack a los valores por defecto de ESP32, coincidiendo con la configuración v2.8.9 que funcionaba correctamente.

### Configuración de Stack

**ANTES (v2.9.7):**
```ini
-DCONFIG_ARDUINO_LOOP_STACK_SIZE=20480  ; 20 KB
-DCONFIG_ESP_MAIN_TASK_STACK_SIZE=12288 ; 12 KB
```

**AHORA (v2.9.8):**
```ini
; Comentado - usando defaults de ESP32
; -DCONFIG_ARDUINO_LOOP_STACK_SIZE=20480
; -DCONFIG_ESP_MAIN_TASK_STACK_SIZE=12288
```

**Valores por defecto de ESP32:**
- Loop stack: **8192 bytes (8 KB)**
- Main task stack: **4096 bytes (4 KB)**

---

## 🎯 Por Qué Este Cambio

### Análisis del Problema

1. **Usuario reportó errores continuos** de stack overflow incluso después de v2.9.7
2. **SHA256 del firmware sin cambios** (`bfab1c7398593f10`) en todos los reboots
3. **Configuración v2.8.9 funcional** NO tenía configuraciones custom de stack
4. **Coincidencia importante**: v2.8.9 usaba defaults de ESP32 y funcionaba

### Posibles Causas del Problema

1. **Firmware no reflasheado**: Usuario puede no haber ejecutado upload del nuevo firmware
2. **Conflicto de configuración**: Los defines de stack personalizados pueden no aplicarse correctamente
3. **Bug en código**: Podría haber recursión infinita o aloc masiva que desborda cualquier stack

---

## 📋 Instrucciones de Uso

### 1. Compilar y Flashear (REQUERIDO)

**IMPORTANTE**: Debes hacer un rebuild completo para que los cambios se apliquen:

```bash
# Limpiar build anterior
pio run -t clean

# Compilar nuevo firmware
pio run -e esp32-s3-devkitc

# Flashear al ESP32-S3
pio run -e esp32-s3-devkitc -t upload --upload-port COM4

# Monitorizar salida
pio device monitor --port COM4
```

### 2. Verificar Que Se Flasheó Correctamente

Después del upload, el ESP32 debería arrancar con un **SHA256 diferente**. 

**ANTES (firmware antiguo):**
```
ELF file SHA256: bfab1c7398593f10
```

**DESPUÉS (firmware nuevo):**
```
ELF file SHA256: [DIFERENTE - será un hash nuevo]
```

Si el SHA256 es el mismo, el firmware **NO** se flasheó correctamente.

### 3. Si Touch Causa Problemas

Si la pantalla táctil causa conflictos en el bus SPI o no funciona, usa el entorno sin touch:

```bash
pio run -e esp32-s3-devkitc-no-touch -t upload --upload-port COM4
```

Este entorno está **restaurado desde v2.8.9** y desactiva completamente el touch.

---

## 🔧 Entornos Disponibles

### `esp32-s3-devkitc` (Por defecto)
- Touch activado con TFT_eSPI integrado
- Stack: Defaults ESP32 (8KB/4KB)
- Debug level: 5 (máximo)

### `esp32-s3-devkitc-no-touch` (Restaurado)
- Touch **desactivado** completamente
- Usa flag `-DDISABLE_TOUCH`
- Para hardware con problemas de touch

### `esp32-s3-devkitc-test`
- Modo test con todas las features
- STANDALONE_DISPLAY activado
- Test de LEDs y sensores

### `esp32-s3-devkitc-touch-debug`
- Touch debug verbose
- SPI frequency reducida a 1MHz
- Z_THRESHOLD bajado a 250

### `esp32-s3-devkitc-release`
- Producción sin debug
- Optimización -O3
- Logs desactivados

---

## ⚙️ Si Vuelve el Stack Overflow

Si después de flashear v2.9.8 vuelve el error de stack overflow:

### Opción 1: Habilitar Stack Aumentado

Edita `platformio.ini` y **descomenta** las líneas:

```ini
; Descomentar estas líneas si hay stack overflow:
-DCONFIG_ARDUINO_LOOP_STACK_SIZE=20480
-DCONFIG_ESP_MAIN_TASK_STACK_SIZE=12288
```

Luego recompila:
```bash
pio run -t clean
pio run -e esp32-s3-devkitc -t upload --upload-port COM4
```

### Opción 2: Usar Entorno Sin Touch

```bash
pio run -e esp32-s3-devkitc-no-touch -t upload --upload-port COM4
```

### Opción 3: Modo STANDALONE_DISPLAY

Para testing rápido de pantalla sin sensores:

1. Edita `platformio.ini` línea 182:
   ```ini
   -DSTANDALONE_DISPLAY    ; Descomentar esta línea
   ```

2. Recompila y flashea

---

## 📊 Comparación de Versiones

| Versión | Loop Stack | Main Task | Touch | Estado |
|---------|-----------|-----------|-------|---------|
| **v2.8.9** | 8 KB (default) | 4 KB (default) | ✅ Con XPT2046 | ✅ Funcionaba |
| **v2.9.6** | 12 KB | 8 KB | ✅ Con XPT2046 | ⚠️ Stack overflow |
| **v2.9.7** | 20 KB | 12 KB | ✅ Con XPT2046 | ❌ Persistía error |
| **v2.9.8** | 8 KB (default) | 4 KB (default) | ✅ + opción no-touch | ✅ Debería funcionar |

---

## 🐛 Debugging

### Si el error persiste después de flashear v2.9.8:

1. **Verificar SHA256 cambió** en el monitor serial
2. **Capturar backtrace completo** del crash
3. **Usar addr2line** para ver dónde ocurre el overflow:
   ```bash
   xtensa-esp32s3-elf-addr2line -e .pio/build/esp32-s3-devkitc/firmware.elf 0x40378990
   ```
4. **Revisar código** en busca de:
   - Arrays grandes en stack (usar static o heap)
   - Recursión profunda
   - Llamadas anidadas complejas durante init()

### Logs Importantes

El firmware imprime durante boot:
```
[BOOT] Enabling TFT backlight...
[BOOT] Resetting TFT display...
[BOOT] Initializing System...
```

Si crash antes de estos logs: problema en setup() temprano.  
Si crash después: problema en algún init() de módulo específico.

---

## ✅ Checklist de Verificación

- [ ] Ejecutaste `pio run -t clean`
- [ ] Compilaste con `pio run -e esp32-s3-devkitc`
- [ ] Flasheaste con `pio run -e esp32-s3-devkitc -t upload --upload-port COM4`
- [ ] Verificaste que SHA256 cambió en monitor serial
- [ ] El firmware ahora arranca sin stack overflow
- [ ] La pantalla se enciende (backlight)
- [ ] Aparece logo o dashboard

---

## 📞 Siguiente Paso

Si después de seguir todos estos pasos el error persiste:
1. Captura el **backtrace completo** del crash
2. Captura los **primeros logs** antes del crash
3. Indica qué **entorno** usaste (esp32-s3-devkitc, no-touch, etc.)
4. Reporta si el **SHA256 cambió** o sigue igual

Esto ayudará a identificar si es un problema de firmware o de configuración.

---

**Última actualización**: 2025-12-06  
**Versión**: 2.9.8  
**Configuración**: ESP32 defaults (8KB/4KB)  
**Touch**: Habilitado (con opción de deshabilitar)
