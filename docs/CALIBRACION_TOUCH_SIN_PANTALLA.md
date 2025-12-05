# CALIBRACIÓN TÁCTIL SIN PANTALLA FUNCIONAL

## 🎯 PROBLEMA RESUELTO

**Situación:** El touch de la pantalla no funciona en absoluto. No puedes tocar el icono de batería para acceder al menú oculto y calibrar el touch.

**Solución:** Ahora puedes activar la calibración del touch usando un **botón físico** sin necesidad de que la pantalla táctil funcione.

---

## ✅ CÓMO CALIBRAR EL TOUCH CON BOTÓN FÍSICO

### PASO 1: Mantén presionado el botón 4X4

1. Localiza el **botón físico 4X4** en tu sistema
2. **Mantén presionado** el botón durante **5 segundos completos**
3. Escucharás un sonido de confirmación (AUDIO_MENU_OCULTO)
4. **NO sueltes el botón** hasta escuchar el sonido

### PASO 2: La calibración se inicia automáticamente

Una vez que suenes el botón después de 5 segundos:
- La pantalla mostrará la pantalla de calibración táctil
- Aparecerán instrucciones en la pantalla
- La calibración iniciará automáticamente

### PASO 3: Sigue las instrucciones en pantalla

La calibración del touch te pedirá:
1. Tocar puntos específicos en la pantalla (esquinas, centro)
2. Presionar cada punto cuando aparezca
3. Esperar a que se complete el proceso

### PASO 4: Calibración completa

- Si la calibración es exitosa: Escucharás AUDIO_MODULO_OK
- Si falla o se cancela: Escucharás AUDIO_ERROR_GENERAL
- Los valores se guardan automáticamente en la memoria EEPROM

---

## 🔧 DETALLES TÉCNICOS

### Tiempos de presión del botón 4X4:

- **Presión corta** (< 2 segundos): Toggle normal del modo 4X4
- **Presión larga** (2-5 segundos): Acción especial (sonido de confirmación)
- **Presión muy larga** (≥ 5 segundos): **Inicia calibración táctil directa** 🎯

### Logs en Serial Monitor (115200 baud):

Cuando presionas el botón por 5 segundos, verás:
```
Buttons: 4X4 very-long-press (5s) - Iniciando calibración táctil
activateTouchCalibration() llamada desde botón físico
Iniciando calibración táctil directa (activación por botón físico)
Iniciando calibración de touch screen
```

### ¿Qué pasa si ya estabas en el menú oculto?

- El menú se cierra automáticamente
- Cualquier calibración en curso se cancela
- Se inicia la calibración táctil directamente

---

## 🚨 TROUBLESHOOTING

### El botón no responde después de 5 segundos

**Verificar:**
1. ¿El botón está correctamente conectado al pin GPIO correcto? (ver `pins.h`)
2. ¿El pullup está habilitado? (se hace automáticamente en `Buttons::init()`)
3. Revisa Serial Monitor para mensajes de error

**Solución:**
- Verifica las conexiones físicas del botón 4X4
- Comprueba que el pin `PIN_BTN_4X4` esté definido correctamente en `pins.h`

### La calibración se inicia pero no responde

**Verificar:**
1. ¿El touch está correctamente conectado?
2. Revisa los pines en `platformio.ini`:
   - `TOUCH_CS=21`
   - SPI bus compartido con display

**Solución:**
- Verifica conexiones hardware del XPT2046
- Comprueba que `TOUCH_CS` esté conectado a GPIO 21
- Prueba con el modo `esp32-s3-devkitc-touch-debug` en platformio.ini

### El touch sigue sin funcionar después de calibrar

**Opciones:**
1. **Intenta calibrar de nuevo** (repite el proceso con botón 4X4)
2. **Ajusta la sensibilidad** en `platformio.ini`:
   ```ini
   -DZ_THRESHOLD=250  ; Más sensible
   ```
3. **Reduce la frecuencia SPI** del touch:
   ```ini
   -DSPI_TOUCH_FREQUENCY=1000000  ; Más lento pero más confiable
   ```

---

## 📋 RESUMEN RÁPIDO

| Acción | Método |
|--------|--------|
| **Calibrar touch (sin touch funcional)** | Mantén presionado botón 4X4 por 5 segundos |
| **Cancelar calibración en curso** | Espera timeout (30 segundos) o reinicia el sistema |
| **Ver logs de calibración** | Serial Monitor a 115200 baud |
| **Ajustar sensibilidad** | Edita `Z_THRESHOLD` en platformio.ini |

---

## 🎓 ¿CÓMO FUNCIONA INTERNAMENTE?

1. **buttons.cpp** detecta presión de 5 segundos en botón 4X4
2. Llama a función `activateTouchCalibration()` en **main.cpp**
3. main.cpp llama a `MenuHidden::startTouchCalibrationDirectly()`
4. **menu_hidden.cpp** cancela cualquier estado previo e inicia calibración
5. La rutina de calibración en **touch_calibration.cpp** se ejecuta
6. Los valores calibrados se guardan en EEPROM
7. El sistema vuelve al dashboard automáticamente

---

## ✅ VENTAJAS DE ESTE MÉTODO

- ✅ **No necesitas touch funcional** para calibrar
- ✅ **Acceso directo** sin menús ni códigos
- ✅ **Confirmación sonora** al activar
- ✅ **Simple:** Solo mantén un botón 5 segundos
- ✅ **Seguro:** No interfiere con uso normal del botón
- ✅ **Robusto:** Funciona incluso si el menú oculto está deshabilitado

---

## 📞 SOPORTE

Si tienes problemas:

1. **Revisa Serial Monitor** (115200 baud) para mensajes de diagnóstico
2. **Verifica hardware:**
   - Botón 4X4 conectado correctamente
   - Touch XPT2046 conectado y alimentado
   - SPI bus compartido funcionando
3. **Lee documentación adicional:**
   - `docs/TOUCH_FIX_v2.9.3.md` - Detalles técnicos del touch
   - `docs/GUIA_RAPIDA_TOUCH.md` - Guía rápida
   - `docs/TOUCH_TROUBLESHOOTING.md` - Solución de problemas

---

**Versión:** 2.9.4  
**Fecha:** 2024-12-05  
**Característica:** Calibración táctil por botón físico (sin touch funcional)  
**Estado:** ✅ IMPLEMENTADO Y PROBADO
