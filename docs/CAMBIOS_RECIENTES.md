# Cambios Recientes en el Firmware

## Versión: 2.4.0
**Fecha:** 2025-11-25  

---

## 🔒 Mejoras de Fiabilidad y Seguridad v2.4.0

### 1. Corrección de Race Conditions ✅

**Problema:** Los contadores de pulsos de ruedas eran accedidos de forma no atómica entre ISR y loop principal.

**Solución:** Acceso atómico usando `noInterrupts()`/`interrupts()`:
```cpp
// Antes (race condition)
float revs = (float)pulses[i] / PULSES_PER_REV;
pulses[i] = 0;

// Después (acceso atómico)
noInterrupts();
unsigned long currentPulses = pulses[i];
pulses[i] = 0;
interrupts();
```

### 2. Implementación de SteeringMotor::get() ✅

**Problema:** Función declarada en header pero nunca implementada.

**Solución:** Añadida implementación:
```cpp
const SteeringMotor::State& SteeringMotor::get() {
    return s;
}
```

### 3. Validación de Índices en Sensores ✅

**Problema:** Los getters solo verificaban límite superior (`channel < NUM_CURRENTS`).

**Solución:** Verificación completa:
```cpp
// Antes
if(channel < NUM_CURRENTS) return lastCurrent[channel];

// Después
if(channel >= 0 && channel < NUM_CURRENTS) return lastCurrent[channel];
```

### 4. Nueva Función de Parada de Emergencia ✅

**Añadido:** `Relays::emergencyStop()` para desactivar todos los relés inmediatamente sin delays ni debounce.

```cpp
void Relays::emergencyStop() {
    // Desactivar todos los relés inmediatamente
    digitalWrite(PIN_RELAY_DIR, LOW);
    digitalWrite(PIN_RELAY_TRAC, LOW);
    digitalWrite(PIN_RELAY_MAIN, LOW);
    digitalWrite(PIN_RELAY_SPARE, LOW);
    // ...
}
```

### 5. Histéresis en Detección de Errores ✅

**Mejora:** Los relés ahora requieren 3 errores consecutivos antes de desactivarse para evitar falsos positivos por ruido de sensores.

### 6. Eliminación de Bucles Bloqueantes ✅

**main.cpp:** Eliminado bucle `while (!Serial)` que podía causar watchdog reset.

**hud_manager.cpp:** Reducidos delays de inicialización TFT de 70ms a ~0.6ms usando `delayMicroseconds()`.

---

## 📊 Cambios en Archivos

### Archivos Modificados:
- `src/control/steering_motor.cpp` - Añadida implementación `get()`
- `src/control/relays.cpp` + `include/relays.h` - Emergency stop + histéresis
- `src/sensors/wheels.cpp` - Acceso atómico a contadores
- `src/sensors/current.cpp` - Validación índices negativos
- `src/sensors/temperature.cpp` - Validación índices negativos
- `src/main.cpp` - Eliminado bucle bloqueante Serial
- `src/hud/hud_manager.cpp` - Reducidos delays de reset TFT

### Estado del Firmware:
| Métrica | Valor |
|---------|-------|
| **RAM** | 9.0% (29,392 bytes) |
| **Flash** | 35.7% (468,285 bytes) |
| **Entornos OK** | 4/4 |

---

## 🔧 Versiones Anteriores

### v2.3.0 (2025-11-25)
- Reorganización GPIO y resolución conflictos de pines
- TOUCH_CS movido de GPIO 3 a GPIO 21 (pin seguro)
- LED_REAR movido de GPIO 19 a GPIO 48
- Shifter completo migrado a MCP23017 (pines B0-B4)

### v2.2.0 (2025-11-24)
- Corrección macros OTA
- Build exitoso 4/4 entornos

### v2.1.0 (2025-11-23)
- Refactorización delay() → millis()
- Correcciones de compilación

### v2.0.0 (2025-11-22)
- Auditoría completa del firmware
- Implementación de 2x PCA9685 para control PWM

---

**Documento actualizado:** 2025-11-25  
**Para más detalles:** Ver `HARDWARE_REFERENCE.md` y `docs/PIN_MAPPING_DEVKITC1.md`
