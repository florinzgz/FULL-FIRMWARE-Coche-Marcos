# Cambios Recientes en el Firmware

## Versión: 2.8.1
**Fecha:** 2025-11-28  

---

## 🆕 Novedades v2.8.1

### 1. Diagnósticos de Arranque Mejorados ✅

**Problema resuelto:** Pantalla en blanco y LED verde apagado después de flashear el firmware.

**Solución implementada:**
- ✅ Mensajes de diagnóstico por Serial desde el primer momento del boot
- ✅ Backlight del TFT habilitado INMEDIATAMENTE al iniciar
- ✅ Pantalla azul con "ESP32-S3 Booting..." visible durante la inicialización
- ✅ Cada módulo imprime su estado de inicialización

**Output esperado en Serial Monitor (115200 baud):**
```
========================================
ESP32-S3 Car Control System v2.8.1
========================================
CPU Freq: 240 MHz
Free heap: XXXXX bytes
Boot sequence starting...
[BOOT] Enabling TFT backlight...
[BOOT] Backlight enabled on GPIO42
[BOOT] Resetting TFT display...
[BOOT] TFT reset complete
[BOOT] Initializing System...
[BOOT] Initializing Storage...
[BOOT] Initializing Logger...
...
[BOOT] Setup complete! Entering main loop...
```

### 2. Documentación de Solución de Problemas ✅

**Archivo actualizado:** `docs/STANDALONE_MODE.md`

**Nuevas secciones:**
- Solución para "pantalla negra y LED verde apagado"
- Guía para recuperar ESP32-S3 que no responde
- Instrucciones para borrar flash completamente
- Pines SPI corregidos según pins.h

---

## Versión: 2.8.0
**Fecha:** 2025-11-27  

---

## 🆕 Novedades v2.8.0

### 1. Sistema de Telemetría Avanzada ✅

**Nuevo módulo:** `telemetry.h` + `telemetry.cpp`

**Características:**
- ✅ Checksum FNV-1a + Magic Number para detección de corrupción
- ✅ Persistencia automática en NVS (Preferences)
- ✅ Métricas extendidas: distancia, energía, velocidad, batería, temperatura
- ✅ Exportación JSON para SD/WiFi/app móvil
- ✅ Funciones resetSession() y resetTrip()

```cpp
// Ejemplo de uso
Telemetry::init();
Telemetry::updateSpeed(25.5f);
Telemetry::addDistance(0.1f);
Telemetry::updateBattery(24.5f, 10.2f, 85.0f);
String json = Telemetry::exportToJson();
```

### 2. Estructura RedundantSensor ✅

**Añadido a:** `sensors.h`

**Propósito:** Tolerancia a fallos para sensores críticos.

```cpp
struct RedundantSensor {
    float primaryValue;
    float secondaryValue;
    bool primaryValid;
    bool secondaryValid;
    
    float getSafeValue() const;     // Promedio o fallback
    bool hasDiscrepancy() const;    // Detecta diferencias
    bool isOperational() const;     // Al menos uno funciona
};
```

---

## 🔒 Mejoras de Fiabilidad v2.4.0-v2.7.0

### Race Conditions Corregidas ✅

**Problema:** Contadores de pulsos de ruedas accedidos de forma no atómica.

**Solución:** Acceso atómico con `noInterrupts()`/`interrupts()`.

### SteeringMotor::get() Implementado ✅

**Problema:** Función declarada pero nunca implementada.

### Validación de Índices ✅

**Problema:** Solo se verificaba límite superior.

**Solución:** Verificación completa `channel >= 0 && channel < NUM`.

### Parada de Emergencia ✅

**Añadido:** `Relays::emergencyStop()` para desactivar todos los relés inmediatamente.

### Histéresis en Errores ✅

**Mejora:** 3 errores consecutivos antes de desactivar (evita falsos positivos).

---

## 📊 Estado Actual

| Métrica | Valor |
|---------|-------|
| **RAM** | 9.0% (~29,500 bytes) |
| **Flash** | 36.6% (~480,000 bytes) |
| **Entornos OK** | 4/4 |

---

## 🔧 Versiones Anteriores

### v2.7.0 (2025-11-27)
- Documentación sincronizada con pins.h
- Verificación línea por línea del firmware

### v2.4.0 (2025-11-25)
- Race conditions corregidas
- Histéresis en detección de errores

### v2.3.0 (2025-11-25)
- Reorganización GPIO
- Shifter migrado a MCP23017

### v2.2.0 (2025-11-24)
- Corrección macros OTA

---

**Documento actualizado:** 2025-11-27  
**Versión actual:** v2.8.0
