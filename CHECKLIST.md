# ESP32-S3 Car Control System - Checklist de Verificación

## Versión: 2.11.1
## Fecha: 2025-12-19

---

## 🔎 Modo de arranque progresivo y validaciones críticas
- [x] Flag opcional `PROGRESSIVE_BRINGUP` para inicializar pantalla/HUD primero y pausar tras cada módulo (I2C, relés, sensores, entradas, control, seguridad, telemetría).
- [x] Autotest obliga palanca en **PARK** y pedal en reposo (<5%) antes de habilitar potencia.
- [x] Úsalo para añadir sensores y módulos uno a uno comprobando que cada paso está bien configurado.

---

## ✅ Directorio `include/` - Headers (61 archivos)

### Core System
- [x] `addresses.h` - Direcciones I2C y TCA9548A
- [x] `config_manager.h` - Gestión de configuración
- [x] `config_storage.h` - Almacenamiento de configuración (NVS)
- [x] `constants.h` - Constantes del sistema
- [x] `eeprom_persistence.h` - Persistencia EEPROM
- [x] `i2c_recovery.h` - Recuperación I2C
- [x] `logger.h` - Sistema de logging
- [x] `pins.h` - Definición de pines GPIO
- [x] `settings.h` - Configuraciones del usuario
- [x] `storage.h` - Almacenamiento genérico
- [x] `system.h` - Sistema principal
- [x] `watchdog.h` - Watchdog timer

### Display y HUD
- [x] `display_types.h` - Tipos de datos para display
- [x] `gauges.h` - Indicadores visuales
- [x] `hud.h` - Head-Up Display principal
- [x] `hud_manager.h` - Gestión de HUD
- [x] `icons.h` - Iconos del sistema
- [x] `touch_map.h` - Mapeo táctil
- [x] `wheels_display.h` - Visualización de ruedas
- [x] `obstacle_display.h` - Visualización de obstáculos

### Menús
- [x] `led_control_menu.h` - Menú control LED (clase)
- [x] `menu_auto_exit.h` - Auto-salida de menú
- [x] `menu_encoder_calibration.h` - Calibración encoder
- [x] `menu_hidden.h` - Menú oculto
- [x] `menu_ina226_monitor.h` - Monitor INA226
- [x] `menu_led_control.h` - Control LED (estático)
- [x] `menu_power_config.h` - Configuración de potencia
- [x] `menu_sensor_config.h` - Configuración de sensores
- [x] `menu_wifi_ota.h` - WiFi y OTA

### Sensores
- [x] `car_sensors.h` - Sensores del coche
- [x] `current.h` - Sensores de corriente (INA226)
- [x] `filters.h` - Filtros de señal
- [x] `obstacle_config.h` - Configuración obstáculos
- [x] `obstacle_detection.h` - Detección de obstáculos
- [x] `obstacle_logger.h` - Logger de obstáculos
- [x] `sensors.h` - Sensores genéricos
- [x] `temperature.h` - Sensores de temperatura (DS18B20)
- [x] `wheels.h` - Sensores de ruedas

### Control
- [x] `abs_system.h` - Sistema ABS
- [x] `adaptive_cruise.h` - Control crucero adaptativo
- [x] `obstacle_safety.h` - Seguridad obstáculos
- [x] `relays.h` - Control de relés
- [x] `regen_ai.h` - Regeneración IA
- [x] `steering.h` - Control de dirección
- [x] `steering_model.h` - Modelo de dirección
- [x] `steering_motor.h` - Motor de dirección
- [x] `tcs_system.h` - Sistema de control de tracción
- [x] `traction.h` - Control de tracción

### Input
- [x] `buttons.h` - Botones físicos
- [x] `pedal.h` - Pedal de acelerador
- [x] `shifter.h` - Palanca de cambios

### Audio
- [x] `alerts.h` - Sistema de alertas
- [x] `dfplayer.h` - Control DFPlayer
- [x] `queue.h` - Cola de audio

### Lighting
- [x] `led_controller.h` - Controlador LED WS2812B

### Comunicaciones
- [x] `bluetooth_controller.h` - Control Bluetooth
- [x] `telemetry.h` - Telemetría
- [x] `wifi_manager.h` - Gestión WiFi

### Utilidades
- [x] `debug.h` - Funciones de debug
- [x] `math_utils.h` - Utilidades matemáticas
- [x] `power_mgmt.h` - Gestión de energía

---

## ✅ Directorio `src/` - Implementaciones (54 archivos .cpp)

### src/audio/ (3 archivos)
- [x] `alerts.cpp` - Implementación alertas
- [x] `dfplayer.cpp` - Implementación DFPlayer
- [x] `queue.cpp` - Implementación cola audio

### src/control/ (5 archivos)
- [x] `adaptive_cruise.cpp` - Control crucero adaptativo
- [x] `relays.cpp` - Control de relés
- [x] `steering_model.cpp` - Modelo dirección
- [x] `steering_motor.cpp` - Motor dirección
- [x] `tcs_system.cpp` - Sistema TCS
- [x] `traction.cpp` - Control tracción

### src/core/ (11 archivos)
- [x] `bluetooth_controller.cpp` - Control Bluetooth
- [x] `config_manager.cpp` - Gestión configuración
- [x] `config_storage.cpp` - Almacenamiento config
- [x] `eeprom_persistence.cpp` - **[NUEVO v2.8.3]** Persistencia EEPROM
- [x] `i2c_recovery.cpp` - Recuperación I2C
- [x] `logger.cpp` - Sistema logging
- [x] `menu_ina226_monitor.cpp` - Monitor INA226
- [x] `storage.cpp` - Almacenamiento
- [x] `system.cpp` - Sistema principal
- [x] `telemetry.cpp` - Telemetría
- [x] `watchdog.cpp` - Watchdog
- [x] `wifi_manager.cpp` - Gestión WiFi

### src/hud/ (11 archivos)
- [x] `gauges.cpp` - Indicadores
- [x] `hud.cpp` - HUD principal
- [x] `hud_manager.cpp` - Gestión HUD
- [x] `icons.cpp` - Iconos
- [x] `led_control_menu.cpp` - **[NUEVO v2.8.3]** Menú LED (clase)
- [x] `menu_encoder_calibration.cpp` - **[NUEVO v2.8.3]** Calibración encoder
- [x] `menu_hidden.cpp` - Menú oculto
- [x] `menu_led_control.cpp` - **[NUEVO v2.8.3]** Control LED (estático)
- [x] `menu_power_config.cpp` - **[NUEVO v2.8.3]** Config potencia
- [x] `menu_sensor_config.cpp` - **[NUEVO v2.8.3]** Config sensores
- [x] `obstacle_display.cpp` - Display obstáculos
- [x] `touch_map.cpp` - Mapeo táctil
- [x] `wheels_display.cpp` - Display ruedas

### src/input/ (4 archivos)
- [x] `buttons.cpp` - Botones
- [x] `pedal.cpp` - Pedal
- [x] `shifter.cpp` - Shifter
- [x] `steering.cpp` - Dirección

### src/lighting/ (1 archivo)
- [x] `led_controller.cpp` - Controlador LED

### src/logging/ (1 archivo)
- [x] `obstacle_logger.cpp` - Logger obstáculos

### src/menu/ (3 archivos) - **[UNIFICADO v2.8.4]**
- [x] `menu_auto_exit.cpp` - Auto-salida
- [x] `menu_wifi_ota.cpp` - WiFi/OTA
- [x] `menu_obstacle_config.cpp` - **[NUEVO v2.8.4]** Config obstáculos (movido desde src/menus/)

### src/safety/ (3 archivos)
- [x] `abs_system.cpp` - Sistema ABS
- [x] `obstacle_safety.cpp` - Seguridad obstáculos
- [x] `regen_ai.cpp` - Regeneración IA

### src/sensors/ (6 archivos)
- [x] `car_sensors.cpp` - Sensores coche
- [x] `current.cpp` - Corriente
- [x] `obstacle_detection.cpp` - Detección obstáculos
- [x] `sensors.cpp` - Sensores
- [x] `temperature.cpp` - Temperatura
- [x] `wheels.cpp` - Ruedas

### src/system/ (1 archivo)
- [x] `power_mgmt.cpp` - Gestión energía

### src/utils/ (3 archivos)
- [x] `debug.cpp` - Debug
- [x] `filters.cpp` - Filtros
- [x] `math_utils.cpp` - Utilidades math

### Archivos raíz src/
- [x] `main.cpp` - Punto de entrada principal
- [x] `i2c.cpp` - Configuración I2C

---

## ✅ Verificación de Correspondencia Header ↔ Implementación

### Headers con implementación obligatoria ✅
| Header | Implementación | Estado |
|--------|---------------|--------|
| `abs_system.h` | `safety/abs_system.cpp` | ✅ |
| `adaptive_cruise.h` | `control/adaptive_cruise.cpp` | ✅ |
| `alerts.h` | `audio/alerts.cpp` | ✅ |
| `bluetooth_controller.h` | `core/bluetooth_controller.cpp` | ✅ |
| `buttons.h` | `input/buttons.cpp` | ✅ |
| `car_sensors.h` | `sensors/car_sensors.cpp` | ✅ |
| `config_manager.h` | `core/config_manager.cpp` | ✅ |
| `config_storage.h` | `core/config_storage.cpp` | ✅ |
| `current.h` | `sensors/current.cpp` | ✅ |
| `debug.h` | `utils/debug.cpp` | ✅ |
| `dfplayer.h` | `audio/dfplayer.cpp` | ✅ |
| `eeprom_persistence.h` | `core/eeprom_persistence.cpp` | ✅ |
| `filters.h` | `utils/filters.cpp` | ✅ |
| `gauges.h` | `hud/gauges.cpp` | ✅ |
| `hud.h` | `hud/hud.cpp` | ✅ |
| `hud_manager.h` | `hud/hud_manager.cpp` | ✅ |
| `i2c_recovery.h` | `core/i2c_recovery.cpp` | ✅ |
| `icons.h` | `hud/icons.cpp` | ✅ |
| `led_control_menu.h` | `hud/led_control_menu.cpp` | ✅ |
| `led_controller.h` | `lighting/led_controller.cpp` | ✅ |
| `logger.h` | `core/logger.cpp` | ✅ |
| `math_utils.h` | `utils/math_utils.cpp` | ✅ |
| `menu_auto_exit.h` | `menu/menu_auto_exit.cpp` | ✅ |
| `menu_encoder_calibration.h` | `hud/menu_encoder_calibration.cpp` | ✅ |
| `menu_hidden.h` | `hud/menu_hidden.cpp` | ✅ |
| `menu_ina226_monitor.h` | `core/menu_ina226_monitor.cpp` | ✅ |
| `menu_led_control.h` | `hud/menu_led_control.cpp` | ✅ |
| `menu_power_config.h` | `hud/menu_power_config.cpp` | ✅ |
| `menu_sensor_config.h` | `hud/menu_sensor_config.cpp` | ✅ |
| `menu_wifi_ota.h` | `menu/menu_wifi_ota.cpp` | ✅ |
| `obstacle_detection.h` | `sensors/obstacle_detection.cpp` | ✅ |
| `obstacle_display.h` | `hud/obstacle_display.cpp` | ✅ |
| `obstacle_logger.h` | `logging/obstacle_logger.cpp` | ✅ |
| `obstacle_safety.h` | `safety/obstacle_safety.cpp` | ✅ |
| `pedal.h` | `input/pedal.cpp` | ✅ |
| `power_mgmt.h` | `system/power_mgmt.cpp` | ✅ |
| `queue.h` | `audio/queue.cpp` | ✅ |
| `regen_ai.h` | `safety/regen_ai.cpp` | ✅ |
| `relays.h` | `control/relays.cpp` | ✅ |
| `sensors.h` | `sensors/sensors.cpp` | ✅ |
| `shifter.h` | `input/shifter.cpp` | ✅ |
| `steering.h` | `input/steering.cpp` | ✅ |
| `steering_model.h` | `control/steering_model.cpp` | ✅ |
| `steering_motor.h` | `control/steering_motor.cpp` | ✅ |
| `storage.h` | `core/storage.cpp` | ✅ |
| `system.h` | `core/system.cpp` | ✅ |
| `tcs_system.h` | `control/tcs_system.cpp` | ✅ |
| `telemetry.h` | `core/telemetry.cpp` | ✅ |
| `temperature.h` | `sensors/temperature.cpp` | ✅ |
| `touch_map.h` | `hud/touch_map.cpp` | ✅ |
| `traction.h` | `control/traction.cpp` | ✅ |
| `watchdog.h` | `core/watchdog.cpp` | ✅ |
| `wheels.h` | `sensors/wheels.cpp` | ✅ |
| `wheels_display.h` | `hud/wheels_display.cpp` | ✅ |
| `wifi_manager.h` | `core/wifi_manager.cpp` | ✅ |

### Headers solo de definiciones (sin .cpp necesario)
| Header | Descripción | Estado |
|--------|-------------|--------|
| `addresses.h` | Direcciones I2C (constexpr) | ✅ |
| `constants.h` | Constantes del sistema | ✅ |
| `display_types.h` | Tipos enumerados y estructuras | ✅ |
| `obstacle_config.h` | Configuración obstáculos | ✅ |
| `pins.h` | Definiciones GPIO | ✅ |
| `settings.h` | Configuraciones usuario | ✅ |

---

## ✅ Compilación y Build

| Métrica | Valor | Estado |
|---------|-------|--------|
| **Build Status** | SUCCESS | ✅ |
| **RAM Usage** | 17.3% (56,620 / 327,680 bytes) | ✅ |
| **Flash Usage** | 71.2% (932,857 / 1,310,720 bytes) | ✅ |
| **Tiempo de Build** | ~85 segundos | ✅ |

---

## ✅ Bibliotecas Externas (lib_deps)

| Biblioteca | Versión | Estado |
|-----------|---------|--------|
| TFT_eSPI | ^2.5.43 | ✅ |
| DFRobotDFPlayerMini | ^1.0.6 | ✅ |
| DallasTemperature | ^4.0.5 | ✅ |
| OneWire | ^2.3.8 | ✅ |
| Adafruit PWM Servo Driver | git | ✅ |
| INA226 | ^0.6.4 | ✅ |
| XPT2046_Touchscreen | git | ✅ |
| FastLED | 3.6.0 | ✅ |
| Adafruit MCP23017 | ^2.3.2 | ✅ |

---

## 📝 Notas de la versión 2.8.5 - Revisión Exhaustiva de Código

### 🔍 Resumen de Revisión Completa

**Archivos revisados:** 57 archivos .cpp + 61 archivos .h
**Estado del build:** ✅ SUCCESS (0 errores, 0 warnings críticos)
**RAM:** 17.3% (56,620 / 327,680 bytes)
**Flash:** 71.2% (933,161 / 1,310,720 bytes)

### ✅ Verificaciones realizadas por directorio:

#### src/audio/ (3 archivos)
- [x] `alerts.cpp` - Sistema de alertas con Audio::Priority
- [x] `dfplayer.cpp` - Control DFPlayer MP3
- [x] `queue.cpp` - Cola de audio no bloqueante

#### src/control/ (6 archivos)
- [x] `adaptive_cruise.cpp` - Control crucero adaptativo
- [x] `relays.cpp` - Secuencia no bloqueante de relés con mutex ESP32
- [x] `steering_model.cpp` - Modelo cinemático Ackermann
- [x] `steering_motor.cpp` - Control motor RS390
- [x] `tcs_system.cpp` - Sistema de control de tracción
- [x] `traction.cpp` - Control de tracción 4x4/4x2 con validaciones NaN

#### src/core/ (12 archivos)
- [x] `bluetooth_controller.cpp` - Control Bluetooth para emergencias
- [x] `config_manager.cpp` - Gestión de configuración
- [x] `config_storage.cpp` - Almacenamiento NVS
- [x] `eeprom_persistence.cpp` - Persistencia EEPROM con static_assert
- [x] `i2c_recovery.cpp` - Recuperación de bus I2C
- [x] `logger.cpp` - Sistema de logs con buffer seguro
- [x] `menu_ina226_monitor.cpp` - Monitor de sensores INA226
- [x] `storage.cpp` - Almacenamiento con validación de configuración
- [x] `system.cpp` - Sistema principal con selfTest()
- [x] `telemetry.cpp` - Sistema de telemetría
- [x] `watchdog.cpp` - Watchdog timer
- [x] `wifi_manager.cpp` - Gestión WiFi y OTA

#### src/hud/ (14 archivos)
- [x] `gauges.cpp` - Indicadores visuales optimizados
- [x] `hud.cpp` - HUD principal con layout adaptativo
- [x] `hud_manager.cpp` - Gestión de menús y estados
- [x] `icons.cpp` - Iconos en PROGMEM
- [x] `led_control_menu.cpp` - Menú LED con hueToRGB565()
- [x] `menu_encoder_calibration.cpp` - Calibración 3 pasos
- [x] `menu_hidden.cpp` - Menú oculto de diagnóstico
- [x] `menu_led_control.cpp` - Control LED estático
- [x] `menu_power_config.cpp` - Config potencia con coordenadas corregidas
- [x] `menu_sensor_config.cpp` - Config sensores con thresholds mejorados
- [x] `obstacle_display.cpp` - Visualización de obstáculos
- [x] `touch_map.cpp` - Mapeo táctil con calibración
- [x] `wheels_display.cpp` - Visualización de ruedas

#### src/input/ (4 archivos)
- [x] `buttons.cpp` - Botones con debounce y long-press
- [x] `pedal.cpp` - Lectura ADC con filtro EMA
- [x] `shifter.cpp` - Palanca de cambios vía MCP23017
- [x] `steering.cpp` - Encoder de dirección con calibración

#### src/safety/ (3 archivos)
- [x] `abs_system.cpp` - Sistema ABS con slip ratio
- [x] `obstacle_safety.cpp` - Seguridad de obstáculos
- [x] `regen_ai.cpp` - Regeneración adaptativa IA

#### src/sensors/ (6 archivos)
- [x] `car_sensors.cpp` - Lectura unificada de sensores
- [x] `current.cpp` - Sensores INA226 con validación
- [x] `obstacle_detection.cpp` - Detección VL53L5X
- [x] `sensors.cpp` - API unificada con diagnóstico
- [x] `temperature.cpp` - Sensores DS18B20
- [x] `wheels.cpp` - Encoders de rueda con ISR atómicas

### ✅ Patrones de Código Verificados

| Patrón | Estado | Descripción |
|--------|--------|-------------|
| Guards contra nullptr | ✅ | Verificado en Logger, Sensors, HUD |
| Validación NaN/Inf | ✅ | std::isfinite() en Traction, Steering |
| Clamp centralizado | ✅ | MathUtils::clamp() + clampf() local |
| Logs consistentes | ✅ | Logger::info/warn/error/debug uniforme |
| Secuencias de relés | ✅ | Non-blocking con timeout 5s |
| Watchdog | ✅ | Feed en cada loop() |
| ISR-safe flags | ✅ | portMUX_TYPE en Relays, noInterrupts() en Wheels |
| Constantes config | ✅ | settings.h + constants.h + cfg |

### 📋 TODOs Identificados (Mejoras Futuras)

| Archivo | TODO | Prioridad |
|---------|------|-----------|
| `car_sensors.cpp` | Implementar cálculo RPM desde encoders | Media |
| `car_sensors.cpp` | Sensor DS18B20 dedicado para controlador | Baja |
| `menu_wifi_ota.cpp` | Query GitHub releases para OTA | Media |
| `traction.cpp` | cfg.maxBatteryCurrentA configurable | Alta |
| `touch_map.cpp` | Calibración dinámica de touch | Media |
| `shifter.cpp` | Audio tracks específicos por marcha | Baja |

### 🔒 Seguridad y Robustez

- **Relays:** Secuencia Main→Trac→Dir con delays no bloqueantes
- **Emergency Stop:** ISR-safe con portMUX_TYPE para ESP32
- **Traction:** Validación de reparto anómalo (>115% esperado)
- **ABS/TCS:** Slip ratio con mínimo de velocidad
- **Sensores:** Timeouts configurables (SENSOR_TIMEOUT_MS)
- **I2C Recovery:** Sistema de recuperación de bus

---

## 📝 Notas de la versión 2.8.4

### Cambios de estructura:
- **Unificado src/menu/ y src/menus/**: El directorio `src/menus/` ha sido eliminado y su contenido movido a `src/menu/` para evitar confusión
- `menu_obstacle_config.cpp` - Implementación completa del menú de configuración de obstáculos

### Nuevas funcionalidades en menu_obstacle_config.cpp:
- Sliders para configurar distancias crítica/warning/caution
- Toggles para habilitar/deshabilitar sensores individuales (Front, Rear, Left, Right)
- Configuración de alertas de audio y visuales
- Botones de Save, Reset y Back
- Navegación por botones físicos y táctil

---

## 📝 Notas de la versión 2.8.3

### Nuevas implementaciones añadidas:
1. `eeprom_persistence.cpp` - Sistema de persistencia EEPROM completo
2. `led_control_menu.cpp` - Clase de menú control LED
3. `menu_encoder_calibration.cpp` - Calibración de encoder paso a paso
4. `menu_led_control.cpp` - Control LED estático con patrones
5. `menu_power_config.cpp` - Configuración de relés y tiempos
6. `menu_sensor_config.cpp` - Configuración de sensores on/off

### Correcciones aplicadas:
- LED default pattern: 0 → 1 (SOLID) para coherencia con enabled=true
- Integer overflow: casts a uint32_t en cálculos de slider
- Touch coordinates: alineadas con posiciones de dibujado
- Helper function: hueToRGB565() para eliminar duplicación
- Static assert: verificación de conteo de namespaces
- Warning thresholds: mejorados en sensor status bar
- Pattern names: estandarizados entre archivos LED

---

## ✅ Verificación Final

- [x] Todos los headers tienen implementación o son solo definiciones
- [x] El proyecto compila sin errores
- [x] El proyecto compila sin warnings críticos
- [x] RAM usage dentro de límites (<20%) - Actual: 17.3%
- [x] Flash usage dentro de límites (<80%) - Actual: 71.2%
- [x] Todas las dependencias resueltas
- [x] platformio.ini actualizado a v2.8.5
- [x] Directorios src/menu y src/menus unificados
- [x] Revisión exhaustiva de código completada
- [x] Patrones de seguridad verificados (nullptr, NaN, ISR-safe)
- [x] TODOs documentados y priorizados
- [x] Código listo para producción
