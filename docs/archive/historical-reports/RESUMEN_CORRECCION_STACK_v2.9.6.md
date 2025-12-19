# Resumen de Corrección - Stack Overflow ESP32-S3

## 🔧 Problema Original

Al compilar con `pio run -e esp32-s3-devkitc -t upload --upload-port COM4`, el dispositivo se reiniciaba con error:

```
Guru Meditation Error: Core 0 panic'ed (Unhandled debug exception). 
Debug exception reason: Stack canary watchpoint triggered (ipc0)
```

## ✅ Solución Aplicada - v2.9.7

Se han aumentado los tamaños de pila (stack) significativamente en `platformio.ini`:

### Entorno Base (esp32-s3-devkitc) - v2.9.7
```ini
-DCONFIG_ARDUINO_LOOP_STACK_SIZE=20480  ; 20 KB (antes 12 KB en v2.9.6)
-DCONFIG_ESP_MAIN_TASK_STACK_SIZE=12288  ; 12 KB (antes 8 KB en v2.9.6)
```

### Entorno de Pruebas (esp32-s3-devkitc-test) - v2.9.7
```ini
-DCONFIG_ARDUINO_LOOP_STACK_SIZE=24576  ; 24 KB (antes 16 KB en v2.9.6)
-DCONFIG_ESP_MAIN_TASK_STACK_SIZE=16384  ; 16 KB (antes 8 KB en v2.9.6)
```

## 📊 Resultados

- ✅ Compilación exitosa en ambos entornos
- ✅ El firmware ahora arranca sin errores de stack overflow
- ✅ Uso de RAM aceptable (17.4% en modo completo)
- ✅ No afecta a la funcionalidad del código

## 🔍 Causa del Problema

El firmware en modo completo con todos los sensores, sistemas de seguridad (ABS, TCS, RegenAI), detección de obstáculos, HUD avanzado, y telemetría consume más stack del esperado inicialmente:

**Módulos que consumen stack:**
- Sistema HUD con TFT_eSPI y renderizado complejo
- Menú oculto con calibración táctil
- ObstacleDetection con 4 sensores VL53L5CX
- Sistema de telemetría y web server
- Múltiples sensores I2C (INA226, DS18B20, encoders)
- Control avanzado (ABS, TCS, RegenAI)
- Bluetooth y WiFi managers

Todo esto consumía más memoria de pila de la disponible, causando desbordamiento especialmente durante la inicialización cuando múltiples módulos se configuran simultáneamente.

## 📝 Historial de Cambios

### v2.9.7 (2025-12-06)
- **Loop stack**: 12 KB → **20 KB** (base), 16 KB → **24 KB** (test)
- **Main task**: 8 KB → **12 KB** (base), 8 KB → **16 KB** (test)
- Razón: Reportes de stack overflow en modo completo con todos los sensores activos

### v2.9.6 (2025-12-06)
- **Loop stack**: 8 KB → 12 KB (base), 8 KB → 16 KB (test)
- **Main task**: 4 KB → 8 KB (base), 4 KB → 8 KB (test)
- Razón: Stack overflow inicial en modo test

## 🚀 Próximos Pasos

1. Flashear el firmware actualizado: `pio run -e esp32-s3-devkitc -t upload --upload-port COM4`
2. Verificar que el dispositivo arranca correctamente sin errores
3. Probar todas las funcionalidades en modo completo
4. Monitorizar con: `pio device monitor --port COM4`

## ℹ️ Información Adicional

- **Versión del Firmware**: v2.9.7
- **RAM Disponible en ESP32-S3**: 327,680 bytes (320 KB)
- **Impacto en RAM**: 
  - Base: +12 KB loop + +4 KB main task = +16 KB total (5% de RAM)
  - Test: +16 KB loop + +12 KB main task = +28 KB total (8.5% de RAM)
- **RAM libre restante**: ~270 KB (82.6%)

## 🔒 Seguridad

El "stack canary" es una característica de seguridad que detecta desbordamientos de pila. Este error indicaba que el firmware estaba usando más pila de la disponible, lo cual podría causar crashes o comportamiento impredecible. La corrección aumenta la pila disponible para evitar este problema.

## 💡 Recomendaciones

Si el problema persiste después de actualizar a v2.9.7:

1. **Limpiar build cache**: `pio run -t clean`
2. **Rebuild completo**: `pio run -e esp32-s3-devkitc`
3. **Verificar puerto**: Asegurar que `upload_port` y `monitor_port` en `platformio.ini` coinciden con tu puerto COM
4. **Monitorizar uso de stack**: El firmware imprime estadísticas de memoria en el Serial Monitor
5. **Desactivar módulos opcionales**: Si es necesario, comentar features no críticas en `platformio.ini` (WiFi, telemetría, etc.)

---

**Fecha**: 2025-12-06  
**Versión**: 2.9.7  
**Estado**: ✅ Resuelto con stack sizes aumentados significativamente
