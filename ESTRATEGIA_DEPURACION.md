# Estrategia de Depuración - Firmware v2.10.8+

## 🎯 Objetivo

Esta guía proporciona una estrategia sistemática para identificar y resolver problemas de bucle de reinicio (boot loop) en el firmware del ESP32-S3.

---

## 📋 Checklist Inicial de Verificación

Antes de comenzar la depuración, verifica:

- [ ] **Firmware actualizado** a v2.10.8 o superior
- [ ] **CONFIG_ESP_IPC_TASK_STACK_SIZE=2048** presente en TODOS los entornos
- [ ] **Watchdog alimentado** durante inicialización
- [ ] **Stack sizes** adecuados en platformio.ini
- [ ] **Hardware conectado** correctamente (especialmente I2C)
- [ ] **Alimentación estable** (mínimo 2A para ESP32-S3)

---

## 1️⃣ Identificar el Entorno Fallido

### Paso 1: Probar Cada Entorno Sistemáticamente

```bash
# Entorno base (desarrollo)
pio run -e esp32-s3-devkitc -t upload --upload-port COM4

# Entorno release (producción)
pio run -e esp32-s3-devkitc-release -t upload --upload-port COM4

# Entorno OTA
pio run -e esp32-s3-devkitc-ota -t upload --upload-port COM4

# Entorno touch-debug
pio run -e esp32-s3-devkitc-touch-debug -t upload --upload-port COM4

# Entorno predeployment (testing)
pio run -e esp32-s3-devkitc-predeployment -t upload --upload-port COM4

# Entorno no-touch
pio run -e esp32-s3-devkitc-no-touch -t upload --upload-port COM4
```

### Paso 2: Verificar Configuración IPC en Cada Entorno

```bash
# Para cada entorno, verifica que tenga CONFIG_ESP_IPC_TASK_STACK_SIZE
pio run -e esp32-s3-devkitc -v 2>&1 | grep CONFIG_ESP_IPC_TASK_STACK_SIZE
pio run -e esp32-s3-devkitc-release -v 2>&1 | grep CONFIG_ESP_IPC_TASK_STACK_SIZE
pio run -e esp32-s3-devkitc-ota -v 2>&1 | grep CONFIG_ESP_IPC_TASK_STACK_SIZE
pio run -e esp32-s3-devkitc-touch-debug -v 2>&1 | grep CONFIG_ESP_IPC_TASK_STACK_SIZE
pio run -e esp32-s3-devkitc-predeployment -v 2>&1 | grep CONFIG_ESP_IPC_TASK_STACK_SIZE
pio run -e esp32-s3-devkitc-no-touch -v 2>&1 | grep CONFIG_ESP_IPC_TASK_STACK_SIZE
```

**Resultado esperado:** Todos deben mostrar `-DCONFIG_ESP_IPC_TASK_STACK_SIZE=2048`

### Paso 3: Registrar el Comportamiento

| Entorno | ¿Arranca? | Error Principal | Stack IPC OK | Notas |
|---------|-----------|-----------------|--------------|-------|
| esp32-s3-devkitc | ⬜ | | ⬜ | |
| esp32-s3-devkitc-release | ⬜ | | ⬜ | |
| esp32-s3-devkitc-ota | ⬜ | | ⬜ | |
| esp32-s3-devkitc-touch-debug | ⬜ | | ⬜ | |
| esp32-s3-devkitc-predeployment | ⬜ | | ⬜ | |
| esp32-s3-devkitc-no-touch | ⬜ | | ⬜ | |

---

## 2️⃣ Decodificar el Backtrace

### Capturar el Error Completo

```bash
pio device monitor --port COM4 --baud 115200 > error_log.txt
```

Deja que capture varios reinicios (15-30 segundos) y luego detén el monitor.

### Tipos de Errores Comunes

#### A. Stack Canary Watchpoint (IPC)

```
Guru Meditation Error: Core 0 panic'ed (Unhandled debug exception).
Debug exception reason: Stack canary watchpoint triggered (ipc0)
Backtrace: 0x40379990:0x3fcf0d50 0x0005002d:0xa5a5a5a5 |<-CORRUPTED
```

**Causa:** Stack IPC insuficiente (1KB)  
**Solución:** Agregar `-DCONFIG_ESP_IPC_TASK_STACK_SIZE=2048`  
**Documentación:** Ver `FIX_BOOT_LOOP_v2.10.7.md`

#### B. Task Watchdog Timeout

```
E (10234) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:
E (10234) task_wdt:  - IDLE (CPU 0)
```

**Causa:** Inicialización tarda >10s sin alimentar watchdog  
**Solución:** Agregar `Watchdog::feed()` durante setup()  
**Documentación:** Ver `SOLUCION_BUCLE_BOOT_v2.10.5.md`

#### C. Stack Overflow en Loop/Main Task

```
***ERROR*** A stack overflow in task loopTask has been detected.
Backtrace: 0x400d1234:0x3ffb5678
```

**Causa:** Stack de Arduino loop o main task insuficiente  
**Solución:** Aumentar `CONFIG_ARDUINO_LOOP_STACK_SIZE` y `CONFIG_ESP_MAIN_TASK_STACK_SIZE`  
**Documentación:** Ver `RESUMEN_CORRECCION_STACK_v2.9.6.md`

### Decodificar Direcciones de Memoria

#### Método 1: Usando PlatformIO

```bash
# Reemplaza las direcciones con las de tu backtrace
pio device monitor --port COM4 --filter esp32_exception_decoder
```

Esto decodifica automáticamente los backtraces mientras monitoreas.

#### Método 2: Manual con addr2line

```bash
# 1. Localizar el archivo .elf
find .pio -name "*.elf"

# 2. Decodificar dirección específica
xtensa-esp32s3-elf-addr2line -e .pio/build/esp32-s3-devkitc/firmware.elf 0x40379990

# 3. Obtener función y línea
xtensa-esp32s3-elf-addr2line -e .pio/build/esp32-s3-devkitc/firmware.elf -f 0x40379990
```

**Nota:** Necesitas el toolchain de ESP32 instalado.

#### Método 3: Backtrace Completo

Si tienes múltiples direcciones:

```bash
# Crear archivo con direcciones (una por línea)
echo "0x40379990
0x40379abc
0x40379def" > addresses.txt

# Decodificar todas
while read addr; do
  echo "Dirección: $addr"
  xtensa-esp32s3-elf-addr2line -e .pio/build/esp32-s3-devkitc/firmware.elf -f $addr
  echo "---"
done < addresses.txt
```

---

## 3️⃣ Revisar Inicialización Temprana

### system.cpp

```cpp
// ✅ Verificar que System::init() es simple y no falla
void System::init() {
    Logger::info("System init: entrando en PRECHECK");
    currentState = PRECHECK;
    // No debe hacer operaciones pesadas aquí
}
```

**Problemas potenciales:**
- ❌ Operaciones I2C/SPI bloqueantes
- ❌ Llamadas a hardware no inicializado
- ❌ Allocaciones grandes de memoria

**Solución:** Mover lógica pesada a después de todos los inits.

### logger.cpp

```cpp
// ✅ Verificar que Logger::init() es robusto
void Logger::init() {
    Serial.begin(115200);
    serialReady = true;
    // Debe ser tolerante si Serial no está listo
    if(!Serial) {
        warn("Serial no reporta disponibilidad inmediata");
    }
}
```

**Problemas potenciales:**
- ❌ Espera bloqueante por Serial (>1s)
- ❌ Writes sin verificar serialReady

**Solución:** Usar timeouts y verificaciones null-safe.

### storage.cpp

```cpp
// ✅ Verificar que Storage::init() maneja fallos
void Storage::init() {
  if (!prefs.begin(kNamespace, false)) {
    Logger::warn("Storage init: fallo al abrir namespace");
    System::logError(970);
    // CRÍTICO: No debe hacer abort() aquí
  }
}
```

**Problemas potenciales:**
- ❌ Crash si EEPROM corrupta
- ❌ Operaciones bloqueantes largas

**Solución:** Tolerancia a fallos, usar defaults si falla.

### Secuencia de Inicialización en main.cpp

```cpp
void setup() {
  // ✅ ORDEN CORRECTO:
  
  // 1. Serial PRIMERO (para debugging)
  Serial.begin(115200);
  
  // 2. Backlight TEMPRANO (feedback visual)
  digitalWrite(PIN_TFT_BL, HIGH);
  
  // 3. Inicialización básica
  System::init();          // ← Debe ser rápido
  Storage::init();         // ← Debe tolerar fallos
  
  // 4. Watchdog TEMPRANO
  Watchdog::init();
  Watchdog::feed();        // ← CRÍTICO
  
  // 5. Logger
  Logger::init();
  Watchdog::feed();        // ← Después de cada paso importante
  
  // 6. Resto de inicialización con Watchdog::feed() regular
}
```

**Verificar en logs:**

```
[BOOT] ESP32-S3 Car Control System v2.10.7
[BOOT] Enabling TFT backlight...          ← Debe aparecer TEMPRANO
[BOOT] Backlight enabled on GPIO42
[BOOT] Initializing System...
[STACK] After System::init - Free: XXXX bytes
[BOOT] Initializing Storage...
[STACK] After Storage::init - Free: XXXX bytes
[BOOT] Initializing Watchdog early...     ← CRÍTICO
[BOOT] Watchdog initialized and fed
```

Si NO ves estos mensajes → crash durante inicialización básica.

---

## 4️⃣ Auditar Tareas Propias

### Encontrar Tareas Personalizadas

```bash
# Buscar todas las creaciones de tareas
grep -r "xTaskCreate\|xTaskCreatePinnedToCore" src/ --include="*.cpp"
```

### Monitorear Stack Watermark

#### Agregar Monitoreo Periódico

Añadir en `main.cpp` loop():

```cpp
void loop() {
  static unsigned long lastStackCheck = 0;
  
  // Verificar stack cada 10 segundos
  if (millis() - lastStackCheck > 10000) {
    lastStackCheck = millis();
    
    UBaseType_t stackFree = uxTaskGetStackHighWaterMark(NULL);
    Logger::infof("Loop task - Stack libre: %u bytes", stackFree);
    
    // ⚠️ ADVERTENCIA si stack bajo
    if (stackFree < 1024) {  // Menos de 1KB libre
      Logger::warnf("⚠️ STACK BAJO: Solo %u bytes libres!", stackFree);
    }
    
    // 🚨 CRÍTICO si muy bajo
    if (stackFree < 512) {  // Menos de 512 bytes
      Logger::errorf("🚨 STACK CRÍTICO: %u bytes! Posible overflow inminente", stackFree);
    }
  }
  
  // ... resto del código loop() ...
}
```

#### Verificar Stack de Tareas Específicas

Si tienes tareas propias:

```cpp
TaskHandle_t myTaskHandle;

// En la función de la tarea
void myTaskFunction(void* param) {
  while(1) {
    // Tu código aquí
    
    // Verificar stack periódicamente
    UBaseType_t stackFree = uxTaskGetStackHighWaterMark(NULL);
    if (stackFree < 512) {
      Logger::errorf("Tarea myTask - Stack crítico: %u bytes", stackFree);
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Al crear la tarea
xTaskCreatePinnedToCore(
    myTaskFunction,
    "myTask",
    4096,  // ← Stack size en BYTES (4KB)
    NULL,
    1,
    &myTaskHandle,
    1  // Core 1 (App CPU)
);
```

### Cómo Interpretar Stack Watermark

| Stack Libre | Estado | Acción |
|-------------|--------|--------|
| >2048 bytes | ✅ Excelente | No hacer nada |
| 1024-2048 bytes | ⚠️ Aceptable | Monitorear |
| 512-1024 bytes | ⚠️ Bajo | Aumentar stack size |
| <512 bytes | 🚨 CRÍTICO | Aumentar URGENTE |

### Aumentar Stack Size

#### Para Arduino Loop Task

En `platformio.ini`:

```ini
build_flags =
    ${env:esp32-s3-devkitc.build_flags}
    -DCONFIG_ARDUINO_LOOP_STACK_SIZE=32768  ; 32KB (era 8KB default)
```

#### Para Main Task

```ini
build_flags =
    ${env:esp32-s3-devkitc.build_flags}
    -DCONFIG_ESP_MAIN_TASK_STACK_SIZE=20480  ; 20KB (era 4KB default)
```

#### Para Tareas Propias

Al crear la tarea, aumentar el parámetro de stack:

```cpp
// ANTES: 4KB
xTaskCreate(myFunction, "myTask", 4096, NULL, 1, &handle);

// DESPUÉS: 8KB
xTaskCreate(myFunction, "myTask", 8192, NULL, 1, &handle);
```

---

## 5️⃣ Verificar platformio.ini

### Script de Verificación Automática

Crear archivo `verify_platformio.sh`:

```bash
#!/bin/bash

echo "======================================"
echo "Verificación de platformio.ini"
echo "======================================"
echo ""

# Colores
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Lista de entornos
ENVIRONMENTS=(
    "esp32-s3-devkitc"
    "esp32-s3-devkitc-release"
    "esp32-s3-devkitc-ota"
    "esp32-s3-devkitc-touch-debug"
    "esp32-s3-devkitc-predeployment"
    "esp32-s3-devkitc-no-touch"
)

# Verificar cada entorno
for env in "${ENVIRONMENTS[@]}"; do
    echo -n "Verificando [$env]... "
    
    # Compilar y buscar CONFIG_ESP_IPC_TASK_STACK_SIZE
    result=$(pio run -e $env -v 2>&1 | grep "CONFIG_ESP_IPC_TASK_STACK_SIZE=2048")
    
    if [ -n "$result" ]; then
        echo -e "${GREEN}✅ OK${NC} - IPC stack = 2048"
    else
        echo -e "${RED}❌ FALLO${NC} - IPC stack NO configurado o diferente de 2048"
    fi
done

echo ""
echo "======================================"
echo "Verificación completa"
echo "======================================"
```

Hacer ejecutable y correr:

```bash
chmod +x verify_platformio.sh
./verify_platformio.sh
```

### Verificación Manual

```bash
# Entorno base debe tener en build_flags:
grep -A 100 "^\[env:esp32-s3-devkitc\]" platformio.ini | grep CONFIG_ESP_IPC_TASK_STACK_SIZE
# Resultado esperado: -DCONFIG_ESP_IPC_TASK_STACK_SIZE=2048

# Entornos derivados deben heredar con:
grep "extends = env:esp32-s3-devkitc" platformio.ini
# Resultado esperado: Todos los entornos menos el base

# Entornos derivados deben usar:
grep "\${env:esp32-s3-devkitc.build_flags}" platformio.ini
# Resultado esperado: Todos los entornos derivados lo incluyen
```

### Configuración Correcta Verificada

**✅ Configuración del Entorno Base:**

```ini
[env:esp32-s3-devkitc]
build_flags =
    # ... otros flags ...
    -DCONFIG_ESP_IPC_TASK_STACK_SIZE=2048  ; ← DEBE ESTAR AQUÍ
```

**✅ Configuración de Entornos Derivados:**

```ini
[env:esp32-s3-devkitc-release]
extends = env:esp32-s3-devkitc  ; ← Hereda del base
build_flags =
    ${env:esp32-s3-devkitc.build_flags}  ; ← INCLUYE flags del base
    -DCORE_DEBUG_LEVEL=0  ; ← Flags adicionales
```

**⚠️ INCORRECTO - No incluye flags del base:**

```ini
[env:esp32-s3-devkitc-BAD-EXAMPLE]
extends = env:esp32-s3-devkitc
build_flags =
    # ❌ FALTA: ${env:esp32-s3-devkitc.build_flags}
    -DCORE_DEBUG_LEVEL=0  ; ← Solo tiene sus propios flags
```

---

## 6️⃣ Diagnosticar Origen del Fallo

### Árbol de Decisión

```
¿El ESP32 arranca?
│
├─ NO → Error muy temprano (antes de Serial)
│   │
│   ├─ ¿Backtrace muestra "ipc0"?
│   │   ├─ SÍ → Problema de CONFIG_ESP_IPC_TASK_STACK_SIZE
│   │   │        Ver sección 1️⃣ y 5️⃣
│   │   │
│   │   └─ NO → ¿Hay alimentación estable?
│   │            ├─ NO → Verificar fuente de alimentación (>2A)
│   │            └─ SÍ → Problema de hardware o flash corrupta
│   │                     Probar: pio run -t erase
│   │
│   └─ ¿Llega a mostrar mensajes [BOOT]?
│       ├─ NO → Crash en System::init() o Storage::init()
│       │        Ver sección 3️⃣
│       │
│       └─ SÍ → Continuar abajo...
│
└─ SÍ → Arranca pero reinicia después
    │
    ├─ ¿Muestra "Task watchdog got triggered"?
    │   └─ SÍ → Setup() tarda más de 10s sin alimentar watchdog
    │            Ver sección 3️⃣ - Agregar Watchdog::feed()
    │
    ├─ ¿Muestra "Stack overflow in task loopTask"?
    │   └─ SÍ → Stack de loop insuficiente
    │            Ver sección 4️⃣ - Aumentar ARDUINO_LOOP_STACK_SIZE
    │
    └─ ¿Reinicia aleatoriamente durante operación?
        ├─ ¿Stack watermark bajo (<512 bytes)?
        │   └─ SÍ → Ver sección 4️⃣ - Aumentar stack de tarea específica
        │
        └─ ¿Operaciones I2C involucradas?
            └─ SÍ → Posible bus lock o sensor defectuoso
                     Probar: I2C recovery, desconectar sensores uno a uno
```

### Matriz de Síntomas vs Soluciones

| Síntoma | Causa Probable | Solución | Sección |
|---------|----------------|----------|---------|
| Reinicio inmediato, sin Serial | IPC stack overflow | CONFIG_ESP_IPC_TASK_STACK_SIZE=2048 | 1️⃣, 5️⃣ |
| "Stack canary watchpoint (ipc0)" | IPC stack overflow | CONFIG_ESP_IPC_TASK_STACK_SIZE=2048 | 1️⃣, 5️⃣ |
| "Task watchdog got triggered" | Setup >10s sin feed | Agregar Watchdog::feed() | 3️⃣ |
| "Stack overflow in loopTask" | Loop stack insuficiente | Aumentar ARDUINO_LOOP_STACK_SIZE | 4️⃣ |
| Reinicia durante operación | Stack bajo en tarea | Aumentar stack de tarea | 4️⃣ |
| Reinicia en init de sensor | I2C/SPI lock o timeout | I2C recovery, verificar hardware | 3️⃣ |
| Pantalla negra | Backlight no configurado | Verificar PIN_TFT_BL init temprano | 3️⃣ |

---

## 📊 Logs de Referencia

### ✅ Boot Exitoso (Esperado)

```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
...
========================================
ESP32-S3 Car Control System v2.10.7
========================================
CPU Freq: 240 MHz
Free heap: 320412 bytes
PSRAM: 8388608 bytes (Free: 8388480 bytes)
Stack high water mark: 31456 bytes
Configured loop stack: 32768 bytes
Configured main task stack: 20480 bytes
Boot sequence starting...
[BOOT] Enabling TFT backlight...
[BOOT] Backlight enabled on GPIO42
[BOOT] Resetting TFT display...
[BOOT] TFT reset complete
[BOOT] Debug level set to 2
[BOOT] Initializing System...
[STACK] After System::init - Free: 31432 bytes
[BOOT] Initializing Storage...
[STACK] After Storage::init - Free: 31408 bytes
[BOOT] Initializing Watchdog early...
[BOOT] Watchdog initialized and fed
[BOOT] Loading configuration from EEPROM...
[BOOT] Display brightness loaded: 200
[BOOT] Initializing Logger...
[STACK] After Logger::init - Free: 31384 bytes
[BOOT] FULL MODE: Starting hardware initialization...
...
[BOOT] Setup complete! Entering main loop...
```

### ❌ IPC Stack Error

```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce3810,len:0x178c
load:0x403c9700,len:0x4
load:0x403c9704,len:0xcb8
load:0x403cc700,len:0x2db0
entry 0x403c9908

Guru Meditation Error: Core 0 panic'ed (Unhandled debug exception).
Debug exception reason: Stack canary watchpoint triggered (ipc0)
Core  0 register dump:
...
Backtrace: 0x40379990:0x3fcf0d50 0x0005002d:0xa5a5a5a5 |<-CORRUPTED

ELF file SHA256: ...

Rebooting...
```

### ❌ Watchdog Timeout

```
[BOOT] Initializing System...
[BOOT] Initializing Storage...
[BOOT] Initializing Watchdog early...
[BOOT] Watchdog initialized and fed
[BOOT] Initializing WiFi Manager...
[WiFi] Connecting to SSID...
[WiFi] Waiting for connection...

E (10234) task_wdt: Task watchdog got triggered. The following tasks did not reset the watchdog in time:
E (10234) task_wdt:  - IDLE (CPU 0)
E (10234) task_wdt: Tasks currently running:
E (10234) task_wdt: CPU 0: loopTask
E (10234) task_wdt: CPU 1: IDLE
E (10234) task_wdt: Aborting.

Guru Meditation Error: Core 0 panic'ed (Task watchdog timeout)
...
Rebooting...
```

---

## 🛠️ Herramientas de Diagnóstico

### 1. Monitor Serial con Filtros

```bash
# Monitor básico
pio device monitor --port COM4 --baud 115200

# Monitor con decodificación de excepciones
pio device monitor --port COM4 --filter esp32_exception_decoder

# Monitor con múltiples filtros
pio device monitor --port COM4 --filter colorize --filter esp32_exception_decoder
```

### 2. Análisis de Memoria

Agregar en setup() después de cada módulo:

```cpp
Logger::infof("Heap libre: %d bytes", ESP.getFreeHeap());
Logger::infof("PSRAM libre: %d bytes", ESP.getFreePsram());
Logger::infof("Stack libre: %d bytes", uxTaskGetStackHighWaterMark(NULL));
```

### 3. Profiler de Tiempo

Medir tiempo de inicialización:

```cpp
unsigned long start = millis();
ModuloX::init();
unsigned long elapsed = millis() - start;
Logger::infof("ModuloX init took: %lu ms", elapsed);
Watchdog::feed();  // Si >2000ms
```

### 4. I2C Scanner

Si sospechas problema I2C:

```cpp
void scanI2C() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Logger::info("Scanning I2C bus...");
  
  uint8_t found = 0;
  for(uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) {
      Logger::infof("Device found at 0x%02X", addr);
      found++;
    }
  }
  
  Logger::infof("Scan complete: %d devices found", found);
}
```

---

## 📝 Plantilla de Reporte de Problema

Cuando reportes un problema, incluye:

```markdown
## Información del Sistema
- **Firmware version:** vX.X.X
- **Entorno:** esp32-s3-devkitc / release / ota / etc
- **Placa:** ESP32-S3-DevKitC-1 (44 pines)
- **Hardware conectado:** [Lista de sensores/módulos]
- **Alimentación:** [Fuente, voltaje, corriente]

## Descripción del Problema
[Describe qué observas]

## Logs Completos
```
[Pega aquí el output de `pio device monitor` durante 30 segundos]
```

## Verificaciones Realizadas
- [ ] CONFIG_ESP_IPC_TASK_STACK_SIZE presente
- [ ] Watchdog alimentado durante setup
- [ ] Stack watermarks verificados
- [ ] Hardware conectado correctamente
- [ ] Flash borrada y reflasheada
- [ ] Probado en múltiples entornos

## Backtrace Decodificado (si aplica)
[Resultado de addr2line o exception decoder]

## Entornos Probados
- [ ] esp32-s3-devkitc: [OK/FALLO]
- [ ] esp32-s3-devkitc-release: [OK/FALLO]
- [ ] esp32-s3-devkitc-no-touch: [OK/FALLO]
```

---

## 🎯 Resumen de Acciones Rápidas

### Si tienes bucle de reinicio:

1. **Captura logs completos** (30s mínimo)
2. **Identifica el error** usando la Matriz de Síntomas
3. **Verifica CONFIG_ESP_IPC_TASK_STACK_SIZE** en todos los entornos
4. **Agrega Watchdog::feed()** si setup() tarda >5s
5. **Aumenta stack sizes** si watermark <1KB
6. **Prueba diferentes entornos** para aislar el problema
7. **Decodifica backtrace** para identificar función fallida

### Comandos de Emergencia:

```bash
# Borrar flash completamente
pio run -t erase

# Rebuild completo
rm -rf .pio
pio run -t clean
pio run -e esp32-s3-devkitc

# Flashear y monitorear
pio run -e esp32-s3-devkitc -t upload --upload-port COM4
pio device monitor --port COM4 --filter esp32_exception_decoder
```

---

**Versión:** 1.0  
**Fecha:** 2025-12-15  
**Compatible con:** Firmware v2.10.8+  
**Autor:** Sistema de desarrollo

**¡Usa esta guía para diagnosticar y resolver cualquier problema de boot! 🔧**
