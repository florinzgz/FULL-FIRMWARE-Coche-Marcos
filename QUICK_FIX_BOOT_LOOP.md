# 🚀 Quick Fix: ESP32-S3 Boot Loop (Stack Canary IPC Error)

## ⚡ Fast Solution (5 minutes)

### Síntomas del Problema
```
Guru Meditation Error: Core 0 panic'ed (Unhandled debug exception).
Debug exception reason: Stack canary watchpoint triggered (ipc0)
Backtrace: 0x40379230:0x3fcf0e30 0x0005002d:0xa5a5a5a5 |<-CORRUPTED
```
El ESP32-S3 se reinicia continuamente sin llegar a ejecutar el firmware.

### ✅ Solución (3 pasos)

#### Paso 1: Descargar el código actualizado
```bash
cd /ruta/a/FULL-FIRMWARE-Coche-Marcos
git pull origin main
```

#### Paso 2: Limpiar y compilar
```bash
pio run -t clean
pio run -e esp32-s3-devkitc
```

#### Paso 3: Flashear
```bash
# Cambia COM4 por tu puerto (COM3, /dev/ttyUSB0, etc.)
pio run -e esp32-s3-devkitc -t upload --upload-port COM4
```

### 🎯 Verificación

Abre el monitor serial:
```bash
pio device monitor --port COM4 --baud 115200
```

Deberías ver:
```
========================================
ESP32-S3 Car Control System v2.11.1
========================================
CPU Freq: 240 MHz
Configured IPC task stack: 4096 bytes    ← ¡Debe ser 4096!
[BOOT] Enabling TFT backlight...
[I2CRecovery] Initializing I2C bus...    ← I2C se inicia temprano
[I2CRecovery] I2C bus initialized
...
[BOOT] Setup complete! Entering main loop...
```

**¡Listo!** Si ves estos mensajes, el problema está resuelto. ✅

---

## 🔧 ¿Qué se Arregló?

### Cambios Técnicos

1. **IPC Stack aumentado de 3KB a 4KB**
   - Más espacio para inicialización del sistema
   - Margen de seguridad de 1KB

2. **I2C inicializado temprano**
   - Wire.begin() se llama al inicio del boot
   - Reduce presión en el stack del IPC

3. **Eliminada inicialización duplicada**
   - Wire.begin() solo se llama una vez
   - Previene conflictos

### Archivos Modificados
- `platformio.ini` - CONFIG_ESP_IPC_TASK_STACK_SIZE = 4096
- `src/core/i2c_recovery.cpp` - Wire.begin() agregado a init()
- `src/sensors/current.cpp` - Wire.begin() duplicado eliminado

---

## 🛠️ Si el Problema Persiste

### Opción 1: Limpieza Completa
```bash
rm -rf .pio
pio run -t clean
pio run -e esp32-s3-devkitc
pio run -e esp32-s3-devkitc -t upload --upload-port COM4
```

### Opción 2: Borrar Flash
```bash
# ⚠️ Esto borra todas las configuraciones guardadas
pio run -t erase
pio run -e esp32-s3-devkitc -t upload --upload-port COM4
```

### Opción 3: Verificar Configuración
```bash
# Verificar que el IPC stack está configurado correctamente
pio run -e esp32-s3-devkitc -v 2>&1 | grep CONFIG_ESP_IPC_TASK_STACK_SIZE
```
Debe mostrar: `-DCONFIG_ESP_IPC_TASK_STACK_SIZE=4096`

---

## 📋 Checklist de Éxito

Después de flashear, verifica que:

- [ ] No hay error "Stack canary watchpoint triggered"
- [ ] Aparecen mensajes `[BOOT]` en el monitor serial
- [ ] La versión mostrada es `v2.11.1` o superior
- [ ] El display se enciende con backlight
- [ ] El mensaje "IPC task stack: 4096 bytes" aparece
- [ ] El mensaje "I2C bus initialized" aparece temprano
- [ ] El sistema completa el boot y entra al loop principal

Si todos estos checks están OK, **¡el problema está resuelto!** ✅

---

## 📚 Documentación Completa

Para análisis técnico detallado, ver:
- **FIX_BOOT_LOOP_v2.11.1.md** - Análisis técnico completo
- **platformio.ini** - Configuración de build (línea 301)
- **src/core/i2c_recovery.cpp** - Inicialización temprana de I2C

---

## 📞 Soporte

Si después de seguir estos pasos el problema persiste:

1. Captura los logs completos:
   ```bash
   pio device monitor --port COM4 --baud 115200 > logs.txt
   ```
   Espera 10-15 segundos (varios reinicios) y luego detén el monitor.

2. Reporta el problema incluyendo:
   - Archivo `logs.txt` completo
   - Versión del firmware (debe ser v2.11.1)
   - Salida de `pio run -v 2>&1 | grep CONFIG_ESP_IPC_TASK_STACK_SIZE`
   - Hardware conectado (sensores, displays, etc.)

---

**Versión:** 2.11.1  
**Fecha:** 2025-12-18  
**Estado:** ✅ PROBADO Y VERIFICADO

**¡Disfruta tu firmware funcionando sin reinicios! 🎉**
