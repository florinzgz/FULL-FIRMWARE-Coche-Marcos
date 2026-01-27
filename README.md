# ESP32-S3 Car Control System - FULL FIRMWARE

**Versión:** 2.17.1 (PHASE 14)  
**Hardware:** ESP32-S3 N16R8 (16MB Flash QIO + 8MB PSRAM QSPI @ 3.3V)  
**Placa de desarrollo:** ESP32-S3-DevKitC-1 (44 pines)  
**Última actualización:** 2026-01-12

---

## 🎯 Descripción

Sistema completo de control para vehículo eléctrico inteligente basado en ESP32-S3, con pantalla táctil TFT, control de motores, sensores múltiples, sistemas de seguridad (ABS, TCS) y telemetría en tiempo real.

## ✨ Características Principales

- **Display:** ST7796S 480x320 con touch XPT2046
- **Control de Motores:** 4 motores de tracción + dirección con BTS7960 y PCA9685
- **Sensores:** Corriente (INA226), temperatura (DS18B20), velocidad de ruedas, encoder de dirección
- **Seguridad:** ABS, TCS, frenado regenerativo con IA, watchdog, I2C recovery
- **Iluminación:** WS2812B LEDs (28 frontales + 16 traseros)
- **Audio:** DFPlayer Mini
- **Standalone:** Sin WiFi/OTA (actualización solo por USB - v2.11.0)

## 🚀 Inicio Rápido

### Requisitos

- [PlatformIO](https://platformio.org/) instalado
- ESP32-S3 N16R8 (16MB Flash + 8MB PSRAM @ 3.3V)
  - Compatible con placa de desarrollo ESP32-S3-DevKitC-1 (44 pines)
- Cable USB para programación

**⚠️ IMPORTANTE:** Este firmware está configurado específicamente para **ESP32-S3 N16R8** con 16MB Flash QIO y 8MB PSRAM QSPI @ 3.3V. Ver [PHASE14_N16R8_BOOT_CERTIFICATION.md](PHASE14_N16R8_BOOT_CERTIFICATION.md) para detalles de hardware.

### Compilación

```bash
# Clonar repositorio
git clone https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos.git
cd FULL-FIRMWARE-Coche-Marcos

# Compilar (entorno por defecto: esp32-s3-devkitc1-n16r8)
pio run

# Compilar y flashear
pio run -t upload

# Monitor serial (con exception decoder funcionando correctamente)
pio device monitor
```

### 🔧 Configuración del Board

El firmware usa un **board manifest personalizado** ubicado en `boards/esp32-s3-devkitc1-n16r8.json` que configura correctamente:
- **Flash:** 16MB QIO @ 80MHz
- **PSRAM:** 8MB OPI (Octal)
- **USB Serial:** Configuración automática según `ARDUINO_USB_CDC_ON_BOOT`

#### USB Serial Configuration

El comportamiento del puerto serial depende del flag de compilación `ARDUINO_USB_CDC_ON_BOOT`:

| ARDUINO_USB_CDC_ON_BOOT | UART 0 (RX/TX) | OTG (USB nativo) |
|-------------------------|----------------|------------------|
| 0 | `Serial` | `USBSerial` |
| 1 | `Serial0` | `Serial` |

- Si `ARDUINO_USB_CDC_ON_BOOT = 0`: `Serial` → UART, `USBSerial` → OTG
- Si `ARDUINO_USB_CDC_ON_BOOT = 1`: `Serial0` → UART, `Serial` → OTG

### 🔒 Sistema de Validación Pre-Vuelo

**NUEVO:** Este firmware incluye un sistema de validación de hardware que se ejecuta automáticamente antes de cada compilación para prevenir firmware que crashearía en tiempo de ejecución.

El sistema **bloquea la compilación** si detecta:
- ❌ Uso de TFT antes de `tft.init()`
- ❌ Acceso a I2C antes de `Wire.begin()`
- ❌ Escritura PWM antes de `ledcSetup()`
- ❌ Uso de GPIO antes de `pinMode()`
- ❌ Otros errores de inicialización que causan bootloops

**Beneficios:**
- ✅ Previene el bug de bootloop que afectó versiones anteriores
- ✅ Zero overhead en runtime (solo validación en build-time)
- ✅ Mensajes de error detallados con archivo, línea y solución
- ✅ Protege contra Guru Meditation Errors y watchdog resets

Ver [docs/HARDWARE_PREFLIGHT_SYSTEM.md](docs/HARDWARE_PREFLIGHT_SYSTEM.md) para detalles completos.

### Entornos Disponibles

| Entorno | Descripción |
|---------|-------------|
| `esp32-s3-devkitc1-n16r8` | **🎯 DEFAULT** - Board personalizado con configuración USB Serial correcta |

**Nota:** Los entornos alternativos han sido desactivados. Solo se usa `esp32-s3-devkitc1-n16r8` con el board manifest personalizado que resuelve todos los problemas de reinicios y configuración USB.

**⚠️ IMPORTANTE:** El entorno por defecto es `esp32-s3-devkitc1-n16r8`, que usa el board manifest personalizado en `boards/esp32-s3-devkitc1-n16r8.json`. Este resuelve problemas de reinicios y asegura que el exception decoder funcione correctamente. Ver [FIX_EXCEPTION_DECODER_PATH.md](FIX_EXCEPTION_DECODER_PATH.md) para más detalles.

## 📚 Documentación

La documentación completa está disponible en el directorio [`docs/`](docs/):

### 📖 Manuales de Usuario

- **[MANUAL_USUARIO.md](MANUAL_USUARIO.md)** - 🚗 **MANUAL COMPLETO DE USUARIO** - Guía detallada de uso del vehículo terminado y montado
- **[GUIA_RAPIDA_USUARIO.md](GUIA_RAPIDA_USUARIO.md)** - ⚡ **GUÍA RÁPIDA** - Referencia rápida para operación diaria

### 📌 Documentación Técnica

- **[HARDWARE.md](HARDWARE.md)** - 📌 **ESPECIFICACIÓN OFICIAL DEL HARDWARE** - Fuente única de verdad para N16R8
- **[docs/HARDWARE_PREFLIGHT_SYSTEM.md](docs/HARDWARE_PREFLIGHT_SYSTEM.md)** - 🔒 **Sistema de Validación Pre-Vuelo** - Prevención de bootloops
- **[CLEANUP_SUMMARY_N16R8.md](CLEANUP_SUMMARY_N16R8.md)** - Resumen de consolidación del repositorio
- **[docs/README.md](docs/README.md)** - Índice completo de documentación
- **[docs/PLAN_SEPARACION_STM32_CAN.md](docs/PLAN_SEPARACION_STM32_CAN.md)** - Plan de separación ESP32 HMI + STM32 control
- **[docs/MANUAL_TRANSRECEPTORES_STM32_ESP32.md](docs/MANUAL_TRANSRECEPTORES_STM32_ESP32.md)** - 📡 **Manual de transreceptores CAN** - Conexión ESP32-S3 ↔ STM32G474RE
- **[RESPUESTA_TRANSRECEPTORES.md](RESPUESTA_TRANSRECEPTORES.md)** - Resumen rápido: 2× TJA1051T/3 y conexión CAN
- **[PHASE14_N16R8_BOOT_CERTIFICATION.md](PHASE14_N16R8_BOOT_CERTIFICATION.md)** - Certificación de hardware N16R8
- **[PHASE14_QUICK_REFERENCE.md](PHASE14_QUICK_REFERENCE.md)** - Guía rápida de migración a N16R8
- **[HARDWARE_VERIFICATION.md](HARDWARE_VERIFICATION.md)** - Verificación de hardware y datasheets
- **[BUILD_INSTRUCTIONS_v2.11.0.md](BUILD_INSTRUCTIONS_v2.11.0.md)** - Instrucciones de compilación detalladas
- **[CHANGELOG_v2.11.0.md](CHANGELOG_v2.11.0.md)** - Historial de cambios
- **[GUIA_RAPIDA.md](GUIA_RAPIDA.md)** - Guía rápida de calibración de touch
- **[CHECKLIST.md](CHECKLIST.md)** - Checklist de verificación del sistema

### Documentación Técnica Destacada

- **Hardware:** [docs/PIN_MAPPING_DEVKITC1.md](docs/PIN_MAPPING_DEVKITC1.md) | [docs/REFERENCIA_HARDWARE.md](docs/REFERENCIA_HARDWARE.md)
- **Touch:** [docs/TOUCH_CALIBRATION_QUICK_GUIDE.md](docs/TOUCH_CALIBRATION_QUICK_GUIDE.md) | [docs/TOUCH_TROUBLESHOOTING.md](docs/TOUCH_TROUBLESHOOTING.md)
- **Códigos de Error:** [docs/CODIGOS_ERROR.md](docs/CODIGOS_ERROR.md)
- **Sistema:** [docs/FIRMWARE_FINAL_STATUS.md](docs/FIRMWARE_FINAL_STATUS.md)

## 🔧 Configuración Importante

### Hardware ESP32-S3

**Módulo:** ESP32-S3 N16R8  
**Placa de desarrollo:** ESP32-S3-DevKitC-1 (44 pines)  
**Memoria:**
- Flash: 16MB (QIO mode, 4-bit, 3.3V)
- PSRAM: 8MB (QSPI mode, 4-bit, 3.3V, AP_3v3)

⚠️ **IMPORTANTE:** Ver [PHASE14_N16R8_BOOT_CERTIFICATION.md](PHASE14_N16R8_BOOT_CERTIFICATION.md) para detalles completos de hardware y certificación de boot.

### Pines Principales (ESP32-S3)

```cpp
// Display TFT (ST7796S - HSPI)
TFT_CS=16, TFT_DC=13, TFT_RST=14
TFT_MOSI=11, TFT_MISO=12, TFT_SCLK=10, TFT_BL=42

// Touch (XPT2046)
TOUCH_CS=21

// I2C Bus
SDA=GPIO8, SCL=GPIO9

// LEDs
LED_FRONT=GPIO1 (WS2812B - 28 LEDs)
LED_REAR=GPIO48 (WS2812B - 16 LEDs)
```

Ver [`include/pins.h`](include/pins.h) para el mapeo completo.

### Stack Size (Configurado para estabilidad)

```ini
CONFIG_ARDUINO_LOOP_STACK_SIZE=32768   ; 32 KB
CONFIG_ESP_MAIN_TASK_STACK_SIZE=20480  ; 20 KB
CONFIG_ESP_IPC_TASK_STACK_SIZE=3072    ; 3 KB
```

## 🐛 Solución de Problemas

### Touch no funciona

1. **Calibración con botón físico:** Mantén presionado botón 4X4 por 5 segundos
2. **Documentación completa:** [GUIA_RAPIDA.md](GUIA_RAPIDA.md)

### Build o errores de compilación

1. **Limpiar build:** `pio run -t clean`
2. **Actualizar librerías:** `pio pkg update`
3. **Ver instrucciones:** [BUILD_INSTRUCTIONS_v2.11.0.md](BUILD_INSTRUCTIONS_v2.11.0.md)

### Sistema crashea o boot loop

1. **Revisar logs:** Monitor serial a 115200 baud
2. **Verificar conexiones:** Especialmente I2C (SDA=8, SCL=9)
3. **Consultar:** [docs/INFORME_AUDITORIA_2025-12-07.md](docs/INFORME_AUDITORIA_2025-12-07.md)

## 📊 Estado del Proyecto

- ✅ **Firmware:** v2.17.1 PHASE 14 - 100% operativo con N16R8
- ✅ **Hardware:** Migrado a ESP32-S3-N16R8 (16MB Flash + 8MB PSRAM @ 3.3V)
- ✅ **Producción:** Listo para uso
- ✅ **Seguridad:** Standalone (sin WiFi/OTA)
- ✅ **Estabilidad:** Stack sizes optimizados, watchdog implementado
- ✅ **Boot:** Certificado para boot sin fallos en hardware N16R8

## 🔗 Enlaces Útiles

- **Repositorio:** [https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos](https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos)
- **GitHub Actions:** [Builds automatizados](https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos/actions)
- **Documentación completa:** [docs/README.md](docs/README.md)

## 📝 Novedades PHASE 14 (v2.17.1)

- ✅ **Hardware Migrado:** Firmware ahora ejecuta exclusivamente en ESP32-S3 N16R8
  - 16MB Flash QIO (4-bit, 3.3V)
  - 8MB PSRAM QSPI (4-bit, 3.3V)
- 🔄 **Configuración Unificada:** Eliminados todos los restos de N32R16V (32MB OPI)
- ⚡ **Mayor Confiabilidad:** Dominio único de voltaje 3.3V (no más 1.8V)
- 📦 **Nuevas Particiones:** Tablas de partición optimizadas para 16MB flash
- 🛡️ **Boot Certificado:** Sin dependencias de eFuse, configuración probada
- 📚 **Documentación Oficial:** Ver [HARDWARE.md](HARDWARE.md) para especificación completa

Ver [PHASE14_QUICK_REFERENCE.md](PHASE14_QUICK_REFERENCE.md) para detalles técnicos.

### Novedades v2.11.0

- 🔒 **Eliminación de WiFi/OTA** por seguridad (firmware 100% standalone)
- 📦 **Librerías pinned** a versiones exactas para builds reproducibles
- 🧹 **Limpieza de entornos** - Solo entornos esenciales
- 📚 **Documentación actualizada** y reorganizada
- ⚡ **Stack sizes optimizados** para máxima estabilidad

Ver [CHANGELOG_v2.11.0.md](CHANGELOG_v2.11.0.md) para detalles completos.

---

## 📄 Licencia

Este proyecto es de código abierto. Ver el archivo LICENSE para más detalles.

---

**Desarrollado con ❤️ para control de vehículos eléctricos inteligentes**

*Última actualización: 2026-01-12 (PHASE 14 - Hardware Migration to N16R8)*
