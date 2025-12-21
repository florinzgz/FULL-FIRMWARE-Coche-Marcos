# Sistema de Tolerancia a Fallos

## 🎯 Objetivo

Implementar un sistema robusto de gestión de errores que **impida reinicios en bucle** cuando fallan sensores o módulos, mejorando la estabilidad total del firmware.

## 📋 Modos de Operación

El sistema implementa 4 modos de operación que permiten degradación progresiva:

### MODE_FULL (Modo Completo)
- **Estado**: Todos los sistemas operativos
- **Comportamiento**: Funcionalidad completa del vehículo
- **Condiciones**: Todos los sensores y módulos críticos/opcionales funcionando
- **Potencia**: Motores habilitados
- **Indicador HUD**: Sin indicador (operación normal)

### MODE_DEGRADED (Modo Degradado)
- **Estado**: Algunos sensores opcionales fallaron, continuar con funcionalidad reducida
- **Comportamiento**: El vehículo puede operar pero con monitoreo reducido
- **Condiciones**: Sensores críticos OK, pero algunos opcionales fallaron:
  - INA226 (sensores de corriente)
  - DS18B20 (sensores de temperatura)
  - Sensores de rueda
  - Motor de dirección
  - Módulo de tracción
  - DFPlayer (audio)
- **Potencia**: Motores habilitados
- **Indicador HUD**: "DEGRADED" en amarillo
- **Mensaje Serial**: `[WARN] Sistema operando en modo degradado - algunos sensores no disponibles`

### MODE_SAFE (Modo Seguro)
- **Estado**: Fallos críticos detectados, solo funciones de monitoreo
- **Comportamiento**: Sin motores, solo monitoreo y diagnóstico
- **Condiciones**: Fallo en componentes críticos:
  - Pedal no responde o no en reposo
  - Encoder de dirección no responde
  - Palanca de cambios no inicializada o no en PARK
  - Relés no responden
- **Potencia**: Motores DESHABILITADOS
- **Indicador HUD**: "SAFE" en amarillo con mensaje de advertencia
- **Mensaje Serial**: `[WARN] Sistema en modo seguro - solo monitoreo`

### MODE_STANDALONE (Modo Standalone)
- **Estado**: Solo pantalla activa para diagnóstico
- **Comportamiento**: Display con datos simulados, sin sensores ni control
- **Condiciones**: Compilación con `-DSTANDALONE_DISPLAY`
- **Uso**: Diagnóstico de pantalla y HUD
- **Compilar con**: `pio run -e esp32-s3-devkitc-standalone`

## 🔧 Componentes del Sistema

### Módulos Críticos (Bloquean arranque si fallan)

| Módulo | Error Code | Acción si falla |
|--------|-----------|----------------|
| Pedal | 100 | → MODE_SAFE |
| Encoder dirección | 200 | → MODE_SAFE |
| Palanca cambios | 650-651 | → MODE_SAFE |
| Relés | 600 | → MODE_SAFE |

### Módulos Opcionales (NO bloquean arranque)

| Módulo | Acción si falla |
|--------|----------------|
| Sensores corriente (INA226) | → MODE_DEGRADED |
| Sensores temperatura (DS18B20) | → MODE_DEGRADED |
| Sensores rueda | → MODE_DEGRADED |
| Motor dirección | → MODE_DEGRADED |
| Módulo tracción | → MODE_DEGRADED |
| DFPlayer (audio) | → MODE_DEGRADED |

## 🚀 Mejoras Implementadas

### 1. Sistema de Modos (`operation_modes.h/cpp`)
```cpp
enum class OperationMode {
    MODE_FULL,       // Todos los sistemas OK
    MODE_DEGRADED,   // Algunos sensores fallaron, continuar
    MODE_SAFE,       // Solo funciones críticas
    MODE_STANDALONE  // Solo pantalla (diagnóstico)
};
```

### 2. SelfTest() Mejorado (`system.cpp`)
- **Antes**: Cualquier fallo → `h.ok = false` → reinicio en bucle
- **Ahora**: Diferencia entre fallos críticos y opcionales
  - Críticos → `MODE_SAFE`
  - Opcionales → `MODE_DEGRADED`
  - Todo OK → `MODE_FULL`

### 3. Timeouts en Inicialización de Sensores

**INA226 (current.cpp)**:
```cpp
const uint32_t INIT_TIMEOUT_MS = 5000;  // 5 segundos máximo
// Si un sensor falla, continuar con los demás
// NO llamar System::logError() - solo warning
```

**DS18B20 (temperature.cpp)**:
```cpp
const uint32_t INIT_TIMEOUT_MS = 3000;  // 3 segundos máximo
// Si un sensor falla, continuar con los demás
// NO marcar como error crítico
```

### 4. Secuencia de Arranque Mejorada (`main.cpp`)

```cpp
auto health = System::selfTest();
OperationMode mode = SystemMode::getMode();

if (mode == MODE_FULL) {
    // Arranque normal
    Relays::enablePower();
}
else if (mode == MODE_DEGRADED) {
    // Continuar con advertencia
    HUDManager::showWarning("Sistema en modo degradado");
    Relays::enablePower();  // ✅ CONTINUAR operando
}
else if (mode == MODE_SAFE) {
    // Solo monitoreo
    HUDManager::showWarning("Modo seguro - funcionalidad limitada");
    // ❌ NO habilitar motores
}
```

### 5. WiFi y Bluetooth Completamente Eliminados

**Archivos eliminados**:
- ❌ `src/core/bluetooth_controller.cpp`
- ❌ `include/bluetooth_controller.h`

**Código eliminado de `main.cpp`**:
- ❌ `#include "bluetooth_controller.h"`
- ❌ `BluetoothController::init()`
- ❌ `BluetoothController::update()`

**Razón**: ESP32-S3 no soporta Bluetooth Classic, y WiFi no es necesario para el vehículo.

## 📊 Escenarios de Prueba

### 1. Desconectar todos los INA226
**Resultado esperado**:
```
[WARN] INA226 ch 0 falló - continuando
[WARN] INA226 ch 1 falló - continuando
...
[WARN] Sistema operando en modo degradado - algunos sensores no disponibles
[INFO] System mode: DEGRADED
```
✅ Sistema debe arrancar en DEGRADED

### 2. Desconectar DS18B20
**Resultado esperado**:
```
[WARN] DS18B20 0 no detectado - continuando
[WARN] DS18B20 init: algunos sensores no disponibles - modo degradado
[INFO] System mode: DEGRADED
```
✅ Sistema debe arrancar en DEGRADED

### 3. Compilar con `-DSTANDALONE_DISPLAY`
**Comando**:
```bash
pio run -e esp32-s3-devkitc-standalone
```
**Resultado esperado**:
```
[BOOT] STANDALONE_DISPLAY MODE: Skipping sensor initialization
[INFO] STANDALONE MODE: Dashboard active with simulated values
```
✅ Solo pantalla debe funcionar con datos simulados

### 4. Desconectar relés
**Resultado esperado**:
```
[ERROR] SelfTest: CRÍTICO - Relés no responden - modo seguro
[INFO] System mode: SAFE
[WARN] Sistema en modo seguro - solo monitoreo
```
✅ Sistema debe arrancar en SAFE (sin motores)

### 5. Conectar todo
**Resultado esperado**:
```
[BOOT] Self-test PASSED - MODE_FULL!
[INFO] System mode: FULL
```
✅ Sistema debe arrancar en FULL

## 🔍 Verificación Post-Deploy

### Serial Monitor (115200 baud)

**Modo DEGRADED correcto**:
```
[INFO] System mode: DEGRADED
[WARN] Sensores corriente no disponibles - modo degradado
[INFO] Sistema operando en modo degradado - algunos sensores no disponibles
```

**Modo SAFE correcto**:
```
[INFO] System mode: SAFE
[ERROR] SelfTest: CRÍTICO - pedal no responde
[WARN] Sistema en modo seguro - solo monitoreo
```

**NO debe aparecer**:
```
[ERROR] Critical sensor failure - aborting
ESP_RST_SW (software reset)
```

## 🎨 Indicador Visual en HUD

En modo no-FULL, el HUD muestra el modo de operación:
- **MODE_DEGRADED**: Texto amarillo "DEGRADED" en centro inferior
- **MODE_SAFE**: Texto amarillo "SAFE" en centro inferior
- **MODE_FULL**: Sin indicador (operación normal)

## ✅ Beneficios del Sistema

1. **✅ No más bucles de reinicio** - El firmware continúa operando incluso con sensores fallidos
2. **✅ Degradación progresiva** - El sistema se adapta al estado del hardware
3. **✅ Diagnóstico claro** - Mensajes Serial precisos sobre el estado del sistema
4. **✅ Modo standalone** - Permite diagnóstico de pantalla sin hardware
5. **✅ Código limpio** - WiFi/Bluetooth completamente eliminados
6. **✅ Timeouts** - Previene bloqueos en inicialización de sensores

## 📝 Notas Técnicas

### Diferencia con v2.11.0 anterior

**Antes (v2.11.0)**:
```cpp
if (!Sensors::currentInitOK()) {
    System::logError(300);
    h.ok = false;  // ❌ Bloquea arranque
}
```

**Ahora (v2.11.4+)**:
```cpp
if (!Sensors::currentInitOK()) {
    Logger::warn("Sensores corriente no disponibles - modo degradado");
    mode = MODE_DEGRADED;
    h.currentOK = false;
    // ✅ NO bloquea arranque, continúa operación
}
```

### Arquitectura del Sistema

```
System::init()
    ├─> SystemMode::init() → MODE_FULL
    └─> ... inicializar módulos ...

System::selfTest()
    ├─> Verificar sensores opcionales
    │   ├─> Fallo → MODE_DEGRADED (continuar)
    │   └─> OK → mantener MODE_FULL
    ├─> Verificar componentes críticos
    │   ├─> Fallo → MODE_SAFE (sin motores)
    │   └─> OK → mantener modo actual
    └─> SystemMode::setMode(mode)

setup() en main.cpp
    ├─> health = System::selfTest()
    ├─> mode = SystemMode::getMode()
    └─> Actuar según modo:
        ├─> MODE_FULL → Relays::enablePower()
        ├─> MODE_DEGRADED → Warning + Relays::enablePower()
        └─> MODE_SAFE → Warning + sin motores
```

## 🔧 Configuración en platformio.ini

```ini
[env:esp32-s3-devkitc-standalone]
extends = env:esp32-s3-devkitc
build_flags =
    ${env:esp32-s3-devkitc.build_flags}
    -DSTANDALONE_DISPLAY        ; Modo standalone
    -DDISABLE_SENSORS           ; Sin sensores
    -DSTANDALONE_TIMEOUT=30000  ; 30s timeout
```

## 📚 Referencias

- `include/operation_modes.h` - Definición de modos
- `src/core/operation_modes.cpp` - Implementación de modos
- `src/core/system.cpp` - selfTest() mejorado
- `src/sensors/current.cpp` - Timeouts INA226
- `src/sensors/temperature.cpp` - Timeouts DS18B20
- `src/main.cpp` - Secuencia de arranque mejorada
