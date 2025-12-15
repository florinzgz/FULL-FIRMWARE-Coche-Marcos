# Debug Build Configuration v2.10.9

## 🎯 Propósito

Este documento explica cómo usar el nuevo entorno de debug añadido en la versión 2.10.9 para diagnosticar excepciones, stack overflows y crashes en el ESP32-S3.

## 📋 ¿Cuándo Usar el Build de Debug?

Usa el entorno `esp32-s3-devkitc-debug` cuando:

- ✅ El sistema entra en un bucle de reinicios
- ✅ Aparecen errores "Stack canary watchpoint triggered"
- ✅ Hay "Guru Meditation Error" o panics
- ✅ Necesitas stacktraces más detallados
- ✅ Quieres ver el flujo exacto de ejecución
- ✅ Necesitas depurar lógica compleja
- ✅ Quieres hacer profiling de memoria o stack usage

❌ **NO uses debug para producción** - es más lento y usa más memoria.

## 🔧 Características del Build de Debug

### Optimizaciones para Debugging

| Configuración | Valor | Propósito |
|--------------|-------|-----------|
| **Optimization** | `-Og` | Optimiza para debugging (balance entre velocidad y debug info) |
| **Debug Symbols** | `-g3 -ggdb` | Máxima información de debug incluyendo macros |
| **Frame Pointers** | `-fno-omit-frame-pointer` | Stack traces precisos |
| **Unwinding** | `-funwind-tables` | Información completa para desenrollar el stack |
| **Stack Protection** | `-fstack-protector-strong` | Detección mejorada de stack overflow |
| **No Inlining** | `-fno-inline` | Funciones no inlined = debugging más fácil |

### Logging Habilitado

- ✅ ESP32 Core debugging (`DEBUG_ESP_CORE`)
- ✅ WiFi debugging (`DEBUG_ESP_WIFI`)
- ✅ HTTP client debugging (`DEBUG_ESP_HTTP_CLIENT`)
- ✅ Nivel de log máximo (CORE_DEBUG_LEVEL=5)
- ✅ Todos los warnings visibles (`-Wall -Wextra`)

### Diferencias vs Build Normal

| Aspecto | Build Normal | Build Debug |
|---------|-------------|-------------|
| Velocidad | ⚡ Rápido | 🐌 Más lento (20-30%) |
| Tamaño binario | 📦 ~1.17 MB | 📦 ~1.4-1.6 MB (más símbolos) |
| Stack traces | ❌ Parciales | ✅ Completos con líneas |
| Logging | ⚠️ Mínimo | ✅ Exhaustivo |
| Optimizaciones | ✅ Máximas (-O3) | ⚠️ Mínimas (-Og) |
| Uso en producción | ✅ Sí | ❌ No |

## 🚀 Cómo Compilar y Flashear

### Opción 1: Compilar y Flashear en Un Paso

```bash
cd /ruta/al/proyecto/FULL-FIRMWARE-Coche-Marcos
pio run -e esp32-s3-devkitc-debug -t upload --upload-port COM4
```

### Opción 2: Compilar Primero, Luego Flashear

```bash
# Paso 1: Compilar
pio run -e esp32-s3-devkitc-debug

# Paso 2: Flashear
pio run -e esp32-s3-devkitc-debug -t upload --upload-port COM4
```

### Opción 3: Clean Build (Recomendado si hay problemas)

```bash
# Limpiar build anterior
pio run -e esp32-s3-devkitc-debug -t clean

# Compilar desde cero
pio run -e esp32-s3-devkitc-debug

# Flashear
pio run -e esp32-s3-devkitc-debug -t upload --upload-port COM4
```

### Cambiar Puerto Serial

Si tu ESP32-S3 está en un puerto diferente:

```bash
# Windows
pio run -e esp32-s3-devkitc-debug -t upload --upload-port COM5

# Linux
pio run -e esp32-s3-devkitc-debug -t upload --upload-port /dev/ttyUSB0

# macOS
pio run -e esp32-s3-devkitc-debug -t upload --upload-port /dev/cu.usbserial-*
```

## 📊 Monitorear el Debug Output

### Monitor Serial Básico

```bash
pio device monitor --port COM4 --baud 115200
```

### Monitor con Filtros ESP32

```bash
# Con filtro de excepciones ESP32
pio device monitor --port COM4 --baud 115200 --filter esp32_exception_decoder

# Con colores
pio device monitor --port COM4 --baud 115200 --filter colorize
```

### Guardar Output en Archivo

```bash
# Guardar todo el output
pio device monitor --port COM4 --baud 115200 --filter log2file

# El log se guardará en: log-YYYY-MM-DD_HH-MM-SS.txt
```

## 🔍 Interpretar el Debug Output

### Output Esperado en Boot Exitoso

```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
...
========================================
ESP32-S3 Car Control System v2.10.9 (DEBUG BUILD)
========================================
CPU Freq: 240 MHz
Free heap: XXXXX bytes
PSRAM: XXXXX bytes (Free: XXXXX bytes)
Stack high water mark: XXXXX bytes
Configured loop stack: 32768 bytes
Configured main task stack: 20480 bytes
IPC task stack: 2048 bytes
Boot sequence starting...
[DEBUG] Storage: Initializing NVS...
[DEBUG] Storage: NVS initialized successfully
[DEBUG] Display: Initializing TFT...
[DEBUG] Display: TFT initialized
[DEBUG] WiFi: Scanning networks...
...
```

### Identificar Stack Overflow

```
Guru Meditation Error: Core  0 panic'ed (Unhandled debug exception).
Debug exception reason: Stack canary watchpoint triggered (ipc0)
                                                           ^^^^^ 
                                                           Indica qué task tuvo overflow
Core  0 register dump:
PC      : 0x40379913  PS      : 0x00060036  ...
  #0  0x40379913 in _xt_lowint1 at xtensa_vectors.S:1085
```

**Posibles valores:**
- `(ipc0)` - IPC task core 0 overflow → Necesitas aumentar `CONFIG_ESP_IPC_TASK_STACK_SIZE`
- `(loopTask)` - Arduino loop overflow → Necesitas aumentar `CONFIG_ARDUINO_LOOP_STACK_SIZE`
- `(main)` - Main task overflow → Necesitas aumentar `CONFIG_ESP_MAIN_TASK_STACK_SIZE`

### Stacktrace Mejorado con Debug Build

**Build Normal (sin debug):**
```
Backtrace: 0x40379910:0x3fcf0d50 0x0005002d:0xa5a5a5a5 |<-CORRUPTED
```

**Build Debug:**
```
Backtrace: 
  #0  0x40379910 in _xt_lowint1 at xtensa_vectors.S:1084
  #1  0x4200cf24 in vPortTaskWrapper at port.c:141
  #2  0x42008d88 in prvProcessTimerOrBlockTask at timers.c:675
  #3  0x42008b40 in xTimerCreateTimerTask at timers.c:598
  #4  0x4037a1d0 in esp_startup_start_app_other_cores at startup.c:234
```

Mucho más útil con nombres de funciones y números de línea! 🎉

## 🐛 Debugging Avanzado

### Usar GDB para Debugging Interactivo

```bash
# Iniciar debug session con GDB
pio debug -e esp32-s3-devkitc-debug

# Una vez en GDB:
(gdb) break setup              # Breakpoint en setup()
(gdb) continue                 # Continuar ejecución
(gdb) bt                       # Ver backtrace completo
(gdb) print variable_name      # Inspeccionar variables
(gdb) info locals              # Ver todas las variables locales
```

### Comandos Útiles de GDB

```gdb
# Navegación
step          # Ejecutar siguiente línea (entra en funciones)
next          # Ejecutar siguiente línea (salta funciones)
finish        # Terminar función actual
continue      # Continuar hasta siguiente breakpoint

# Inspección
print var     # Ver valor de variable
info locals   # Ver todas las variables locales
bt            # Backtrace completo
frame N       # Cambiar a frame N del stack

# Breakpoints
break file.cpp:123    # Breakpoint en línea específica
break function_name   # Breakpoint en función
delete N              # Eliminar breakpoint N
info breakpoints      # Listar todos los breakpoints
```

### Analizar Core Dumps

Si el sistema crashea, puedes analizar el core dump:

```bash
# Decodificar backtrace
pio device monitor --port COM4 --filter esp32_exception_decoder

# O usar el script decode_backtrace.sh
./decode_backtrace.sh .pio/build/esp32-s3-devkitc-debug/firmware.elf
```

## 📈 Profiling de Stack Usage

### Verificar Stack High Water Mark

Añade esto en tu código (ya incluido en la versión 2.10.9):

```cpp
void loop() {
    // Monitorear stack usage
    UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    if (stackHighWaterMark < 1024) {  // Menos de 1KB libre
        Serial.printf("[WARNING] Stack casi lleno! Solo %d bytes libres\n", 
                      stackHighWaterMark * sizeof(StackType_t));
    }
    
    // Tu código...
}
```

### Ver Uso de Memoria

```cpp
void printMemoryStats() {
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Min free heap: %d bytes\n", ESP.getMinFreeHeap());
    Serial.printf("Max alloc heap: %d bytes\n", ESP.getMaxAllocHeap());
    
    if (psramFound()) {
        Serial.printf("PSRAM total: %d bytes\n", ESP.getPsramSize());
        Serial.printf("PSRAM free: %d bytes\n", ESP.getFreePsram());
    }
}
```

## ⚠️ Limitaciones del Debug Build

1. **Rendimiento**: ~20-30% más lento que build optimizado
2. **Tamaño**: Binario más grande por símbolos de debug
3. **Memoria**: Puede usar más stack/heap por código no optimizado
4. **Producción**: NUNCA usar debug build en producción

## 🔄 Volver a Build de Producción

Cuando hayas terminado el debugging:

```bash
# Compilar versión optimizada
pio run -e esp32-s3-devkitc-release

# Flashear versión optimizada
pio run -e esp32-s3-devkitc-release -t upload --upload-port COM4
```

## 📚 Entornos Disponibles

| Entorno | Propósito | Cuándo Usar |
|---------|-----------|-------------|
| `esp32-s3-devkitc` | Base/desarrollo | Desarrollo general |
| `esp32-s3-devkitc-debug` | ⭐ Debug intensivo | Crashes, excepciones, profiling |
| `esp32-s3-devkitc-release` | Producción | Deploy final optimizado |
| `esp32-s3-devkitc-touch-debug` | Debug touch | Problemas con touchscreen |
| `esp32-s3-devkitc-predeployment` | Testing | Tests antes de producción |
| `esp32-s3-devkitc-no-touch` | Sin touch | Hardware sin touchscreen |
| `esp32-s3-devkitc-ota` | OTA updates | Actualizaciones remotas |

## 🆘 Resolución de Problemas

### Error: "Cannot open COM port"

```bash
# Verificar puerto disponible
pio device list

# Usar el puerto correcto
pio run -e esp32-s3-devkitc-debug -t upload --upload-port COMX
```

### Error: "Flash size not supported"

```bash
# Borrar flash completa
pio run -t erase

# Compilar y flashear
pio run -e esp32-s3-devkitc-debug -t upload
```

### Build Falla por Falta de Espacio

```bash
# Limpiar builds antiguos
pio run -t clean

# Limpiar TODOS los builds
rm -rf .pio/build
```

### El Debugging es Muy Lento

Esto es normal con `-Og` y `-fno-inline`. Si necesitas más velocidad pero mantienes symbols:

```ini
; En platformio.ini, modificar build_flags del debug environment:
-O2                    ; Más optimización
-g                     ; Menos debug info que -g3
; Comentar: -fno-inline
```

## 💡 Tips y Mejores Prácticas

1. **Siempre usa debug build al diagnosticar crashes** - Los stacktraces son infinitamente más útiles
2. **Guarda el output serial** - Usa `--filter log2file` para tener registro completo
3. **Compara con build normal** - A veces el problema solo aparece con optimizaciones
4. **No asumas que debug y release se comportan igual** - Siempre testea ambos
5. **Usa watchdog para detectar hangs** - El debug build puede ser más lento
6. **Monitor de stack high water marks** - Previene crashes antes de que ocurran

## 📖 Referencias

- [PlatformIO Build Configurations](https://docs.platformio.org/page/projectconf/build_configurations.html)
- [ESP32-S3 Debugging Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-guides/jtag-debugging/)
- [GDB Quick Reference](https://sourceware.org/gdb/current/onlinedocs/gdb/)

---

**Versión:** 2.10.9  
**Fecha:** 2025-12-15  
**Hardware:** ESP32-S3-DevKitC-1 (44 pines)  
**Estado:** ✅ Verificado - Build exitoso

---

## 🎯 Resumen Rápido

```bash
# Para diagnosticar crashes/exceptions:
pio run -e esp32-s3-devkitc-debug -t upload --upload-port COM4
pio device monitor --port COM4 --baud 115200 --filter esp32_exception_decoder

# Para GDB interactivo:
pio debug -e esp32-s3-devkitc-debug

# Para volver a producción:
pio run -e esp32-s3-devkitc-release -t upload --upload-port COM4
```

**¡Usa el debug build siempre que necesites investigar crashes! 🐛🔍**
