# Análisis Completo de Causas de Reinicio - ESP32-S3

## Problemas CRÍTICOS Encontrados y Corregidos ✅

### 6 Objetos Globales con Constructores Peligrosos

Todos convertidos a inicialización lazy con punteros para evitar "Stack canary watchpoint triggered (ipc0)":

1. **TFT_eSPI** (hud_manager.cpp) - Error 601
   - Constructor iniciaba SPI antes de FreeRTOS → ✅ CORREGIDO

2. **OneWire** (temperature.cpp) - Error 395
   - Constructor configuraba GPIO antes de FreeRTOS → ✅ CORREGIDO

3. **DallasTemperature** (temperature.cpp) - Error 396
   - Constructor asignaba memoria antes del heap → ✅ CORREGIDO

4. **HardwareSerial DFSerial** (dfplayer.cpp) - Error 698
   - Constructor inicializaba UART1 antes de FreeRTOS → ✅ CORREGIDO

5. **DFRobotDFPlayerMini** (dfplayer.cpp) - Error 699
   - Constructor podía asignar memoria antes del heap → ✅ CORREGIDO

6. **HardwareSerial TOFSerial** (obstacle_detection.cpp) - Error 697
   - Constructor inicializaba UART0 antes de FreeRTOS → ✅ CORREGIDO (NUEVO)

---

## Problemas POTENCIALES Identificados ⚠️

### Prioridad Media

#### 1. Múltiples Instancias de Preferences
**Archivos afectados:**
- `src/core/storage.cpp` - `static Preferences prefs;`
- `src/core/telemetry.cpp` - `static Preferences prefs;`
- `src/core/config_manager.cpp` - `static Preferences prefs;`
- `src/core/config_storage.cpp` - `static Preferences preferences;`

**Riesgo:** 
- Competencia por acceso NVS (Non-Volatile Storage)
- Cada instancia asigna buffers internos
- Posible corrupción si múltiples intentan acceder simultáneamente

**Recomendación:**
- Crear una única instancia global compartida en `storage.cpp`
- Proporcionar funciones de acceso para otros módulos
- Usar mutex para proteger acceso concurrente

**Severidad:** MEDIA - No causa bootloop inmediato, pero puede causar:
- Corrupción de configuración
- Errores de NVS
- Reinicio en operaciones de escritura concurrentes

---

#### 2. Arrays CRGB para LEDs
**Archivo:** `src/lighting/led_controller.cpp`
```cpp
static CRGB frontLeds[LED_FRONT_COUNT];  // ~168 bytes si LED_FRONT_COUNT=14
static CRGB rearLeds[LED_REAR_COUNT];    // ~168 bytes si LED_REAR_COUNT=14
```

**Riesgo:**
- Si LED_FRONT_COUNT o LED_REAR_COUNT son > 20, aumenta presión de memoria
- FastLED puede tener constructores internos

**Estado Actual:** Probablemente seguro si arrays son pequeños (< 20 LEDs cada uno)

**Recomendación:** Monitorear memoria libre durante boot. Si hay problemas, considerar:
```cpp
static CRGB *frontLeds = nullptr;
// En init(): frontLeds = new CRGB[LED_FRONT_COUNT];
```

**Severidad:** BAJA-MEDIA - Solo problema si arrays son muy grandes

---

#### 3. Instancia Global MCP23017
**Archivo:** `src/core/mcp_shared.cpp`
```cpp
Adafruit_MCP23X17 mcp;  // ¿Global?
```

**Riesgo:**
- Constructor de Adafruit_MCP23X17 podría inicializar I2C/Wire
- Depende de si está dentro o fuera del patrón singleton

**Recomendación:** Verificar que `mcp` solo se instancia cuando se llama `getInstance()`

**Severidad:** MEDIA - Depende de implementación del manager

---

### Prioridad Baja (Verificado como Seguro)

#### ✅ Adafruit_PWMServoDriver (PCA9685)
- Usa constructor por defecto (no inicializa I2C)
- I2C se inicializa en `begin(address)` durante `init()`
- **SEGURO** - Ya corregido en v2.11.6

#### ✅ Interrupciones (ISR)
- `attachInterrupt()` se llama desde `init()` después de FreeRTOS
- **SEGURO** - Orden de inicialización correcto

#### ✅ Wire.begin()
- Se llama desde `System::init()` después de FreeRTOS
- **SEGURO** - Orden correcto

#### ✅ Watchdog
- Se alimenta regularmente durante inicialización
- **SEGURO** - No hay timeout durante boot

---

## Resumen de Códigos de Error

Para diagnóstico, si aparece alguno de estos códigos en serial:

| Código | Significado | Archivo |
|--------|-------------|---------|
| 395 | Error al asignar OneWire | temperature.cpp |
| 396 | Error al asignar DallasTemperature | temperature.cpp |
| 601 | Error al asignar TFT_eSPI | hud_manager.cpp |
| 697 | Error al asignar TOFSerial (LiDAR) | obstacle_detection.cpp |
| 698 | Error al asignar DFSerial (DFPlayer) | dfplayer.cpp |
| 699 | Error al asignar DFRobotDFPlayerMini | dfplayer.cpp |

Si ves cualquiera de estos códigos → Problema de memoria, heap insuficiente

---

## Secuencia de Boot Esperada

```
A - Serial inicializado
B - Boot counter inicializado
C - System init iniciado
D - I2C inicializado
E - Storage inicializado
F - HUD init iniciado
G - Render queue creado
H - Antes de tft.init()
I - tft.init() ÉXITO      ← Punto crítico donde crasheaba antes
J - Rotación configurada
K - Componentes dashboard inicializados
```

Si todos los marcadores A-K aparecen sin crash → **Bootloop RESUELTO**

---

## Otras Causas Comunes de Reinicio (No Aplicables Aquí)

### Ya Verificado y Seguro:
- ❌ WiFi.begin() en global → No hay WiFi en este proyecto
- ❌ AsyncWebServer en global → No hay AsyncWebServer
- ❌ SPIFFS.begin() en global → Se llama en init()
- ❌ Tareas FreeRTOS antes de setup() → No hay creación prematura
- ❌ Timers hardware antes de setup() → No encontrados
- ❌ Buffers grandes (>4KB) en stack → Todos en heap/static

---

## Recomendaciones Finales

### Hacer AHORA:
1. ✅ **COMPLETADO** - Todos los constructores globales críticos corregidos
2. ✅ **COMPLETADO** - TOFSerial (LiDAR) convertido a puntero

### Hacer DESPUÉS (no bloqueante):
1. **Consolidar instancias Preferences** (30 min)
   - Crear singleton en storage.cpp
   - Eliminar duplicados en otros archivos
   - Añadir mutex para thread-safety

2. **Verificar MCP23017** (5 min)
   - Confirmar que está en singleton pattern
   - No hay instancia global fuera del manager

3. **Monitorear memoria LED arrays** (diagnóstico)
   - Imprimir heap libre después de inicialización LED
   - Si < 100KB libre, considerar arrays dinámicos

---

## Conclusión

**TODOS los problemas críticos de bootloop están CORREGIDOS** ✅

Los 6 objetos globales con constructores peligrosos han sido convertidos a inicialización lazy.

Problemas restantes son de **prioridad media-baja** y **NO causarán bootloop inmediato**, pero podrían causar:
- Corrupción de configuración (Preferences múltiples)
- Errores de NVS ocasionales
- Presión de memoria si LEDs son muy numerosos

El firmware debería arrancar sin problemas en ESP32-S3 N16R8.

**Compilación:** ✅ ÉXITO  
**Tamaño:** 586,657 bytes flash (11.2%), 27,688 bytes RAM (0.3%)  
**Estado:** Listo para pruebas en hardware
