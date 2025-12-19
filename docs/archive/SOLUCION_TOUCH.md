# Solución: Pantalla Táctil no Funciona

## Problema Identificado

El táctil de la pantalla **SÍ está implementado** en el firmware, pero necesita calibración para funcionar correctamente con tu hardware específico.

## ¿Por Qué No Funciona?

1. **Calibración por Defecto**: Los valores predeterminados (RAW_MIN=200, RAW_MAX=3900) pueden no coincidir con tu controlador táctil XPT2046 específico
2. **Sin Calibrar**: El sistema usa valores genéricos hasta que ejecutes la calibración
3. **Menú Oculto**: La opción de calibración está en un menú secreto que requiere código de acceso

## Solución: 3 Pasos Simples

### Paso 1: Verificar que el Touch Responde

Después de flashear el firmware actualizado:

1. **Enciende el sistema**
2. **Mira la pantalla en la esquina donde tocas**
   - Si el touch funciona, verás una **cruz cian** y un **punto rojo**
   - Esto confirma que el touch está detectando tu toque

3. **Revisa la consola Serial** (115200 baud)
   - Verás mensajes como: `Touch detected at (123, 456)`
   - Esto muestra las coordenadas donde tocaste

### Paso 2: Acceder al Menú de Calibración

1. **Toca el icono de batería** (esquina superior izquierda) **4 veces**
   - Secuencia: 8-9-8-9
   - Código de acceso: 8989

2. **Verás el menú oculto** con 9 opciones

### Paso 3: Calibrar la Pantalla Táctil

1. **Selecciona opción 3: "Calibrar touch"**

2. **Toca el primer objetivo rojo** (esquina superior izquierda)
   - Mantén presionado hasta ver barra de progreso completa
   - Verás la barra verde llenarse

3. **Toca el segundo objetivo rojo** (esquina inferior derecha)
   - Mantén presionado hasta ver barra de progreso completa

4. **¡Listo!**
   - El sistema calcula y guarda la calibración automáticamente
   - La calibración se guarda en memoria permanente

## Mensajes de Ayuda en Pantalla

El firmware actualizado ahora muestra:

### En el Arranque (Pantalla READY):
```
READY
Touch no calibrado
Toca bateria 4 veces: 8-9-8-9
Opcion 3: Calibrar touch
```

### En la Consola Serial:
```
[INFO] Touch: Using default calibration. If touch doesn't work properly:
[WARN]   1. Tap battery icon 4 times to enter code 8989
[WARN]   2. Select option 3: 'Calibrar touch'
[WARN]   3. Follow on-screen instructions
```

## Indicadores Visuales Nuevos

### Cruz Cian + Punto Rojo
- **Qué es**: Indicador visual de donde tocas
- **Cuándo aparece**: Cada vez que tocas la pantalla
- **Para qué sirve**: Te ayuda a verificar que el touch funciona y está correctamente calibrado

### Coordenadas en Serial
- **Qué son**: Valores numéricos (x, y) de tu toque
- **Ejemplo**: `Touch detected at (240, 160)`
- **Para qué sirven**: Debugging y verificación de calibración

## Troubleshooting

### "No veo la cruz cian cuando toco"

**Posibles causas:**
1. Touch no está conectado físicamente
2. Pines incorrectos (verificar TOUCH_CS=GPIO21, TOUCH_IRQ=GPIO47)
3. Firmware compilado con `-DDISABLE_TOUCH`

**Solución:**
1. Verificar conexiones hardware
2. Revisar Serial para mensaje: `Touchscreen XPT2046 integrado TFT_eSPI inicializado OK`

### "Veo la cruz pero está en posición incorrecta"

**Causa:** Calibración incorrecta o por defecto

**Solución:** Ejecutar calibración (Paso 2 y 3 arriba)

### "No puedo acceder al menú (batería no responde)"

**Opciones:**
1. **Usar Serial**: Los mensajes te guiarán
2. **Touch básico**: Intenta tocar varias veces hasta que entre código correcto
3. **Calibración manual**: Ver `docs/TOUCH_CALIBRATION.md`

## Archivos Modificados

Los cambios están en:
- `src/hud/hud.cpp` - Visual debug indicators + mensajes
- `docs/TOUCH_CALIBRATION.md` - Guía completa (inglés)
- `docs/SOLUCION_TOUCH.md` - Esta guía (español)

## Especificaciones Técnicas

### Hardware:
- **Display**: ST7796S 480x320 (4")
- **Touch**: XPT2046 (resistivo, 12-bit ADC)
- **SPI**: Compartido con display, CS separados
- **Pines**:
  - TOUCH_CS: GPIO 21
  - TOUCH_IRQ: GPIO 47
  - SPI_MOSI: GPIO 11
  - SPI_MISO: GPIO 12
  - SPI_SCK: GPIO 10

### Calibración:
- **Rango ADC**: 0-4095 (teórico)
- **Rango útil**: 200-3900 (calibrado)
- **Puntos de calibración**: 2 (esquinas opuestas)
- **Persistencia**: EEPROM/NVS

## ¿Necesitas Más Ayuda?

Ver documentación completa en:
- `docs/TOUCH_CALIBRATION.md` - Guía detallada en inglés
- Issues en GitHub del proyecto

## Verificación de Éxito

✅ **El touch funciona correctamente si:**
1. Ves cruz cian + punto rojo donde tocas
2. El menú oculto se activa tocando batería 4 veces
3. Puedes seleccionar opciones tocando la pantalla
4. Las coordenadas en Serial coinciden con posición visual

## Cambios Implementados (v2.9.1)

### Nuevas Características:
- ✅ Indicador visual de touch (cruz cian + punto rojo)
- ✅ Logging de coordenadas en Serial
- ✅ Mensajes de ayuda en pantalla READY
- ✅ Mensajes informativos en consola Serial
- ✅ Documentación completa de calibración
- ✅ Validación mejorada de calibración guardada

### Mejoras de Usabilidad:
- ✅ Usuario sabe inmediatamente si touch funciona
- ✅ Instrucciones claras para calibrar
- ✅ Debugging más fácil con visual feedback
- ✅ Sin necesidad de código fuente para calibrar

## Próximos Pasos Recomendados

1. **Flashear el firmware actualizado**
2. **Verificar touch con indicador visual**
3. **Ejecutar calibración si es necesario**
4. **Disfrutar del touch calibrado** 🎉
