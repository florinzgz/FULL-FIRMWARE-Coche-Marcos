# 📌 ESP32-S3-DevKitC-1 - Layout Físico de Pines

**Versión:** 1.0  
**Fecha:** 2025-11-24  
**Firmware:** Coche Inteligente Marcos

---

## 📐 Vista Superior del ESP32-S3-DevKitC-1

```
                    ╔═══════════════════════════════════════════════════════════════════╗
                    ║                         USB Type-C                                ║
                    ╚═══════════════════════════════════════════════════════════════════╝
                                                  │
                    ┌─────────────────────────────┴─────────────────────────────┐
                    │                     ESP32-S3-WROOM-1                      │
                    │                  (16MB Flash, 8MB PSRAM)                  │
                    └───────────────────────────────────────────────────────────┘
     
  LADO 1 (DERECHO)                                                        LADO 2 (IZQUIERDO)
  ─────────────────                                                       ──────────────────
        
  ┌───┐ GND  ●────────────────────────────────────────────────────────● GND   ┌───┐
  │   │ GND  ●────────────────────────────────────────────────────────● 5V    │   │
  │ P │ 19   ●  LED_REAR ←────────────────────────────────────────────● 14    │ P │  TFT_RST
  │ I │ 20   ●  ONEWIRE ←─────────────────────────────────────────────● 13    │ I │  TFT_DC
  │ N │ 21   ●  WHEEL_FL ←────────────────────────────────────────────● 12    │ N │  TFT_MISO
  │ E │ 47   ●  SHIFTER_P ←───────────────────────────────────────────● 11    │ E │  TFT_MOSI
  │ S │ 48   ●  SHIFTER_D2 ←──────────────────────────────────────────● 10    │ S │  TFT_SCK
  │   │ 45*  ●  BTN_LIGHTS ←──────────────────────────────────────────● 9     │   │  I2C_SCL
  │   │ 0*   ●  KEY_SYSTEM ←──────────────────────────────────────────● 46*   │   │  TOUCH_IRQ
  │   │ 35   ●  PEDAL (ADC) ←─────────────────────────────────────────● 3     │   │  TOUCH_CS
  │   │ 36   ●  WHEEL_FR ←────────────────────────────────────────────● 8     │   │  TFT_CS
  │   │ 37   ●  ENCODER_A ←───────────────────────────────────────────● 18    │   │  SHIFTER_N
  │   │ 38   ●  ENCODER_B ←───────────────────────────────────────────● 17    │   │  WHEEL_RL
  │   │ 39   ●  ENCODER_Z ←───────────────────────────────────────────● 16    │   │  I2C_SDA
  │   │ 40   ●  BTN_MEDIA ←───────────────────────────────────────────● 15    │   │  WHEEL_RR
  │   │ 41   ●  BTN_4X4 ←─────────────────────────────────────────────● 7     │   │  SHIFTER_D1
  │   │ 42   ●  TFT_BL ←──────────────────────────────────────────────● 6     │   │  RELAY_SPARE
  │   │ 2    ●  RELAY_MAIN ←──────────────────────────────────────────● 5     │   │  RELAY_DIR
  │   │ 1    ●  LED_FRONT ←───────────────────────────────────────────● 4     │   │  RELAY_TRAC
  │   │ 44   ●  DFPLAYER_RX ←─────────────────────────────────────────● RST   │   │
  │   │ 43   ●  DFPLAYER_TX ←─────────────────────────────────────────● 3V3   │   │
  └───┘ GND  ●────────────────────────────────────────────────────────● 3V3   └───┘
```

---

## ⚠️ Strapping Pins (Pines Especiales)

Los siguientes pines afectan el modo de arranque del ESP32-S3. **Usar con cuidado:**

| GPIO | Función Boot | Estado Requerido | Uso en Firmware | Notas |
|------|--------------|------------------|-----------------|-------|
| **0** | Boot Mode Select | HIGH (pull-up) = Normal Boot | KEY_SYSTEM | Pull-up 10kΩ externo recomendado |
| **45** | VDD_SPI Voltage | LOW = 3.3V (default) | BTN_LIGHTS | Solo lectura (input-only) |
| **46** | ROM Log Messages | Libre | TOUCH_IRQ | Solo lectura (input-only) |

### Recomendaciones:
1. **GPIO 0**: Agregar resistencia pull-up de 10kΩ a 3.3V para arranque confiable
2. **GPIO 45/46**: No conectar cargas que puedan forzar estado LOW durante boot

---

## 📊 Tabla Completa de Asignación

### Lado 1 (Derecho) - De arriba a abajo

| Pos | GPIO | Función | Tipo | Descripción |
|-----|------|---------|------|-------------|
| 1 | GND | Tierra | - | Tierra común |
| 2 | GND | Tierra | - | Tierra común |
| 3 | 19 | LED_REAR | Output | LEDs WS2812B traseros (16 LEDs) |
| 4 | 20 | ONEWIRE | I/O | Bus DS18B20 (4x temp. motores) |
| 5 | 21 | WHEEL_FL | Input | Sensor rueda Frontal Izquierda |
| 6 | 47 | SHIFTER_P | Input | Palanca Park (via optoacoplador) |
| 7 | 48 | SHIFTER_D2 | Input | Palanca D2 (via optoacoplador) |
| 8 | 45* | BTN_LIGHTS | Input | Botón luces (⚠️ strapping pin) |
| 9 | 0* | KEY_SYSTEM | Input | Boot/Llave sistema (⚠️ strapping) |
| 10 | 35 | PEDAL | Analog | Sensor Hall pedal (ADC1_CH4) |
| 11 | 36 | WHEEL_FR | Input | Sensor rueda Frontal Derecha |
| 12 | 37 | ENCODER_A | Input | Encoder dirección fase A |
| 13 | 38 | ENCODER_B | Input | Encoder dirección fase B |
| 14 | 39 | ENCODER_Z | Input | Encoder índice Z (centro) |
| 15 | 40 | BTN_MEDIA | Input | Botón multimedia |
| 16 | 41 | BTN_4X4 | Input | Switch modo 4x4/4x2 |
| 17 | 42 | TFT_BL | Output | Backlight pantalla (PWM) |
| 18 | 2 | RELAY_MAIN | Output | Relé principal (Power Hold) |
| 19 | 1 | LED_FRONT | Output | LEDs WS2812B frontales (28 LEDs) |
| 20 | 44 | DFPLAYER_RX | Input | UART RX audio |
| 21 | 43 | DFPLAYER_TX | Output | UART TX audio |
| 22 | GND | Tierra | - | Tierra común |

### Lado 2 (Izquierdo) - De arriba a abajo

| Pos | GPIO | Función | Tipo | Descripción |
|-----|------|---------|------|-------------|
| 1 | GND | Tierra | - | Tierra común |
| 2 | 5V | Alimentación | - | 5V USB/Vin |
| 3 | 14 | TFT_RST | Output | Reset pantalla |
| 4 | 13 | TFT_DC | Output | Data/Command pantalla |
| 5 | 12 | TFT_MISO | Input | SPI MISO |
| 6 | 11 | TFT_MOSI | Output | SPI MOSI |
| 7 | 10 | TFT_SCK | Output | SPI Clock |
| 8 | 9 | I2C_SCL | I/O | Bus I²C Clock |
| 9 | 46* | TOUCH_IRQ | Input | IRQ táctil (⚠️ strapping pin) |
| 10 | 3 | TOUCH_CS | Output | SPI CS táctil |
| 11 | 8 | TFT_CS | Output | SPI CS pantalla |
| 12 | 18 | SHIFTER_N | Input | Palanca Neutral |
| 13 | 17 | WHEEL_RL | Input | Sensor rueda Trasera Izquierda |
| 14 | 16 | I2C_SDA | I/O | Bus I²C Data |
| 15 | 15 | WHEEL_RR | Input | Sensor rueda Trasera Derecha |
| 16 | 7 | SHIFTER_D1 | Input | Palanca D1 (via optoacoplador) |
| 17 | 6 | RELAY_SPARE | Output | Relé auxiliar |
| 18 | 5 | RELAY_DIR | Output | Relé dirección 12V |
| 19 | 4 | RELAY_TRAC | Output | Relé tracción 24V |
| 20 | RST | Reset | - | Reset chip |
| 21 | 3V3 | Alimentación | - | 3.3V regulado |
| 22 | 3V3 | Alimentación | - | 3.3V regulado |

---

## 🔌 Expansores I²C (Pines Virtuales)

### MCP23017 (Dirección: 0x20)

| Pin MCP | GPIO Bank | Función | Descripción |
|---------|-----------|---------|-------------|
| 0 | GPIOA0 | MCP_PIN_FL_IN1 | Motor FL dirección |
| 1 | GPIOA1 | MCP_PIN_FL_IN2 | Motor FL dirección |
| 2 | GPIOA2 | MCP_PIN_FR_IN1 | Motor FR dirección |
| 3 | GPIOA3 | MCP_PIN_FR_IN2 | Motor FR dirección |
| 4 | GPIOA4 | MCP_PIN_RL_IN1 | Motor RL dirección |
| 5 | GPIOA5 | MCP_PIN_RL_IN2 | Motor RL dirección |
| 6 | GPIOA6 | MCP_PIN_RR_IN1 | Motor RR dirección |
| 7 | GPIOA7 | MCP_PIN_RR_IN2 | Motor RR dirección |
| **8** | **GPIOB0** | **MCP_PIN_SHIFTER_R** | **Palanca Reverse (movido de GPIO 19)** |

---

## 🎛️ Dispositivos I²C

| Dispositivo | Dirección | Función |
|-------------|-----------|---------|
| TCA9548A | 0x70 | Multiplexor I²C (6x INA226) |
| PCA9685 #1 | 0x40 | PWM motores eje delantero |
| PCA9685 #2 | 0x41 | PWM motores eje trasero |
| PCA9685 #3 | 0x42 | PWM motor dirección |
| MCP23017 | 0x20 | Expansor GPIO (IN1/IN2 + Shifter R) |

---

## ⚡ Buses de Comunicación

### SPI (Pantalla + Táctil)
```
SCK  → GPIO 10 (compartido)
MOSI → GPIO 11 (compartido)
MISO → GPIO 12 (compartido)
TFT_CS → GPIO 8
TOUCH_CS → GPIO 3
```

### I²C (Sensores + Expansores)
```
SDA → GPIO 16
SCL → GPIO 9
Pull-up: 4.7kΩ a 3.3V (en ambas líneas)
```

### UART (DFPlayer)
```
TX → GPIO 43 (ESP32 TX → DFPlayer RX)
RX → GPIO 44 (DFPlayer TX → ESP32 RX)
Baud: 9600
```

---

## 🔧 Notas de Implementación

### 1. Conflicto GPIO 19 Resuelto
- **Problema**: GPIO 19 estaba asignado a LED_REAR y SHIFTER_R
- **Solución**: SHIFTER_R movido a MCP23017 GPIOB0
- **Código**: Ver `shifter.cpp` para lectura vía I²C

### 2. Optoacopladores HY-M158
- Aíslan señales 12V del vehículo → 3.3V del ESP32
- Usados para: Shifter, Encoder, Sensores rueda
- Lógica: LOW = activo (pull-up interno)

### 3. Convertidores de Nivel
- Sensores 12V/5V requieren conversión a 3.3V
- TXS0104E o similar bidireccional recomendado
- Encoder E6B2-CWZ6C: 5-24V → 3.3V

---

## 📋 Checklist de Conexiones

- [ ] Strapping pins con resistencias pull-up/down correctas
- [ ] I²C con pull-ups de 4.7kΩ
- [ ] Optoacopladores con alimentación 12V correcta
- [ ] Bus SPI verificado (no cortos entre CS)
- [ ] LEDs WS2812B con capacitor 1000µF
- [ ] Convertidores de nivel instalados

---

**Documento generado por FirmwareAuditor - 2025-11-24**
