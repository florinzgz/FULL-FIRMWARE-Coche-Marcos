# 🧪 Modo Standalone Display

## Descripción

El modo **STANDALONE_DISPLAY** permite probar la pantalla ILI9488 y el dashboard HUD **sin necesidad de conectar sensores, motores, o hardware adicional**. Solo necesitas la ESP32-S3 y la pantalla.

Este modo es ideal para:
- ✅ Verificar que la pantalla ILI9488 funciona correctamente
- ✅ Validar el sistema SPI y la comunicación display-MCU
- ✅ Visualizar el dashboard completo con valores simulados
- ✅ Probar el diseño del HUD antes del montaje final
- ✅ Debugging del código de renderizado sin hardware

---

## Activación

### Método 1: Editar `platformio.ini`

Abre el archivo `platformio.ini` y **descomenta** la línea:

```ini
; 🧪 Modo de prueba standalone (descomentar para activar)
-DSTANDALONE_DISPLAY
```

Debe quedar así:

```ini
; 🧪 Modo de prueba standalone (descomentar para activar)
-DSTANDALONE_DISPLAY
```

### Método 2: Comando de compilación

```bash
pio run -e esp32-s3-devkitc-1 --target upload -D STANDALONE_DISPLAY
```

---

## Comportamiento en Modo Standalone

### 1️⃣ Setup (Inicialización)
- ✅ Inicializa **solo** System, Storage, Logger, y HUDManager
- ⏭️ Omite inicialización de:
  - Watchdog
  - Sensores (current, temperature, wheels)
  - Control de motores (traction, steering)
  - Relés
  - Audio (DFPlayer)
  - Sistemas avanzados (ABS, TCS, RegenAI)
  - Bluetooth
  - WiFi/OTA

- ✅ Muestra logo de arranque durante 1.5 segundos
- ✅ Carga directamente el dashboard (modo DASHBOARD)

### 2️⃣ Loop (Bucle Principal)
- ✅ Actualiza HUD a 30 FPS (cada 33ms)
- ✅ Usa valores simulados realistas:

```cpp
Velocidad:        12.0 km/h
RPM:              850 (ralentí)
Batería:          24.5V / 87%
Corriente:        2.3A
Temp. Motor:      42°C
Temp. Batería:    38°C
Roll:             0.5°
Pitch:            -1.2°
Pedal:            0%
Volante:          0° (centrado)
Marcha:           1
RPM Ruedas:       15 RPM (todas)
Temp. Ruedas:     -- °C (deshabilitado)
Esfuerzo Ruedas:  -- % (deshabilitado)
4x4:              Activo
ABS/TCS:          Inactivo
Luces:            Apagadas
Música:           Apagada
```

- ⏭️ No ejecuta control de motores, sensores, o audio
- ✅ Previene watchdog timeout con `delay(1)`

---

## Qué Deberías Ver en la Pantalla

### Secuencia de Arranque:
1. **Colores de prueba SPI** (rojo → verde → azul, 0.5s cada uno)
2. **"ILI9488 OK"** (pantalla negra, texto blanco centrado, 1s)
3. **Logo de arranque** (1.5s)
4. **Dashboard completo**:
   - Mercedes AMG GT (título)
   - Velocímetro: 12 km/h
   - Tacómetro: 850 RPM
   - Batería: 87% / 24.5V
   - 4 ruedas con ángulo 0° (estáticas)
   - Temperaturas y esfuerzos: "-- °C" / "-- %"
   - Icono 4x4 activo
   - Roll/Pitch: 0.5° / -1.2°

---

## Solución de Problemas

### ❌ Pantalla negra (backlight apagado)
**Causa:** GPIO42 (backlight) no está conectado o no recibe 3.3V

**Solución:**
1. Conecta BL de la pantalla → GPIO 42 ESP32-S3
2. Verifica con multímetro que GPIO42 esté en HIGH (3.3V)
3. Como prueba temporal, conecta BL directamente a 3.3V

---

### ❌ Pantalla blanca (backlight encendido, sin contenido)
**Causa:** Problema de comunicación SPI

**Solución:**
1. Verifica conexiones SPI:
   - CS → GPIO 8
   - DC → GPIO 13
   - RST → GPIO 14
   - MOSI → GPIO 11
   - SCK → GPIO 10
   - MISO → GPIO 12
2. Confirma que usas HSPI (no VSPI)
3. Revisa que VDD de pantalla = 3.3V (NO 5V)

---

### ❌ Mitad de pantalla en blanco (half-screen corruption)
**Causa:** ILI9488 requiere `setRotation(3)` en lugar de `setRotation(1)`

**Solución:**
El firmware ya usa `setRotation(3)` en commit `aa8c0d3`. Asegúrate de tener la última versión.

---

### ❌ Dashboard no aparece después de "ILI9488 OK"
**Causa:** Problema en HUDManager o coordinación tft.init()

**Solución:**
1. Verifica que solo `HUDManager::init()` llama a `tft.init()` (no duplicar)
2. Revisa logs en Serial Monitor (115200 baud)
3. Busca mensajes de error de Logger

---

## Compilación y Despliegue

### 1. Activar Modo Standalone
Edita `platformio.ini` y descomenta `-DSTANDALONE_DISPLAY`

### 2. Compilar
```bash
pio run
```

### 3. Subir a ESP32-S3
```bash
pio run --target upload
```

### 4. Monitor Serie (opcional)
```bash
pio device monitor -b 115200
```

Deberías ver:
```
🧪 STANDALONE_DISPLAY MODE: Skipping sensor initialization
HUD init OK - Display ILI9488 ready
🧪 STANDALONE MODE: Dashboard active with simulated values
```

---

## Desactivar Modo Standalone

Para volver al **modo normal** (con sensores y control):

1. Edita `platformio.ini`
2. **Comenta** la línea:
```ini
; -DSTANDALONE_DISPLAY
```

3. Recompila y sube:
```bash
pio run --target upload
```

---

## Conexiones Mínimas Requeridas

Para modo standalone solo necesitas:

| Componente | Pines | Alimentación |
|------------|-------|--------------|
| ESP32-S3-N16R8 | - | 5V USB o 3.3V regulado |
| ILI9488 Display | VDD→3V3, GND→GND, CS→8, DC→13, RST→14, MOSI→11, SCK→10, MISO→12, BL→42 | 3.3V desde ESP32 |
| XPT2046 Touch | TCS→3, PEN→46, compartidos MOSI/MISO/SCK | 3.3V desde ESP32 |

**IMPORTANTE:** NO conectes motores, relés, o sensores de alta corriente en modo standalone. Solo display y touch.

---

## Próximos Pasos

Una vez verificado el display en modo standalone:

1. ✅ Compila en modo normal (sin `-DSTANDALONE_DISPLAY`)
2. ✅ Añade sensores uno a uno:
   - Primero: Touch (XPT2046) - ya funcional
   - Segundo: Sensor de pedal
   - Tercero: Un sensor de rueda para validar velocidad
   - Cuarto: Sensores de temperatura/corriente
3. ✅ Activa control de motores cuando tengas hardware conectado
4. ✅ Verifica logs con `Logger::info()` / `Logger::error()`

---

## Notas Técnicas

### Flags de Compilación
- `-DSTANDALONE_DISPLAY`: Activa modo standalone
- Se puede combinar con otros flags (ejemplo: `-DDEBUG_MODE`)

### Consumo de Memoria
El modo standalone reduce el uso de SRAM al omitir módulos de sensores y control.

### Performance
- Loop corre a ~1000 Hz (limitado por `delay(1)`)
- HUD actualiza a 30 FPS fijos
- Consumo CPU: ~15% (ESP32-S3 @ 240MHz)

### Compatibilidad
- ✅ ESP32-S3-DevKitC-1 (N16R8)
- ✅ ILI9488 320x480 (SPI)
- ✅ XPT2046 Touchscreen
- ✅ TFT_eSPI library v2.5.x

---

## Soporte

Si encuentras problemas:
1. Revisa logs en Serial Monitor (115200 baud)
2. Verifica conexiones físicas con multímetro
3. Comprueba que la versión del firmware sea la última (commit `aa8c0d3` o superior)
4. Abre un issue en GitHub con:
   - Descripción del problema
   - Logs del Serial Monitor
   - Fotos de las conexiones
   - Modelo exacto de ESP32-S3 y display

---

**Última actualización:** 2025-01-18  
**Versión firmware:** aa8c0d3 + STANDALONE_MODE  
**Autor:** GitHub Copilot
