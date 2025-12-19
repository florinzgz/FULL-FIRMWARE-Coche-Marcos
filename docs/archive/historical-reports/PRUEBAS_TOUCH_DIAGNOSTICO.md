# 🔧 GUÍA DE PRUEBAS Y DIAGNÓSTICO - TOUCH NO FUNCIONA

## Problema Reportado
> "dame soluciones pruebas para hacer y descartar fallos, no funciona el touch de la pantalla"

## ✅ SOLUCIONES RÁPIDAS - PRUEBA EN ORDEN

### 🎯 SOLUCIÓN #1: Calibrar con Botón Físico (MÁS FÁCIL)

**Si el touch NO funciona, usa el botón 4X4:**

1. **Mantén presionado** el botón 4X4 durante **5 segundos**
2. Escucharás confirmación sonora
3. La calibración del touch se inicia automáticamente
4. Sigue las instrucciones en pantalla

📖 **Guía completa:** `docs/CALIBRACION_TOUCH_SIN_PANTALLA.md`

---

### 🎯 SOLUCIÓN #2: Ajustar Sensibilidad (RÁPIDO)

Edita `platformio.ini` y cambia estas líneas:

```ini
; Más sensible - menos presión necesaria
-DZ_THRESHOLD=200        ; Era 300, prueba 200

; Más lento pero más confiable
-DSPI_TOUCH_FREQUENCY=1000000  ; Era 2500000, prueba 1MHz
```

Recompila y sube:
```bash
platformio run -t upload
```

---

### 🎯 SOLUCIÓN #3: Usar Entorno de Debug Touch

Usa el entorno pre-configurado para debug:

```bash
platformio run -e esp32-s3-devkitc-touch-debug --target upload
```

Este entorno ya tiene:
- ✅ SPI touch más lento (1MHz)
- ✅ Umbral más sensible (Z=250)
- ✅ Logs de debug activados

---

## 🔍 PRUEBAS DE DIAGNÓSTICO (PASO A PASO)

### PRUEBA 1: Verificar Hardware ⚡

**Qué hacer:**
1. Apaga el sistema
2. Verifica estas conexiones con multímetro:

| Pin Touch | ESP32-S3 GPIO | Estado Correcto |
|-----------|---------------|-----------------|
| T_CS | 21 | ✅ Conectado |
| T_CLK | 10 (compartido con TFT) | ✅ Conectado |
| T_DIN | 11 (compartido con TFT) | ✅ Conectado |
| T_DO | 12 (compartido con TFT) | ✅ Conectado |
| T_IRQ | 47 | ⚠️ Opcional (no usado) |

**¿Qué buscar?**
- ✅ CORRECTO: Continuidad entre pin touch y GPIO
- ❌ PROBLEMA: Sin continuidad = cable suelto o roto
- ❌ PROBLEMA: T_CS en GPIO 16 (ese es TFT_CS, no touch!)

**Acción:**
- Si hay cables sueltos: Reconectar y soldar bien
- Si T_CS está mal: Mover a GPIO 21

---

### PRUEBA 2: Serial Monitor - Ver Logs 📊

**Qué hacer:**
1. Conecta Serial Monitor (115200 baud)
2. Reinicia el ESP32-S3
3. Busca estos mensajes:

**✅ TOUCH FUNCIONA:**
```
Touch: Using default calibration [min_x=200, max_x=3900, ...]
Touch: Controller responding, raw values: X=..., Y=..., Z=...
Touchscreen XPT2046 integrado TFT_eSPI inicializado OK
```

**❌ TOUCH NO FUNCIONA:**
```
Touch: Controller not responding to getTouchRaw()
Touch: Invalid values detected - possible hardware or SPI issue
```

**Acción según resultado:**
- Si dice "not responding": Problema hardware (ver PRUEBA 1)
- Si dice "initialized OK": Problema de calibración (ver SOLUCIÓN #1)

---

### PRUEBA 3: Touch Debug Activado 🔬

**Qué hacer:**
1. Edita `platformio.ini` línea 129:

```ini
; Touch debug - DESCOMENTAR esta línea:
-DTOUCH_DEBUG
```

2. Recompila y sube
3. Abre Serial Monitor (115200 baud)
4. Toca la pantalla en diferentes puntos

**✅ FUNCIONA (verás esto cada vez que toques):**
```
Touch detected at (240, 160)
Touch RAW: X=2048, Y=2048, Z=450
```

**❌ PROBLEMA - Valores incorrectos:**

| Síntoma | Causa | Solución |
|---------|-------|----------|
| X=0, Y=0 siempre | No hay comunicación SPI | PRUEBA 1 (hardware) |
| X>4095, Y>4095 | Error SPI | Reducir SPI_TOUCH_FREQUENCY |
| Z siempre 0 | No detecta presión | Reducir Z_THRESHOLD a 200 |
| Z siempre 4095 | Cortocircuito | Revisar soldaduras |
| Valores erráticos | Ruido eléctrico | Añadir capacitor 100nF en TOUCH_CS |

---

### PRUEBA 4: Test de Frecuencia SPI 📡

**Qué hacer - Prueba estas frecuencias en orden:**

**a) Actual (2.5 MHz):**
```ini
-DSPI_TOUCH_FREQUENCY=2500000
```

**b) Más lento (1 MHz):**
```ini
-DSPI_TOUCH_FREQUENCY=1000000
```

**c) Muy lento (600 kHz):**
```ini
-DSPI_TOUCH_FREQUENCY=600000
```

**d) Ultra lento (100 kHz):**
```ini
-DSPI_TOUCH_FREQUENCY=100000
```

**Cómo probar:**
1. Cambia la frecuencia en `platformio.ini`
2. Recompila: `platformio run -t upload`
3. Toca la pantalla
4. Si funciona con una frecuencia más baja, quédate con esa

---

### PRUEBA 5: Test de Sensibilidad 🎚️

**Qué hacer - Prueba estos umbrales:**

**a) Actual (Z=300):**
```ini
-DZ_THRESHOLD=300
```

**b) Más sensible (Z=200):**
```ini
-DZ_THRESHOLD=200
```

**c) Muy sensible (Z=150):**
```ini
-DZ_THRESHOLD=150
```

**⚠️ Advertencia:**
- Valores muy bajos (< 150) pueden causar toques falsos
- Si tienes toques fantasma, AUMENTA el valor

---

### PRUEBA 6: Verificar Voltaje 🔋

**Qué hacer:**
1. Con multímetro, mide voltaje en pins de la pantalla:

| Pin | Voltaje Correcto | Problema si... |
|-----|------------------|----------------|
| VCC | 3.3V ± 0.1V | < 3.2V o > 3.4V |
| GND | 0V | > 0.1V |

**Acción:**
- Si voltaje bajo: Fuente insuficiente, usar fuente externa 3.3V/500mA
- Si voltaje alto: Regulador de voltaje defectuoso

---

### PRUEBA 7: Test de Aislamiento ⚡

**Objetivo:** Verificar si el problema es touch o display

**Qué hacer:**
1. Edita `platformio.ini` y activa modo standalone:

```ini
; Descomentar estas líneas:
-DSTANDALONE_DISPLAY
```

2. Recompila y sube
3. Sistema arranca con valores simulados
4. Toca la pantalla

**Resultado:**
- ✅ Si funciona en standalone: Problema de interferencia con sensores
- ❌ Si NO funciona en standalone: Problema hardware del touch

---

### PRUEBA 8: Verificar Transacciones SPI 🔄

**Qué hacer:**
Verifica que estén habilitadas en `platformio.ini`:

```ini
; Estas líneas DEBEN estar presentes:
-DSPI_HAS_TRANSACTION
-DSUPPORT_TRANSACTIONS
```

**¿Por qué?**
- Display y touch comparten el bus SPI
- Sin transacciones, pueden interferirse
- Causa toques erráticos o no detectados

---

## 🎯 DIAGNÓSTICO POR SÍNTOMAS

### Síntoma A: Touch nunca detecta nada

**Causas probables:**
1. ❌ T_CS no conectado → Verificar GPIO 21
2. ❌ MISO no conectado → Verificar GPIO 12
3. ❌ Touch defectuoso → Probar con otro módulo

**Pruebas a realizar:**
- ✅ PRUEBA 1 (Hardware)
- ✅ PRUEBA 2 (Serial Monitor)
- ✅ PRUEBA 3 (Touch Debug)

---

### Síntoma B: Touch detecta pero coordenadas incorrectas

**Causas probables:**
1. ❌ Calibración incorrecta
2. ❌ Rotación de pantalla no coincide

**Soluciones:**
- ✅ SOLUCIÓN #1 (Calibrar con botón)
- Verificar `setRotation(3)` en código

---

### Síntoma C: Touch funciona intermitentemente

**Causas probables:**
1. ❌ Cable suelto o con mal contacto
2. ❌ Frecuencia SPI demasiado alta
3. ❌ Interferencia eléctrica

**Soluciones:**
- ✅ PRUEBA 1 (Hardware - revisar soldaduras)
- ✅ PRUEBA 4 (Reducir frecuencia SPI)
- Añadir capacitor 100nF entre T_CS y GND

---

### Síntoma D: Touch requiere mucha presión

**Causas probables:**
1. ❌ Z_THRESHOLD demasiado alto
2. ❌ Touch defectuoso

**Soluciones:**
- ✅ PRUEBA 5 (Reducir Z_THRESHOLD)
- ✅ SOLUCIÓN #2 (Ajustar sensibilidad)

---

### Síntoma E: Toques fantasma (detecta sin tocar)

**Causas probables:**
1. ❌ Z_THRESHOLD demasiado bajo
2. ❌ Ruido eléctrico
3. ❌ Pantalla sucia

**Soluciones:**
- Aumentar Z_THRESHOLD a 400-500
- Limpiar pantalla con alcohol isopropílico
- Añadir filtro RC en línea de T_IRQ

---

## 📋 CHECKLIST DE VERIFICACIÓN COMPLETA

Marca cada item que hayas verificado:

### Hardware
- [ ] T_CS conectado a GPIO 21 (no GPIO 16)
- [ ] MISO conectado a GPIO 12
- [ ] MOSI conectado a GPIO 11
- [ ] SCLK conectado a GPIO 10
- [ ] Voltaje VCC = 3.3V ± 0.1V
- [ ] Continuidad en todos los cables
- [ ] Soldaduras bien hechas (sin puntos fríos)

### Configuración platformio.ini
- [ ] `-DUSER_SETUP_LOADED` presente
- [ ] `-DST7796_DRIVER` presente (NO ILI9488_DRIVER)
- [ ] `-DTOUCH_CS=21` presente
- [ ] `-DSPI_HAS_TRANSACTION` presente
- [ ] `-DSUPPORT_TRANSACTIONS` presente
- [ ] `Z_THRESHOLD` entre 150-400
- [ ] `SPI_TOUCH_FREQUENCY` entre 100kHz-2.5MHz

### Software
- [ ] Código compila sin errores
- [ ] Firmware flasheado correctamente
- [ ] Serial Monitor muestra "Touchscreen...inicializado OK"
- [ ] TFT_eSPI version 2.5.43 instalada

### Calibración
- [ ] Calibración intentada con botón 4X4 (5 segundos)
- [ ] Calibración guardada en EEPROM
- [ ] Valores de calibración en logs parecen razonables

---

## 🚀 PLAN DE ACCIÓN RECOMENDADO

### DÍA 1: Hardware
1. ✅ Ejecutar **PRUEBA 1** (Verificar conexiones)
2. ✅ Ejecutar **PRUEBA 6** (Verificar voltajes)
3. Si todo OK → Día 2
4. Si hay problemas → Corregir hardware primero

### DÍA 2: Software
1. ✅ Ejecutar **PRUEBA 2** (Serial Monitor)
2. ✅ Ejecutar **PRUEBA 3** (Touch Debug)
3. Si logs dicen "not responding" → Volver a hardware
4. Si logs dicen "initialized OK" → Día 3

### DÍA 3: Calibración y Ajustes
1. ✅ Intentar **SOLUCIÓN #1** (Calibrar con botón)
2. ✅ Intentar **SOLUCIÓN #2** (Ajustar sensibilidad)
3. ✅ Ejecutar **PRUEBA 4** (Frecuencias SPI)
4. ✅ Ejecutar **PRUEBA 5** (Umbrales Z)

---

## 📖 DOCUMENTACIÓN ADICIONAL

### Guías Existentes
- `docs/CALIBRACION_TOUCH_SIN_PANTALLA.md` - Calibrar sin touch funcional
- `docs/TOUCH_TROUBLESHOOTING.md` - Troubleshooting detallado
- `docs/TOUCH_CALIBRATION_GUIDE.md` - Guía de calibración
- `docs/DISPLAY_TOUCH_VERIFICATION.md` - Verificación técnica
- `RESUMEN_TOUCH_FIX.md` - Resumen de fixes anteriores

### Logs a Revisar
Abre Serial Monitor (115200 baud) y busca:
- Mensajes de inicialización del touch
- Errores o warnings
- Valores raw cuando tocas pantalla

### Soporte
Si ninguna prueba funciona:
1. Captura los logs del Serial Monitor completos
2. Anota qué pruebas realizaste y resultados
3. Toma foto de las conexiones hardware
4. Abre issue en GitHub con esta información

---

**Creado:** 2025-12-05  
**Versión Firmware:** 2.9.4+  
**Estado:** ✅ GUÍA COMPLETA DE PRUEBAS Y SOLUCIONES
