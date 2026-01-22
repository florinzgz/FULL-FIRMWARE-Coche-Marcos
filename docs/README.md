# 📚 Documentación del Sistema

## ESP32-S3 Car Control System - Coche Inteligente Marcos

**Versión Actual:** v2.17.1  
**Fecha:** 2026-01-12  
**Estado:** ✅ Actualizado con hardware N16R8 (PHASE 14)

---

## 🎯 DOCUMENTOS PRINCIPALES

### 🔌 **[CONEXIONES_HARDWARE_v2.15.0.md](CONEXIONES_HARDWARE_v2.15.0.md)** ⭐ GUÍA MAESTRA
**📘 LA BIBLIA DE CONEXIONES - DOCUMENTO PRINCIPAL**

Guía completa y actualizada de todas las conexiones hardware:
- ✅ TOFSense-M S 8x8 LiDAR (UART0, 921600 baud, 4m, 65° FOV)
- ✅ DFPlayer Mini Audio (UART1, GPIO 18/17)
- ✅ Power Control (GPIO 40/41 - pines estables, no strapping)
- ✅ Pantalla TFT + Touch (SPI, GPIO 10-16)
- ✅ Bus I²C reorganizado (TCA9548A, INA226, PCA9685)
- ✅ Motores tracción y dirección (BTS7960)
- ✅ Sensores ruedas, encoder, temperatura
- ✅ LEDs WS2812B, relés, pedal
- ✅ GPIOs libres (0, 45, 46) y notas strapping pins
- ✅ Checklist verificación completo

**Cada cable identificado por color, calibre y función. Diagramas ASCII claros.**

---

## 📖 DOCUMENTOS COMPLEMENTARIOS

### 🔧 Hardware y Sensores
- [**PIN_MAPPING_DEVKITC1.md**](PIN_MAPPING_DEVKITC1.md) - Mapeo detallado pines ESP32-S3
- [**TOFSENSE_INTEGRATION.md**](TOFSENSE_INTEGRATION.md) - Protocolo TOFSense-M S 8x8 Matrix
- [**OBSTACLE_SAFETY_FEATURES.md**](OBSTACLE_SAFETY_FEATURES.md) - Sistema seguridad anticolisión
- [**SENSORES_TEMPERATURA_DS18B20.md**](SENSORES_TEMPERATURA_DS18B20.md) - Temp DS18B20
- [**REFERENCIA_HARDWARE.md**](REFERENCIA_HARDWARE.md) - Especificaciones hardware

### 🖥️ Display y Táctil
- [**CONFIGURACION_TFT_ESPI.md**](CONFIGURACION_TFT_ESPI.md) - Config TFT_eSPI library
- [**DISPLAY_TOUCH_VERIFICATION.md**](DISPLAY_TOUCH_VERIFICATION.md) - Verificación display/touch
- [**TOUCH_CALIBRATION_QUICK_GUIDE.md**](TOUCH_CALIBRATION_QUICK_GUIDE.md) - Calibración táctil
- [**HY-M158_MAPPING.md**](HY-M158_MAPPING.md) - Optoacopladores

### 🔊 Audio
- [**AUDIO_TRACKS_GUIDE.md**](AUDIO_TRACKS_GUIDE.md) - Guía de pistas de audio
- [**AUDIO_IMPLEMENTATION_SUMMARY.md**](AUDIO_IMPLEMENTATION_SUMMARY.md) - Implementación audio

### 🏗️ Arquitectura y Seguridad
- [**ARCHITECTURE.md**](ARCHITECTURE.md) - Arquitectura del sistema
- [**PLAN_SEPARACION_STM32_CAN.md**](PLAN_SEPARACION_STM32_CAN.md) - Plan de separación ESP32 HMI + STM32 control
- [**SISTEMAS_SEGURIDAD_AVANZADOS.md**](SISTEMAS_SEGURIDAD_AVANZADOS.md) - Sistemas seguridad
- [**TOLERANCIA_FALLOS.md**](TOLERANCIA_FALLOS.md) - Tolerancia a fallos

### 🧪 Testing y Validación
- [**DEPLOYMENT_TESTING_GUIDE.md**](DEPLOYMENT_TESTING_GUIDE.md) - Guía testing deployment
- [**GUIA_PRUEBAS_INCREMENTALES.md**](GUIA_PRUEBAS_INCREMENTALES.md) - Pruebas incrementales
- [**TESTING_IMPLEMENTATION_SUMMARY.md**](TESTING_IMPLEMENTATION_SUMMARY.md) - Resumen testing

### 📝 Informes y Estado
- [**CAMBIOS_RECIENTES.md**](CAMBIOS_RECIENTES.md) - Changelog actualizado
- [**FIRMWARE_FINAL_STATUS.md**](FIRMWARE_FINAL_STATUS.md) - Estado final firmware
- [**INFORME_CHECKLIST.md**](INFORME_CHECKLIST.md) - Checklist validación

### 🛠️ Configuración y Tools
- [**GIT_CONFIGURACION.md**](GIT_CONFIGURACION.md) - Configuración Git
- [**CODIGOS_ERROR.md**](CODIGOS_ERROR.md) - Códigos de error sistema

---

## 🗂️ ARCHIVO HISTÓRICO

Documentos obsoletos movidos a `archive/`:
- Manuales de conexión antiguos (pre-v2.15.0)
- Documentación VL53L5X (sensor reemplazado)
- Informes de migración intermedios
- Guías de touch antiguas

Ver carpeta [archive/](archive/) para historial completo.

---

## ⚡ ACCESO RÁPIDO

### Solución de Problemas
1. **Touch no funciona**: Ver [TOUCH_CALIBRATION_QUICK_GUIDE.md](TOUCH_CALIBRATION_QUICK_GUIDE.md)
2. **Errores en menú**: Ver [CODIGOS_ERROR.md](CODIGOS_ERROR.md)
3. **Sensor de obstáculos**: Ver [OBSTACLE_SAFETY_FEATURES.md](OBSTACLE_SAFETY_FEATURES.md)
4. **Conexión incorrecta**: Ver [CONEXIONES_HARDWARE_v2.15.0.md](CONEXIONES_HARDWARE_v2.15.0.md)

### Cambios Importantes v2.15.0
- ✅ VL53L5X I²C → TOFSense-M S UART (8x8 matrix, 64 puntos)
- ✅ Power control: GPIO 0/45 → GPIO 40/41 (strapping pins liberados)
- ✅ DFPlayer: UART0 → UART1 (GPIO 18/17)
- ✅ GPIOs 0, 45, 46 completamente libres
- ✅ HUD limpio sin iconos multimedia/luces
- ✅ Tracción 4x4 solo por touch

Ver [CAMBIOS_RECIENTES.md](CAMBIOS_RECIENTES.md) para detalles completos.

---

**Documentación mantenida por:** Sistema de control automático  
**Última revisión:** 2026-01-12  
**Versión firmware:** v2.17.1
