# Cambios Recientes en el Firmware

## Versión: 2.8.9
**Fecha:** 2025-12-02  

---

## 🆕 Novedades v2.8.9

### 1. Optimización de Rendimiento del Display ✅

**Problema resuelto:** Display ST7796S funcionando a frecuencia SPI subóptima.

**Solución implementada:**
- ✅ Frecuencia SPI aumentada de 20MHz a 40MHz para mejor rendimiento
- ✅ Basado en configuración TFT_eSPI mySetup27_ST7796_ESP32.h
- ✅ ESP32-S3 soporta frecuencias altas mejor que ESP32-C3
- ✅ SPI_READ_FREQUENCY aumentada a 20MHz para lecturas más rápidas
- ✅ Touch mantiene 2.5MHz (requisito del controlador XPT2046)

**Resultados:**
- Mejor fluidez en animaciones y actualizaciones de pantalla
- Reducción de tiempo de refresco del HUD
- Sin efectos adversos en estabilidad

### 2. Touch XPT2046 - Librería Separada ✅

**Configuración actualizada:** Se utiliza librería XPT2046_Touchscreen separada para mejor fiabilidad.

**Solución implementada:**
- ✅ Añadida librería PaulStoffregen/XPT2046_Touchscreen @ ^1.4
- ✅ Configuración mediante pines GPIO (CS=21, IRQ=47)
- ✅ SPI compartido con display (MOSI=11, MISO=12, SCK=10)
- ✅ Mayor estabilidad que el driver integrado de TFT_eSPI

**Pines configurados:**
```cpp
TOUCH_CS = GPIO 21
TOUCH_IRQ = GPIO 47
```

### 3. Optimizaciones de Performance ✅

**Actualizaciones de librerías:**
- ✅ TFT_eSPI: mantenida en 2.5.43 (versión 2.5.50 presenta errores de compatibilidad)
- ✅ INA226: 0.6.4 → 0.7.0 (mejor sensado de corriente)
- ✅ FastLED: mantenida en 3.6.0 (versión 3.7.0 presenta errores de compatibilidad)
- ✅ XPT2046_Touchscreen: 1.4 (librería separada para touch - mayor estabilidad)
- ✅ Añadido ESP Async WebServer 1.2.4 (soporte para dashboard web)

**Optimizaciones del compilador (release):**
```ini
-O3                              ; Máxima optimización de performance
-DCONFIG_ARDUHAL_ESP_LOG=0       ; Desactivar logs Arduino HAL
-DCONFIG_ESP_CONSOLE_UART_NONE=1 ; Sin console UART en producción
```

**Beneficios:**
- Mejora significativa en velocidad de ejecución
- Reducción del tamaño del binario
- Menor consumo de recursos en producción
- Base preparada para dashboard web futuro

### 4. Limpieza de Configuración ✅

**Mejoras en platformio.ini:**
- ✅ Eliminadas referencias a versiones antiguas (v2.8.3, v2.8.4, v2.8.6, v2.8.7, v2.8.8)
- ✅ Changelog consolidado con solo versiones relevantes
- ✅ Comentarios inline simplificados
- ✅ Mejor legibilidad y mantenimiento

**Mejoras en project_config.ini:**
- ✅ Actualizado a versión 2.8.9
- ✅ Frecuencias SPI actualizadas en documentación
- ✅ Changelog consolidado y organizado
- ✅ Sincronizado con docs/PROJECT_CONFIG.ini

---

## Versión: 2.8.5
**Fecha:** 2025-11-30  

---

## 🆕 Novedades v2.8.5

### 1. Revisión Exhaustiva de Código ✅

**Problema resuelto:** Necesidad de verificar calidad y seguridad en todos los módulos.

**Solución implementada:**
- ✅ Verificación completa de 57 archivos .cpp y 61 archivos .h
- ✅ Patrones de seguridad documentados (nullptr guards, NaN validation, ISR-safe)
- ✅ TODOs identificados y priorizados para mejoras futuras
- ✅ Estado general confirmado: Listo para producción

### 2. Nuevos Archivos de Utilidades ✅

**Nuevos archivos añadidos:**

| Archivo | Ubicación | Descripción |
|---------|-----------|-------------|
| `pin_utils.h` | include/ | Funciones de validación de GPIO (pin_is_reserved, pin_is_valid_gpio, etc.) |
| `pwm_channels.h` | include/ | Definiciones de canales PWM y funciones de validación |
| `test_display.h` | include/ | Header para pruebas standalone de display |
| `test_display.cpp` | src/ | Implementación de pruebas de display (setupDisplayTest, loopDisplayTest) |

### 3. Mejoras en math_utils.cpp ✅

**Correcciones de seguridad:**
- ✅ Validación NaN/Inf en `mapf()`
- ✅ Validación NaN/Inf en `clamp()`
- ✅ Validación NaN/Inf en `kmhToRpm()` y `rpmToKmh()`
- ✅ Validación NaN/Inf en `ackermannFactors()`
- ✅ Validación NaN/Inf en `ema()`

### 4. Mejoras en led_controller.cpp ✅

**Correcciones de seguridad:**
- ✅ Validación de pines LED antes de inicializar FastLED
- ✅ Verificación de hardware OK antes de update
- ✅ Timeout de seguridad en emergency flash (10 segundos)
- ✅ Límite de brillo máximo para prevenir sobrecalentamiento

### 5. CI Workflow para Testing ✅

**Nuevo workflow:** `.github/workflows/build_test.yml`

**Características:**
- ✅ Build automático del entorno `esp32-s3-devkitc-test`
- ✅ Caché de PlatformIO para builds más rápidos
- ✅ Generación de reporte de build
- ✅ Upload de artefactos de firmware

### 6. Mejoras en Validación de PWM ✅

**Nueva función:** `pwm_channels_match_steering_config()`

**Uso:**
```cpp
if (pwm_channels_match_steering_config(fwd_ch, rev_ch)) {
    // Canales configurados correctamente
}
```

---

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

**Documento actualizado:** 2025-12-02  
**Versión actual:** v2.8.9
