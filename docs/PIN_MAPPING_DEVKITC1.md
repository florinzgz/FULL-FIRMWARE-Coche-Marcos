# Mapeo de Pines para ESP32-S3-DevKitC-1 (44 pines)

## 📌 Versión: 2.8.5
## 📅 Fecha: 2025-12-01

Este documento refleja la configuración actual del firmware según `include/pins.h`.

---

## ⚠️ IMPORTANTE
Este firmware ha sido adaptado para funcionar con **ESP32-S3-DevKitC-1** que expone GPIOs 0-48.

## 🔧 Hardware Integrado

- **ESP32-S3-DevKitC-1** (44 pines, 36 GPIOs utilizables)
- **6x INA226** con shunts externos CG FL-2C (1x100A batería + 4x50A motores + 1x50A dirección)
- **1x TCA9548A** multiplexor I²C (para 6 INA226 sin conflicto dirección)
- **2x PCA9685** PWM driver motores tracción (0x40 delantero, 0x41 trasero)
- **1x PCA9685** PWM driver motor dirección (0x42)
- **1x MCP23017** expansor GPIO I²C (16 pines, 0x20)
- **2x HY-M158** optoacopladores PC817 (8 canales c/u = 16 total, aislamiento 12V→3.3V)
- **4x BTS7960** drivers motor 43A (tracción 4 ruedas independientes)
- **1x BTS7960** driver motor dirección (RS390 12V 6000RPM + reductora 1:50)
- **1x Encoder E6B2-CWZ6C** 1200PR (dirección, ratio 1:1 al volante)
- **4x Sensores inductivos LJ12A3-4-Z/BX** (velocidad ruedas)
- **1x Sensor inductivo LJ12A3-4-Z/BX** (señal Z encoder centrado)
- **1x Sensor Hall A1324LUA-T** (pedal analógico)
- **4x DS18B20** sensores temperatura (motores tracción)
- **1x Pantalla ST7796S** 480x320 + táctil XPT2046 (SPI)
- **1x DFPlayer Mini** (audio, UART)
- **2x Tiras LEDs WS2812B** (iluminación delantera 28 LEDs + trasera 16 LEDs)
- **4x Relés SRD-05VDC** (control potencia, luces, tracción, dirección)

---

## 📋 Asignación Completa de GPIOs

### Comunicaciones I²C
| GPIO | Función | Notas |
|------|---------|-------|
| 8 | I2C_SDA | Bus I²C Data |
| 9 | I2C_SCL | Bus I²C Clock |

### Direcciones I²C del Sistema
| Dispositivo | Dirección | Función |
|-------------|-----------|---------|
| PCA9685 #1 | 0x40 | Motores eje delantero (FL+FR) |
| PCA9685 #2 | 0x41 | Motores eje trasero (RL+RR) |
| PCA9685 #3 | 0x42 | Motor dirección |
| MCP23017 | 0x20 | Expansor GPIO (IN1/IN2 + Shifter) |
| TCA9548A | 0x70 | Multiplexor I²C para INA226 |

### Canales TCA9548A (INA226)
| Canal | Sensor | Aplicación |
|-------|--------|------------|
| 0 | INA226 @ 0x40 | Motor FL - Shunt 50A 75mV |
| 1 | INA226 @ 0x40 | Motor FR - Shunt 50A 75mV |
| 2 | INA226 @ 0x40 | Motor RL - Shunt 50A 75mV |
| 3 | INA226 @ 0x40 | Motor RR - Shunt 50A 75mV |
| 4 | INA226 @ 0x40 | Batería 24V - Shunt 100A 75mV |
| 5 | INA226 @ 0x40 | Motor Dirección - Shunt 50A 75mV |

### Comunicaciones SPI - Pantalla TFT ST7796S 480x320
| GPIO | Función | Notas |
|------|---------|-------|
| 10 | TFT_SCK | SPI Clock |
| 11 | TFT_MOSI | SPI MOSI |
| 12 | TFT_MISO | SPI MISO |
| 13 | TFT_DC | Data/Command |
| 14 | TFT_RST | Reset |
| 16 | TFT_CS | Chip Select TFT |
| 42 | TFT_BL | Backlight PWM |

### Táctil XPT2046
| GPIO | Función | Notas |
|------|---------|-------|
| 21 | TOUCH_CS | ✅ Pin seguro (antes GPIO 3) |
| 47 | TOUCH_IRQ | Interrupción táctil |

### Audio DFPlayer Mini
| GPIO | Función | Notas |
|------|---------|-------|
| 43 | DFPLAYER_TX | UART0 TX nativo |
| 44 | DFPLAYER_RX | UART0 RX nativo |

### Relés de Potencia (4x SRD-05VDC-SL-C)
| GPIO | Función | Notas |
|------|---------|-------|
| 4 | RELAY_MAIN | Relé principal (Power Hold) |
| 5 | RELAY_TRAC | Relé tracción 24V |
| 6 | RELAY_DIR | Relé dirección 12V |
| 7 | RELAY_SPARE | Relé auxiliar (luces/media) |

### Encoder Dirección E6B2-CWZ6C 1200PR
| GPIO | Función | Notas |
|------|---------|-------|
| 37 | ENCODER_A | Canal A (cuadratura) |
| 38 | ENCODER_B | Canal B (cuadratura) |
| 39 | ENCODER_Z | Señal Z (centrado, 1 pulso/vuelta) |

### Sensores Ruedas (4x LJ12A3-4-Z/BX)
| GPIO | Función | Notas |
|------|---------|-------|
| 3 | WHEEL_FL | Front Left ✅ (antes GPIO 21) |
| 36 | WHEEL_FR | Front Right |
| 17 | WHEEL_RL | Rear Left |
| 15 | WHEEL_RR | Rear Right |

### Pedal y Temperatura
| GPIO | Función | Notas |
|------|---------|-------|
| 35 | PEDAL | ADC1_CH4 - Sensor Hall A1324LUA-T |
| 20 | ONEWIRE | Bus OneWire - 4x DS18B20 |

### Shifter (Palanca de cambios) - ✅ TODO en MCP23017
| MCP Pin | Función | Notas |
|---------|---------|-------|
| GPIOB0 (8) | SHIFTER_P | Park |
| GPIOB1 (9) | SHIFTER_R | Reverse |
| GPIOB2 (10) | SHIFTER_N | Neutral |
| GPIOB3 (11) | SHIFTER_D1 | Drive 1 |
| GPIOB4 (12) | SHIFTER_D2 | Drive 2 |

### Botones
| GPIO | Función | Notas |
|------|---------|-------|
| 2 | BTN_LIGHTS | Botón luces ✅ (antes GPIO 45) |
| 40 | BTN_MEDIA | Botón multimedia |
| 41 | BTN_4X4 | Botón 4x4/4x2 |

### LEDs WS2812B
| GPIO | Función | Cantidad | Notas |
|------|---------|----------|-------|
| 1 | LED_FRONT | 28 LEDs | Frontales |
| 48 | LED_REAR | 16 LEDs | Traseros ✅ (antes GPIO 19) |

### Sistema
| GPIO | Función | Notas |
|------|---------|-------|
| 0 | KEY_SYSTEM | Boot button (strapping, pull-up ext) |

### Control Motores vía MCP23017 (GPIOA)
| MCP Pin | Función | Notas |
|---------|---------|-------|
| GPIOA0 | FL_IN1 | Motor FL dirección |
| GPIOA1 | FL_IN2 | Motor FL dirección |
| GPIOA2 | FR_IN1 | Motor FR dirección |
| GPIOA3 | FR_IN2 | Motor FR dirección |
| GPIOA4 | RL_IN1 | Motor RL dirección |
| GPIOA5 | RL_IN2 | Motor RL dirección |
| GPIOA6 | RR_IN1 | Motor RR dirección |
| GPIOA7 | RR_IN2 | Motor RR dirección |

### Control PWM Motores vía PCA9685
#### PCA9685 #1 - Eje Delantero (0x40)
| Canal | Función |
|-------|---------|
| 0 | FL Forward PWM |
| 1 | FL Reverse PWM |
| 2 | FR Forward PWM |
| 3 | FR Reverse PWM |

#### PCA9685 #2 - Eje Trasero (0x41)
| Canal | Función |
|-------|---------|
| 0 | RL Forward PWM |
| 1 | RL Reverse PWM |
| 2 | RR Forward PWM |
| 3 | RR Reverse PWM |

#### PCA9685 #3 - Dirección (0x42)
| Canal | Función |
|-------|---------|
| 0 | Steering Forward PWM |
| 1 | Steering Reverse PWM |

---

## 📊 Tabla Resumen de Uso de GPIOs

```
┌──────┬─────────────────────────┬───────────┬─────────────────────────────────┐
│ GPIO │ Función                 │ Tipo      │ Notas                           │
├──────┼─────────────────────────┼───────────┼─────────────────────────────────┤
│  0   │ KEY_SYSTEM              │ Input     │ ⚠️ Strapping (Boot), pull-up ext │
│  1   │ LED_FRONT (WS2812B)     │ Output    │ 28 LEDs frontales               │
│  2   │ BTN_LIGHTS              │ Input     │ Botón luces                     │
│  3   │ WHEEL_FL                │ Input     │ Sensor rueda delantera izq      │
│  4   │ RELAY_MAIN              │ Output    │ Relé principal (Power Hold)     │
│  5   │ RELAY_TRAC              │ Output    │ Relé tracción 24V               │
│  6   │ RELAY_DIR               │ Output    │ Relé dirección 12V              │
│  7   │ RELAY_SPARE             │ Output    │ Relé auxiliar                   │
│  8   │ I2C_SDA                 │ I/O       │ Bus I²C Data                    │
│  9   │ I2C_SCL                 │ I/O       │ Bus I²C Clock                   │
│ 10   │ TFT_SCK                 │ Output    │ SPI Clock                       │
│ 11   │ TFT_MOSI                │ Output    │ SPI MOSI                        │
│ 12   │ TFT_MISO                │ Input     │ SPI MISO                        │
│ 13   │ TFT_DC                  │ Output    │ Data/Command                    │
│ 14   │ TFT_RST                 │ Output    │ Reset pantalla                  │
│ 15   │ WHEEL_RR                │ Input     │ Sensor rueda trasera derecha    │
│ 16   │ TFT_CS                  │ Output    │ Chip Select TFT                 │
│ 17   │ WHEEL_RL                │ Input     │ Sensor rueda trasera izquierda  │
│ 18   │ 🆓 LIBRE                │ -         │ Disponible para expansión       │
│ 19   │ 🆓 LIBRE                │ -         │ Disponible para expansión       │
│ 20   │ ONEWIRE                 │ I/O       │ 4x DS18B20 temperatura          │
│ 21   │ TOUCH_CS                │ Output    │ ✅ CS Touch (seguro)             │
│ 35   │ PEDAL (ADC)             │ Analog In │ Sensor Hall pedal               │
│ 36   │ WHEEL_FR                │ Input     │ Sensor rueda delantera derecha  │
│ 37   │ ENCODER_A               │ Input     │ Encoder dirección A             │
│ 38   │ ENCODER_B               │ Input     │ Encoder dirección B             │
│ 39   │ ENCODER_Z               │ Input     │ Encoder dirección Z             │
│ 40   │ BTN_MEDIA               │ Input     │ Botón multimedia                │
│ 41   │ BTN_4X4                 │ Input     │ Botón 4x4/4x2                   │
│ 42   │ TFT_BL (PWM)            │ Output    │ Backlight pantalla              │
│ 43   │ DFPLAYER_TX             │ Output    │ ⚠️ UART0 nativo                  │
│ 44   │ DFPLAYER_RX             │ Input     │ ⚠️ UART0 nativo                  │
│ 45   │ 🆓 LIBRE                │ -         │ ⚠️ Strapping, disponible         │
│ 46   │ 🆓 LIBRE                │ -         │ ⚠️ Strapping, disponible         │
│ 47   │ TOUCH_IRQ               │ Input     │ Interrupción táctil             │
│ 48   │ LED_REAR (WS2812B)      │ Output    │ 16 LEDs traseros                │
└──────┴─────────────────────────┴───────────┴─────────────────────────────────┘
```

---

## ✅ Mejoras v2.4.0

- ✅ `SteeringMotor::get()` implementado
- ✅ Race condition en sensores de ruedas corregida (acceso atómico)
- ✅ Validación de índices negativos en getters de sensores
- ✅ `Relays::emergencyStop()` añadido para parada de emergencia
- ✅ Histéresis en detección de errores de relés (3 consecutivos)
- ✅ Bucle bloqueante de Serial eliminado
- ✅ Delays de inicialización HUD reducidos de 70ms a 0.6ms

## ✅ Mejoras v2.3.0

- TOUCH_CS: GPIO 3 → GPIO 21 (evita strapping pin)
- LED_REAR: GPIO 19 → GPIO 48 (resuelve conflicto)
- TOUCH_IRQ: GPIO 46 → GPIO 47 (evita strapping pin)
- SHIFTER COMPLETO: GPIOs dispersos → MCP23017 GPIOB0-B4 (pines consecutivos)

## 📊 Estadísticas

- **GPIOs ESP32 utilizados**: 30/36 (83% eficiencia)
- **GPIOs MCP23017 utilizados**: 13/16 (81% eficiencia)
- **GPIOs libres**: 18, 19, 45, 46

---

## 🚀 Compilación

```bash
cd firmware
pio run                      # Build todos los entornos
pio run -e esp32-s3-devkitc  # Build entorno debug
pio run --target upload      # Flash
pio device monitor           # Monitor serie
```

---

**Fecha de actualización**: 2025-12-01  
**Hardware objetivo**: ESP32-S3-DevKitC-1 (44 pines)  
**Firmware compatible**: Coche Inteligente Marcos v2.8.5

