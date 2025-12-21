# ESP32-S3 Car Control System - FULL FIRMWARE

**Versión:** 2.11.0  
**Hardware:** ESP32-S3-DevKitC-1 (44 pines)  
**Última actualización:** 2025-12-19

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
- ESP32-S3-DevKitC-1 (44 pines)
- Cable USB para programación

### Compilación

```bash
# Clonar repositorio
git clone https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos.git
cd FULL-FIRMWARE-Coche-Marcos

# Compilar (entorno de desarrollo)
pio run -e esp32-s3-devkitc

# Compilar y flashear (producción)
pio run -e esp32-s3-devkitc-release -t upload

# Monitor serial
pio device monitor
```

### Entornos Disponibles

| Entorno | Descripción |
|---------|-------------|
| `esp32-s3-devkitc` | Desarrollo con debug (CORE_DEBUG_LEVEL=5) |
| `esp32-s3-devkitc-release` | **Producción** - Optimizado (-O3, sin debug) |
| `esp32-s3-devkitc-touch-debug` | Debug de touch (logs verbosos) |
| `esp32-s3-devkitc-no-touch` | Sin touch (diagnóstico SPI) |
| `esp32-s3-test-incremental` | **Test incremental** - Añadir hardware paso a paso |

> 💡 **Nuevo:** Usa `esp32-s3-test-incremental` para verificar la pantalla primero y luego añadir sensores progresivamente. Ver [docs/TEST_INCREMENTAL.md](docs/TEST_INCREMENTAL.md) para instrucciones detalladas.

## 📚 Documentación

La documentación completa está disponible en el directorio [`docs/`](docs/):

- **[docs/README.md](docs/README.md)** - Índice completo de documentación
- **[BUILD_INSTRUCTIONS_v2.11.0.md](BUILD_INSTRUCTIONS_v2.11.0.md)** - Instrucciones de compilación detalladas
- **[CHANGELOG_v2.11.0.md](CHANGELOG_v2.11.0.md)** - Historial de cambios
- **[GUIA_RAPIDA.md](GUIA_RAPIDA.md)** - Guía rápida de calibración de touch
- **[CHECKLIST.md](CHECKLIST.md)** - Checklist de verificación del sistema

### Documentación Técnica Destacada

- **Hardware:** [docs/PIN_MAPPING_DEVKITC1.md](docs/PIN_MAPPING_DEVKITC1.md) | [docs/REFERENCIA_HARDWARE.md](docs/REFERENCIA_HARDWARE.md)
- **Touch:** [docs/TOUCH_CALIBRATION_QUICK_GUIDE.md](docs/TOUCH_CALIBRATION_QUICK_GUIDE.md) | [docs/TOUCH_TROUBLESHOOTING.md](docs/TOUCH_TROUBLESHOOTING.md)
- **Test Incremental:** [docs/TEST_INCREMENTAL.md](docs/TEST_INCREMENTAL.md) - Verificación paso a paso del hardware
- **Códigos de Error:** [docs/CODIGOS_ERROR.md](docs/CODIGOS_ERROR.md)
- **Sistema:** [docs/FIRMWARE_FINAL_STATUS.md](docs/FIRMWARE_FINAL_STATUS.md)

## 🔧 Configuración Importante

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

- ✅ **Firmware:** v2.11.0 - 100% operativo
- ✅ **Hardware:** Completamente testeado en ESP32-S3-DevKitC-1
- ✅ **Producción:** Listo para uso
- ✅ **Seguridad:** Standalone (sin WiFi/OTA)
- ✅ **Estabilidad:** Stack sizes optimizados, watchdog implementado

## 🔗 Enlaces Útiles

- **Repositorio:** [https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos](https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos)
- **GitHub Actions:** [Builds automatizados](https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos/actions)
- **Documentación completa:** [docs/README.md](docs/README.md)

## 📝 Novedades v2.11.0

- 🔒 **Eliminación de WiFi/OTA** por seguridad (firmware 100% standalone)
- 📦 **Librerías pinned** a versiones exactas para builds reproducibles
- 🧹 **Limpieza de entornos** - Solo 4 entornos esenciales
- 📚 **Documentación actualizada** y reorganizada
- ⚡ **Stack sizes optimizados** para máxima estabilidad

Ver [CHANGELOG_v2.11.0.md](CHANGELOG_v2.11.0.md) para detalles completos.

---

## 📄 Licencia

Este proyecto es de código abierto. Ver el archivo LICENSE para más detalles.

---

**Desarrollado con ❤️ para control de vehículos eléctricos inteligentes**

*Última actualización: 2025-12-19*
