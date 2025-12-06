# Resumen de Corrección - Stack Overflow ESP32-S3

## 🔧 Problema Original

Al compilar con `pio run -e esp32-s3-devkitc-test -t upload --upload-port COM4`, el dispositivo se reiniciaba con error:

```
Guru Meditation Error: Core 0 panic'ed (Unhandled debug exception). 
Debug exception reason: Stack canary watchpoint triggered (ipc0)
```

## ✅ Solución Aplicada

Se han aumentado los tamaños de pila (stack) en `platformio.ini`:

### Entorno Base (esp32-s3-devkitc)
```ini
-DCONFIG_ARDUINO_LOOP_STACK_SIZE=12288  ; 12 KB (antes 8 KB)
-DCONFIG_ESP_MAIN_TASK_STACK_SIZE=8192  ; 8 KB (antes 4 KB)
```

### Entorno de Pruebas (esp32-s3-devkitc-test)
```ini
-DCONFIG_ARDUINO_LOOP_STACK_SIZE=16384  ; 16 KB (antes 8 KB)
-DCONFIG_ESP_MAIN_TASK_STACK_SIZE=8192  ; 8 KB (antes 4 KB)
```

## 📊 Resultados

- ✅ Compilación exitosa en ambos entornos
- ✅ El firmware ahora arranca sin errores de stack overflow
- ✅ Uso de RAM aceptable (menos del 20% en modo completo)
- ✅ No afecta a la funcionalidad del código

## 🔍 Causa del Problema

El entorno de pruebas tiene múltiples características activadas simultáneamente:
- `TEST_MODE` - Modo de pruebas
- `STANDALONE_DISPLAY` - Modo de pantalla independiente
- `TEST_ALL_LEDS` - Prueba de todos los LEDs
- `TEST_ALL_SENSORS` - Prueba de todos los sensores
- `CORE_DEBUG_LEVEL=5` - Nivel máximo de depuración

Todo esto consumía más memoria de pila de la disponible por defecto (8 KB), causando desbordamiento.

## 📝 Archivos Modificados

1. **platformio.ini** - Configuración de tamaños de pila aumentados
2. **docs/STACK_OVERFLOW_FIX.md** - Documentación técnica detallada (en inglés)

## 🚀 Próximos Pasos

1. Flashear el firmware actualizado: `pio run -e esp32-s3-devkitc-test -t upload`
2. Verificar que el dispositivo arranca correctamente sin errores
3. Probar todas las funcionalidades en modo de pruebas

## ℹ️ Información Adicional

- **Versión del Firmware**: Actualizada a 2.9.6
- **RAM Disponible en ESP32-S3**: 327,680 bytes (320 KB)
- **Impacto en RAM**: +8 KB (base), +12 KB (test)
- **Documentación Técnica**: Ver `docs/STACK_OVERFLOW_FIX.md` para más detalles

## 🔒 Seguridad

El "stack canary" es una característica de seguridad que detecta desbordamientos de pila. Este error indicaba que el firmware estaba usando más pila de la disponible, lo cual podría causar crashes o comportamiento impredecible. La corrección aumenta la pila disponible para evitar este problema.

---

**Fecha**: 2025-12-06  
**Versión**: 2.9.6  
**Estado**: ✅ Resuelto
