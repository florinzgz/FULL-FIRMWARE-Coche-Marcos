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

**🚨 PROBLEMAS DE BOOTLOOP:** Si experimentas bootloops, errores de core dump o el puerto COM desaparece, consulta:
- **[BOOTLOADER_RECOVERY_QUICKSTART.md](BOOTLOADER_RECOVERY_QUICKSTART.md)** - Solución rápida en 3 pasos
- **[docs/ESP32_S3_BOOTLOADER_TROUBLESHOOTING.md](docs/ESP32_S3_BOOTLOADER_TROUBLESHOOTING.md)** - Guía completa de troubleshooting

### Compilación

```bash
# Clonar repositorio
git clone https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos.git
cd FULL-FIRMWARE-Coche-Marcos

# Compilar (entorno de desarrollo)
pio run -e esp32-s3-n16r8

# Compilar y flashear (producción)
pio run -e esp32-s3-n16r8-release -t upload

# Monitor serial
pio device monitor
```

### Entornos Disponibles

| Entorno | Descripción |
|---------|-------------|
| `esp32-s3-n16r8` | Desarrollo con debug (CORE_DEBUG_LEVEL=3) |
| `esp32-s3-n16r8-release` | **Producción** - Optimizado (-O3, sin debug) |
| `esp32-s3-n16r8-touch-debug` | Debug de touch (logs verbosos) |
| `esp32-s3-n16r8-no-touch` | Sin touch (diagnóstico SPI) |
| `esp32-s3-n16r8-standalone` | Display standalone sin sensores |
| `esp32-s3-n16r8-standalone-debug` | Standalone con debug verboso |

## 📚 Documentación

La documentación completa está disponible en el directorio [`docs/`](docs/):

- **[docs/README.md](docs/README.md)** - Índice completo de documentación
- **[BOOTLOADER_RECOVERY_QUICKSTART.md](BOOTLOADER_RECOVERY_QUICKSTART.md)** - 🚨 **Solución rápida para bootloop/core dump**
- **[docs/ESP32_S3_BOOTLOADER_TROUBLESHOOTING.md](docs/ESP32_S3_BOOTLOADER_TROUBLESHOOTING.md)** - 🔧 **Troubleshooting completo de bootloader**
- **[PHASE14_N16R8_BOOT_CERTIFICATION.md](PHASE14_N16R8_BOOT_CERTIFICATION.md)** - ⚠️ **NUEVO PHASE 14** - Certificación de hardware N16R8
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

- 🔄 **Migración de Hardware:** De N32R16V (32MB OPI) a N16R8 (16MB QIO + 8MB QSPI @ 3.3V)
- 🚀 **Simplificación:** Eliminación completa de OPI/OCT - Solo QIO + QSPI estándar
- ⚡ **Mayor Confiabilidad:** Dominio único de voltaje 3.3V (no más 1.8V)
- 📦 **Nuevas Particiones:** Tablas de partición optimizadas para 16MB flash
- 🛡️ **Boot Certificado:** Sin dependencias de eFuse, configuración probada
- 📚 **Documentación Completa:** Certificación detallada en PHASE14_N16R8_BOOT_CERTIFICATION.md

Ver [PHASE14_QUICK_REFERENCE.md](PHASE14_QUICK_REFERENCE.md) para guía de migración.

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
