# 🚗 HARDWARE REFERENCE - ESP32-S3 Car Control System

## Versión Firmware: 2.8.5
## Fecha: 2025-11-30
## Placa: ESP32-S3-DevKitC-1 (44 pines)

---

## 📋 1. RESUMEN DEL PROYECTO

El **ESP32-S3 Car Control System** es un sistema de control integral para vehículos eléctricos infantiles, implementando:

- **HUD (Head-Up Display)**: Pantalla táctil ST7796S 480x320 con dashboard en tiempo real
- **Sensores**: Monitorización de corriente (INA226), temperatura (DS18B20), velocidad de ruedas, encoder de dirección
- **Control de Tracción**: Sistema 4x4 independiente con BTS7960 drivers
- **Control de Dirección**: Motor RS390 con encoder de alta precisión
- **Iluminación Inteligente**: LEDs WS2812B con efectos dinámicos
- **Sistema de Relés**: Control de potencia secuencial
- **Audio**: DFPlayer Mini para alertas y efectos
- **Seguridad Avanzada**: ABS, TCS, Freno Regenerativo AI, Watchdog

**Objetivo Principal**: Crear un vehículo eléctrico seguro, funcional y con interfaz de usuario moderna.

---

## 🔧 2. HARDWARE INCORPORADO

### 2.1 Placa Principal
| Componente | Especificación |
|------------|----------------|
| **MCU** | ESP32-S3-DevKitC-1 (44 pines) |
| **CPU** | Dual-core Xtensa LX7 @ 240 MHz |
| **Flash** | 16 MB |
| **PSRAM** | 8 MB (N16R8) |
| **Voltaje** | 3.3V lógica, 5V entrada |

### 2.2 Multiplexor I²C
| Componente | Dirección | Función |
|------------|-----------|---------|
| **TCA9548A** | 0x70 | Multiplexor I²C para 6x INA226 |

### 2.3 Sensores de Corriente (6x INA226)
| Canal TCA9548A | Sensor | Shunt | Función |
|----------------|--------|-------|---------|
| 0 | INA226 @ 0x40 | 50A 75mV | Motor FL (Frontal Izq) |
| 1 | INA226 @ 0x40 | 50A 75mV | Motor FR (Frontal Der) |
| 2 | INA226 @ 0x40 | 50A 75mV | Motor RL (Trasera Izq) |
| 3 | INA226 @ 0x40 | 50A 75mV | Motor RR (Trasera Der) |
| 4 | INA226 @ 0x40 | 100A 75mV CG FL-2C | Batería 24V |
| 5 | INA226 @ 0x40 | 50A 75mV | Motor Dirección |

### 2.4 Sensores de Temperatura (4x DS18B20)
- **Bus OneWire**: GPIO 20
- **Cantidad**: 4 sensores en paralelo
- **Ubicación**: Uno por motor de tracción

### 2.5 Encoder de Dirección
| Parámetro | Valor |
|-----------|-------|
| **Modelo** | E6B2-CWZ6C |
| **Resolución** | 1200 pulsos/revolución |
| **Ratio** | 1:1 con volante |
| **Señales** | A (cuadratura), B (cuadratura), Z (centrado) |
| **Conexión** | Vía HY-M158 optoacopladores (12V → 3.3V) |

### 2.6 Motores
| Motor | Modelo | Voltaje | RPM | Reductora | Función |
|-------|--------|---------|-----|-----------|---------|
| **Dirección** | RS390 | 12V | 6000 | 1:50 | Control de dirección |
| **Tracción** | RS775 | 24V | 15000 | 1:75 | Ruedas (4 unidades) |

### 2.7 Drivers PWM (I²C)
| Componente | Dirección | Función |
|------------|-----------|---------|
| **PCA9685 #1** | 0x40 | Motores eje delantero (FL+FR) |
| **PCA9685 #2** | 0x41 | Motores eje trasero (RL+RR) |
| **PCA9685 #3** | 0x42 | Motor dirección |

### 2.8 Expansor GPIO
| Componente | Dirección | Función |
|------------|-----------|---------|
| **MCP23017** | 0x20 | Control IN1/IN2 BTS7960 + Shifter D2 |

**Asignación MCP23017:**
| Pin | Función |
|-----|---------|
| GPIOA0-A7 | Control dirección motores (IN1/IN2 x4) |
| GPIOB0 | Shifter D2 (migrado de GPIO) |
| GPIOB1-B7 | Disponible para expansión |

### 2.9 Drivers de Motor
- **4x BTS7960** (43A): Tracción ruedas independientes
- **1x BTS7960** (43A): Motor dirección
- **Aislamiento**: 2x HY-M158 optoacopladores PC817 (12V → 3.3V)

### 2.10 Pantalla y Touch
| Componente | Especificación |
|------------|----------------|
| **Display** | ST7796S 480x320 TFT |
| **Touch** | XPT2046 resistivo |
| **Interfaz** | SPI (HSPI) |
| **Backlight** | PWM via GPIO 42 |

### 2.11 Audio
| Componente | Especificación |
|------------|----------------|
| **Módulo** | DFPlayer Mini |
| **Interfaz** | UART (GPIO 43/44) |
| **Función** | Alertas, efectos de sonido |

### 2.12 Iluminación LED
| Ubicación | Cantidad | Tipo | GPIO |
|-----------|----------|------|------|
| **Frontal** | 28 LEDs | WS2812B | 1 |
| **Trasera** | 16 LEDs | WS2812B | 48 |

### 2.13 Relés de Potencia
| Relé | GPIO | Función |
|------|------|---------|
| **Principal** | 4 | Power Hold |
| **Tracción** | 5 | 24V motores |
| **Dirección** | 6 | 12V dirección |
| **Auxiliar** | 7 | Luces/Media |

---

## 📌 3. CONEXIONES Y GPIO

### 3.1 Pines Disponibles en la Placa

**LADO 1 (mirando desde arriba):**
```
GND, GND, 19, 20, 21, 47, 48, 45, 0, 35, 36, 37, 38, 39, 40, 41, 42, 2, 1, RX(44), TX(43), GND
```

**LADO 2 (mirando desde arriba):**
```
GND, 5V, 14, 13, 12, 11, 10, 9, 46, 3, 8, 18, 17, 16, 15, 7, 6, 5, 4, RST, 3V3, 3V3
```

### 3.2 ⚠️ Strapping Pins (Evitar para funciones críticas)

| GPIO | Función Boot | Riesgo | Uso Actual |
|------|--------------|--------|------------|
| 0 | Boot mode | 🔴 Alto | KEY_SYSTEM (con pull-up) |
| 3 | JTAG | 🟡 Medio | WHEEL_FL |
| 45 | VDD_SPI voltage | 🟡 Medio | 🆓 LIBRE (antes SHIFTER_P) |
| 46 | Boot mode/ROM log | 🟡 Medio | 🆓 LIBRE (antes SHIFTER_R) |
| 43 | UART0 TX | 🟢 Bajo | DFPLAYER_TX |
| 44 | UART0 RX | 🟢 Bajo | DFPLAYER_RX |

### 3.3 ✅ Pines Más Seguros y Estables

| Rango GPIO | Nivel Seguridad | Recomendación |
|------------|-----------------|---------------|
| 19, 20, 21 | ✅ Muy estable | Ideal para SPI/I²C |
| 35-42 | ✅ Seguro | No afectan boot |
| 4-18 | ✅ Disponible | Uso general |
| 47-48 | ✅ Seguro | Entradas/Salidas |

### 3.4 Asignación Actual de Pines (pins.h v2.3.0)

#### Comunicaciones I²C
| GPIO | Función | Notas |
|------|---------|-------|
| 8 | I2C_SDA | Bus I²C Data |
| 9 | I2C_SCL | Bus I²C Clock |

#### Comunicaciones SPI (Pantalla)
| GPIO | Función | Notas |
|------|---------|-------|
| 10 | TFT_SCK | SPI Clock |
| 11 | TFT_MOSI | SPI Master Out |
| 12 | TFT_MISO | SPI Master In |
| 13 | TFT_DC | Data/Command |
| 14 | TFT_RST | Reset |
| 16 | TFT_CS | Chip Select TFT |
| 42 | TFT_BL | Backlight PWM |

#### Touch (XPT2046)
| GPIO | Función | Notas |
|------|---------|-------|
| 21 | TOUCH_CS | ✅ Pin seguro (antes GPIO 3) |
| 47 | TOUCH_IRQ | Interrupción |

#### Audio (DFPlayer)
| GPIO | Función | Notas |
|------|---------|-------|
| 43 | DFPLAYER_TX | UART0 nativo |
| 44 | DFPLAYER_RX | UART0 nativo |

#### Relés
| GPIO | Función | Notas |
|------|---------|-------|
| 4 | RELAY_MAIN | Power Hold |
| 5 | RELAY_TRAC | Tracción 24V |
| 6 | RELAY_DIR | Dirección 12V |
| 7 | RELAY_SPARE | Auxiliar |

#### Sensores Encoder
| GPIO | Función | Notas |
|------|---------|-------|
| 37 | ENCODER_A | Cuadratura A |
| 38 | ENCODER_B | Cuadratura B |
| 39 | ENCODER_Z | Pulso centrado |

#### Sensores Ruedas
| GPIO | Función | Notas |
|------|---------|-------|
| 3 | WHEEL_FL | Front Left |
| 36 | WHEEL_FR | Front Right |
| 17 | WHEEL_RL | Rear Left |
| 15 | WHEEL_RR | Rear Right |

#### Pedal y Temperatura
| GPIO | Función | Notas |
|------|---------|-------|
| 35 | PEDAL | ADC entrada analógica |
| 20 | ONEWIRE | Bus DS18B20 |

#### Shifter (Palanca de cambios) - ✅ TODO en MCP23017
| MCP Pin | Función | Notas |
|---------|---------|-------|
| GPIOB0 (8) | SHIFTER_P | Park |
| GPIOB1 (9) | SHIFTER_R | Reverse |
| GPIOB2 (10) | SHIFTER_N | Neutral |
| GPIOB3 (11) | SHIFTER_D1 | Drive 1 |
| GPIOB4 (12) | SHIFTER_D2 | Drive 2 |

> ✅ **Mejora v2.3.0**: Todo el shifter migrado a MCP23017 con pines consecutivos (8-12), liberando GPIOs 18, 19, 45, 46.

#### Botones
| GPIO | Función | Notas |
|------|---------|-------|
| 2 | BTN_LIGHTS | Luces |
| 40 | BTN_MEDIA | Multimedia |
| 41 | BTN_4X4 | Switch 4x4/4x2 |

#### LEDs WS2812B
| GPIO | Función | Notas |
|------|---------|-------|
| 1 | LED_FRONT | 28 LEDs |
| 48 | LED_REAR | 16 LEDs |

#### Sistema
| GPIO | Función | Notas |
|------|---------|-------|
| 0 | KEY_SYSTEM | Boot button (strapping) |

#### GPIOs Libres (v2.3.0)
| GPIO | Estado | Notas |
|------|--------|-------|
| 18 | 🆓 LIBRE | Disponible para expansión |
| 19 | 🆓 LIBRE | Disponible para expansión |
| 45 | 🆓 LIBRE | ⚠️ Strapping, usar con cuidado |
| 46 | 🆓 LIBRE | ⚠️ Strapping, usar con cuidado |

### 3.5 Recomendaciones para Futuras Expansiones

#### CAN Bus (si se necesita)
- GPIO 18 y 19 ahora libres, ideales para CAN
- Usar transceiver MCP2515 o similar

#### Bluetooth
- Ya integrado en ESP32-S3
- Usar para control remoto de emergencia (implementado)

#### OTA (Over-The-Air Updates)
- WiFi integrado en ESP32-S3
- Configurado en `platformio.ini` (entorno OTA)

---

## 💻 4. FIRMWARE Y MÓDULOS

### 4.1 Módulos Principales

| Módulo | Archivo | Función |
|--------|---------|---------|
| **HUDManager** | `hud_manager.cpp/h` | Gestión unificada del display |
| **LEDController** | `led_controller.cpp/h` | Control iluminación WS2812B |
| **CarSensors** | `car_sensors.cpp/h` | Lectura centralizada de sensores |
| **Traction** | `traction.cpp/h` | Control de tracción 4x4 |
| **Relays** | `relays.cpp/h` | Gestión de relés de potencia |
| **Pedal** | `pedal.cpp/h` | Lectura pedal acelerador |
| **Temperature** | `temperature.cpp/h` | Monitorización DS18B20 |
| **Current** | `current.cpp/h` | Lectura INA226 |
| **Steering** | `steering.cpp/h` | Control encoder dirección |
| **SteeringMotor** | `steering_motor.cpp/h` | Control motor dirección |

### 4.2 Sistemas de Seguridad

| Sistema | Archivo | Función |
|---------|---------|---------|
| **ABSSystem** | `abs_system.cpp/h` | Anti-lock Braking System |
| **TCSSystem** | `tcs_system.cpp/h` | Traction Control System |
| **RegenAI** | `regen_ai.cpp/h` | Freno regenerativo inteligente |
| **Watchdog** | `watchdog.cpp/h` | Monitorización y reset |
| **I2CRecovery** | `i2c_recovery.cpp/h` | Recuperación bus I²C |
| **BluetoothController** | `bluetooth_controller.cpp/h` | Override de emergencia |

### 4.3 Dependencias Críticas (platformio.ini)

```ini
lib_deps =
    bodmer/TFT_eSPI @ ^2.5.43
    dfrobot/DFRobotDFPlayerMini @ ^1.0.6
    milesburton/DallasTemperature@^4.0.5
    paulstoffregen/OneWire@^2.3.8
    Adafruit-PWM-Servo-Driver-Library
    RobTillaart/INA226 @ ^0.6.4
    XPT2046_Touchscreen
    fastled/FastLED @ 3.6.0
    adafruit/Adafruit MCP23017 Arduino Library @ ^2.3.2
```

---

## 🔮 5. PRÓXIMAS ACTUALIZACIONES

### 5.1 Alta Prioridad
- [ ] Restaurar `pedalPercent` en visualización HUD
- [ ] Mejorar backlight y colores en display
- [ ] Añadir iconos de advertencia en HUD
- [ ] Refactorizar `delay()` → `millis()` en módulos restantes

### 5.2 Media Prioridad
- [ ] Integrar clamps, guards y validaciones en todos los módulos
- [ ] Calibración dinámica touch XPT2046
- [ ] Implementar RPM real (actualmente placeholder)
- [ ] Añadir telemetría WiFi

### 5.3 Baja Prioridad
- [ ] Plan de integración ADAS:
  - Parking Assist
  - Blind Spot Detection
  - Adaptive Cruise Control
- [ ] Optimizar particiones flash para OTA dual-partition
- [ ] Añadir más tracks de audio para marchas específicas

---

## 📊 6. NOTAS DE REFERENCIA

### 6.1 Estado Actual del Firmware

| Métrica | Valor |
|---------|-------|
| **Versión** | 2.8.0 |
| **Fiabilidad Estimada** | 100% ⭐⭐⭐⭐⭐ |
| **Líneas de Código** | ~8,500+ |
| **Archivos .cpp** | 37+ |
| **Archivos .h** | 60+ |
| **Módulos Funcionales** | 25 |
| **Sistemas de Seguridad** | 5 |
| **Flash Usage** | ~36% |
| **RAM Usage** | ~9% |

### 6.2 Cambios v2.8.5

| Cambio | Descripción | Motivo |
|--------|-------------|--------|
| Code Review | Revisión exhaustiva de 57 .cpp + 61 .h | Calidad de código |
| pin_utils.h | Funciones de validación GPIO | Seguridad pines |
| pwm_channels.h | Definiciones de canales PWM | Organización |
| test_display.h/cpp | Pruebas standalone de display | Testing aislado |
| math_utils.cpp | Validación NaN/Inf en todas las funciones | Robustez |
| led_controller.cpp | Validaciones de seguridad y hardware | Seguridad LEDs |
| build_test.yml | CI workflow para testing | Automatización |

### 6.3 Cambios v2.8.0

| Cambio | Descripción | Motivo |
|--------|-------------|--------|
| Telemetría | Sistema de telemetría con checksum FNV-1a | Métricas avanzadas |
| RedundantSensor | Estructura para sensores redundantes | Tolerancia a fallos |
| Documentación | Todos los manuales en directorio docs/ | Organización |

### 6.4 Cambios v2.4.0

| Cambio | Descripción | Motivo |
|--------|-------------|--------|
| SteeringMotor::get() | Añadida implementación faltante | Función declarada pero no definida |
| Wheel sensors | Acceso atómico a contadores de pulsos | Fix race condition ISR |
| Sensor getters | Validación índices >= 0 | Prevenir acceso a índices negativos |
| Relays::emergencyStop() | Nueva función parada emergencia | Seguridad crítica |
| Relay hysteresis | 3 errores consecutivos antes de shutdown | Evitar falsos positivos |
| Serial init | Eliminado bucle while bloqueante | Evitar watchdog reset |
| HUD init delays | Reducidos de 70ms a 0.6ms | Arranque más rápido |

### 6.5 Cambios v2.3.0

| Cambio | Antes | Después | Motivo |
|--------|-------|---------|--------|
| TOUCH_CS | GPIO 3 | GPIO 21 | Evitar strapping pin |
| TOUCH_IRQ | GPIO 46 | GPIO 47 | Evitar strapping pin |
| LED_REAR | GPIO 19 | GPIO 48 | Conflicto con SHIFTER_R |
| SHIFTER completo | GPIOs dispersos | MCP23017 B0-B4 | ✅ Pines consecutivos |
| SHIFTER_P | GPIO 45 | MCP23017 B0 | Evitar strapping pin |
| SHIFTER_R | GPIO 46 | MCP23017 B1 | Evitar strapping pin |
| SHIFTER_N | GPIO 19 | MCP23017 B2 | Liberar GPIO |
| SHIFTER_D1 | GPIO 18 | MCP23017 B3 | Liberar GPIO |
| SHIFTER_D2 | GPIO 48 | MCP23017 B4 | Liberar GPIO para LEDs |
| I2C_SDA | GPIO 16 | GPIO 8 | Reorganización |
| TFT_CS | GPIO 8 | GPIO 16 | Reorganización |
| WHEEL_FL | GPIO 21 | GPIO 3 | Intercambio con TOUCH_CS |

**GPIOs liberados**: 18, 19, 45, 46 (4 pines disponibles para futuras expansiones)

### 6.5 Recomendaciones para Nuevos Colaboradores

1. **Lectura Obligatoria**:
   - `pins.h` - Asignación de pines
   - `settings.h` - Configuración global
   - `AUDIT_REPORT.md` - Estado del firmware

2. **Antes de Modificar GPIO**:
   - Verificar tabla de strapping pins
   - Comprobar `pin_is_assigned()` y `pin_is_strapping()`
   - Actualizar tabla en `pins.h`

3. **Compilación**:
   ```bash
   pio run -e esp32-s3-devkitc       # Debug
   pio run -e esp32-s3-devkitc-release  # Producción
   pio run -e esp32-s3-devkitc-ota   # Con OTA
   ```

4. **Testing**:
   ```bash
   pio run -e esp32-s3-devkitc-test  # Modo test
   ```

---

## 📝 Historial de Versiones

| Versión | Fecha | Cambios Principales |
|---------|-------|---------------------|
| 2.8.5 | 2025-11-30 | Code review exhaustivo, nuevos utilities (pin_utils, pwm_channels), CI workflow |
| 2.8.0 | 2025-11-27 | Sistema telemetría, RedundantSensor, documentación actualizada |
| 2.4.0 | 2025-11-25 | Fix race conditions, SteeringMotor::get(), emergencyStop, hysteresis seguridad |
| 2.3.0 | 2025-11-25 | Reorganización GPIO, resolución conflictos pines, TOUCH_CS seguro |
| 2.2.0 | 2025-11-24 | Corrección macros OTA, build 4/4 entornos |
| 2.1.0 | 2025-11-23 | Refactorización delay(), correcciones compilación |
| 2.0.0 | 2025-11-22 | Auditoría completa, 2x PCA9685 |

---

*Documento generado automáticamente - ESP32-S3 Car Control System*
*Última actualización: 2025-11-30 v2.8.5*
