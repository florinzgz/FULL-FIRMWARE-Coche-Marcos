# Lista Completa de Asignación de GPIOs - ESP32-S3

**Fecha**: 2026-01-09  
**Versión**: v2.11.3+  
**Hardware**: ESP32-S3-WROOM-2 N32R16V (32MB Flash + 16MB PSRAM)

---

## GPIOs DISPONIBLES EN LA PLACA (36 total)

```
LADO 1: 19, 20, 21, 47, 48, 45, 0, 35, 36, 37, 38, 39, 40, 41, 42, 2, 1, 44, 43
LADO 2: 14, 13, 12, 11, 10, 9, 46, 3, 8, 18, 17, 16, 15, 7, 6, 5, 4
```

---

## TABLA COMPLETA DE ASIGNACIÓN

| GPIO | Estado | Función | Tipo | Notas |
|------|--------|---------|------|-------|
| **0** | 🆓 **LIBRE** | - | - | ⚠️ Strapping pin (Boot mode). Antes KEY_SYSTEM |
| **1** | ✅ EN USO | WHEEL_RR | Input | Rueda trasera derecha (v2.16.1) |
| **2** | ✅ EN USO | BTN_LIGHTS | Input | Botón luces físico |
| **3** | ✅ EN USO | WHEEL_FL | Input | Sensor rueda delantera izquierda |
| **4** | ✅ EN USO | PEDAL (ADC) | Analog In | Sensor Hall pedal (ADC1_CH3) |
| **5** | ✅ EN USO | RELAY_TRAC | Output | Relé tracción 24V |
| **6** | ✅ EN USO | RELAY_DIR | Output | Relé dirección 12V |
| **7** | 🆓 **LIBRE** | - | - | ✅ Disponible (RELAY_SPARE movido a GPIO 46) |
| **8** | ✅ EN USO | I2C_SDA | I/O | Bus I²C Data |
| **9** | ✅ EN USO | I2C_SCL | I/O | Bus I²C Clock |
| **10** | ✅ EN USO | TFT_SCK | Output | SPI Clock (display) |
| **11** | ✅ EN USO | TFT_MOSI | Output | SPI MOSI (display) |
| **12** | ✅ EN USO | TFT_MISO | Input | SPI MISO (display) |
| **13** | ✅ EN USO | TFT_DC | Output | Data/Command (display) |
| **14** | ✅ EN USO | TFT_RST | Output | Reset pantalla |
| **15** | ✅ EN USO | WHEEL_RL | Input | Rueda trasera izquierda |
| **16** | ✅ EN USO | TFT_CS | Output | Chip Select display (SPI) |
| **17** | ✅ EN USO | DFPLAYER_RX | Input | UART1 - Recibe del DFPlayer Mini |
| **18** | ✅ EN USO | DFPLAYER_TX | Output | UART1 - Envía al DFPlayer Mini |
| **19** | ✅ EN USO | LED_FRONT | Output | 28 LEDs frontales WS2812B |
| **20** | ✅ EN USO | ONEWIRE | I/O | Bus OneWire (4x DS18B20 temperatura) |
| **21** | ✅ EN USO | TOUCH_CS | Output | Chip Select Touch XPT2046 |
| **22** | ❌ NO EXISTE | - | - | No disponible en ESP32-S3 |
| **23-34** | ❌ NO EXISTE | - | - | No disponibles en ESP32-S3 |
| **35** | ✅ EN USO | RELAY_MAIN | Output | Relé principal (Power Hold) |
| **36** | ✅ EN USO | WHEEL_FR | Input | Sensor rueda delantera derecha |
| **37** | ✅ EN USO | ENCODER_A | Input | Encoder dirección canal A |
| **38** | ✅ EN USO | ENCODER_B | Input | Encoder dirección canal B |
| **39** | ✅ EN USO | ENCODER_Z | Input | Encoder dirección señal Z (centrado) |
| **40** | ✅ EN USO | KEY_ON | Input | Detección ignición ON (v2.15.0) |
| **41** | ✅ EN USO | KEY_OFF | Input | Solicitud shutdown (v2.15.0) |
| **42** | ✅ EN USO | TFT_BL | Output | Backlight pantalla (PWM) |
| **43** | ✅ EN USO | TOFSENSE_TX | Output | UART0 TX - TOFSense (no usado por sensor) |
| **44** | ✅ EN USO | TOFSENSE_RX | Input | UART0 RX - TOFSense recibe datos LiDAR |
| **45** | 🆓 **LIBRE** | - | - | ⚠️ Strapping pin (VDD_SPI). Antes KEY_DETECT |
| **46** | ✅ EN USO | RELAY_SPARE | Output | ⚠️ Strapping pin - Relé auxiliar (v2.11.3) |
| **47** | ✅ EN USO | TOUCH_IRQ | Input | Interrupción táctil XPT2046 |
| **48** | ✅ EN USO | LED_REAR | Output | 16 LEDs traseros WS2812B |

---

## RESUMEN DE PINES LIBRES

### GPIOs Completamente Libres (3 total)
| GPIO | Tipo Strapping | Notas de Seguridad |
|------|----------------|-------------------|
| **0** | ⚠️ SÍ | Boot mode pin - Evitar señales LOW durante boot |
| **7** | ❌ NO | **PIN SEGURO** - Liberado en v2.11.3 |
| **45** | ⚠️ SÍ | VDD_SPI voltage select - Usar con precaución |

### Recomendación de Uso
- **GPIO 7**: 🟢 **MÁS RECOMENDADO** - Pin seguro, no strapping, sin restricciones
- **GPIO 0**: 🟡 **USAR CON CUIDADO** - Strapping pin, requiere pull-up externo
- **GPIO 45**: 🟡 **USAR CON CUIDADO** - Strapping pin, puede afectar voltaje VDD_SPI

---

## STRAPPING PINS - INFORMACIÓN CRÍTICA

Los siguientes GPIOs son strapping pins y afectan el comportamiento de boot:

| GPIO | Función Strapping | Estado Actual | Notas |
|------|-------------------|---------------|-------|
| **0** | Boot mode | 🆓 LIBRE | Requiere pull-up externo. LOW durante boot = Download mode |
| **3** | JTAG enable | ✅ EN USO (WHEEL_FL) | OK si no se usa JTAG para debug |
| **45** | VDD_SPI voltage | 🆓 LIBRE | Afecta voltaje flash. Usar con precaución |
| **46** | Boot/ROM log | ✅ EN USO (RELAY_SPARE) | Debe estar HIGH durante boot. Configurar OUTPUT early |

---

## PINES EN USO - CLASIFICACIÓN POR FUNCIÓN

### Relés (4 pines)
- GPIO 35: RELAY_MAIN (Relé principal)
- GPIO 5: RELAY_TRAC (Relé tracción 24V)
- GPIO 6: RELAY_DIR (Relé dirección 12V)
- GPIO 46: RELAY_SPARE (Relé auxiliar) ⚠️ Strapping pin

### Control de Potencia (2 pines)
- GPIO 40: KEY_ON (Detección ignición)
- GPIO 41: KEY_OFF (Solicitud apagado)

### Display y Touch (9 pines)
- GPIO 10: TFT_SCK (SPI Clock)
- GPIO 11: TFT_MOSI (SPI MOSI)
- GPIO 12: TFT_MISO (SPI MISO)
- GPIO 13: TFT_DC (Data/Command)
- GPIO 14: TFT_RST (Reset)
- GPIO 16: TFT_CS (Chip Select)
- GPIO 42: TFT_BL (Backlight PWM)
- GPIO 21: TOUCH_CS (Touch Chip Select)
- GPIO 47: TOUCH_IRQ (Touch Interrupt)

### Sensores de Ruedas (4 pines)
- GPIO 3: WHEEL_FL (Rueda delantera izquierda)
- GPIO 36: WHEEL_FR (Rueda delantera derecha)
- GPIO 15: WHEEL_RL (Rueda trasera izquierda)
- GPIO 1: WHEEL_RR (Rueda trasera derecha)

### Encoder de Dirección (3 pines)
- GPIO 37: ENCODER_A (Canal A)
- GPIO 38: ENCODER_B (Canal B)
- GPIO 39: ENCODER_Z (Señal Z - centrado)

### Comunicaciones (6 pines)
- GPIO 8: I2C_SDA (Bus I²C Data)
- GPIO 9: I2C_SCL (Bus I²C Clock)
- GPIO 18: DFPLAYER_TX (UART1 Audio TX)
- GPIO 17: DFPLAYER_RX (UART1 Audio RX)
- GPIO 43: TOFSENSE_TX (UART0 LiDAR TX)
- GPIO 44: TOFSENSE_RX (UART0 LiDAR RX)

### Sensores y Entradas (3 pines)
- GPIO 4: PEDAL (ADC - Sensor Hall pedal)
- GPIO 20: ONEWIRE (4x DS18B20 temperatura)
- GPIO 2: BTN_LIGHTS (Botón luces físico)

### LEDs (2 pines)
- GPIO 19: LED_FRONT (28 LEDs WS2812B frontales)
- GPIO 48: LED_REAR (16 LEDs WS2812B traseros)

---

## EXPANSOR GPIO - MCP23017 (I²C 0x20)

Además de los GPIOs del ESP32-S3, el sistema usa un expansor MCP23017 con 16 pines adicionales:

### Bank A (GPIOA0-A7) - Control Dirección Motores Tracción
- GPIOA0: FL_IN1 (Motor Front Left dirección)
- GPIOA1: FL_IN2 (Motor Front Left dirección)
- GPIOA2: FR_IN1 (Motor Front Right dirección)
- GPIOA3: FR_IN2 (Motor Front Right dirección)
- GPIOA4: RL_IN1 (Motor Rear Left dirección)
- GPIOA5: RL_IN2 (Motor Rear Left dirección)
- GPIOA6: RR_IN1 (Motor Rear Right dirección)
- GPIOA7: RR_IN2 (Motor Rear Right dirección)

### Bank B (GPIOB0-B7) - Shifter y Expansión
- GPIOB0: SHIFTER_D1 (Posición cambio bit 0)
- GPIOB1: SHIFTER_D2 (Posición cambio bit 1)
- GPIOB2: SHIFTER_D3 (Posición cambio bit 2)
- GPIOB3: SHIFTER_D4 (Posición cambio bit 3)
- GPIOB4: SHIFTER_D5 (Posición cambio bit 4)
- GPIOB5-B7: 🆓 LIBRES (disponibles para expansión futura)

---

## CAMBIOS RECIENTES

### v2.16.1
- ✅ **GPIO 1**: Asignado a WHEEL_RR (movido desde GPIO 16/46)

### v2.15.0 (2026-01-05)
- ✅ **GPIO 40**: Asignado a KEY_ON (antes BTN_MEDIA)
- ✅ **GPIO 41**: Asignado a KEY_OFF (antes BTN_4X4)

### v2.11.3 (2026-01-09) - CAMBIO ACTUAL
- ✅ **GPIO 46**: Asignado a RELAY_SPARE (antes libre desde v2.12.0)
- ✅ **GPIO 7**: Liberado (antes RELAY_SPARE)

---

## NOTAS IMPORTANTES

### ⚠️ Precauciones con Strapping Pins
1. **GPIO 0**: Si se usa, necesita pull-up externo. LOW durante boot entra en modo descarga.
2. **GPIO 46**: Ahora usado para RELAY_SPARE. Debe configurarse como OUTPUT lo antes posible en boot.
3. **GPIO 45**: Si se usa, considerar impacto en voltaje VDD_SPI.

### ✅ Pines Más Seguros para Expansión
Si necesitas más GPIOs, los más recomendados son:
1. **GPIO 7** - Completamente seguro, sin restricciones
2. Pines del MCP23017 Bank B (GPIOB5-B7) - 3 pines adicionales disponibles

### 🔒 Pines Reservados del Sistema
Los siguientes pines están reservados por hardware y NO están disponibles:
- GPIO 22-34: No existen en ESP32-S3
- GPIO 26-32: Reservados para SPI Flash/PSRAM (QSPI)
- GPIO 33-34: Reservados para SPI Flash (QIO mode)

---

## MAPA VISUAL DE DISPONIBILIDAD

```
GPIO 0:  🆓 LIBRE (strapping)
GPIO 1:  ✅ EN USO (WHEEL_RR)
GPIO 2:  ✅ EN USO (BTN_LIGHTS)
GPIO 3:  ✅ EN USO (WHEEL_FL, strapping)
GPIO 4:  ✅ EN USO (PEDAL ADC)
GPIO 5:  ✅ EN USO (RELAY_TRAC)
GPIO 6:  ✅ EN USO (RELAY_DIR)
GPIO 7:  🆓 LIBRE ⭐ RECOMENDADO
GPIO 8:  ✅ EN USO (I2C_SDA)
GPIO 9:  ✅ EN USO (I2C_SCL)
GPIO 10: ✅ EN USO (TFT_SCK)
GPIO 11: ✅ EN USO (TFT_MOSI)
GPIO 12: ✅ EN USO (TFT_MISO)
GPIO 13: ✅ EN USO (TFT_DC)
GPIO 14: ✅ EN USO (TFT_RST)
GPIO 15: ✅ EN USO (WHEEL_RL)
GPIO 16: ✅ EN USO (TFT_CS)
GPIO 17: ✅ EN USO (DFPLAYER_RX)
GPIO 18: ✅ EN USO (DFPLAYER_TX)
GPIO 19: ✅ EN USO (LED_FRONT)
GPIO 20: ✅ EN USO (ONEWIRE)
GPIO 21: ✅ EN USO (TOUCH_CS)
GPIO 35: ✅ EN USO (RELAY_MAIN)
GPIO 36: ✅ EN USO (WHEEL_FR)
GPIO 37: ✅ EN USO (ENCODER_A)
GPIO 38: ✅ EN USO (ENCODER_B)
GPIO 39: ✅ EN USO (ENCODER_Z)
GPIO 40: ✅ EN USO (KEY_ON)
GPIO 41: ✅ EN USO (KEY_OFF)
GPIO 42: ✅ EN USO (TFT_BL)
GPIO 43: ✅ EN USO (TOFSENSE_TX)
GPIO 44: ✅ EN USO (TOFSENSE_RX)
GPIO 45: 🆓 LIBRE (strapping)
GPIO 46: ✅ EN USO (RELAY_SPARE, strapping)
GPIO 47: ✅ EN USO (TOUCH_IRQ)
GPIO 48: ✅ EN USO (LED_REAR)
```

---

**Total GPIOs ESP32-S3**: 36 disponibles  
**En uso**: 33 pines  
**Libres**: 3 pines (GPIO 0, 7, 45)  
**Mejor opción para expansión**: **GPIO 7** ⭐

---

*Documento generado automáticamente desde include/pins.h*  
*Para cambios en asignación, consultar y modificar include/pins.h*
