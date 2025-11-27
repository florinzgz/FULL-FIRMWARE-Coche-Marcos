# 📚 Documentación del Firmware - ESP32-S3 Car Control System

**Versión Firmware:** 2.8.0  
**Placa:** ESP32-S3-DevKitC-1 (44 pines)  
**Última actualización:** 2025-11-27

---

## 📋 Índice de Documentación

Este directorio contiene toda la documentación técnica, manuales y configuraciones del sistema de control del coche eléctrico inteligente.

---

## 🔧 Configuración del Proyecto

| Archivo | Descripción |
|---------|-------------|
| [PROJECT_CONFIG.ini](PROJECT_CONFIG.ini) | Configuración completa del proyecto: hardware, pines, librerías y características |

---

## 📊 Informes de Estado

| Archivo | Descripción |
|---------|-------------|
| [FIRMWARE_FINAL_STATUS.md](FIRMWARE_FINAL_STATUS.md) | Estado final del firmware v2.8.0 - Sistema 100% operativo |
| [INFORME_AUDITORIA.md](INFORME_AUDITORIA.md) | Auditoría completa del firmware con verificación de todos los módulos |
| [INFORME_CHECKLIST.md](INFORME_CHECKLIST.md) | Checklist de verificación del sistema completo |
| [CAMBIOS_RECIENTES.md](CAMBIOS_RECIENTES.md) | Historial de cambios y novedades del firmware |

---

## 🔌 Hardware y Conexiones

| Archivo | Descripción |
|---------|-------------|
| [REFERENCIA_HARDWARE.md](REFERENCIA_HARDWARE.md) | **📌 Referencia principal del hardware** - Componentes, especificaciones, arquitectura y conexiones GPIO actualizadas (v2.8.0) |
| [PIN_MAPPING_DEVKITC1.md](PIN_MAPPING_DEVKITC1.md) | **📌 Mapeo oficial de pines GPIO** - Asignación completa de pines para ESP32-S3-DevKitC-1 (v2.4.0) |
| [HARDWARE_CONFIGURACION_COMPLETA.md](HARDWARE_CONFIGURACION_COMPLETA.md) | Configuración completa del hardware del sistema |
| [HY-M158_MAPPING.md](HY-M158_MAPPING.md) | Mapeo de canales de los módulos optoacopladores HY-M158 |

> ⚠️ **IMPORTANTE**: Para conexiones de hardware, consultar siempre `REFERENCIA_HARDWARE.md` y `PIN_MAPPING_DEVKITC1.md` ya que contienen las asignaciones GPIO actualizadas y verificadas con el firmware.

---

## 🛡️ Sistemas de Seguridad

| Archivo | Descripción |
|---------|-------------|
| [SISTEMAS_SEGURIDAD_AVANZADOS.md](SISTEMAS_SEGURIDAD_AVANZADOS.md) | Sistemas avanzados: ABS, TCS y Frenado Regenerativo AI |

---

## 📡 Conectividad

| Archivo | Descripción |
|---------|-------------|
| [CONFIGURACION_WIFI_OTA.md](CONFIGURACION_WIFI_OTA.md) | Guía de configuración WiFi y actualizaciones OTA |

---

## ⚙️ Arquitectura del Firmware

| Archivo | Descripción |
|---------|-------------|
| [NON_BLOCKING_TIMING.md](NON_BLOCKING_TIMING.md) | Arquitectura de temporización non-blocking con millis() |
| [STANDALONE_MODE.md](STANDALONE_MODE.md) | Modo standalone para pruebas de pantalla sin hardware |

---

## 🔊 Audio y Alertas

| Archivo | Descripción |
|---------|-------------|
| [AUDIO_TRACKS_GUIDE.md](AUDIO_TRACKS_GUIDE.md) | Guía completa de tracks de audio para DFPlayer Mini (38 actuales + 30 sugeridos) |

---

## 📈 Mejoras y Roadmap

| Archivo | Descripción |
|---------|-------------|
| [MEJORAS_PROPUESTAS.md](MEJORAS_PROPUESTAS.md) | Análisis de limitaciones y propuestas de mejora para todos los módulos |

---

## 📁 Estructura del Repositorio

```
FULL-FIRMWARE-Coche-Marcos/
├── docs/                    # ← Estás aquí - Documentación completa
├── include/                 # Headers (.h)
│   ├── pins.h              # Definición de pines GPIO
│   ├── constants.h         # Constantes del sistema
│   ├── settings.h          # Configuración global
│   └── ...                 # Otros headers de módulos
├── src/                     # Código fuente (.cpp)
│   ├── main.cpp            # Punto de entrada
│   ├── core/               # Módulos core del sistema
│   ├── control/            # Control de tracción y dirección
│   ├── sensors/            # Lectura de sensores
│   ├── hud/                # Interfaz de usuario (HUD)
│   ├── lighting/           # Control de LEDs
│   ├── audio/              # Audio y DFPlayer
│   ├── menu/               # Sistema de menús
│   ├── safety/             # Sistemas de seguridad
│   └── system/             # Sistema y utilidades
├── data/                    # Recursos (imágenes, iconos)
├── audio/                   # Archivos de audio MP3
├── platformio.ini          # Configuración de compilación
└── project_config.ini      # Documentación de configuración
```

---

## 🚀 Comenzar

### Compilación del Firmware

```bash
# Compilar todos los entornos
pio run

# Compilar entorno de desarrollo (debug)
pio run -e esp32-s3-devkitc

# Compilar entorno de producción
pio run -e esp32-s3-devkitc-release

# Flashear al ESP32
pio run --target upload

# Monitor serie
pio device monitor
```

### Entornos de Compilación

| Entorno | Descripción |
|---------|-------------|
| `esp32-s3-devkitc` | Desarrollo con debug habilitado |
| `esp32-s3-devkitc-release` | Producción optimizada |
| `esp32-s3-devkitc-ota` | Con soporte para actualizaciones WiFi |
| `esp32-s3-devkitc-test` | Modo test con standalone display |

---

## 📊 Resumen del Sistema

### Hardware Principal
- **MCU:** ESP32-S3-DevKitC-1 (Dual-core LX7 @ 240MHz)
- **Flash:** 16MB
- **PSRAM:** 8MB
- **Display:** ST7796S 480x320 + Touch XPT2046
- **Audio:** DFPlayer Mini
- **LEDs:** WS2812B (28 frontales + 16 traseros)

### Sensores
- **Corriente:** 6x INA226 vía TCA9548A
- **Temperatura:** 4x DS18B20 (OneWire)
- **Velocidad ruedas:** 4x LJ12A3-4-Z/BX
- **Dirección:** Encoder E6B2-CWZ6C 1200PR
- **Pedal:** Sensor Hall A1324LUA-T

### Control de Motores
- **Tracción:** 4x BTS7960 (24V) vía 2x PCA9685
- **Dirección:** 1x BTS7960 (12V) vía PCA9685
- **Expansor GPIO:** MCP23017 para IN1/IN2 + Shifter

### Sistemas de Seguridad
- ✅ ABS (Anti-lock Braking System)
- ✅ TCS (Traction Control System)
- ✅ Frenado Regenerativo con IA
- ✅ Watchdog con recuperación automática
- ✅ I2C Recovery

---

## 📝 Notas Importantes

1. **Strapping Pins:** Evitar GPIO 0, 3, 45, 46 para funciones críticas
2. **I2C:** SDA=GPIO8, SCL=GPIO9 con pull-ups de 4.7kΩ
3. **Shifter:** Completamente migrado a MCP23017 GPIOB0-4
4. **LEDs:** Front=GPIO1, Rear=GPIO48

---

## 🔗 Enlaces Útiles

- **Código fuente:** [include/pins.h](../include/pins.h) - Definición de pines
- **Configuración build:** [platformio.ini](../platformio.ini) - Flags de compilación
- **Configuración proyecto:** [project_config.ini](PROJECT_CONFIG.ini) - Documentación completa

---

**Firmware 100% Operativo y Listo para Producción** ✅

*Documentación actualizada: 2025-11-27*
