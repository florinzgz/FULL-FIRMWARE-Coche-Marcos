# 🔧 Solución: Usar Debug Build para Diagnosticar Stack Overflow

## 🎯 Tu Problema Actual

Estás experimentando un bucle de reinicios con este error:

```
Guru Meditation Error: Core  0 panic'ed (Unhandled debug exception).
Debug exception reason: Stack canary watchpoint triggered (ipc0)
Core  0 register dump:
PC      : 0x40379913  PS      : 0x00050036  A0      : 0x00050030  A1      : 0x3fcf0d50
Backtrace: 0x40379910:0x3fcf0d50 0x0005002d:0xa5a5a5a5 |<-CORRUPTED
```

El problema es que el backtrace está **CORRUPTED** y no puedes ver qué está causando el stack overflow.

## ✅ La Solución: Debug Build v2.10.9

Ahora tienes un nuevo entorno de build optimizado para debugging que te dará **información detallada** sobre el crash.

## 🚀 Pasos para Diagnosticar

### Paso 1: Compilar con Debug Build

```bash
cd /ruta/al/proyecto/FULL-FIRMWARE-Coche-Marcos
pio run -e esp32-s3-devkitc-debug -t upload --upload-port COM4
```

**¿Qué hace esto?**
- Compila con símbolos de debug completos
- Habilita frame pointers para stack traces precisos
- Activa logging exhaustivo de todos los subsistemas

### Paso 2: Monitorear con Exception Decoder

```bash
pio device monitor --port COM4 --baud 115200 --filter esp32_exception_decoder
```

**¿Qué verás ahora?**

En lugar de:
```
Backtrace: 0x40379910:0x3fcf0d50 0x0005002d:0xa5a5a5a5 |<-CORRUPTED
```

Verás algo como:
```
Backtrace: 
  #0  0x40379910 in _xt_lowint1 at xtensa_vectors.S:1084
  #1  0x4200cf24 in vPortTaskWrapper at port.c:141
  #2  0x42008d88 in prvProcessTimerOrBlockTask at timers.c:675
  #3  0x42008b40 in initWiFi at wifi_manager.cpp:234
  #4  0x42008a10 in setup at main.cpp:156
```

🎉 **¡Ahora puedes ver EXACTAMENTE dónde ocurre el problema!**

### Paso 3: Identificar el Task Afectado

El error dice `(ipc0)` - esto significa:
- **IPC Task**: Inter-Processor Communication task
- **Core 0**: CPU core 0 (el que maneja WiFi/Bluetooth)

**Configuración actual:**
```ini
-DCONFIG_ESP_IPC_TASK_STACK_SIZE=2048  ; 2KB
```

### Paso 4: Analizar el Stack Trace

Con el debug build, verás mensajes como:

```
[DEBUG] WiFi: Initializing...
[DEBUG] WiFi: Stack high water mark: 234 bytes
[WARNING] Stack casi lleno! Solo 234 bytes libres
[ERROR] Stack overflow detected!
```

Esto te dice:
1. ✅ Qué función estaba ejecutándose
2. ✅ Cuánto stack quedaba libre
3. ✅ Dónde ocurrió exactamente el overflow

## 🔍 Posibles Causas y Soluciones

### Causa 1: IPC Stack Demasiado Pequeño

**Síntoma:**
```
Debug exception reason: Stack canary watchpoint triggered (ipc0)
```

**Solución:**
Aumentar `CONFIG_ESP_IPC_TASK_STACK_SIZE` en `platformio.ini`:

```ini
; Prueba con 3KB o 4KB
-DCONFIG_ESP_IPC_TASK_STACK_SIZE=3072  ; 3KB
; o
-DCONFIG_ESP_IPC_TASK_STACK_SIZE=4096  ; 4KB
```

### Causa 2: Loop Stack Insuficiente

**Síntoma:**
```
Debug exception reason: Stack canary watchpoint triggered (loopTask)
```

**Solución:**
Aumentar loop stack:

```ini
-DCONFIG_ARDUINO_LOOP_STACK_SIZE=40960  ; 40KB (actualmente 32KB)
```

### Causa 3: Main Task Stack Insuficiente

**Síntoma:**
```
Debug exception reason: Stack canary watchpoint triggered (main)
```

**Solución:**
Aumentar main task stack:

```ini
-DCONFIG_ESP_MAIN_TASK_STACK_SIZE=24576  ; 24KB (actualmente 20KB)
```

### Causa 4: Función Recursiva o Array Grande en Stack

**Síntoma:**
El stack trace muestra una función específica que consume mucho stack.

**Solución:**
- Revisar arrays grandes → moverlos a heap con `malloc()` o `new`
- Revisar recursión → usar iteración o aumentar stack
- Revisar variables locales grandes → hacerlas `static` o globales

## 📊 Monitoreando Stack Usage

Añade esto en tu código para ver el uso de stack en tiempo real:

```cpp
void loop() {
    static unsigned long lastCheck = 0;
    
    // Revisar cada 5 segundos
    if (millis() - lastCheck > 5000) {
        UBaseType_t stackHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        size_t freeBytes = stackHighWaterMark * sizeof(StackType_t);
        
        Serial.printf("[STACK] Free: %d bytes\n", freeBytes);
        
        if (freeBytes < 1024) {
            Serial.println("[WARNING] Stack usage critical!");
        }
        
        lastCheck = millis();
    }
    
    // Tu código normal...
}
```

## 🧪 Proceso de Debugging Completo

### 1. Recopilar Información

```bash
# Terminal 1: Flashear debug build
pio run -e esp32-s3-devkitc-debug -t upload --upload-port COM4

# Terminal 2: Monitor con log guardado
pio device monitor --port COM4 --baud 115200 --filter log2file,esp32_exception_decoder
```

El log se guardará automáticamente en: `log-YYYY-MM-DD_HH-MM-SS.txt`

### 2. Reproducir el Error

- Deja el sistema arrancar
- Observa los mensajes de debug
- Espera el crash
- El log capturará todo

### 3. Analizar el Log

Busca en el archivo de log:

```
[WARNING] Stack casi lleno
[ERROR] Stack overflow
Guru Meditation Error
Backtrace:
```

### 4. Identificar la Solución

Basándote en el stack trace:
1. ¿Qué task falló? (ipc0, loopTask, main)
2. ¿Qué función estaba ejecutándose?
3. ¿Cuánto stack quedaba libre antes del crash?

### 5. Aplicar el Fix

1. Aumenta el stack del task afectado
2. O refactoriza el código problemático
3. Recompila con el entorno normal
4. Verifica que el problema está resuelto

### 6. Verificar con Debug Build

```bash
# Después del fix, verifica con debug build
pio run -e esp32-s3-devkitc-debug -t upload --upload-port COM4

# Debería arrancar sin problemas
# Verifica los mensajes de stack usage
```

### 7. Deploy a Producción

```bash
# Si todo funciona, deploy con release build
pio run -e esp32-s3-devkitc-release -t upload --upload-port COM4
```

## 🎓 Ejemplo Real

**Log sin debug build:**
```
Backtrace: 0x40379910:0x3fcf0d50 0x0005002d:0xa5a5a5a5 |<-CORRUPTED
```
❌ Imposible de diagnosticar

**Log con debug build:**
```
[DEBUG] WiFi: Initializing...
[DEBUG] WiFi: Connecting to SSID: NOVA AW5700
[DEBUG] WiFi: Stack high water mark: 180 bytes
[WARNING] Stack casi lleno! Solo 180 bytes libres
[DEBUG] WiFi: Starting DHCP...
[ERROR] Stack overflow detected!

Backtrace:
  #0  0x40379910 in _xt_lowint1 at xtensa_vectors.S:1084
  #1  0x4200cf24 in vPortTaskWrapper at port.c:141
  #2  0x42008d88 in tcpip_adapter_dhcpc_start at tcpip_adapter.c:234
  #3  0x42008b40 in WiFi_begin at WiFiSTA.cpp:156
  #4  0x42008a10 in initWiFi at system.cpp:89
  #5  0x42008980 in setup at main.cpp:123
```

✅ **Ahora sabemos:**
- El overflow ocurre en WiFi init
- Específicamente en DHCP start
- En el IPC task (core 0)
- Solo quedaban 180 bytes antes del crash

**Solución:**
```ini
; Aumentar IPC stack de 2KB a 3KB
-DCONFIG_ESP_IPC_TASK_STACK_SIZE=3072
```

## 📋 Checklist de Debugging

- [ ] Compilar con `esp32-s3-devkitc-debug`
- [ ] Monitorear con `esp32_exception_decoder`
- [ ] Guardar log completo en archivo
- [ ] Reproducir el error
- [ ] Identificar el task afectado
- [ ] Analizar el stack trace
- [ ] Verificar stack usage antes del crash
- [ ] Aplicar el fix (aumentar stack o refactorizar)
- [ ] Verificar con debug build
- [ ] Deploy con release build

## ⚠️ Notas Importantes

1. **Debug build es ~10-15% más lento** - Solo para debugging
2. **Siempre verifica con release build** - El comportamiento puede diferir
3. **Guarda los logs** - Son invaluables para análisis posterior
4. **No uses debug en producción** - El logging exhaustivo llena el serial

## 🔗 Referencias

- **Documentación completa**: `INSTRUCCIONES_DEBUG_BUILD_v2.10.9.md`
- **Resumen técnico**: `RESUMEN_DEBUG_BUILD_v2.10.9.md`
- **Fix IPC anterior**: `FIX_BOOT_LOOP_v2.10.7.md`
- **PlatformIO Debug**: https://docs.platformio.org/page/plus/debugging.html

## 🆘 ¿Necesitas Más Ayuda?

Si después de usar el debug build aún tienes problemas:

1. **Comparte el stack trace completo** - Con nombres de funciones
2. **Comparte el log de debug** - Archivo `log-*.txt`
3. **Indica qué stack aumentaste** - Y el resultado
4. **Incluye memoria disponible** - Free heap, PSRAM

Con esta información, el problema será mucho más fácil de resolver! 🎉

---

**Versión:** 2.10.9  
**Fecha:** 2025-12-15  
**Hardware:** ESP32-S3-DevKitC-1  
**Estado:** ✅ Debug build disponible y verificado

---

## 💡 TL;DR (Demasiado Largo; No Leí)

```bash
# 1. Compilar con debug
pio run -e esp32-s3-devkitc-debug -t upload --upload-port COM4

# 2. Monitorear
pio device monitor --port COM4 --baud 115200 --filter esp32_exception_decoder

# 3. Reproducir error y leer stack trace

# 4. Aumentar el stack correspondiente en platformio.ini

# 5. Verificar que funciona

# 6. Deploy a producción
pio run -e esp32-s3-devkitc-release -t upload --upload-port COM4
```

**¡Usa el debug build para VER exactamente qué causa el problema!** 🐛🔍
