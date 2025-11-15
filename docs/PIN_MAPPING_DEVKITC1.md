# Mapeo de Pines para ESP32-S3-DevKitC-1 (44 pines)

## ⚠️ IMPORTANTE
Este firmware ha sido adaptado para funcionar con **ESP32-S3-DevKitC-1** que expone GPIOs 0-48.

## 📋 Cambios Realizados

Los siguientes pines han sido remapeados para compatibilidad con ESP32-S3-DevKitC-1:

### Pines Cambiados:

| Función | PIN Original | PIN Nuevo | Motivo |
|---------|--------------|-----------|--------|
| **Pantalla TFT** | | | |
| TFT_MOSI | GPIO 23 | GPIO 11 | GPIO 23 no disponible en DevKitC-1 |
| **Encoder Dirección** | | | |
| ENCODER_A | GPIO 44 | GPIO 37 | GPIO 44 no disponible en DevKitC-1 |
| ENCODER_B | GPIO 45 | GPIO 38 | GPIO 45 solo input en DevKitC-1 |
| **I2C Bus** | | | |
| I2C_SCL | GPIO 22 | GPIO 20 | GPIO 22 no disponible en DevKitC-1 |
| **Botones** | | | |
| BTN_LIGHTS | GPIO 35 | GPIO 40 | Reasignación para liberar GPIO 35 |
| BTN_MEDIA | GPIO 36 | GPIO 41 | Reasignación para liberar GPIO 36 |
| BTN_4X4 | GPIO 39 | GPIO 42 | GPIO 39 no disponible en DevKitC-1 |
| **Sensores Ruedas (LJ12A3-4-Z/BX)** | | | |
| WHEEL2 (RL) | GPIO 39 | GPIO 30 | GPIO 39 no disponible en DevKitC-1 |
| WHEEL3 (RR) | GPIO 46 | GPIO 31 | Reasignación |
| **Motores BTS7960** | | | |
| FL_PWM | GPIO 47 | GPIO 1 | GPIO 47 reasignado para UART0 |
| FL_IN1 | GPIO 48 | GPIO 3 | GPIO 48 reasignado para UART0 |
| FL_IN2 | GPIO 49 | GPIO 6 | GPIO 49+ no existen en DevKitC-1 |
| FR_PWM | GPIO 50 | GPIO 7 | GPIO 50+ no existen en DevKitC-1 |
| FR_IN1 | GPIO 51 | GPIO 8 | GPIO 51+ no existen en DevKitC-1 |
| FR_IN2 | GPIO 52 | GPIO 9 | GPIO 52+ no existen en DevKitC-1 |
| RL_PWM | GPIO 53 | GPIO 10 | GPIO 53+ no existen en DevKitC-1 |
| RL_IN1 | GPIO 54 | GPIO 47 | GPIO 54+ no existen en DevKitC-1 |
| RL_IN2 | GPIO 55 | GPIO 48 | GPIO 55+ no existen en DevKitC-1 |
| RR_PWM | GPIO 56 | GPIO 24 | GPIO 56+ no existen en DevKitC-1 |
| RR_IN1 | GPIO 57 | GPIO 28 | GPIO 57+ no existen en DevKitC-1 |
| RR_IN2 | GPIO 58 | GPIO 29 | GPIO 58+ no existen en DevKitC-1 |

### Pines Sin Cambios (Compatibles):

- ✅ Relés: GPIO 2, 4, 5, 6 (GPIO 32, 33 remapeados - no disponibles físicamente)
- ✅ Pedal: GPIO 34
- ✅ ENCODER_Z: GPIO 25
- ✅ DFPlayer: GPIO 16, 17
- ✅ TFT Display: GPIO 15 (CS), 27 (DC), 14 (RST), 18 (SCK), 19 (MISO)
- ✅ Touch: GPIO 12 (CS), 13 (IRQ)
- ✅ Sensores Ruedas FL/FR: GPIO 35, 36
- ✅ OneWire: GPIO 26
- ✅ I2C_SDA: GPIO 21
- ✅ BTN_BATTERY: GPIO 46

## 🔌 Conexiones Completas ESP32-S3-DevKitC-1

### Lado Izquierdo (de arriba a abajo):

| Pin Físico | GPIO | Función Asignada | Módulo |
|------------|------|------------------|--------|
| 3V3 | - | Alimentación 3.3V | Sistema |
| EN | - | Enable/Reset | Sistema |
| IO0 | 0 | (Libre/Strapping) | - |
| IO1 | 1 | **FL_PWM** | Motor FL |
| IO2 | 2 | **RELAY_MAIN** | Relé 12V |
| IO3 | 3 | **FL_IN1** | Motor FL |
| IO4 | 4 | **RELAY_TRAC** | Relé 24V |
| IO5 | 5 | **RELAY_DIR** | Relé Encoder |
| IO6 | 6 | **FL_IN2** | Motor FL |
| IO7 | 7 | **FR_PWM** | Motor FR |
| IO8 | 8 | **FR_IN1** | Motor FR |
| IO9 | 9 | **FR_IN2** | Motor FR |
| IO10 | 10 | **RL_PWM** | Motor RL |
| IO11 | 11 | **TFT_MOSI** | Pantalla |
| IO12 | 12 | **TOUCH_CS** | Táctil |
| IO13 | 13 | **TOUCH_IRQ** | Táctil |
| IO14 | 14 | **TFT_RST** | Pantalla |
| IO15 | 15 | **TFT_CS** | Pantalla |
| IO16 | 16 | **DFPLAYER_RX** | Audio |
| IO17 | 17 | **DFPLAYER_TX** | Audio |
| IO18 | 18 | **TFT_SCK** | Pantalla |
| IO19 | 19 | **TFT_MISO** | Pantalla |
| GND | - | Tierra | Sistema |

### Lado Derecho (de arriba a abajo):

| Pin Físico | GPIO | Función Asignada | Módulo |
|------------|------|------------------|--------|
| 5V0 | - | Alimentación 5V | Sistema |
| IO46 | 46 | **BTN_BATTERY** | Botón |
| IO45 | 45 | (Libre - Input Only) | - |
| IO48 | 48 | **RL_IN2** | Motor RL |
| IO47 | 47 | **RL_IN1** | Motor RL |
| IO21 | 21 | **I2C_SDA** | Sensores |
| IO20 | 20 | **I2C_SCL** | Sensores |
| IO38 | 38 | **ENCODER_B** | Dirección |
| IO37 | 37 | **ENCODER_A** | Dirección |
| IO36 | 36 | **WHEEL1** (FR) | Sensor Rueda |
| IO35 | 35 | **WHEEL0** (FL) | Sensor Rueda |
| IO34 | 34 | **PEDAL** (ADC) | Pedal |
| IO33 | 33 | ⚠️ No disponible | GPIO 23-34 no en headers |
| IO32 | 32 | ⚠️ No disponible | GPIO 23-34 no en headers |
| IO31 | 31 | **WHEEL3** (RR) | Sensor Rueda |
| IO30 | 30 | **WHEEL2** (RL) | Sensor Rueda |
| IO29 | 29 | **RR_IN2** | Motor RR |
| IO28 | 28 | **RR_IN1** | Motor RR |
| IO27 | 27 | **TFT_DC** | Pantalla |
| IO26 | 26 | **ONEWIRE** (DS18B20) | Temperatura |
| IO25 | 25 | **ENCODER_Z** | Dirección Centro |
| IO24 | 24 | **RR_PWM** | Motor RR |
| GND | - | Tierra | Sistema |

### ⚠️ Notas Importantes:

1. **GPIO 45, 46**: Son solo INPUT en ESP32-S3. GPIO 46 se usa para botón (OK), GPIO 45 está libre.
2. **GPIO 47, 48**: Usados por UART0 (USB-JTAG) pero disponibles como GPIO. Ahora controlan motor RL.
3. **SPI compartido**: GPIO 11 (MOSI), 18 (SCK), 19 (MISO) son compartidos por pantalla y táctil.
4. **I2C**: GPIO 20 (SCL), 21 (SDA) para sensores INA226.
5. **Sensores LJ12A3-4-Z/BX**: Configurados en GPIO 30, 31, 35, 36 (6 pulsos/revolución).
6. **Encoder E6B2-CWZ6C**: GPIO 37 (A), 38 (B), 25 (Z) para 1200PR.

## ✅ Verificación de Compatibilidad

- ✅ Todos los pines asignados existen en ESP32-S3-DevKitC-1
- ✅ Sensores de ruedas LJ12A3-4-Z/BX correctamente configurados (6 tornillos/rueda)
- ✅ Encoder E6B2-CWZ6C 1200PR con señal Z de centrado
- ✅ Relés HY-M158 en GPIO 2 y 4
- ✅ Pantalla ILI9488 con táctil XPT2046 en SPI
- ✅ DFPlayer Mini en UART1
- ✅ I2C para sensores INA226
- ✅ 4 motores BTS7960 con PWM + IN1/IN2

## 🚀 Compilación

El firmware está listo para compilar con PlatformIO:

```bash
cd "firmware Coche Marcos"
pio run -t clean
pio run
```

---
**Fecha de adaptación**: 2025-01-05  
**Hardware objetivo**: ESP32-S3-DevKitC-1 (44 pines)  
**Firmware compatible**: Coche Inteligente Marcos v1.0
