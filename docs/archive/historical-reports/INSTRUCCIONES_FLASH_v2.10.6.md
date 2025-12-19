# Instrucciones Rápidas de Flasheo - Firmware v2.10.6

## 🚀 Flasheo Rápido (Para Usuarios Avanzados)

### 1. Preparación

```bash
cd /ruta/al/proyecto
pio run -t clean
```

### 2. Compilar y Flashear

**Opción A: Entorno Base (Producción)**
```bash
pio run -e esp32-s3-devkitc -t upload --upload-port COM4
```

**Opción B: Entorno Touch Debug (Recomendado para este fix)**
```bash
pio run -e esp32-s3-devkitc-touch-debug -t upload --upload-port COM4
```

**Opción C: Sin Touch**
```bash
pio run -e esp32-s3-devkitc-no-touch -t upload --upload-port COM4
```

### 3. Monitorear

```bash
pio device monitor --port COM4 --baud 115200
```

---

## 📝 Instrucciones Detalladas (Para Usuarios Nuevos)

### Paso 1: Instalar PlatformIO

Si no tienes PlatformIO instalado:

**Windows:**
```powershell
pip install platformio
```

**Linux/Mac:**
```bash
pip3 install platformio
```

### Paso 2: Clonar/Descargar Firmware

**Con Git:**
```bash
git clone https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos.git
cd FULL-FIRMWARE-Coche-Marcos
```

**O descarga ZIP desde GitHub y extrae.**

### Paso 3: Identificar tu Puerto COM

**Windows:**
1. Abre "Administrador de dispositivos"
2. Expande "Puertos (COM y LPT)"
3. Busca "USB Serial Port (COMX)" o "Silicon Labs CP210x"
4. Anota el número (ej: COM4, COM5, etc.)

**Linux:**
```bash
ls /dev/ttyUSB*
# O
ls /dev/ttyACM*
```

**Mac:**
```bash
ls /dev/cu.usb*
```

### Paso 4: Conectar ESP32-S3

1. Conecta el ESP32-S3 al PC con cable USB
2. Asegúrate que el cable sea de **DATOS**, no solo de carga
3. Espera a que Windows instale drivers (si es primera vez)

### Paso 5: Compilar

Abre terminal/PowerShell en la carpeta del proyecto:

```bash
cd /ruta/al/proyecto/FULL-FIRMWARE-Coche-Marcos

# Limpiar compilación anterior
pio run -t clean

# Compilar (esto tarda ~2 minutos la primera vez)
pio run -e esp32-s3-devkitc-touch-debug
```

Espera a ver:
```
======================== [SUCCESS] Took XX.XX seconds ========================
```

### Paso 6: Flashear

**IMPORTANTE:** Cambia `COM4` por tu puerto real.

```bash
pio run -e esp32-s3-devkitc-touch-debug -t upload --upload-port COM4
```

Si ves error "Cannot find device", verifica:
- [ ] Cable conectado correctamente
- [ ] Puerto COM correcto
- [ ] Drivers instalados
- [ ] ESP32 encendido

### Paso 7: Verificar

```bash
pio device monitor --port COM4 --baud 115200
```

Deberías ver:
```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x1 (POWERON),boot:0x8 (SPI_FAST_FLASH_BOOT)
...
========================================
ESP32-S3 Car Control System v2.10.6 (Dec 14 2025 XX:XX:XX)
========================================
...
[BOOT] Setup complete! Entering main loop...
```

✅ **¡Éxito!** Si ves estos mensajes, el firmware está funcionando.

Para salir del monitor: `Ctrl+C`

---

## 🔧 Troubleshooting

### Error: "Cannot find device"

**Solución 1:** Verifica puerto COM
```bash
# Windows
mode

# Linux
ls /dev/ttyUSB* /dev/ttyACM*
```

**Solución 2:** Instala drivers CP210x
- Descarga de: https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers
- Instala y reinicia PC

**Solución 3:** Prueba otro cable USB
- Usa un cable USB de **datos** (no todos los cables transmiten datos)

### Error: "Chip not found"

**Solución:** Pon el ESP32 en modo bootloader manual:
1. Desconecta USB
2. Mantén presionado botón **BOOT** en el ESP32
3. Conecta USB (mientras mantienes BOOT)
4. Suelta BOOT
5. Intenta flashear de nuevo

### Error: "Stack canary watchpoint triggered (ipc0)" persiste

**Solución 1:** Verifica versión
```bash
pio device monitor --port COM4
```
Busca: `v2.10.6` en el output.

**Solución 2:** Limpia y recompila
```bash
rm -rf .pio
pio run -t clean
pio run -e esp32-s3-devkitc-touch-debug
pio run -e esp32-s3-devkitc-touch-debug -t upload --upload-port COM4
```

**Solución 3:** Borra flash completa
```bash
pio run -t erase
pio run -e esp32-s3-devkitc-touch-debug -t upload --upload-port COM4
```
⚠️ **Esto borrará configuración guardada.**

---

## 📊 Entornos Disponibles

| Entorno | Propósito | Cuándo Usar |
|---------|-----------|-------------|
| `esp32-s3-devkitc` | Producción | Uso normal |
| `esp32-s3-devkitc-touch-debug` | Debug táctil | Problemas con touch |
| `esp32-s3-devkitc-no-touch` | Sin touch | Touch deshabilitado |
| `esp32-s3-devkitc-release` | Optimizado | Máximo rendimiento |
| `esp32-s3-devkitc-predeployment` | Testing | Pre-producción |
| `esp32-s3-devkitc-ota` | Over-The-Air | Updates WiFi |

### Cambiar de Entorno

Solo cambia `-e nombre-entorno` en los comandos:

```bash
# Producción
pio run -e esp32-s3-devkitc -t upload --upload-port COM4

# Touch debug
pio run -e esp32-s3-devkitc-touch-debug -t upload --upload-port COM4

# Sin touch
pio run -e esp32-s3-devkitc-no-touch -t upload --upload-port COM4
```

---

## 🎯 Checklist Post-Flash

Después de flashear, verifica:

- [ ] No hay error "Stack canary watchpoint triggered (ipc0)"
- [ ] Versión mostrada es **v2.10.6**
- [ ] Pantalla enciende con backlight
- [ ] Dashboard visible en pantalla
- [ ] WiFi se conecta (si configurado)
- [ ] Sensores responden correctamente
- [ ] Sin reinicios inesperados

---

## 📞 Soporte

Si necesitas ayuda:

1. **Captura logs:**
   ```bash
   pio device monitor --port COM4 --baud 115200 > logs.txt
   ```

2. **Reporta con:**
   - Archivo `logs.txt`
   - Versión de firmware
   - Sistema operativo
   - Modelo de ESP32

3. **Abre issue en:** https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos/issues

---

## 📚 Documentación Relacionada

- **SOLUCION_ERROR_IPC_v2.10.6.md** - Guía del fix
- **RESUMEN_FIX_IPC_STACK_v2.10.6.md** - Detalles técnicos
- **platformio.ini** - Configuración de entornos

---

**Versión:** v2.10.6  
**Fecha:** 2025-12-14  
**Estado:** ✅ Listo para producción

**¡Éxito con tu proyecto! 🚗⚡**
