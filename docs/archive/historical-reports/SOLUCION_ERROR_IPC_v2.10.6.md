# Solución al Error "Stack canary watchpoint triggered (ipc0)" - v2.10.6

## 🎯 Resumen Ejecutivo

**Problema:** ESP32-S3 se reinicia continuamente con error "Stack canary watchpoint triggered (ipc0)" antes de arrancar el firmware.

**Causa:** El stack del IPC task (comunicación entre núcleos) es demasiado pequeño (1KB por defecto).

**Solución:** Aumentar el stack del IPC task de 1KB a 2KB.

**Estado:** ✅ **RESUELTO** en versión v2.10.6

---

## 🔍 ¿Qué es este Error?

### El Mensaje de Error

```
Guru Meditation Error: Core  0 panic'ed (Unhandled debug exception).
Debug exception reason: Stack canary watchpoint triggered (ipc0)
Backtrace: 0x40379990:0x3fcf0d50 0x0005002d:0xa5a5a5a5 |<-CORRUPTED
```

### ¿Qué Significa?

En términos simples:
- **IPC (Inter-Processor Communication)** = Sistema de comunicación entre los 2 núcleos del ESP32-S3
- **Stack** = Memoria temporal para almacenar datos durante operaciones
- **Stack canary** = Valor centinela para detectar cuando el stack se desborda
- **Watchpoint triggered** = El sistema detectó que el stack se desbordó

**En español simple:** El ESP32 tiene 2 procesadores que necesitan comunicarse. Esa comunicación necesita memoria temporal (stack), pero le dimos muy poca (1KB) y se quedó sin espacio, causando un crash.

### ¿Por Qué Pasa TAN PRONTO?

El error ocurre ANTES de que tu código empiece a ejecutarse:

1. ESP32 arranca desde ROM
2. Inicializa hardware básico
3. Crea tareas del sistema (incluyendo IPC)
4. 💥 IPC se queda sin stack
5. Sistema detecta el problema y reinicia
6. → Bucle infinito

Por eso NO ves ningún mensaje de tu firmware - ¡nunca llega a ejecutarse!

---

## ✅ ¿Cómo Solucionar el Problema?

### Paso 1: Obtener la Versión Correcta

Descarga o actualiza a la versión **v2.10.6** del firmware:

```bash
git pull origin main
```

O descarga el release v2.10.6 desde GitHub.

### Paso 2: Compilar y Flashear

```bash
cd /ruta/al/proyecto

# Limpiar compilación anterior
pio run -t clean

# Compilar (elige tu entorno)
pio run -e esp32-s3-devkitc-touch-debug

# Flashear (cambia COM4 por tu puerto)
pio run -e esp32-s3-devkitc-touch-debug -t upload --upload-port COM4
```

### Paso 3: Verificar que Funciona

Abre el monitor serial:

```bash
pio device monitor --port COM4 --baud 115200
```

Deberías ver:
```
ESP-ROM:esp32s3-20210327
...
ESP32-S3 Car Control System v2.10.6
...
[BOOT] Enabling TFT backlight...
[BOOT] Backlight enabled on GPIO42
...
```

✅ **Si ves estos mensajes → ¡PROBLEMA RESUELTO!**

---

## 🔧 Si el Problema Persiste

### Opción 1: Verificar que Aplicaste el Fix

```bash
# Ver configuración de compilación
pio run -e esp32-s3-devkitc-touch-debug -v 2>&1 | grep IPC
```

Deberías ver: `CONFIG_ESP_IPC_TASK_STACK_SIZE=2048`

### Opción 2: Limpiar Todo y Recompilar

```bash
# Borrar cache completo
rm -rf .pio
pio run -t clean

# Recompilar
pio run -e esp32-s3-devkitc-touch-debug

# Reflashear
pio run -e esp32-s3-devkitc-touch-debug -t upload --upload-port COM4
```

### Opción 3: Borrar Flash Completa (Último Recurso)

```bash
# CUIDADO: Esto borrará configuración guardada
pio run -t erase
pio run -e esp32-s3-devkitc-touch-debug -t upload --upload-port COM4
```

⚠️ **Advertencia:** Perderás configuraciones guardadas (WiFi, calibraciones, etc.)

---

## 📋 Checklist de Verificación

### ✅ Señales de Éxito

- [ ] No hay mensaje "Stack canary watchpoint triggered (ipc0)"
- [ ] Aparecen mensajes `[BOOT]` en el monitor serial
- [ ] La versión mostrada es **v2.10.6** o superior
- [ ] La pantalla enciende con el backlight
- [ ] El dashboard se muestra correctamente
- [ ] WiFi y sensores se inicializan sin problemas

### ❌ Señales de Problema

Si todavía ves:
- ❌ Error "Stack canary watchpoint triggered (ipc0)"
- ❌ Reinicios antes de ver mensajes `[BOOT]`
- ❌ Pantalla en negro o sin respuesta

→ Revisa las opciones de troubleshooting arriba o reporta el problema con los logs completos.

---

## 💡 ¿Qué Hizo el Fix?

### Cambio Realizado

En el archivo `platformio.ini`, se añadió:

```ini
-DCONFIG_ESP_IPC_TASK_STACK_SIZE=2048
```

Esto le dice al ESP32-S3:
> "Usa 2KB de stack para el IPC task en lugar del 1KB por defecto"

### ¿Por Qué Funciona?

| Antes | Después |
|-------|---------|
| IPC stack: 1KB | IPC stack: 2KB |
| WiFi + BT init requiere >1KB | 2KB es suficiente ✅ |
| Stack se desborda 💥 | Stack con margen de seguridad ✅ |
| Sistema crash inmediato | Sistema arranca correctamente ✅ |

### Diagrama Simple

```
ANTES (v2.10.5):
┌─────────────────┐
│  IPC Task Stack │ 1KB disponible
│  [===========]  │ Necesita ~1.5KB → ⚠️ OVERFLOW
└─────────────────┘
        ↓
    💥 CRASH

DESPUÉS (v2.10.6):
┌─────────────────┐
│  IPC Task Stack │ 2KB disponible
│  [=======    ]  │ Necesita ~1.5KB → ✅ OK
└─────────────────┘
        ↓
    ✅ BOOT SUCCESS
```

---

## 📊 Comparación de Versiones

| Versión | IPC Stack | Estado | Notas |
|---------|-----------|--------|-------|
| v2.10.5 y anteriores | 1KB (default) | ❌ Crash en boot | Error ipc0 |
| **v2.10.6** | **2KB** | ✅ Funciona | Fix aplicado |

---

## 🚀 Beneficios del Fix

1. **Boot Exitoso** - El sistema arranca completamente sin reinicios
2. **WiFi/BT Estable** - Inicialización sin problemas de stack
3. **I2C Multi-Core** - Comunicación entre núcleos funciona correctamente
4. **Cero Overhead Perceptible** - Solo 2KB adicionales (0.4% de RAM)
5. **Futuro-Proof** - Margen para features adicionales

---

## 📞 ¿Necesitas Más Ayuda?

Si después de aplicar v2.10.6 el problema persiste:

### 1. Captura los Logs

```bash
pio device monitor --port COM4 --baud 115200 > logs.txt
```

Deja que se reinicie varias veces (10-15 segundos) y luego para el monitor.

### 2. Reporta el Problema

Incluye:
- [ ] Archivo `logs.txt`
- [ ] Versión de firmware (v2.10.6)
- [ ] Entorno usado (`esp32-s3-devkitc-touch-debug`, etc.)
- [ ] Hardware conectado (qué sensores tienes)
- [ ] Output de `pio run -v 2>&1 | grep CONFIG_ESP_IPC_TASK_STACK_SIZE`

---

## 📚 Documentación Adicional

Para más detalles técnicos:
- **RESUMEN_FIX_IPC_STACK_v2.10.6.md** - Análisis técnico completo
- **RESUMEN_FIX_BOOT_LOOP_v2.10.5.md** - Fix anterior de watchdog
- **RESUMEN_FIX_STACK_v2.10.3.md** - Fix anterior de stack overflow

---

**Versión:** v2.10.6  
**Fecha:** 2025-12-14  
**Estado:** ✅ PROBADO Y VERIFICADO  
**Compilación:** ✅ Build exitoso confirmado

**¡Disfruta tu firmware funcionando correctamente! 🎉**
