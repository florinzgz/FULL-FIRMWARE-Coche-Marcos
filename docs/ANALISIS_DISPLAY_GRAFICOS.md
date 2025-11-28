# Análisis Detallado del Display - Gráficos Línea por Línea

## Información General del Display

- **Pantalla**: ST7796S 4" TFT LCD
- **Resolución**: 480x320 píxeles (landscape)
- **Rotación**: 3 (modo horizontal - paisaje invertido)
  - El valor de rotación 3 en TFT_eSPI orienta el display en modo paisaje con:
    - X=0 en el borde izquierdo, X=480 en el borde derecho
    - Y=0 en el borde superior, Y=320 en el borde inferior
    - El conector USB del ESP32 queda en el lado derecho
- **Touch Controller**: XPT2046

---

## 1. PANTALLA PRINCIPAL (DASHBOARD)

### 1.1 Inicialización y Logo (HUD::init)

Al iniciar el sistema, se muestran los siguientes elementos:

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 118 | Texto título | (240, 50) | TFT_WHITE | "Mercedes AMG GT" - Fuente 4 |
| 122 | Texto estado | (240, 80) | TFT_DARKGREY | "Esperando sensores..." - Fuente 2 |
| 126 | Rectángulo cuerpo | (175, 100, 130, 150) | TFT_DARKGREY | Contorno coche placeholder |
| 127 | Círculo FL | (195, 115, R=10) | TFT_DARKGREY | Rueda delantera izquierda placeholder |
| 128 | Círculo FR | (285, 115, R=10) | TFT_DARKGREY | Rueda delantera derecha placeholder |
| 129 | Círculo RL | (195, 235, R=10) | TFT_DARKGREY | Rueda trasera izquierda placeholder |
| 130 | Círculo RR | (285, 235, R=10) | TFT_DARKGREY | Rueda trasera derecha placeholder |

### 1.2 Pantalla de Logo (HUD::showLogo)

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 137 | Fondo | Pantalla completa | TFT_BLACK | Limpia pantalla |
| 141 | Texto principal | (240, 120) | TFT_WHITE | "Mercedes AMG" - Fuente 4 |
| 142 | Texto secundario | (240, 170) | TFT_WHITE | "Sistema de arranque" - Fuente 2 |

### 1.3 Pantalla Ready (HUD::showReady)

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 146 | Fondo | Pantalla completa | TFT_BLACK | Limpia pantalla |
| 150 | Texto | (240, 40) | TFT_GREEN | "READY" - Fuente 4 |

### 1.4 Pantalla Error (HUD::showError)

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 154 | Fondo | Pantalla completa | TFT_BLACK | Limpia pantalla |
| 158 | Texto | (240, 40) | TFT_RED | "ERROR" - Fuente 4 |

### 1.5 Cuerpo del Coche (drawCarBody)

Vista superior del vehículo dibujada en el centro del dashboard:

| Línea | Elemento | Posición/Coordenadas | Color | Descripción |
|-------|----------|---------------------|-------|-------------|
| 202 | Rectángulo cuerpo | (175, 100, 130, 150) | TFT_DARKGREY | Contorno principal del coche |
| 207 | Rectángulo capó | (190, 100, 100, 20) | TFT_DARKGREY | Zona frontal (capó) |
| 210 | Rectángulo maletero | (190, 230, 100, 20) | TFT_DARKGREY | Zona trasera (maletero) |
| 216 | Línea central | (240, 125) a (240, 225) | TFT_DARKGREY | Eje longitudinal del coche |
| 219 | Línea eje frontal | (195, 115) a (285, 115) | TFT_DARKGREY | Conecta ruedas delanteras |
| 220 | Línea eje trasero | (195, 235) a (285, 235) | TFT_DARKGREY | Conecta ruedas traseras |
| 223 | Línea lateral izq | (195, 115) a (195, 235) | TFT_DARKGREY | Rail lateral izquierdo |
| 224 | Línea lateral der | (285, 115) a (285, 235) | TFT_DARKGREY | Rail lateral derecho |

### 1.6 Indicador de Ángulo del Volante (drawSteeringAngle) - v2.8.3

Muestra el ángulo del volante en grados en el centro del coche:

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 230-263 | Área limpieza | (cx - 35, cy - 8, 70, 16) = (205, 177, 70, 16)<br><sub>*Calculado en código con cx=240, cy=185*</sub> | TFT_BLACK | Limpiar zona de texto |
| - | Texto ángulo | (240, 185) | Variable* | Formato: "+0°" o "-15°" |

*Colores del indicador de ángulo:
- Verde (TFT_GREEN): < 5° (centrado)
- Cian (TFT_CYAN): 5-20° (ligeramente girado)
- Amarillo (TFT_YELLOW): 20-35° (girado moderadamente)
- Naranja (TFT_ORANGE): > 35° (muy girado)

### 1.7 Barra de Pedal (HUD::drawPedalBar)

Barra horizontal en la parte inferior de la pantalla:

| Línea | Condición | Elemento | Posición | Color | Descripción |
|-------|-----------|----------|----------|-------|-------------|
| 270-274 | Pedal inválido | Rectángulo negro | (0, 300, 480, 18) | TFT_BLACK | Fondo vacío |
| 273 | Pedal inválido | Texto | (240, 309) | TFT_DARKGREY | "-- PEDAL --" |
| 292 | Pedal válido | Barra progreso | (0, 300, ancho%, 18) | Variable* | Barra de progreso |
| 296 | Pedal válido | Resto barra | (ancho%, 300, resto, 18) | TFT_DARKGREY | Fondo no rellenado |
| 304 | Pedal válido | Texto porcentaje | (240, 309) | TFT_WHITE | "XX%" |

*Color de la barra:
- Verde (TFT_GREEN): 0-80%
- Amarillo (TFT_YELLOW): 80-95%
- Rojo (TFT_RED): >95%

---

## 2. MEDIDORES (GAUGES)

### 2.1 Fondo de Medidor (drawGaugeBackground)

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 34 | Círculo relleno | (cx, cy, R=70) | TFT_BLACK | Fondo negro del gauge |
| 35 | Círculo exterior | (cx, cy, R=70) | TFT_BLUE | Arco externo azul |
| 36 | Círculo medio | (cx, cy, R=60) | TFT_GREEN | Arco medio verde |
| 37 | Círculo interior | (cx, cy, R=50) | TFT_RED | Arco interno rojo |

### 2.2 Velocímetro (Gauges::drawSpeed)

Posición: Centro en (70, 175) - Lado izquierdo

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 65 | Aguja | Centro (70, 175), R=60 | TFT_WHITE | Aguja indicadora velocidad |
| 70 | Rectángulo limpieza | (30, 165, 80, 20) | TFT_BLACK | Zona para texto |
| 73 | Texto velocidad | (70, 175) | TFT_WHITE | "XXX km/h" - Fuente 2 |
| 77 | Texto pedal | (70, 197) | TFT_WHITE | "XX%" - Fuente 2 |

### 2.3 Tacómetro RPM (Gauges::drawRPM)

Posición: Centro en (410, 175) - Lado derecho

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 97 | Aguja | Centro (410, 175), R=60 | TFT_WHITE | Aguja indicadora RPM |
| 102 | Rectángulo limpieza | (370, 165, 80, 20) | TFT_BLACK | Zona para texto |
| 105 | Texto RPM | (410, 175) | TFT_WHITE | "XXXX RPM" - Fuente 2 |

---

## 3. DISPLAY DE RUEDAS (WHEELS_DISPLAY)

### 3.1 Posiciones de las Ruedas

| Rueda | Centro X | Centro Y | Descripción |
|-------|----------|----------|-------------|
| FL (Front Left) | 195 | 115 | Delantera izquierda |
| FR (Front Right) | 285 | 115 | Delantera derecha |
| RL (Rear Left) | 195 | 235 | Trasera izquierda |
| RR (Rear Right) | 285 | 235 | Trasera derecha |

### 3.2 Dibujo de Cada Rueda (WheelsDisplay::drawWheel)

| Línea | Elemento | Posición Relativa | Color | Descripción |
|-------|----------|-------------------|-------|-------------|
| 60 | Rectángulo limpieza | (cx-22, cy-8, 44, 16) | TFT_BLACK | Fondo para rueda |
| 62-63 | Triángulos rueda | Rotados según ángulo | TFT_DARKGREY | Forma rectangular rueda (40x12) |
| 68 | Línea flecha | Centro hacia adelante | TFT_WHITE | Indicador dirección (20px) |
| 74 | Rectángulo temp | (cx-30, cy-30, 60, 15) | TFT_BLACK | Zona para temperatura |
| 86 | Texto temperatura | (cx, cy-20) | Variable* | "XX °C" o "-- °C" |
| 89 | Rectángulo esfuerzo | (cx-30, cy+10, 60, 15) | TFT_BLACK | Zona para esfuerzo |
| 99 | Texto esfuerzo | (cx, cy+20) | Variable** | "XX %" o "-- %" |
| 103-108 | Barra esfuerzo | (cx-25, cy+28, 50, 6) | Variable** | Barra de progreso esfuerzo |

*Color temperatura:
- Verde (TFT_GREEN): <60°C
- Amarillo (TFT_YELLOW): 60-80°C  
- Rojo (TFT_RED): >80°C
- Gris (TFT_DARKGREY): Sensor deshabilitado

**Color esfuerzo:
- Verde (TFT_GREEN): <60%
- Amarillo (TFT_YELLOW): 60-85%
- Rojo (TFT_RED): >85%
- Gris (TFT_DARKGREY): Sensor deshabilitado

---

## 4. ICONOS (ICONS)

### 4.1 Estado del Sistema (Icons::drawSystemState)

Posición: Esquina superior izquierda

| Línea | Estado | Posición | Color | Texto |
|-------|--------|----------|-------|-------|
| 56 | Área | (5, 5, 80, 20) | TFT_BLACK | Zona de texto |
| 50 | PRECHECK | (10, 10) | TFT_YELLOW | "PRE" |
| 51 | READY | (10, 10) | TFT_GREEN | "READY" |
| 52 | RUN | (10, 10) | TFT_CYAN | "RUN" |
| 53 | ERROR | (10, 10) | TFT_RED | "ERROR" |
| 54 | OFF | (10, 10) | TFT_WHITE | "OFF" |

### 4.2 Marcha/Gear (Icons::drawGear)

Posición: Esquina superior derecha

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 75 | Área | (395, 5, 80, 20) | TFT_BLACK | Zona limpieza |
| 78 | Texto marcha | (410, 10) | TFT_WHITE | "P", "D2", "D1", "N" o "R" |

### 4.3 Funciones (Icons::drawFeatures)

| Función | Posición X1,Y1 | Posición X2,Y2 | Color ON | Color OFF | Texto |
|---------|----------------|----------------|----------|-----------|-------|
| Luces | (0, 0) | (50, 40) | TFT_YELLOW | TFT_DARKGREY | "LUCES" |
| Media | (60, 0) | (110, 40) | TFT_GREEN | TFT_DARKGREY | "MEDIA" |
| 4x4/4x2 | (0, 280) | (60, 320) | TFT_CYAN/TFT_ORANGE | - | "4x4"/"4x2" |
| Regen | (310, 285) | (410, 315) | TFT_BLUE | TFT_DARKGREY | "REGEN" |

### 4.4 Batería (Icons::drawBattery)

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 125 | Área | (420, 0, 50, 40) | TFT_BLACK | Zona limpieza |
| 130 | Texto voltaje | (425, 10) | TFT_WHITE | "XX.X V" |

### 4.5 Advertencia de Errores (Icons::drawErrorWarning)

| Línea | Condición | Elemento | Posición | Color | Descripción |
|-------|-----------|----------|----------|-------|-------------|
| 143 | Hay errores | Triángulo | (200-280, Y variable) | TFT_YELLOW | Triángulo de advertencia |
| 147 | Hay errores | Texto contador | (285, 15) | TFT_YELLOW | "X" (número de errores) |
| 149 | Sin errores | Área limpia | (200, 0, 120, 40) | TFT_BLACK | Limpia indicador |

### 4.6 Estado de Sensores (Icons::drawSensorStatus)

Indicadores LED para grupos de sensores:

| LED | Posición X | Tipo | Etiqueta | Color |
|-----|------------|------|----------|-------|
| 1 | 305 | Corriente (INA226) | "I" | Verde/Amarillo/Rojo |
| 2 | 345 | Temperatura (DS18B20) | "T" | Verde/Amarillo/Rojo |
| 3 | 385 | Ruedas | "W" | Verde/Amarillo/Rojo |

| Línea | Elemento | Posición | Descripción |
|-------|----------|----------|-------------|
| 188 | Círculo LED 1 | (305, 12, R=6) | LED corriente |
| 191 | Texto "I" | (305, 25) | Etiqueta corriente |
| 195 | Círculo LED 2 | (345, 12, R=6) | LED temperatura |
| 197 | Texto "T" | (345, 25) | Etiqueta temperatura |
| 201 | Círculo LED 3 | (385, 12, R=6) | LED ruedas |
| 203 | Texto "W" | (385, 25) | Etiqueta ruedas |

### 4.7 Advertencia de Temperatura (Icons::drawTempWarning)

| Línea | Condición | Elemento | Posición | Color | Descripción |
|-------|-----------|----------|----------|-------|-------------|
| 215 | Área | (70, 280, 60, 20) | TFT_BLACK | Limpia zona |
| 222-223 | Temp crítica | Texto | (75, 290) | TFT_RED | "XXC!" |

---

## 5. MENÚ OCULTO (MENU_HIDDEN)

### 5.1 Código de Acceso: 8989

El menú oculto se activa tocando el icono de batería con la secuencia 8-9-8-9.

### 5.2 Pantalla Principal del Menú (drawMenuFull)

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 575 | Área menú | (60, 40, 360, 240) | TFT_BLACK | Fondo del menú |
| 576 | Borde menú | (60, 40, 360, 240) | TFT_WHITE | Marco exterior |
| 580 | Título | (80, 50) | TFT_WHITE | "MENU OCULTO" |
| 583-586 | Opciones 1-8 | (80, 80+i*20) | Variable | Lista de opciones |
| 591-592 | Código actual | (80, 240) + (140, 240) | TFT_WHITE | "Code: XXXX" |

### 5.3 Opciones del Menú

| # | Texto | Y | Acción |
|---|-------|---|--------|
| 1 | "1) Calibrar pedal" | 80 | Inicia calibración de pedal |
| 2 | "2) Calibrar encoder" | 100 | Inicia calibración de volante |
| 3 | "3) Ajuste regen (%)" | 120 | Ajuste regeneración interactivo |
| 4 | "4) Modulos ON/OFF" | 140 | Activa/desactiva módulos |
| 5 | "5) Guardar y salir" | 160 | Guarda config y cierra |
| 6 | "6) Restaurar fabrica" | 180 | Restaura valores por defecto |
| 7 | "7) Ver errores" | 200 | Muestra errores persistentes |
| 8 | "8) Borrar errores" | 220 | Borra errores (con confirmación) |

Colores de opciones:
- Opción seleccionada: TFT_YELLOW
- Opción no seleccionada: TFT_WHITE

---

## 6. PANTALLAS DE CALIBRACIÓN

### 6.1 Pantalla Base de Calibración (drawCalibrationScreen)

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 88 | Área | (60, 40, 360, 240) | TFT_BLACK | Fondo |
| 89 | Marco | (60, 40, 360, 240) | TFT_CYAN | Borde cyan |
| 93 | Título | (240, 50) | TFT_CYAN | Título centrado - Fuente 4 |
| 97 | Instrucción | (240, 130) | TFT_WHITE | Texto guía - Fuente 2 |
| 100 | Valor | (240, 180) | TFT_GREEN | Valor actual - Fuente 4 |
| 105-106 | Barra progreso | (140, 220, 200, 10) | TFT_DARKGREY/WHITE | Timeout visual |
| 110 | Texto ayuda | (240, 260) | TFT_YELLOW | "Toca pantalla para confirmar" |

### 6.2 Calibración de Pedal

**Fase 1: Mínimo (PEDAL_MIN)**

| Línea | Elemento | Valor | Descripción |
|-------|----------|-------|-------------|
| - | Título | "CALIBRAR PEDAL" | Cabecera |
| - | Instrucción | "Suelta el pedal (mínimo)" | Guía usuario |
| - | Valor | "ADC: XXXX" | Valor raw actual |

**Fase 2: Máximo (PEDAL_MAX)**

| Línea | Elemento | Valor | Descripción |
|-------|----------|-------|-------------|
| - | Título | "CALIBRAR PEDAL" | Cabecera |
| - | Instrucción | "Pisa el pedal al máximo" | Guía usuario |
| - | Valor | "ADC: XXXX" | Valor raw actual |

**Resultado OK**

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 175-178 | Área | (60, 40, 360, 240) | TFT_BLACK | Fondo |
| - | Marco | (60, 40, 360, 240) | TFT_GREEN | Borde verde |
| 179 | Texto OK | (240, 140) | TFT_GREEN | "CALIBRACION OK" - Fuente 4 |
| 182 | Valores | (240, 180) | TFT_GREEN | "MIN:XXX MAX:XXXX" |

**Resultado FALLO**

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 191-195 | Área | (60, 40, 360, 240) | TFT_BLACK/TFT_RED | Fondo y borde |
| 196 | Texto error | (240, 140) | TFT_RED | "CALIBRACION FALLIDA" - Fuente 4 |
| 198 | Mensaje | (240, 180) | TFT_WHITE | "Rango insuficiente" |

### 6.3 Calibración de Encoder/Volante

**Fase: Centrado (ENCODER_CENTER)**

| Elemento | Valor | Descripción |
|----------|-------|-------------|
| Título | "CALIBRAR VOLANTE" | Cabecera |
| Instrucción | "Centra el volante y confirma" | Guía usuario |
| Valor | "Ticks: XXXX" | Valor encoder actual |

**Resultado OK**

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 236-239 | Área | (60, 40, 360, 240) | TFT_BLACK/TFT_GREEN | Fondo y borde |
| 240 | Texto OK | (240, 140) | TFT_GREEN | "VOLANTE CENTRADO" - Fuente 4 |
| 243 | Offset | (240, 180) | TFT_GREEN | "Offset: XXXX" |

### 6.4 Ajuste de Regeneración (REGEN_ADJUST)

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 272-273 | Área | (60, 40, 360, 240) | TFT_BLACK | Fondo |
| - | Marco | (60, 40, 360, 240) | TFT_CYAN | Borde cyan |
| 277 | Título | (240, 50) | TFT_CYAN | "AJUSTE REGENERACION" - Fuente 4 |
| 284 | Valor grande | (240, 120) | TFT_GREEN | "XX%" - Fuente 6 |
| 288-290 | Barra visual | (100, 170, 280, 20) | DARKGREY/GREEN | Barra de progreso |
| 293-296 | Botón [-] | (80, 200, 60, 40) | TFT_RED | "-10" |
| 299-301 | Botón [+] | (340, 200, 60, 40) | TFT_GREEN | "+10" |
| 304-307 | Botón guardar | (180, 200, 120, 40) | TFT_BLUE | "GUARDAR" |
| 311 | Instrucción | (240, 260) | TFT_YELLOW | "Toca [-] o [+] para ajustar" |

### 6.5 Pantalla Ver Errores (showErrors)

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 406-407 | Área | (60, 40, 360, 240) | TFT_BLACK | Fondo |
| - | Marco | (60, 40, 360, 240) | TFT_ORANGE | Borde naranja |
| 410 | Título | (240, 50) | TFT_ORANGE | "ERRORES PERSISTENTES" |
| 418 | Sin errores | (240, 150) | TFT_GREEN | "Sin errores" - Fuente 4 |
| 425-426 | Lista errores | (80, 80+i*18) | TFT_WHITE | "Error X: Codigo XXX" |
| 432-434 | Total | (80, Y+10) | TFT_YELLOW | "Total: X errores" |
| 439 | Instrucción | (240, 260) | TFT_CYAN | "Toca para volver" |

### 6.6 Confirmación Borrar Errores (CLEAR_ERRORS_CONFIRM)

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 483-484 | Área | (60, 40, 360, 240) | TFT_BLACK | Fondo |
| - | Marco | (60, 40, 360, 240) | TFT_RED | Borde rojo (peligro) |
| 488 | Título | (240, 50) | TFT_RED | "CONFIRMAR BORRADO" - Fuente 4 |
| 495 | Mensaje 1 | (240, 100) | TFT_WHITE | "Borrar X errores?" |
| 496 | Mensaje 2 | (240, 125) | TFT_WHITE | "Esta accion es irreversible" |
| 499-502 | Botón Cancelar | (80, 180, 120, 50) | TFT_DARKGREY | "CANCELAR" |
| 505-508 | Botón Borrar | (280, 180, 120, 50) | TFT_RED | "BORRAR" |

**Resultado: Errores Borrados**

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 534-536 | Área | (60, 40, 360, 240) | TFT_BLACK/TFT_GREEN | Fondo y borde |
| 538 | Texto OK | (240, 140) | TFT_GREEN | "ERRORES BORRADOS" - Fuente 4 |

---

## 7. PANTALLAS DEL HUD MANAGER

### 7.1 Pantalla de Arranque (Boot Screen) - ELIMINADA v2.8.3

> **Nota v2.8.3**: La pantalla azul de boot ha sido eliminada. Ahora el display va directo a negro y luego al dashboard, evitando parpadeo visual innecesario.

| Línea | Elemento | Descripción |
|-------|----------|-------------|
| 53-56 | Limpieza directa | `tft.fillScreen(TFT_BLACK)` - Directo a pantalla negra |

### 7.2 Pantalla de Configuración (renderSettings)

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 262-263 | Título | (20, 20) | TFT_CYAN | "CONFIGURACION" - Size 2 |
| 269 | Opción 1 | (20, 60) | TFT_CYAN | "[ ] Ajustes de pantalla" |
| 271 | Opción 2 | (20, 90) | TFT_CYAN | "[ ] Calibracion sensores" |
| 273 | Opción 3 | (20, 120) | TFT_CYAN | "[ ] WiFi/OTA" |
| 275 | Opción 4 | (20, 150) | TFT_CYAN | "[ ] Test hardware" |

### 7.3 Pantalla de Calibración (renderCalibration)

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 279-280 | Título | (20, 20) | TFT_YELLOW | "CALIBRACION" - Size 2 |
| 286-288 | Ángulo volante | (20, 60) | TFT_YELLOW | "Angulo volante: XX grados" |
| 289-291 | Pedal | (20, 90) | TFT_YELLOW | "Pedal acelerador: XX%" |

### 7.4 Test de Hardware (renderHardwareTest)

#### Columna Izquierda - Sensores

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 296-299 | Título | (20, 5) | TFT_GREEN | "TEST HARDWARE" - Size 2 |
| 309-310 | Cabecera | (10, 35) | TFT_YELLOW | "-- SENSORES --" |
| 317 | INA226 | (10, 50) | Variable | "INA226: X/X" |
| 324 | DS18B20 | (10, 65) | Variable | "DS18B20: X/X" |
| 331 | Ruedas | (10, 80) | Variable | "Ruedas: X/X" |
| 336 | Batería | (10, 95) | Variable | "Bateria: OK/FAIL" |
| 343-351 | Temp máx | (10, 115) | Variable | "Temp max: XX.XC" o "TEMP CRIT: XX.XC" |

#### Columna Derecha - Entradas

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 355 | Cabecera | (250, 35) | TFT_YELLOW | "-- ENTRADAS --" |
| 363 | Pedal | (250, 50) | Variable | "Pedal: XX.X% [raw]" |
| 370 | Volante | (250, 65) | Variable | "Volante: XX.X deg" |
| 372 | Centrado | (250, 80) | Variable | "Centrado: SI/NO" |
| 380 | Marcha | (250, 95) | Variable | "Marcha: P/D2/D1/N/R" |
| 385 | Botones | (250, 110) | Variable | "Botones: OK/FAIL" |
| 388-391 | Estado btns | (250, 125) | TFT_CYAN | "Estado: L:ON/off M:ON/off 4:ON/off" |

#### Estado General

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 394-395 | Cabecera | (10, 150) | TFT_YELLOW | "-- ESTADO GENERAL --" |
| 403-404 | Sensores | (10, 165) | TFT_GREEN/TFT_YELLOW/TFT_RED* | "Sensores: TODOS OK/PARCIAL/FALLO" |
| 409-410 | Entradas | (10, 180) | TFT_GREEN/TFT_RED* | "Entradas: TODOS OK/FALLO" |
| 419-420 | Sistema | (10, 200) | TFT_GREEN/TFT_YELLOW/TFT_RED* | "SISTEMA: OK/WARN/FAIL" - Size 2 |
| 429 | Cal pedal | (250, 165) | TFT_MAGENTA | "Pedal cal: XXX-XXXX" |
| 431 | Curva | (250, 180) | TFT_MAGENTA | "Curva: Lineal/Suave/Agresiva" |

*Colores de estado:
- TFT_GREEN: Todos los componentes funcionan correctamente (OK/TODOS OK)
- TFT_YELLOW: Funcionamiento parcial o advertencia (WARN/PARCIAL)
- TFT_RED: Fallo crítico o error (FAIL/FALLO)

### 7.5 Monitor INA226 (renderINA226Monitor)

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 449-451 | Título | (20, 20) | TFT_ORANGE | "MONITOR INA226" - Size 2 |
| 455-460 | Motores 1-4 | (20, 60+i*30) | TFT_ORANGE | "Motor X: XXX.XA XXX.XV XXXW" |
| 466 | Dirección | (20, 220) | TFT_ORANGE | "Direccion: XXX.XA" |
| 468 | Batería | (20, 250) | TFT_ORANGE | "Bateria: XXX.XA XXX.XV" |

### 7.6 Menú Oculto Completo (renderHiddenMenu)

Esta es la pantalla más completa con todos los datos del sistema:

#### Sección 1: Energía

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 506-509 | Título | (10, 5) | TFT_RED | "=== MENU OCULTO ===" - Size 2 |
| 516 | Cabecera | (5, 30) | TFT_YELLOW | "ENERGIA:" |
| 518 | Voltaje | (5, 45) | TFT_YELLOW | "Voltaje: XXX.XXV (XXX%)" |
| 520 | Corriente | (5, 60) | TFT_YELLOW | "Corriente: XXX.XXA" |
| 522 | Potencia | (5, 75) | TFT_YELLOW | "Potencia: XXXXX.XW" |

#### Sección 2: Motores

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 525 | Cabecera | (5, 95) | TFT_YELLOW | "MOTORES:" |
| 527 | FL/FR | (5, 110) | TFT_YELLOW | "FL:XXX.XA FR:XXX.XA" |
| 529 | RL/RR | (5, 125) | TFT_YELLOW | "RL:XXX.XA RR:XXX.XA" |
| 531 | Dirección | (5, 140) | TFT_YELLOW | "Direccion: XXX.XA" |

#### Sección 3: Temperaturas

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 534 | Cabecera | (250, 30) | TFT_YELLOW | "TEMPERATURAS:" |
| 536 | Principal | (250, 45) | TFT_YELLOW | "Motor Principal: XX.XC" |
| 538 | M1/M2 | (250, 60) | TFT_YELLOW | "M1:XXX M2:XXX" |
| 540 | M3/M4 | (250, 75) | TFT_YELLOW | "M3:XXX M4:XXX" |
| 542 | Ambiente | (250, 90) | TFT_YELLOW | "Ambiente: XX.XC" |
| 544 | Controlador | (250, 105) | TFT_YELLOW | "Controlador: XX.XC" |

#### Sección 4: Entradas

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 550 | Cabecera | (250, 125) | TFT_YELLOW | "ENTRADAS:" |
| 559 | Pedal | (250, 140) | Variable | "Pedal: XXX.X% [raw]" |
| 566 | Volante | (250, 155) | Variable | "Volante: XXX.X deg" |
| 574 | Marcha | (250, 170) | Variable | "Marcha: P/D2/D1/N/R" |

#### Sección 5: Movimiento

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 578 | Cabecera | (5, 160) | TFT_YELLOW | "MOVIMIENTO:" |
| 580 | Velocidad | (5, 175) | TFT_YELLOW | "Velocidad: XXX.X km/h" |
| 582 | RPM | (5, 190) | TFT_YELLOW | "RPM: XXXXXX" |

#### Sección 6: Sensores

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 586-587 | Cabecera | (5, 210) | TFT_YELLOW | "SENSORES:" |
| 595 | INA226 | (5, 225) | Variable | "INA226: X/X" |
| 602 | DS18B20 | (80, 225) | Variable | "DS18B20: X/X" |
| 609 | Ruedas | (175, 225) | Variable | "RUEDAS: X/X" |
| 615 | Temp crítica | (5, 240) | TFT_RED | "!! TEMP CRITICA: XX.XC !!" |

#### Sección 7: Estado General

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 620 | Cabecera | (250, 200) | TFT_YELLOW | "SISTEMA:" |
| 627 | Inputs | (250, 215) | Variable | "Inputs: OK/FAIL" |
| 635 | Botones | (250, 230) | TFT_CYAN | "BTN: L-M-4" |
| 640 | Odómetro | (250, 245) | TFT_YELLOW | "Odo: XXX.X/XXX.X" |
| 649 | Estado sys | (250, 260) | Variable | "SYS: OK/WARN/FAIL" |
| 654 | Instrucción | (5, 300) | TFT_CYAN | "Pulse 3s bateria para salir" |

---

## 8. DETECCIÓN DE OBSTÁCULOS (OBSTACLE_DISPLAY)

### 8.1 Indicadores de Proximidad

| Línea | Sensor | Posición | Color | Descripción |
|-------|--------|----------|-------|-------------|
| 38 | 0 (Frente) | (240, 100) | Variable | Círculo R=10 |
| 38 | 1 (Izq) | (180, 160) | Variable | Círculo R=10 |
| 38 | 2 (Atrás) | (240, 220) | Variable | Círculo R=10 |
| 38 | 3 (Der) | (300, 160) | Variable | Círculo R=10 |

Colores por nivel de proximidad:
- TFT_GREEN: Sin peligro
- TFT_ORANGE: Precaución
- TFT_YELLOW: Advertencia
- TFT_RED: Crítico

### 8.2 Barras de Distancia

| Línea | Sensor | Posición | Color | Descripción |
|-------|--------|----------|-------|-------------|
| 51 | 0-3 | (40+i*110, 300, 100, 10) | TFT_CYAN | Barra proporcional a distancia |
| 53 | 0-3 | (40+i*110, 285) | TFT_CYAN | Texto "XXXcm" |

---

## 9. MODO DEMO (STANDALONE_DISPLAY)

### 9.1 Botón Demo

| Línea | Elemento | Posición | Color | Descripción |
|-------|----------|----------|-------|-------------|
| 168-173 | Botón fondo | (400, 260, 75, 35) | TFT_NAVY | Rectángulo redondeado |
| 180 | Texto DEMO | Centro botón | TFT_CYAN | "DEMO" - Fuente 2 |
| 182 | Texto MENU | Centro botón | TFT_WHITE | "MENU" - Fuente 1 |

---

## 10. RESUMEN DE ZONAS TÁCTILES

| Zona | X1 | Y1 | X2 | Y2 | Acción |
|------|----|----|----|----|--------|
| Batería | 420 | 0 | 470 | 40 | Entrada código menú oculto |
| Luces | 0 | 0 | 50 | 40 | Toggle luces |
| Multimedia | 60 | 0 | 110 | 40 | Toggle multimedia |
| 4x4 | 0 | 280 | 60 | 320 | Toggle modo 4x4 |
| Warning | 200 | 0 | 280 | 40 | Ver errores |
| Demo (standalone) | 400 | 260 | 475 | 295 | Menú demo |

---

## 11. CONSTANTES DE COLOR

| Constante | Valor Hex | Uso |
|-----------|-----------|-----|
| TFT_BLACK | 0x0000 | Fondos |
| TFT_WHITE | 0xFFFF | Texto principal |
| TFT_RED | 0xF800 | Errores, críticos |
| TFT_GREEN | 0x07E0 | OK, normal |
| TFT_BLUE | 0x001F | Boot, decorativo |
| TFT_YELLOW | 0xFFE0 | Advertencias, seleccionado |
| TFT_CYAN | 0x07FF | Info, instrucciones |
| TFT_ORANGE | 0xFC00 | Precaución |
| TFT_DARKGREY | 0x7BEF | Deshabilitado, placeholder |
| TFT_NAVY | 0x000F | Fondo botones |
| TFT_MAGENTA | 0xF81F | Info calibración |

---

## 12. DIAGRAMA DE LAYOUT PRINCIPAL (480x320)

```
+----------------------------------------------------------+
|  LUCES  |  MEDIA  |         WARNING        | I T W | BAT |  <- Y=0-40
|  0-50   |  60-110 |        200-280         |290-410|420+ |
+----------------------------------------------------------+
|                                                          |
|    [SPEED GAUGE]              [CAR]         [RPM GAUGE]  |  <- Y=100-250
|     Centro: 70                Centro        Centro: 410  |
|                               240                        |
|           FL -----  CUERPO  ----- FR                     |
|               |               |                          |
|               |    [+0°]     |     <- ÁNGULO VOLANTE    |
|               |   VOLANTE    |        (Y=185, centro)   |
|               |               |                          |
|           RL -----  CUERPO  ----- RR                     |
|                                                          |
+----------------------------------------------------------+
|  4x4  |  TEMP  |           |  REGEN  |                   |  <- Y=280-300
+----------------------------------------------------------+
|                    BARRA PEDAL (0-100%)                  |  <- Y=300-318
+----------------------------------------------------------+
```

---

*Documento generado para análisis de gráficos del display del firmware Mercedes AMG GT*
*Versión: 2.8.3*
