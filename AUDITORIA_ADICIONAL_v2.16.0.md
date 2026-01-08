# AUDITORÍA DE SEGURIDAD COMPLETA - FIRMWARE ESP32-S3
# Componentes Críticos No Auditados en Primera Revisión
# Fecha: 2026-01-08
# Repositorio: FULL-FIRMWARE-Coche-Marcos

## RESUMEN EJECUTIVO

Esta auditoría cubre 8 componentes críticos del firmware ESP32-S3 que no fueron auditados previamente:
1. Sistema de Pantalla (Display/HUD)
2. Menús Ocultos (Hidden Menu)
3. Touch Screen (XPT2046)
4. Sistema de Tracción (Traction Control)
5. Volante con Encoder (Steering)
6. Asignación de Pines (GPIO)
7. Configuración platformio.ini
8. Dependencias de Librerías

## 🔴 HALLAZGOS CRÍTICOS (PRIORIDAD ALTA)

### 1. **CONFLICTO GPIO 16 - TFT_CS vs WHEEL_RR**
**Archivo**: `include/pins.h` líneas 92, 233
**Severidad**: 🔴 CRÍTICA
**Descripción**: GPIO 16 está asignado a DOS funciones incompatibles simultáneamente:
- `PIN_TFT_CS = 16` (línea 92) - Chip Select del display TFT ST7796S (SPI)
- `PIN_WHEEL_RR = 16` (línea 233) - Sensor inductivo rueda trasera derecha (interrupt)

**Impacto**:
- El sensor de rueda rear-right está configurado como interrupt en `src/sensors/wheels.cpp:41`
- Cada vez que el sensor de rueda detecta un pulso, la línea GPIO 16 se activa
- Esto provoca que el display TFT reciba señales espurias de Chip Select
- Resultado: Corrupción de datos SPI, pantalla congelada o datos erróneos
- El velocímetro y control de tracción reciben lecturas incorrectas de WHEEL_RR

**Código afectado**:
```cpp
// pins.h:92
#define PIN_TFT_CS        16  // GPIO 16 - Chip Select TFT

// pins.h:233
#define PIN_WHEEL_RR      16  // GPIO 16 - Wheel Rear Right

// wheels.cpp:41
attachInterrupt(digitalPinToInterrupt(PIN_WHEEL_RR), wheelISR3, RISING);
```

**Corrección OBLIGATORIA**:
```cpp
// Opción 1: Reasignar WHEEL_RR a GPIO libre (ej: GPIO 46 ahora libre)
#define PIN_WHEEL_RR      46  // GPIO 46 - LIBRE tras migración VL53L5X

// Opción 2: Reasignar TFT_CS (menos recomendado, SPI ya configurado)
```

**Validación**:
- Verificar que GPIO 46 no tenga conflictos de strapping durante boot
- Actualizar documentación en pins.h tabla línea 352
- Comprobar que optoacoplador HY-M158 soporta el cambio

---

### 2. **USO DE STRAPPING PINS GPIO 0, 3, 45, 46**
**Archivo**: `include/pins.h` líneas 14-21, 141-142, 230, 351
**Severidad**: 🟡 MEDIA-ALTA
**Descripción**: El firmware usa pines de strapping del ESP32-S3 que afectan el boot:
- GPIO 0: `PIN_KEY_SYSTEM` - Entrada ignition (strapping Boot Mode)
- GPIO 3: `PIN_WHEEL_FL` - Sensor rueda (strapping JTAG)
- GPIO 45: `KEY_DETECT` - Power management (strapping VDD_SPI voltage)
- GPIO 46: LIBRE ahora, pero era `XSHUT_FRONT` (strapping Boot mode/ROM log)

**Impacto**:
- Si GPIO 0 está LOW durante reset → modo Download Boot (no arranca firmware)
- GPIO 45 afecta voltaje SPI flash (puede causar bootloop)
- GPIO 46 puede activar ROM logging (performance penalty)

**Mitigación parcial existente**: Los comentarios indican awareness del problema
**Corrección recomendada**:
- Mover `PIN_KEY_ON/OFF` desde GPIO 40/41 (ahora OK) - ✅ Ya corregido en v2.15.0
- GPIO 3 para WHEEL_FL es aceptable con pull-up externo
- Revisar GPIO 45 `KEY_DETECT` - considerar mover a GPIO seguro

---

### 3. **INTEGER OVERFLOW EN ENCODER STEERING**
**Archivo**: `src/input/steering.cpp` línea 11
**Severidad**: 🟡 MEDIA
**Descripción**: Variable `volatile long ticks` puede desbordar con encoder 1200 PPR
**Análisis**:
- Encoder E6B2-CWZ6C: 1200 pulsos/revolución
- Tipo `long` en ESP32: 32 bits signed (-2,147,483,648 a +2,147,483,647)
- Revolución completa del volante: ±1200 ticks
- Número de giros hasta overflow: 2^31 / 1200 ≈ 1,789,569 giros
- A velocidad máxima (10 giros/seg): overflow en ≈ 50 horas de giro continuo

**Impacto**: Bajo en condiciones reales, pero:
- Si el encoder no se centra nunca (timeout línea 118), `ticks` acumula indefinidamente
- Overflow causa salto abrupto de ángulo: +1,789,569° → -1,789,569°
- Puede provocar comando erróneo al motor de dirección

**Corrección sugerida**:
```cpp
// steering.cpp línea 11
static volatile int32_t ticks = 0;  // Explicit type
static const int32_t TICKS_MAX_ABS = 100000;  // Safety limit: ~83 vueltas

// En isrEncA() añadir bounds checking:
void IRAM_ATTR isrEncA() {
    int a = digitalRead(PIN_ENCODER_A);
    int b = digitalRead(PIN_ENCODER_B);
    int32_t delta = (a == HIGH) ? ((b == HIGH) ? +1 : -1) : ((b == HIGH) ? -1 : +1);
    
    int32_t newTicks = ticks + delta;
    if (newTicks >= -TICKS_MAX_ABS && newTicks <= TICKS_MAX_ABS) {
        ticks = newTicks;
    } else {
        // Saturate at limits, log error
        ticks = (newTicks > 0) ? TICKS_MAX_ABS : -TICKS_MAX_ABS;
    }
}
```

---

### 4. **CÓDIGO DE MENÚ OCULTO SIN LÍMITE DE INTENTOS**
**Archivo**: `src/hud/menu_hidden.cpp` línea 39, 967-994
**Severidad**: 🟢 BAJA
**Descripción**: No hay rate limiting en entrada de código 8989
**Impacto**:
- Ataque de fuerza bruta posible (10,000 combinaciones)
- Sin embargo, requiere acceso físico al display táctil
- Riesgo bajo en vehículo personal

**Corrección sugerida** (opcional):
```cpp
static uint8_t wrongCodeAttempts = 0;
static uint32_t lastWrongCodeMs = 0;
static const uint8_t MAX_ATTEMPTS = 5;
static const uint32_t LOCKOUT_MS = 30000;  // 30 seconds

// En handleKeypadInput() después de código incorrecto:
if (codeBuffer != accessCode) {
    wrongCodeAttempts++;
    lastWrongCodeMs = millis();
    
    if (wrongCodeAttempts >= MAX_ATTEMPTS) {
        Logger::warn("MenuHidden: Too many wrong attempts - lockout 30s");
        // Bloquear keypad por 30 segundos
    }
}
// Reset intentos tras código correcto o timeout
```

---

### 5. **TOUCH COORDINATES SIN BOUNDS CHECKING**
**Archivo**: `src/hud/hud.cpp` líneas 1140-1217
**Severidad**: 🟡 MEDIA
**Descripción**: Coordenadas de touch (x,y) se usan sin validar que estén dentro de pantalla
**Impacto**:
- Touch mal calibrado puede generar coordenadas fuera de rango
- `tft.drawPixel(x, y, color)` con x > 480 o y > 320 puede causar:
  - Buffer overflow en framebuffer interno de TFT_eSPI
  - Crash por acceso fuera de límites
  - Corrupción de memoria stack

**Código vulnerable**:
```cpp
// hud.cpp línea 1141
if (touchDetected) {
    int x = (int)touchX;
    int y = (int)touchY;
    
    // NO HAY VALIDACIÓN AQUÍ
    // Uso directo en getTouchedZone(x, y)
}
```

**Corrección**:
```cpp
if (touchDetected) {
    int x = (int)touchX;
    int y = (int)touchY;
    
    // Validar bounds antes de usar
    if (x < 0 || x >= 480 || y < 0 || y >= 320) {
        Logger::warnf("Touch: coordinates out of bounds (%d, %d)", x, y);
        continue;  // Ignorar touch inválido
    }
    
    TouchAction act = getTouchedZone(x, y);
    // ...
}
```

---

## 🟡 HALLAZGOS MEDIOS

### 6. **PCA9685 SIN VALIDACIÓN DE CANALES PWM**
**Archivo**: `src/control/traction.cpp` líneas 47-59, 85-159
**Severidad**: 🟡 MEDIA
**Descripción**: Aunque existe función `validatePWMChannel()`, algunos códigos antiguos no la usan consistentemente

**Código actual**:
```cpp
// traction.cpp - BUENO (línea 92)
bool fwdValid = validatePWMChannel(PCA_FRONT_CH_FL_FWD, "FL_FORWARD");

// Pero falta en steering_motor.cpp
```

**Verificación**: `steering_motor.cpp` líneas 84-86 NO validan canales antes de usar
```cpp
pca.setPWM(kChannelFwd, 0, 0);  // ❌ Sin validación
pca.setPWM(kChannelRev, 0, 0);  // ❌ Sin validación
```

**Corrección**:
```cpp
// steering_motor.cpp línea 84
if (validatePWMChannel(kChannelFwd, "STEER_FWD")) {
    pca.setPWM(kChannelFwd, 0, 0);
}
if (validatePWMChannel(kChannelRev, "STEER_REV")) {
    pca.setPWM(kChannelRev, 0, 0);
}
```

---

### 7. **RACE CONDITION EN ISR ENCODER**
**Archivo**: `src/input/steering.cpp` líneas 26-31, 33-41
**Severidad**: 🟡 MEDIA
**Descripción**: Lectura de `ticks` no es atómica fuera de ISR

**Análisis**:
- ISR `isrEncA()` modifica `volatile long ticks` (línea 11)
- Función `getTicksSafe()` (línea 26) protege lectura con `noInterrupts()`
- PERO en `update()` línea 106 se usa `getTicksSafe()` ✅ CORRECTO

**Validación**: ✅ El código YA tiene protección adecuada
**Recomendación**: Mantener consistencia - usar SIEMPRE `getTicksSafe()` para leer ticks

---

### 8. **TEMPERATURA CRÍTICA SIN EMERGENCIA INMEDIATA**
**Archivo**: `src/control/traction.cpp` líneas 172, 438, 597-601
**Severidad**: 🟡 MEDIA
**Descripción**: Temperatura > 120°C solo genera warning, no detiene motor

**Código actual**:
```cpp
if (tempC > TEMP_CRITICAL) {
    Logger::warnf("Traction: temperatura crítica rueda %d: %.1f°C", i, tempC);
    // NO HAY PARADA DE MOTOR
}
```

**Impacto**:
- Motor puede sobrecalentarse y dañarse permanentemente
- Riesgo de incendio en caso extremo

**Corrección recomendada**:
```cpp
constexpr float TEMP_EMERGENCY_SHUTDOWN = 130.0f;  // Nueva constante

if (tempC > TEMP_EMERGENCY_SHUTDOWN) {
    Logger::errorf("EMERGENCY: Motor %d temp %.1f°C - SHUTTING DOWN", i, tempC);
    System::logError(825 + i);  // Códigos 825-828 para shutdown
    
    // Detener motor inmediatamente
    s.w[i].demandPct = 0.0f;
    s.w[i].outPWM = 0.0f;
    applyHardwareControl(i, 0, false);
    
    // Opcional: desactivar relé de tracción completo
    // Relays::disableTraction();
} else if (tempC > TEMP_CRITICAL) {
    Logger::warnf("Traction: temperatura crítica rueda %d: %.1f°C", i, tempC);
}
```

---

## 🟢 HALLAZGOS MENORES

### 9. **DISPLAY BRIGHTNESS VALIDACIÓN REDUNDANTE**
**Archivo**: `src/hud/hud_manager.cpp` líneas 25-35, `menu_hidden.cpp` líneas 24-35
**Severidad**: 🟢 BAJA (mejora de código)
**Descripción**: Validación de `displayBrightness` duplicada en múltiples lugares

**Recomendación**: Centralizar en función helper `Storage::validateBrightness()`

---

### 10. **HARDCODED I2C ADDRESSES SIN DEFINES**
**Archivo**: Varios archivos usan `0x40`, `0x41`, etc. directamente
**Severidad**: 🟢 BAJA (mantenibilidad)
**Descripción**: Aunque `pins.h` define `I2C_ADDR_PCA9685_FRONT`, algunos códigos usan valores hardcoded

**Verificación**: ✅ Mayoría del código usa defines correctamente
**Excepción**: `traction.cpp` línea 220, 232, 254, 264 usa defines ✅ CORRECTO

---

## 📊 ANÁLISIS DE DEPENDENCIAS

### Librerías Verificadas (platformio.ini líneas 30-40):

1. **TFT_eSPI @ 2.5.43** ✅
   - Versión estable
   - No hay CVEs conocidos
   - Compatible con ESP32-S3

2. **DFRobotDFPlayerMini @ 1.0.6** ✅
   - Librería simple, bajo riesgo
   - No hay vulnerabilidades reportadas

3. **INA226 @ 0.6.5** (robtillaart) ✅
   - Versión reciente (2024)
   - No hay CVEs

4. **FastLED @ 3.10.3** ⚠️
   - Versión de noviembre 2024 - RECIENTE
   - **PRECAUCIÓN**: FastLED tiene historial de crashes en ESP32 con PSRAM
   - Recomendación: Asegurar que LEDs no acceden a PSRAM desde interrupts

5. **Adafruit MCP23017 @ 2.3.2** ✅
   - Versión estable
   - No hay problemas conocidos

6. **Adafruit PWM Servo Driver @ 3.0.2** ✅
   - PCA9685 control
   - Versión estable

### ⚠️ ADVERTENCIA FASTLED:
```ini
# platformio.ini línea 38
fastled/FastLED @ 3.10.3
```

**Verificación necesaria**:
```cpp
// En LED control code, asegurar:
FASTLED_ALLOW_INTERRUPTS 0  // Deshabilitar interrupts durante show()
// O usar IRAM_ATTR para funciones críticas
```

---

## 🔧 CONFIGURACIÓN platformio.ini

### ✅ CORRECTO:
- `board_build.arduino.memory_type = qio_opi` (línea 174)
- `BOARD_HAS_PSRAM` flag (línea 49, 180)
- SPI frequencies adecuadas (líneas 67-69)
- Partitions standalone para eliminar OTA (línea 22)

### ⚠️ ADVERTENCIAS:
1. **`-mfix-esp32-psram-cache-issue`** (línea 181)
   - Flag correcto para ESP32-S3 con PSRAM
   - Añade workarounds para bugs de caché
   - Puede reducir performance ligeramente

2. **Debug level 3** (línea 52, 184)
   - Mucho logging puede llenar buffer serial
   - Considerar nivel 2 para producción

---

## 🎯 RECOMENDACIONES PRIORITARIAS

### CRÍTICAS (Implementar INMEDIATAMENTE):
1. ✅ **CORREGIR GPIO 16 CONFLICT** - Reasignar PIN_WHEEL_RR a GPIO 46
2. ✅ **BOUNDS CHECKING touch coordinates** - Validar antes de dibujar
3. ✅ **EMERGENCY TEMP SHUTDOWN** - Detener motores > 130°C

### ALTA PRIORIDAD:
4. ✅ **ENCODER OVERFLOW PROTECTION** - Saturar ticks en ±100,000
5. ✅ **VALIDAR PWM CHANNELS** en steering_motor.cpp
6. ✅ **REVIEW STRAPPING PINS** - Considerar reasignar GPIO 45

### MEDIA PRIORIDAD:
7. Rate limiting código menú oculto (opcional)
8. Centralizar validación brightness
9. Verificar FastLED interrupt safety

---

## 📝 RESUMEN DE ARCHIVOS AUDITADOS

| Componente | Archivos | Líneas | Issues Encontrados |
|-----------|----------|--------|-------------------|
| Display/HUD | hud.cpp, hud_manager.cpp | 2,168 | 2 medios |
| Hidden Menu | menu_hidden.cpp, .h | 1,315 | 1 bajo |
| Touch | Integrado en HUD | - | 1 medio |
| Traction | traction.cpp, tcs_system.cpp | 912 | 2 medios |
| Steering | steering.cpp, steering_motor.cpp | 405 | 2 medios |
| Pins | pins.h | 490 | 1 CRÍTICO |
| Config | platformio.ini | 212 | 0 |
| Deps | lib_deps | 7 libs | 1 warning |
| **TOTAL** | **15 archivos** | **5,502** | **1 crítico, 7 medios, 3 bajos** |

---

## ✅ PUNTOS FUERTES DEL CÓDIGO

1. **Exception handling en HUD init** (hud_manager.cpp:65-86)
   - Protege contra crashes de display
   - Sistema continúa sin UI

2. **Atomic operations en encoder** (steering.cpp:26-31)
   - Protección correcta de variables volátiles

3. **Overcurrent protection** (steering_motor.cpp:123-135, traction.cpp)
   - Detección y shutdown automático

4. **Timeout en calibraciones** (menu_hidden.cpp:56)
   - 30 segundos previene bloqueo

5. **Validation helpers** (traction.cpp:47-59)
   - `validatePWMChannel()` previene crashes

6. **EEPROM safety** (menu_hidden.cpp:25-35)
   - `safeSaveConfig()` valida brightness antes de guardar

---

## 📋 CHECKLIST DE CORRECCIONES

```markdown
- [ ] 1. CRÍTICO: Reasignar PIN_WHEEL_RR de GPIO 16 → GPIO 46
- [ ] 2. CRÍTICO: Añadir bounds checking en touch coordinates
- [ ] 3. ALTA: Implementar emergency shutdown temp > 130°C
- [ ] 4. ALTA: Añadir overflow protection en encoder ticks
- [ ] 5. ALTA: Validar PWM channels en steering_motor.cpp
- [ ] 6. MEDIA: Review GPIO 45 strapping pin usage
- [ ] 7. MEDIA: Rate limiting menú oculto (opcional)
- [ ] 8. BAJA: Centralizar brightness validation
- [ ] 9. BAJA: Verificar FastLED interrupt configuration
```

---

## 🔐 CONCLUSIÓN

El firmware muestra **buena calidad general** con protecciones adecuadas en la mayoría de componentes.

**Hallazgo crítico**: Conflicto GPIO 16 debe corregirse antes de deployment en hardware.

**Seguridad**: No se encontraron vulnerabilidades de seguridad explotables remotamente (no hay WiFi/OTA activo).

**Recomendación**: Implementar las 5 correcciones de prioridad CRÍTICA y ALTA antes de producción.

---

**Auditor**: GitHub Copilot AI Agent
**Fecha**: 2026-01-08
**Versión firmware**: v2.15.0 (según pins.h línea 7)
**Hardware**: ESP32-S3-WROOM-2 N32R16V (32MB Flash, 16MB PSRAM)

