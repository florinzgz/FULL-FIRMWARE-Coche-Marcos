# Cambios Recientes en el Firmware
## Actualización: Control I²C de Motores y Asignación de LEDs

**Fecha:** 2025-11-15  
**Commits:** dc0732a → 7d0c566  
**Versión:** 2.0

---

## Resumen de Cambios Críticos

### 1. Control de Motores BTS7960 vía I²C ✅ NUEVO

**Problema Resuelto:** Los GPIOs 23-34 asignados originalmente no están disponibles físicamente en ESP32-S3-DevKitC-1.

**Solución Implementada:**

#### PCA9685 @ 0x40 - Control PWM (8 canales usados)
```
Canal 0 → Motor FL PWM Forward
Canal 1 → Motor FL PWM Reverse
Canal 2 → Motor FR PWM Forward
Canal 3 → Motor FR PWM Reverse
Canal 4 → Motor RL PWM Forward
Canal 5 → Motor RL PWM Reverse
Canal 6 → Motor RR PWM Forward
Canal 7 → Motor RR PWM Reverse
```

#### MCP23017 @ 0x20 - Control Dirección (8 GPIOs usados)
```
GPIOA0 → Motor FL IN1
GPIOA1 → Motor FL IN2
GPIOA2 → Motor FR IN1
GPIOA3 → Motor FR IN2
GPIOA4 → Motor RL IN1
GPIOA5 → Motor RL IN2
GPIOA6 → Motor RR IN1
GPIOA7 → Motor RR IN2
```

### 2. Asignación de GPIOs para LEDs WS2812B ✅ NUEVO

```
GPIO 0 → PIN_LED_FRONT (28 LEDs frontales)
GPIO 1 → PIN_LED_REAR (16 LEDs traseros)
```

Definiciones en `include/pins.h`:
```cpp
#define PIN_LED_FRONT     0
#define PIN_LED_REAR      1
#define NUM_LEDS_FRONT    28
#define NUM_LEDS_REAR     16
```

### 3. Mapa I²C Actualizado (Sin Conflictos)

| Dirección | Dispositivo | Función |
|-----------|-------------|---------|
| 0x40 | PCA9685 | PWM motores tracción (bus principal) |
| 0x41 | PCA9685 | PWM motor dirección |
| 0x20 | MCP23017 | Control IN1/IN2 motores |
| 0x70 | TCA9548A | Multiplexor 6x INA226 @ 0x40 |

**Sin conflictos:** PCA9685 @ 0x40 está en bus I²C principal, INA226 @ 0x40 están multiplexados.

---

## Código de Ejemplo

### Inicialización Módulos I²C

```cpp
#include <Adafruit_PWMServoDriver.h>
#include <Adafruit_MCP23017.h>
#include "pins.h"

// Crear objetos
Adafruit_PWMServoDriver pwm_motors = Adafruit_PWMServoDriver(PCA9685_ADDR_MOTORS);
Adafruit_MCP23017 mcp_motors;

void setup() {
    // Inicializar PCA9685
    pwm_motors.begin();
    pwm_motors.setPWMFreq(1600);  // 1.6 kHz para BTS7960
    
    // Inicializar MCP23017
    mcp_motors.begin(MCP23017_ADDR_MOTORS);
    for (int i = 0; i < 8; i++) {
        mcp_motors.pinMode(i, OUTPUT);
        mcp_motors.digitalWrite(i, LOW);
    }
}
```

### Control Motor FL Adelante

```cpp
void setMotorFL_Forward(uint8_t speed) {  // speed: 0-255
    // Configurar dirección (MCP23017)
    mcp_motors.digitalWrite(MCP_PIN_FL_IN1, HIGH);
    mcp_motors.digitalWrite(MCP_PIN_FL_IN2, LOW);
    
    // Configurar PWM (PCA9685)
    uint16_t pwm_value = map(speed, 0, 255, 0, 4095);
    pwm_motors.setPWM(PCA_CH_FL_PWM, 0, pwm_value);      // Forward ON
    pwm_motors.setPWM(PCA_CH_FL_PWM_R, 0, 0);            // Reverse OFF
}
```

### Control Motor FL Atrás

```cpp
void setMotorFL_Reverse(uint8_t speed) {
    // Configurar dirección
    mcp_motors.digitalWrite(MCP_PIN_FL_IN1, LOW);
    mcp_motors.digitalWrite(MCP_PIN_FL_IN2, HIGH);
    
    // Configurar PWM
    uint16_t pwm_value = map(speed, 0, 255, 0, 4095);
    pwm_motors.setPWM(PCA_CH_FL_PWM, 0, 0);              // Forward OFF
    pwm_motors.setPWM(PCA_CH_FL_PWM_R, 0, pwm_value);    // Reverse ON
}
```

### Control Motor FL Freno

```cpp
void setMotorFL_Brake() {
    // Configurar dirección para freno regenerativo
    mcp_motors.digitalWrite(MCP_PIN_FL_IN1, LOW);
    mcp_motors.digitalWrite(MCP_PIN_FL_IN2, LOW);
    
    // PWM a 0
    pwm_motors.setPWM(PCA_CH_FL_PWM, 0, 0);
    pwm_motors.setPWM(PCA_CH_FL_PWM_R, 0, 0);
}
```

### Control LEDs WS2812B

```cpp
#include <FastLED.h>
#include "pins.h"

CRGB leds_front[NUM_LEDS_FRONT];
CRGB leds_rear[NUM_LEDS_REAR];

void setup() {
    FastLED.addLeds<WS2812B, PIN_LED_FRONT, GRB>(leds_front, NUM_LEDS_FRONT);
    FastLED.addLeds<WS2812B, PIN_LED_REAR, GRB>(leds_rear, NUM_LEDS_REAR);
    FastLED.setBrightness(50);
}

void loop() {
    // Ejemplo: LEDs frontales rojos
    fill_solid(leds_front, NUM_LEDS_FRONT, CRGB::Red);
    
    // LEDs traseros blancos
    fill_solid(leds_rear, NUM_LEDS_REAR, CRGB::White);
    
    FastLED.show();
}
```

---

## Librerías Necesarias

Añadir a `platformio.ini`:

```ini
lib_deps =
    bodmer/TFT_eSPI @ ^2.5.43
    dfrobot/DFRobotDFPlayerMini @ ^1.0.6
    milesburton/DallasTemperature@^4.0.5
    paulstoffregen/OneWire@^2.3.8
    https://github.com/adafruit/Adafruit-PWM-Servo-Driver-Library.git
    adafruit/Adafruit MCP23017 Arduino Library @ ^2.3.2  ; ← NUEVA
    RobTillaart/INA226 @ ^0.6.4
    https://github.com/PaulStoffregen/XPT2046_Touchscreen.git
    fastled/FastLED @ 3.6.0
```

---

## Verificación Hardware

### Conexiones PCA9685 @ 0x40

```
ESP32-S3          PCA9685
GPIO 16 (SDA) ──► SDA
GPIO 9 (SCL)  ──► SCL
3.3V          ──► VCC
GND           ──► GND

Pines A0-A5: GND (dirección 0x40)
```

### Conexiones MCP23017 @ 0x20

```
ESP32-S3          MCP23017
GPIO 16 (SDA) ──► SDA
GPIO 9 (SCL)  ──► SCL
3.3V          ──► VCC
GND           ──► GND

Pines A0-A2: GND (dirección 0x20)
RESET: 3.3V
```

### Conexiones BTS7960 (ejemplo Motor FL)

```
PCA9685           BTS7960
CH0 (PIN 0)   ──► RPWM (Forward PWM)
CH1 (PIN 1)   ──► LPWM (Reverse PWM)

MCP23017          BTS7960
GPIOA0        ──► R_EN (IN1)
GPIOA1        ──► L_EN (IN2)

Alimentación:
24V Battery   ──► VCC (BTS7960)
GND           ──► GND (BTS7960)

Salida:
M+ (BTS7960)  ──► Motor Terminal 1
M- (BTS7960)  ──► Motor Terminal 2
```

---

## Cambios en Archivos

### `include/pins.h`

**Eliminado:**
```cpp
#define PIN_FL_PWM        23  // ❌ GPIO no disponible
#define PIN_FL_IN1        24  // ❌ GPIO no disponible
... (12 definiciones eliminadas)
```

**Añadido:**
```cpp
// Control PWM vía PCA9685
#define PCA9685_ADDR_MOTORS  0x40
#define PCA_CH_FL_PWM     0
#define PCA_CH_FL_PWM_R   1
... (8 canales total)

// Control IN1/IN2 vía MCP23017
#define MCP23017_ADDR_MOTORS 0x20
#define MCP_PIN_FL_IN1    0
#define MCP_PIN_FL_IN2    1
... (8 pines total)

// LEDs WS2812B
#define PIN_LED_FRONT     0
#define PIN_LED_REAR      1
#define NUM_LEDS_FRONT    28
#define NUM_LEDS_REAR     16
```

---

## Checklist de Migración

- [ ] **Instalar librería Adafruit MCP23017**
  ```bash
  pio lib install "adafruit/Adafruit MCP23017 Arduino Library"
  ```

- [ ] **Actualizar código de control de motores**
  - Reemplazar digitalWrite() de GPIOs por mcp_motors.digitalWrite()
  - Reemplazar analogWrite() por pwm_motors.setPWM()

- [ ] **Cablear módulos I²C**
  - PCA9685 @ 0x40: configurar jumpers A0-A5=GND
  - MCP23017 @ 0x20: configurar jumpers A0-A2=GND

- [ ] **Cablear BTS7960**
  - 8 señales PWM desde PCA9685
  - 8 señales IN1/IN2 desde MCP23017

- [ ] **Cablear LEDs WS2812B**
  - GPIO 0 → Level Shifter → DIN LEDs Front
  - GPIO 1 → Level Shifter → DIN LEDs Rear
  - 5V externa → LEDs (2A mínimo)

- [ ] **Verificar direcciones I²C**
  ```cpp
  Wire.begin(16, 9);
  // Escanear bus I²C
  // Debe detectar: 0x20, 0x40, 0x41, 0x70
  ```

---

## Beneficios de la Nueva Arquitectura

1. ✅ **Elimina dependencia de GPIOs no disponibles**
2. ✅ **PWM de 12 bits (4096 pasos) vs 8 bits (255 pasos)**
3. ✅ **Frecuencia PWM precisa y configurable**
4. ✅ **Fácil expansión (GPIOB del MCP23017 libre)**
5. ✅ **Menor cableado (solo 2 cables I²C vs 12 cables GPIO)**
6. ✅ **Mayor inmunidad al ruido (señales digitales I²C)**

---

## Troubleshooting

### PCA9685 no detectado (0x40)

1. Verificar jumpers A0-A5 en GND
2. Verificar conexión SDA/SCL
3. Verificar pull-ups 4.7kΩ en SDA/SCL
4. Escanear bus: `i2cdetect -y 1`

### MCP23017 no detectado (0x20)

1. Verificar jumpers A0-A2 en GND
2. Verificar RESET conectado a 3.3V
3. Verificar VCC y GND

### Motor no responde

1. Verificar alimentación 24V en BTS7960
2. Verificar lógica IN1/IN2:
   - IN1=1, IN2=0 → Adelante
   - IN1=0, IN2=1 → Atrás
   - IN1=0, IN2=0 → Freno
   - IN1=1, IN2=1 → ⚠️ PROHIBIDO
3. Verificar PWM > 0 en canal correspondiente

### LEDs no encienden

1. Verificar alimentación 5V externa
2. Verificar level shifter 3.3V→5V
3. Verificar GPIO correcto (0 o 1)
4. Verificar GND común entre ESP32 y LEDs

---

**Documento actualizado:** 2025-11-15  
**Para más detalles:** Ver `docs/MANUAL_TECNICO_CONEXIONES.md`
