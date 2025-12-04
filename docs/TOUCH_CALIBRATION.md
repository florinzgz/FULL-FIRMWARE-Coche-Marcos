# Touch Screen Calibration Guide

## Problem
El táctil de la pantalla puede no funcionar correctamente sin calibración adecuada.

## Symptoms
- Touch no responde
- Touch responde en posición incorrecta
- Touch solo funciona en ciertas áreas de la pantalla

## Hardware
- Display: ST7796S 480x320 (4 pulgadas)
- Touch Controller: XPT2046 (resistivo)
- Interfaz: SPI compartido con display
- Pines:
  - TOUCH_CS: GPIO 21
  - TOUCH_IRQ: GPIO 47

## Solución: Calibrar Touch Screen

### Método 1: Calibración Manual (Recomendado)

1. **Acceder al Menú Oculto:**
   - Toca el icono de batería (esquina superior izquierda) 4 veces
   - Secuencia: 8-9-8-9
   - Código de acceso: 8989

2. **Iniciar Calibración:**
   - Selecciona opción **3: Calibrar touch**
   - El sistema mostrará instrucciones en pantalla

3. **Seguir las Instrucciones:**
   - **Punto 1:** Toca el objetivo ROJO en la esquina superior izquierda
     - Mantén presionado hasta que se complete la barra de progreso
   - **Punto 2:** Toca el objetivo ROJO en la esquina inferior derecha
     - Mantén presionado hasta que se complete la barra de progreso
   
4. **Verificación:**
   - El sistema calculará y guardará la calibración automáticamente
   - La calibración se guarda en la memoria persistente (EEPROM)

5. **Confirmar:**
   - Toca la pantalla para confirmar o espera 3 segundos
   - El sistema volverá al menú oculto

### Método 2: Calibración por Consola Serial

Si el touch no funciona en absoluto, usa los botones físicos:

1. Conecta vía Serial (115200 baud)
2. Los mensajes de log mostrarán:
   ```
   Touch: Using default calibration. If touch doesn't work properly:
     1. Tap battery icon 4 times to enter code 8989
     2. Select option 3: 'Calibrar touch'
     3. Follow on-screen instructions
   ```

## Valores de Calibración

### Valores por Defecto (sin calibrar)
```cpp
minX: 200   // Borde izquierdo
maxX: 3900  // Borde derecho
minY: 200   // Borde superior
maxY: 3900  // Borde inferior
rotation: 3 // Landscape (480x320)
```

### Valores Típicos Después de Calibración
Los valores varían según el panel táctil específico.
Ejemplo de valores calibrados:
```cpp
minX: 180-250
maxX: 3800-3950
minY: 180-250
maxY: 3800-3950
```

## Indicador Visual de Touch

Cuando el touch está funcionando, verás:
- **Cruz cian** en el punto donde tocas
- **Punto rojo** en el centro del touch
- Mensajes en consola serial: `Touch detected at (x, y)`

## Troubleshooting

### Touch no responde en absoluto

1. **Verificar Hardware:**
   ```
   - TOUCH_CS (GPIO 21) conectado
   - TOUCH_IRQ (GPIO 47) conectado
   - Bus SPI compartido con display
   ```

2. **Verificar en Serial:**
   ```
   Touchscreen XPT2046 integrado TFT_eSPI inicializado OK
   ```

3. **Verificar configuración:**
   - En `platformio.ini`: NO debe estar `-DDISABLE_TOUCH`
   - En storage: `cfg.touchEnabled` debe ser `true`

### Touch responde pero en posición incorrecta

**Solución:** Ejecutar calibración (Método 1)

### Touch funciona pero es impreciso

**Posibles causas:**
1. Panel táctil resistivo de baja calidad
2. Interferencia electromagnética
3. Protector de pantalla grueso

**Soluciones:**
1. Re-calibrar con más cuidado
2. Alejar cables de alimentación del display
3. Quitar protector de pantalla si lo hay

### Calibración falla (timeout)

**Causas:**
- No estás tocando el objetivo correcto
- Touch hardware no funciona
- Valores fuera de rango

**Solución:**
1. Reintentar calibración
2. Tocar exactamente en el centro del objetivo rojo
3. Mantener presionado hasta que se complete

## Código de Acceso al Menú Oculto

Para acceder al menú de calibración sin touch funcional:

**Método alternativo (requiere botón físico de batería):**
1. Presiona el botón físico de batería 4 veces
2. Secuencia: espera 0.5s entre presiones
3. El código 8989 se ingresará automáticamente

## Datos Técnicos

### XPT2046 Touch Controller
- Resolución: 12-bit (0-4095)
- Rango útil: ~200-3900 (excluye bordes imprecisos)
- Frecuencia SPI: 2.5 MHz
- Tipo: Resistivo (4 hilos)

### Display ST7796S
- Resolución nativa: 320x480 (portrait)
- Con rotation=3: 480x320 (landscape)
- Frecuencia SPI: 40 MHz

## Referencias

- Código fuente: `src/hud/touch_calibration.cpp`
- Menú oculto: `src/hud/menu_hidden.cpp`
- Configuración: `include/storage.h`
- Inicialización: `src/hud/hud.cpp`
