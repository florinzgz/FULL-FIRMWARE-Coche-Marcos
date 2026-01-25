# 🚗 MANUAL DE USUARIO - Coche Eléctrico Inteligente ESP32-S3

**Versión:** 2.17.1  
**Fecha:** Enero 2026  
**Hardware:** ESP32-S3 N16R8

---

## 📋 ÍNDICE

1. [Introducción](#introducción)
2. [Advertencias de Seguridad](#advertencias-de-seguridad)
3. [Componentes del Vehículo](#componentes-del-vehículo)
4. [Montaje y Configuración Inicial](#montaje-y-configuración-inicial)
5. [Encendido y Apagado](#encendido-y-apagado)
6. [Pantalla Táctil e Interfaz](#pantalla-táctil-e-interfaz)
7. [Modos de Conducción](#modos-de-conducción)
8. [Sistemas de Seguridad](#sistemas-de-seguridad)
9. [Sistema de Iluminación](#sistema-de-iluminación)
10. [Sistema de Audio](#sistema-de-audio)
11. [Calibraciones](#calibraciones)
12. [Mantenimiento](#mantenimiento)
13. [Solución de Problemas](#solución-de-problemas)
14. [Especificaciones Técnicas](#especificaciones-técnicas)

---

## 1️⃣ INTRODUCCIÓN

Bienvenido al manual de usuario de tu coche eléctrico inteligente basado en ESP32-S3. Este vehículo cuenta con tecnología avanzada que incluye:

- ✅ **Control inteligente de tracción 4x4** con motores independientes
- ✅ **Sistema de dirección electrónica** con encoder de alta precisión
- ✅ **Pantalla táctil a color** de 480x320 píxeles
- ✅ **Sistemas de seguridad avanzados** (ABS, TCS, frenado regenerativo)
- ✅ **Iluminación LED inteligente** con 44 LEDs programables
- ✅ **Sistema de audio** con alertas por voz
- ✅ **Monitorización en tiempo real** de corriente, temperatura y velocidad

---

## ⚠️ ADVERTENCIAS DE SEGURIDAD

### ANTES DE USAR EL VEHÍCULO:

1. **Supervisión adulta obligatoria**: Este vehículo debe ser usado siempre bajo supervisión de un adulto.

2. **Edad recomendada**: 3-8 años (peso máximo 30 kg).

3. **Terreno adecuado**: Usar solo en superficies planas, lisas y sin obstáculos.

4. **Batería**: 
   - Usar solo baterías de 24V especificadas
   - No cortocircuitar los terminales
   - Desconectar cuando no se use por periodos prolongados

5. **Inspección pre-uso**:
   - Verificar que todas las conexiones estén firmes
   - Comprobar el estado de las ruedas
   - Verificar que no haya cables sueltos

6. **Límites de operación**:
   - No usar en pendientes pronunciadas
   - No sumergir en agua
   - No exponer a lluvia o humedad extrema

---

## 🔧 COMPONENTES DEL VEHÍCULO

### 3.1 Hardware Principal

```
┌─────────────────────────────────────────────────┐
│  COMPONENTES INSTALADOS                         │
├─────────────────────────────────────────────────┤
│ 1. ESP32-S3-DevKitC-1 (Cerebro del vehículo)  │
│ 2. Pantalla ST7796S 480x320 + Touch XPT2046    │
│ 3. 4x Motores de tracción RS775 24V            │
│ 4. 1x Motor de dirección RS390 12V             │
│ 5. 4x Drivers BTS7960 (motores tracción)       │
│ 6. 1x Driver BTS7960 (motor dirección)         │
│ 7. 6x Sensores de corriente INA226             │
│ 8. 4x Sensores de temperatura DS18B20          │
│ 9. 4x Sensores de velocidad inductivos         │
│ 10. 1x Encoder de dirección E6B2-CWZ6C         │
│ 11. 28 LEDs frontales WS2812B                  │
│ 12. 16 LEDs traseros WS2812B                   │
│ 13. Módulo DFPlayer Mini (audio)               │
│ 14. 4x Relés de potencia                       │
│ 15. Batería 24V (no incluida)                  │
└─────────────────────────────────────────────────┘
```

### 3.2 Diagrama de Conexiones Principales

```
                    ESP32-S3 (Centro de Control)
                            │
        ┌───────────────────┼───────────────────┐
        │                   │                   │
    Pantalla          Bus I²C (GPIO 8/9)    Alimentación
   Táctil TFT              │                   24V/12V
   (480x320)               │                      │
        │          ┌────────┴────────┐            │
        │          │                 │            │
   Touch XPT2046   │                 │         Relés de
   (GPIO 21)   INA226 x6         PCA9685 x3   Potencia
                Corriente         PWM Motores      │
                    │                 │            │
                    │                 │            │
              ┌─────┴─────┐     ┌─────┴─────┐     │
              │           │     │           │     │
          Batería    Motores  BTS7960    4 Motores
           24V       24V    Drivers     Tracción
                             │              │
                             │              │
                      Motor Dirección   Ruedas
                         12V             4x4
```

---

## 🔌 MONTAJE Y CONFIGURACIÓN INICIAL

### 4.1 Primer Encendido

#### Paso 1: Conexión de la Batería
1. Verificar que todos los componentes estén correctamente montados
2. Conectar la batería de 24V a los terminales correspondientes
3. Verificar polaridad: **ROJO (+)** y **NEGRO (-)**

#### Paso 2: Encendido del Sistema
1. Presionar el botón de encendido principal
2. Esperar a que aparezca el logo de inicio en la pantalla
3. El sistema realizará un autodiagnóstico (aprox. 3-5 segundos)
4. Escucharás: *"Bienvenido Marcos. El sistema está listo para comenzar."*

#### Paso 3: Verificación Inicial
La pantalla mostrará:
- ✅ Estado de la batería
- ✅ Temperatura de motores
- ✅ Velocímetro
- ✅ Indicadores de sistema

### 4.2 Configuración de Audio (Primera Vez)

**IMPORTANTE**: Los archivos de audio NO están incluidos. Debes crearlos:

1. **Preparar tarjeta SD**:
   - Formatear una tarjeta SD en FAT32
   - Capacidad: 1GB - 32GB
   - Clase 4 o superior

2. **Crear archivos de audio**:
   - Ir a [TTSMaker.com](https://ttsmaker.com/)
   - Seleccionar idioma: Español (España)
   - Consultar `docs/AUDIO_TRACKS_GUIDE.md` para textos completos
   - Generar y descargar cada archivo
   - Renombrar como: `0001.mp3`, `0002.mp3`, etc.

3. **Copiar a la tarjeta SD**:
   - Copiar TODOS los archivos a la raíz de la SD (no en carpetas)
   - Insertar la SD en el módulo DFPlayer Mini

**Audios principales (mínimo requerido)**:
- 0001.mp3: "Bienvenido Marcos. El sistema está listo para comenzar."
- 0002.mp3: "Cerrando sistemas. Hasta pronto."
- 0020.mp3: "Marcha D uno activada."
- 0022.mp3: "Marcha atrás activada."
- 0024.mp3: "Vehículo en posición de estacionamiento."

---

## 🔋 ENCENDIDO Y APAGADO

### 5.1 Encendido Completo

```
Secuencia de encendido:
┌─────────────────────┐
│ 1. Conectar batería │
│ 2. Pulsar Power     │ ──→ Logo en pantalla
│ 3. Esperar 5 seg    │ ──→ Audio: "Bienvenido..."
│ 4. Sistema listo    │ ──→ Dashboard visible
└─────────────────────┘
```

**Indicadores de encendido correcto**:
- 🟢 Pantalla iluminada con dashboard
- 🟢 Audio de bienvenida
- 🟢 LEDs frontales en modo de espera
- 🟢 Voltaje de batería visible (aprox. 24V)

### 5.2 Apagado Seguro

**IMPORTANTE**: Siempre apagar de forma segura para proteger el sistema.

1. **Detener el vehículo completamente** (velocidad = 0)
2. **Poner en modo PARK (P)** (ver sección 7.2)
3. **Pulsar botón de apagado** o esperar auto-apagado
4. Escucharás: *"Cerrando sistemas. Hasta pronto."*
5. La pantalla se apagará progresivamente
6. **Desconectar batería** si no se usará por varios días

---

## 📱 PANTALLA TÁCTIL E INTERFAZ

### 6.1 Pantalla Principal (Dashboard)

```
┌─────────────────────────────────────────────────┐
│ ⚡24.3V  🌡️25°C           [MODO: DRIVE]  🔊   │
├─────────────────────────────────────────────────┤
│                                                 │
│         ┌─────────────┐     ┌──────────────┐   │
│         │ Velocímetro │     │   Energía    │   │
│         │   0 km/h    │     │   Batería    │   │
│         │  ╱───────╲  │     │              │   │
│         │ ╱    0    ╲ │     │   ████████   │   │
│         │ ╲         ╱ │     │     85%      │   │
│         │  ╲───────╱  │     │              │   │
│         └─────────────┘     └──────────────┘   │
│                                                 │
│   🚗FL  🚗FR              Corriente: 2.5A      │
│   ●25° ●25°              Temp Max: 28°C       │
│                                                 │
│   🚗RL  🚗RR              [Modo 4x4]           │
│   ●25° ●25°                                    │
│                                                 │
│  ┌──┐ ┌──┐ ┌──┐                                │
│  │ P│ │ R│ │ D│  ← Selector de marcha         │
│  └──┘ └──┘ └──┘                                │
│                                                 │
│  [💡Luces] [🔊Audio] [⚙️Config]               │
└─────────────────────────────────────────────────┘
```

### 6.2 Elementos de la Pantalla

| Elemento | Ubicación | Información |
|----------|-----------|-------------|
| **Voltaje batería** | Superior izquierda | Voltaje actual (22-26V típico) |
| **Temperatura** | Superior izquierda | Temperatura máxima de motores |
| **Modo actual** | Superior centro | PARK/REVERSE/DRIVE |
| **Velocímetro** | Centro izquierda | Velocidad actual en km/h |
| **Indicador batería** | Centro derecha | Nivel de carga (0-100%) |
| **Estado ruedas** | Inferior | Temperatura de cada motor |
| **Selector marcha** | Inferior | P/R/D/N para cambiar modo |
| **Botones acción** | Parte inferior | Luces, Audio, Configuración |

### 6.3 Calibración del Touch

Si la pantalla táctil no responde correctamente:

**Método 1: Activación por botón físico** (Recomendado)
1. Mantener presionado el botón físico durante **5 segundos**
2. La pantalla mostrará la interfaz de calibración automáticamente
3. Tocar las 4 esquinas cuando se solicite
4. Verificar precisión tocando el centro

**Método 2: Menú oculto**
1. Tocar el **icono de batería** en la esquina superior derecha 5 veces
2. Acceder al menú de calibración
3. Seguir las instrucciones en pantalla

---

## 🚦 MODOS DE CONDUCCIÓN

### 7.1 Modos Disponibles

El vehículo incluye varios modos de operación que se seleccionan desde la pantalla táctil:

```
┌──────────┬─────────────────────────────────────────┐
│  MODO    │  DESCRIPCIÓN                            │
├──────────┼─────────────────────────────────────────┤
│ PARK (P) │ Vehículo estacionado, motores           │
│          │ bloqueados, freno activado              │
├──────────┼─────────────────────────────────────────┤
│REVERSE(R)│ Marcha atrás, velocidad limitada        │
│          │ Luces traseras en modo reversa          │
├──────────┼─────────────────────────────────────────┤
│ DRIVE(D) │ Marcha adelante normal                  │
│          │ Acceso a todos los sistemas             │
├──────────┼─────────────────────────────────────────┤
│NEUTRAL(N)│ Punto muerto, sin tracción              │
│          │ Para remolque o empuje manual           │
└──────────┴─────────────────────────────────────────┘
```

### 7.2 Cambio de Marcha

**Para cambiar de marcha**:
1. **Detener completamente el vehículo** (muy importante)
2. **Tocar el botón de la marcha deseada** en la pantalla
3. Esperar confirmación visual y audio
4. Escucharás el audio correspondiente:
   - PARK: *"Vehículo en posición de estacionamiento."*
   - REVERSE: *"Marcha atrás activada."*
   - DRIVE: *"Marcha D uno activada."*
   - NEUTRAL: *"Punto muerto."*

**⚠️ IMPORTANTE**: 
- NO cambiar de marcha mientras el vehículo está en movimiento
- Esperar siempre la confirmación antes de acelerar

### 7.3 Tracción 4x4 / 4x2

El sistema de tracción es inteligente y adaptativo:

**Modo 4x4 (Cuatro ruedas motrices)**:
- ✅ Activado automáticamente en arranque
- ✅ Máxima tracción en terrenos irregulares
- ✅ Distribución de potencia independiente
- Audio: *"Tracción 4x4 inteligente activada."*

**Modo 4x2 (Dos ruedas motrices)**:
- ✅ Mayor eficiencia energética
- ✅ Recomendado para terrenos planos
- ✅ Mayor autonomía de batería
- Audio: *"Tracción 4x2 inteligente activada."*

**Cambiar modo tracción**:
- Acceder a: Menú ⚙️ → Configuración → Tracción
- Seleccionar 4x4 o 4x2
- Confirmar selección

---

## 🛡️ SISTEMAS DE SEGURIDAD

El vehículo incluye tres sistemas avanzados de seguridad activa que funcionan automáticamente:

### 8.1 Sistema ABS (Antibloqueo de Frenos)

**¿Qué hace?**
Evita que las ruedas se bloqueen durante el frenado, manteniendo el control del vehículo.

**Funcionamiento**:
- 🔍 Monitorea cada rueda individualmente
- ⚡ Activa cuando detecta deslizamiento > 20%
- 🔄 Modula la presión de frenado 10 veces por segundo
- ✅ Solo activo por encima de 5 km/h

**Indicación de activación**:
- LED de advertencia en dashboard
- Vibración en el pedal de freno (normal)
- Audio: *"Sistema antibloqueo de frenos activado."*

**Configuración**:
- Estado: ON/OFF desde menú de configuración
- Por defecto: **Activado**

### 8.2 Sistema TCS (Control de Tracción)

**¿Qué hace?**
Evita que las ruedas patinen durante la aceleración, optimizando el agarre.

**Funcionamiento**:
- 🔍 Detecta patinaje de cualquier rueda
- ⚡ Reduce potencia de la rueda que patina
- 🔄 Transfiere potencia a ruedas con agarre
- ✅ Activo por encima de 3 km/h

**Indicación de activación**:
- Icono TCS parpadeante en pantalla
- Reducción temporal de aceleración
- Audio: *"Control de tracción activado."*

**Beneficios**:
- ✅ Mejor tracción en superficies resbaladizas
- ✅ Arranques más seguros
- ✅ Protección de neumáticos

### 8.3 Frenado Regenerativo con IA

**¿Qué hace?**
Recupera energía durante el frenado y la devuelve a la batería.

**Niveles de regeneración**:

| Nivel | Intensidad | Recuperación | Uso recomendado |
|-------|------------|--------------|------------------|
| **0** | Desactivado | 0% | Terreno plano |
| **1** | Suave | 20% | Ciudad, tráfico |
| **2** | Moderado | 50% | Uso normal |
| **3** | Fuerte | 80% | Bajadas, máxima eficiencia |

**Configuración**:
1. Menú ⚙️ → Configuración → Frenado Regenerativo
2. Seleccionar nivel deseado (0-3)
3. El cambio es inmediato

**Indicador**:
- Flecha verde en indicador de batería cuando está regenerando
- Muestra corriente negativa (cargando batería)

### 8.4 Protecciones Automáticas

El sistema cuenta con protecciones adicionales:

**Protección por Temperatura**:
- ⚠️ Alerta a 60°C: *"Temperatura del motor elevada."*
- 🛑 Reducción de potencia a 70°C
- 🚫 Parada de emergencia a 80°C

**Protección por Batería Baja**:
- ⚠️ Alerta al 20%: *"Nivel de batería bajo."*
- 🛑 Reducción de potencia al 10%
- 🚫 Modo limitado al 5%: *"Batería en nivel crítico."*

**Watchdog del Sistema**:
- Monitorea continuamente el correcto funcionamiento
- Reinicio automático en caso de bloqueo
- Registro de errores para diagnóstico

---

## 💡 SISTEMA DE ILUMINACIÓN

### 9.1 LEDs Frontales (28 unidades WS2812B)

**Modos disponibles**:

```
┌──────────────┬─────────────────────────────────────┐
│ MODO         │ DESCRIPCIÓN                         │
├──────────────┼─────────────────────────────────────┤
│ OFF          │ Apagado completo                    │
├──────────────┼─────────────────────────────────────┤
│ LOW_BEAM     │ Luz baja (blanco suave)            │
│              │ Para uso normal                     │
├──────────────┼─────────────────────────────────────┤
│ HIGH_BEAM    │ Luz alta (blanco intenso)          │
│              │ Máxima iluminación                  │
├──────────────┼─────────────────────────────────────┤
│ DRL          │ Luces diurnas (blanco intermedio)  │
│              │ Activación automática al encender   │
├──────────────┼─────────────────────────────────────┤
│ HAZARD       │ Emergencia (naranja intermitente)  │
│              │ Parpadeo rápido en todas las luces │
└──────────────┴─────────────────────────────────────┘
```

**Activación**:
- Tocar botón 💡 en dashboard
- Cambiar entre modos con toques sucesivos
- Audio de confirmación en cada cambio

### 9.2 LEDs Traseros (16 unidades WS2812B)

**Modos automáticos**:

```
┌──────────────┬─────────────────────────────────────┐
│ SITUACIÓN    │ COMPORTAMIENTO                      │
├──────────────┼─────────────────────────────────────┤
│ Marcha atrás │ Blanco intenso continuo            │
│ (REVERSE)    │ Indica retroceso                    │
├──────────────┼─────────────────────────────────────┤
│ Frenado      │ Rojo intenso                       │
│              │ Se activa al soltar acelerador     │
├──────────────┼─────────────────────────────────────┤
│ Circulación  │ Rojo suave (luces de posición)     │
│ normal       │                                     │
├──────────────┼─────────────────────────────────────┤
│ PARK         │ Rojo intermitente lento            │
│              │ Indica vehículo estacionado        │
└──────────────┴─────────────────────────────────────┘
```

### 9.3 Efectos Especiales

**Secuencia de Bienvenida**:
- Al encender, las luces realizan una animación de inicio
- Barrido de colores de delante hacia atrás
- Duración: 2 segundos

**Modo Emergencia**:
1. Mantener presionado botón 💡 por 3 segundos
2. Todas las luces parpadearán en naranja
3. Audio: *"Modo de emergencia activado."*
4. Para desactivar: tocar botón 💡 nuevamente

---

## 🔊 SISTEMA DE AUDIO

### 10.1 Características de Audio

- **Módulo**: DFPlayer Mini
- **Salida**: Altavoz 3W
- **Volumen**: Ajustable en 30 niveles
- **Formatos**: MP3
- **Almacenamiento**: Tarjeta microSD (hasta 32GB)

### 10.2 Audios del Sistema

El sistema incluye 68 audios diferentes para distintas situaciones:

**Audios Principales**:
- 🔊 Bienvenida al encender
- 🔊 Confirmación de cambio de marcha
- 🔊 Alertas de seguridad (temperatura, batería)
- 🔊 Confirmación de calibraciones
- 🔊 Activación de sistemas (ABS, TCS, luces)
- 🔊 Despedida al apagar

**Grabación de Audios**:
Ver el archivo `docs/AUDIO_TRACKS_GUIDE.md` para:
- Lista completa de 68 audios
- Textos exactos para grabar
- Instrucciones paso a paso
- Nombres de archivo requeridos

**Pasos rápidos**:
1. Ir a [TTSMaker.com](https://ttsmaker.com/)
2. Seleccionar idioma: Español (España)
3. Copiar texto del AUDIO_TRACKS_GUIDE.md
4. Descargar MP3 y renombrar (ej: 0001.mp3)
5. Copiar todos los archivos a la raíz de la tarjeta SD

### 10.3 Control de Volumen

**Ajustar volumen**:
1. Tocar botón 🔊 en dashboard
2. Aparecerá control deslizante
3. Deslizar para ajustar (0-30)
4. Tocar fuera para cerrar

**Silenciar**:
- Tocar 🔊 y mover a nivel 0
- O apagar desde Menú → Audio → Mute

---

## ⚙️ CALIBRACIONES

### 11.1 ¿Cuándo calibrar?

Calibrar cuando:
- ✅ Primer uso del vehículo
- ✅ La pantalla táctil no responde bien
- ✅ El pedal no responde correctamente
- ✅ La dirección no está centrada
- ✅ Después de reemplazar componentes

### 11.2 Calibración de Pantalla Táctil

**Acceso al menú de calibración**:

**Opción A: Botón físico** (recomendado si touch no funciona)
1. Mantener presionado botón físico **5 segundos**
2. Aparecerá pantalla de calibración automáticamente

**Opción B: Menú táctil**
1. Tocar **icono de batería** 5 veces rápidamente
2. Acceder a menú oculto
3. Seleccionar "Calibrar Touch"

**Proceso de calibración**:
```
┌─────────────────────────────────────────────┐
│  CALIBRACIÓN TOUCH - Siga las instrucciones│
├─────────────────────────────────────────────┤
│                                             │
│  Paso 1/4:                                  │
│  Toque la esquina superior izquierda        │
│     ╭───────────────────────────────╮       │
│     │ ✖                             │       │
│     │                               │       │
│     │                               │       │
│     │                               │       │
│     │                               │       │
│     ╰───────────────────────────────╯       │
│                                             │
│  [Siguiente: Superior derecha →]            │
└─────────────────────────────────────────────┘
```

1. Tocar esquina **superior izquierda** cuando se indique
2. Tocar esquina **superior derecha**
3. Tocar esquina **inferior derecha**
4. Tocar esquina **inferior izquierda**
5. Tocar el **centro** de la pantalla para verificar
6. Si es correcto, tocar "Guardar". Si no, "Reintentar"

### 11.3 Calibración del Pedal

**Acceso**:
1. Menú oculto → "1) Calibrar pedal"
2. O Menú ⚙️ → Calibración → Pedal

**Proceso**:
```
Paso 1: Soltar el pedal completamente
        │
        ├─→ Sistema detecta valor mínimo (0%)
        │
Paso 2: Presionar suavemente hasta el fondo
        │
        ├─→ Sistema detecta valor máximo (100%)
        │
Paso 3: Verificar respuesta
        │
        └─→ "Calibración del pedal completada."
```

**Verificación**:
- La barra de pedal debe mostrar 0% cuando está suelto
- Debe mostrar 100% cuando está presionado a fondo
- La respuesta debe ser suave y lineal

### 11.4 Calibración del Encoder (Dirección)

**Acceso**:
1. Menú oculto → "2) Calibrar encoder"
2. O Menú ⚙️ → Calibración → Dirección

**Proceso**:
```
Paso 1: Centrar el volante físicamente
        │
        ├─→ Asegurarse de que las ruedas apuntan al frente
        │
Paso 2: Presionar "Calibrar centro"
        │
        ├─→ Sistema registra posición central (0°)
        │
Paso 3: Girar a la izquierda al máximo
        │
        ├─→ Sistema detecta límite izquierdo
        │
Paso 4: Girar a la derecha al máximo
        │
        ├─→ Sistema detecta límite derecho
        │
Paso 5: Volver al centro
        │
        └─→ "Encoder sincronizado correctamente."
```

**Verificación**:
- El indicador de dirección debe mostrar 0° con volante centrado
- Ángulos máximos típicos: ±540° (1.5 vueltas a cada lado)
- La respuesta debe ser inmediata sin lag

### 11.5 Calibración de Sensores de Corriente (INA226)

**Acceso**:
1. Menú oculto → "3) Calibrar INA"
2. O Menú ⚙️ → Calibración → Sensores de Corriente

**Pre-requisitos**:
- ⚠️ Motores desconectados o sin carga
- ⚠️ Vehículo en PARK

**Proceso automático**:
```
Sistema calibra:
│
├─→ Offset de cada INA226 (corriente = 0A)
├─→ Shunt resistance verificación
├─→ Bus voltage calibration
└─→ "Calibración de sensores de corriente finalizada."
```

---

## 🔧 MANTENIMIENTO

### 12.1 Mantenimiento Periódico

**Cada uso**:
- ✅ Inspección visual de conexiones
- ✅ Verificar nivel de batería antes de usar
- ✅ Comprobar presión de neumáticos
- ✅ Limpiar pantalla con paño suave

**Semanal** (uso intensivo):
- ✅ Limpiar sensores de rueda
- ✅ Verificar tensión de correas/cadenas
- ✅ Comprobar fijación de componentes
- ✅ Limpiar conectores

**Mensual**:
- ✅ Lubricar motor de dirección
- ✅ Verificar desgaste de neumáticos
- ✅ Comprobar torque de tornillos
- ✅ Limpiar ventilación de motores
- ✅ Actualizar firmware si hay nuevas versiones

**Trimestral**:
- ✅ Reemplazar grasa de rodamientos
- ✅ Verificar calibración completa
- ✅ Prueba de todos los sistemas de seguridad
- ✅ Inspección de cables y conectores

### 12.2 Cuidado de la Batería

**Para maximizar vida útil**:
- 🔋 No descargar por debajo del 20%
- 🔋 Cargar después de cada uso
- 🔋 Almacenar a 50-70% de carga
- 🔋 No exponer a temperaturas extremas (<0°C o >40°C)
- 🔋 Desconectar si no se usa por >1 semana

**Carga**:
- ⚡ Usar solo cargador especificado (24V, 2A-5A)
- ⚡ Cargar en lugar ventilado
- ⚡ No dejar cargando más de 8 horas
- ⚡ Desconectar cuando LED indica carga completa

### 12.3 Actualización de Firmware

**Verificar versión actual**:
- Menú ⚙️ → Acerca de → Versión
- Versión actual debería mostrar: v2.17.1 o superior

**Actualizar firmware**:
1. Descargar última versión desde GitHub
2. Conectar ESP32 al PC vía USB
3. Usar PlatformIO:
   ```bash
   pio run -e esp32-s3-n16r8-release -t upload
   ```
4. Esperar a que termine (2-3 minutos)
5. Desconectar y reiniciar vehículo

**⚠️ Importante**: 
- No interrumpir durante actualización
- Batería debe estar >50%
- Hacer respaldo de calibraciones

---

## 🆘 SOLUCIÓN DE PROBLEMAS

### 13.1 Problemas Comunes

#### ❌ El vehículo no enciende

**Posibles causas y soluciones**:
1. **Batería descargada**:
   - Verificar voltaje (debe ser >22V)
   - Cargar batería completamente
   
2. **Conexión suelta**:
   - Revisar terminales de batería
   - Verificar fusibles principales
   
3. **Fusible quemado**:
   - Localizar fusible principal
   - Reemplazar por uno del mismo amperaje

#### ❌ La pantalla no responde al touch

**Soluciones**:
1. **Calibrar con botón físico**:
   - Mantener botón 5 segundos
   - Seguir proceso de calibración

2. **Verificar conexión touch**:
   - Revisar cable flat del touch (XPT2046)
   - Reconectar si es necesario

3. **Reset de fábrica**:
   - Menú → Configuración → Reset
   - Recalibrar todo

#### ❌ Motor no responde o tiene poca potencia

**Diagnóstico**:
1. **Verificar temperatura**:
   - Ver dashboard, temperatura de motores
   - Si >60°C, dejar enfriar
   
2. **Batería baja**:
   - Verificar voltaje batería
   - Cargar si <23V

3. **Conexión del motor**:
   - Verificar conectores del driver BTS7960
   - Comprobar cables PWM

4. **Calibrar sensores INA226**:
   - Menú oculto → Calibrar INA
   - Verificar lecturas de corriente

#### ❌ La dirección no responde

**Pasos**:
1. **Verificar posición actual**:
   - Dashboard muestra ángulo de dirección
   - Debe responder al girar volante

2. **Recalibrar encoder**:
   - Menú → Calibrar encoder
   - Centrar volante primero

3. **Verificar conexión encoder**:
   - Cable del encoder E6B2-CWZ6C
   - Optoacopladores HY-M158

#### ❌ Audio no funciona

**Verificar**:
1. **Tarjeta SD**:
   - Insertada correctamente
   - Formato FAT32
   - Archivos .mp3 en raíz

2. **Volumen**:
   - Tocar 🔊 y verificar nivel
   - Debe estar >0

3. **DFPlayer Mini**:
   - LED del módulo debe parpadear
   - Verificar conexión UART (GPIO 43/44)

#### ❌ LEDs no encienden

**Verificar**:
1. **Botón de luces**:
   - Tocar 💡 en dashboard
   - Cambiar modo de iluminación

2. **Conexión LEDs**:
   - GPIO 1 (frontales), GPIO 48 (traseros)
   - Cable de datos debe estar bien conectado

3. **Alimentación**:
   - LEDs necesitan 5V
   - Verificar fuente de 5V

### 13.2 Códigos de Error

El sistema puede mostrar códigos de error en la pantalla:

| Código | Significado | Solución |
|--------|-------------|----------|
| **E001** | Error I2C general | Verificar conexiones I2C (GPIO 8/9) |
| **E002** | INA226 no responde | Calibrar sensores de corriente |
| **E003** | PCA9685 no responde | Verificar drivers PWM I2C |
| **E004** | Sensor temperatura fallo | Comprobar DS18B20 (GPIO 20) |
| **E005** | Encoder desconectado | Verificar cable encoder |
| **E006** | Touch no responde | Calibrar pantalla táctil |
| **E007** | DFPlayer error | Verificar tarjeta SD y módulo |
| **E008** | Batería crítica | Cargar inmediatamente |
| **E009** | Temperatura crítica | Apagar y dejar enfriar |
| **E010** | Watchdog reset | Reinicio automático por bloqueo |

**Documentación completa**: Ver `docs/CODIGOS_ERROR.md`

### 13.3 Reset de Fábrica

Si los problemas persisten, realizar reset completo:

**⚠️ ADVERTENCIA**: Esto borrará todas las calibraciones.

**Proceso**:
1. Acceder a: Menú ⚙️ → Configuración → Avanzado
2. Seleccionar "Reset de Fábrica"
3. Confirmar acción
4. El sistema se reiniciará
5. Recalibrar todo (touch, pedal, encoder, INA226)

**Backup antes de reset**:
- Anotar configuraciones personalizadas
- Tomar fotos de calibraciones
- Guardar valores de sensores

---

## 📊 ESPECIFICACIONES TÉCNICAS

### 14.1 Especificaciones Eléctricas

```
┌────────────────────────────────────────────────┐
│ SISTEMA ELÉCTRICO                              │
├────────────────────────────────────────────────┤
│ Voltaje nominal:           24V DC              │
│ Rango de operación:        22V - 26V           │
│ Batería recomendada:       24V 7-12Ah LiPo/    │
│                            Lead-acid           │
│ Consumo en reposo:         <500mA              │
│ Consumo típico:            2-5A                │
│ Consumo máximo:            50A (4 motores)     │
│ Protección:                Fusible 60A         │
└────────────────────────────────────────────────┘
```

### 14.2 Especificaciones de Motores

**Motores de Tracción (4x)**:
- Modelo: RS775
- Voltaje: 24V
- RPM: 15,000 @ 24V
- Corriente máx: 50A por motor
- Potencia: 180W por motor
- Reductora: 1:75

**Motor de Dirección (1x)**:
- Modelo: RS390
- Voltaje: 12V
- RPM: 6,000 @ 12V
- Corriente máx: 5A
- Potencia: 60W
- Reductora: 1:50

### 14.3 Controlador Principal

```
┌────────────────────────────────────────────────┐
│ ESP32-S3-WROOM-2 N16R8                         │
├────────────────────────────────────────────────┤
│ CPU:              Dual-core Xtensa LX7 240MHz  │
│ Flash:            16 MB (QIO mode @ 80MHz)     │
│ PSRAM:            8 MB (QSPI mode @ 80MHz)     │
│ GPIO disponibles: 36 pines                     │
│ I2C:              1x bus (GPIO 8/9)            │
│ SPI:              2x buses                     │
│ UART:             2x puertos                   │
│ PWM:              16 canales LEDC              │
│ ADC:              2x 12-bit                    │
│ Temperatura:      -40°C a +85°C               │
└────────────────────────────────────────────────┘
```

### 14.4 Sensores

**Corriente (6x INA226)**:
- Resolución: 1.25mA
- Precisión: ±0.1%
- Rango: 0-100A
- Actualización: 100Hz

**Temperatura (4x DS18B20)**:
- Resolución: 0.0625°C
- Precisión: ±0.5°C
- Rango: -55°C a +125°C
- Actualización: 1Hz

**Velocidad (4x LJ12A3-4-Z/BX)**:
- Tipo: Inductivo NPN
- Distancia detección: 4mm
- Voltaje: 6-36V DC
- Frecuencia máx: 1kHz

**Encoder (E6B2-CWZ6C)**:
- Resolución: 1200 PPR
- Tipo: Incremental, cuadratura
- Voltaje: 5-24V DC
- Salidas: A, B, Z

### 14.5 Pantalla

```
┌────────────────────────────────────────────────┐
│ ST7796S TFT + XPT2046 Touch                    │
├────────────────────────────────────────────────┤
│ Resolución:       480 x 320 píxeles            │
│ Tamaño:           3.5 pulgadas                 │
│ Colores:          262K (18-bit RGB)            │
│ Touch:            Resistivo 4 hilos            │
│ Backlight:        LED PWM controlado           │
│ Interfaz:         SPI (40 MHz)                 │
│ Consumo:          120mA @ max brillo           │
└────────────────────────────────────────────────┘
```

### 14.6 Iluminación

**LEDs Frontales**:
- Tipo: WS2812B (RGB direccionables)
- Cantidad: 28 LEDs
- Voltaje: 5V
- Corriente: 60mA por LED @ máximo brillo
- Control: GPIO 1

**LEDs Traseros**:
- Tipo: WS2812B (RGB direccionables)
- Cantidad: 16 LEDs
- Voltaje: 5V
- Corriente: 60mA por LED @ máximo brillo
- Control: GPIO 48

### 14.7 Dimensiones y Peso (Aproximados)

- **Largo**: 120 cm
- **Ancho**: 65 cm
- **Alto**: 55 cm
- **Distancia entre ejes**: 75 cm
- **Peso sin batería**: ~15 kg
- **Peso con batería**: ~18 kg
- **Capacidad de carga**: 30 kg máximo

### 14.8 Rendimiento

- **Velocidad máxima**: 8 km/h (configurable, limitado por software)
- **Autonomía**: 2-4 horas (depende de terreno y peso)
- **Tiempo de carga**: 4-6 horas (cargador 2A)
- **Pendiente máxima**: 10° (15% de inclinación)
- **Radio de giro**: 2.5 metros

---

## 📞 SOPORTE Y RECURSOS

### 15.1 Documentación Adicional

El proyecto incluye documentación técnica extensa en el directorio `docs/`:

**Guías de Usuario**:
- `GUIA_RAPIDA.md` - Guía rápida de inicio
- `docs/TOUCH_CALIBRATION_QUICK_GUIDE.md` - Calibración del touch
- `docs/AUDIO_TRACKS_GUIDE.md` - Guía completa de audios (68 tracks)
- `docs/CALIBRACION_TOUCH_SIN_PANTALLA.md` - Calibración sin touch funcional

**Documentación Técnica**:
- `HARDWARE.md` - Especificación oficial de hardware N16R8
- `docs/REFERENCIA_HARDWARE.md` - Referencia completa de hardware
- `docs/SISTEMAS_SEGURIDAD_AVANZADOS.md` - Detalles de ABS, TCS, Regen
- `docs/PIN_MAPPING_DEVKITC1.md` - Mapeo de pines GPIO
- `docs/CONEXIONES_HARDWARE_v2.15.0.md` - Conexiones detalladas

**Solución de Problemas**:
- `docs/CODIGOS_ERROR.md` - Lista completa de códigos de error
- `docs/TOUCH_TROUBLESHOOTING.md` - Solución de problemas touch
- `CHECKLIST.md` - Checklist de verificación del sistema

### 15.2 Repositorio GitHub

- **URL**: [https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos](https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos)
- **Issues**: Reportar problemas y sugerencias
- **Releases**: Nuevas versiones de firmware
- **Actions**: Builds automatizados

### 15.3 Información de Versión

- **Firmware**: v2.17.1 (PHASE 14)
- **Fecha**: Enero 2026
- **Estado**: Producción, 100% operativo
- **Hardware**: ESP32-S3 N16R8 (16MB Flash + 8MB PSRAM)

---

## ✅ CHECKLIST DE VERIFICACIÓN PRE-USO

Antes de cada uso, verificar:

**Sistema Eléctrico**:
- [ ] Nivel de batería > 30%
- [ ] Voltaje batería entre 22-26V
- [ ] Todas las conexiones firmes
- [ ] No hay cables sueltos o dañados

**Pantalla y Controles**:
- [ ] Pantalla enciende correctamente
- [ ] Touch responde (tocar dashboard)
- [ ] Audio funciona (mensaje de bienvenida)
- [ ] Todos los botones responden

**Mecánico**:
- [ ] Ruedas giran libremente
- [ ] Dirección responde suavemente
- [ ] Freno de parking funciona
- [ ] No hay ruidos anormales

**Sensores**:
- [ ] Temperatura de motores <40°C
- [ ] Sensores de corriente leen correctamente
- [ ] Encoder de dirección responde

**Iluminación**:
- [ ] Luces frontales encienden
- [ ] Luces traseras encienden
- [ ] Cambios de modo luz funcionan

**Sistema**:
- [ ] No hay códigos de error en pantalla
- [ ] Todos los sistemas inicializados OK
- [ ] Watchdog funcionando

---

## 📝 REGISTRO DE USO

Se recomienda llevar un registro de uso para seguimiento:

```
┌─────────────────────────────────────────────────┐
│ REGISTRO DE USO                                 │
├─────────────────────────────────────────────────┤
│ Fecha: ___/___/___                              │
│                                                 │
│ Hora inicio: _____  Hora fin: _____             │
│                                                 │
│ Batería inicial: ____V  Batería final: ____V    │
│                                                 │
│ Temp. inicial: ____°C  Temp. final: ____°C      │
│                                                 │
│ Distancia recorrida: _____ km                   │
│                                                 │
│ Tiempo de uso: _____ minutos                    │
│                                                 │
│ Modos usados: [ ] DRIVE  [ ] REVERSE            │
│               [ ] 4x4    [ ] 4x2                │
│                                                 │
│ Sistemas activos: [ ] ABS  [ ] TCS  [ ] Regen   │
│                                                 │
│ Incidencias: _________________________________  │
│ _____________________________________________   │
│ _____________________________________________   │
│                                                 │
│ Mantenimiento realizado:                        │
│ [ ] Limpieza  [ ] Calibración  [ ] Otros        │
└─────────────────────────────────────────────────┘
```

---

## ⚖️ GARANTÍA Y RESPONSABILIDADES

Este es un proyecto de código abierto. El firmware se proporciona "tal cual" sin garantías de ningún tipo.

### Responsabilidad del Usuario

**El usuario es responsable de**:
- ✅ Verificar que todos los componentes estén correctamente instalados
- ✅ Supervisar el uso del vehículo en todo momento
- ✅ Realizar mantenimiento periódico según especificaciones
- ✅ No modificar el firmware sin conocimientos técnicos
- ✅ Cumplir con las normas de seguridad

### Limitación de Responsabilidad

**No nos hacemos responsables de**:
- ❌ Daños por uso inadecuado o negligente
- ❌ Lesiones por falta de supervisión adulta
- ❌ Modificaciones no autorizadas del hardware/software
- ❌ Uso en condiciones no especificadas en este manual
- ❌ Daños causados por no seguir las instrucciones
- ❌ Problemas derivados de componentes de terceros

### Uso Seguro

**Para un uso seguro**:
1. Leer completamente este manual antes del primer uso
2. Seguir todas las advertencias de seguridad
3. Realizar todas las calibraciones necesarias
4. Mantener el vehículo en buen estado
5. Supervisar siempre a los niños durante el uso

---

## 🎓 GLOSARIO DE TÉRMINOS

| Término | Significado |
|---------|-------------|
| **ABS** | Anti-lock Braking System (Sistema Antibloqueo de Frenos) |
| **TCS** | Traction Control System (Sistema de Control de Tracción) |
| **HUD** | Head-Up Display (Pantalla de Información Principal) |
| **PWM** | Pulse Width Modulation (Modulación por Ancho de Pulso) |
| **I2C** | Inter-Integrated Circuit (Bus de comunicación digital) |
| **SPI** | Serial Peripheral Interface (Interfaz Periférica Serial) |
| **GPIO** | General Purpose Input/Output (Entrada/Salida de Propósito General) |
| **Encoder** | Sensor de posición rotacional de alta precisión |
| **Dashboard** | Pantalla principal con información del vehículo |
| **Firmware** | Software embebido en el microcontrolador |
| **PPR** | Pulses Per Revolution (Pulsos por Revolución) |
| **PSRAM** | Pseudo Static RAM (Memoria RAM externa adicional) |
| **LED** | Light Emitting Diode (Diodo Emisor de Luz) |
| **TFT** | Thin Film Transistor (Pantalla de cristal líquido) |
| **Watchdog** | Sistema de vigilancia que reinicia en caso de bloqueo |
| **Regenerativo** | Recuperación de energía durante el frenado |
| **Shunt** | Resistencia de precisión para medir corriente |
| **Optoacoplador** | Dispositivo de aislamiento eléctrico |
| **Multiplexor** | Dispositivo que permite compartir un bus entre múltiples componentes |
| **Touch** | Pantalla táctil |
| **Bootloop** | Bucle de reinicios continuos |

---

## 📖 ANEXOS

### Anexo A: Diagrama de Conexiones I2C

```
Bus I2C (GPIO 8 SDA, GPIO 9 SCL)
│
├─→ TCA9548A (0x70) Multiplexor I2C
│   │
│   ├─→ Canal 0: INA226 (0x40) Motor FL
│   ├─→ Canal 1: INA226 (0x40) Motor FR
│   ├─→ Canal 2: INA226 (0x40) Motor RL
│   ├─→ Canal 3: INA226 (0x40) Motor RR
│   ├─→ Canal 4: INA226 (0x40) Batería
│   └─→ Canal 5: INA226 (0x40) Motor Dirección
│
├─→ PCA9685 (0x40) PWM Motores Delanteros
├─→ PCA9685 (0x41) PWM Motores Traseros
├─→ PCA9685 (0x42) PWM Motor Dirección
└─→ MCP23017 (0x20) Expansor GPIO
```

### Anexo B: Tabla de GPIOs Utilizados

Ver archivo `docs/PIN_MAPPING_DEVKITC1.md` para el mapeo completo y detallado.

**GPIOs Principales**:
- GPIO 1: LEDs frontales WS2812B (28 LEDs)
- GPIO 48: LEDs traseros WS2812B (16 LEDs)
- GPIO 8/9: Bus I2C (SDA/SCL)
- GPIO 10-16: Bus SPI pantalla (SCK, MOSI, MISO, DC, RST, CS)
- GPIO 21: Touch CS (XPT2046)
- GPIO 42: Backlight PWM
- GPIO 43/44: UART Audio (DFPlayer)
- GPIO 4-7: Relés de potencia

### Anexo C: Archivos de Audio Requeridos

Ver `docs/AUDIO_TRACKS_GUIDE.md` para la lista completa de 68 archivos MP3.

**Audios esenciales mínimos**:
- 0001.mp3 - Bienvenida
- 0002.mp3 - Apagado
- 0012.mp3 - Batería baja
- 0013.mp3 - Batería crítica
- 0020.mp3 - Marcha D1
- 0022.mp3 - Marcha atrás
- 0024.mp3 - Park
- 0039.mp3 - ABS activado
- 0041.mp3 - TCS activado

---

## 🏁 CONCLUSIÓN

Este manual cubre todas las funcionalidades del vehículo eléctrico inteligente basado en ESP32-S3. 

### Características Principales Resumidas

✅ **Sistema de Control Avanzado**: ESP32-S3 con 16MB Flash y 8MB PSRAM  
✅ **Interfaz Táctil**: Pantalla 480x320 con dashboard en tiempo real  
✅ **Tracción 4x4**: Control independiente de 4 motores  
✅ **Seguridad**: ABS, TCS y frenado regenerativo con IA  
✅ **Iluminación**: 44 LEDs RGB programables  
✅ **Monitorización**: Sensores de corriente, temperatura y velocidad  
✅ **Audio**: 68 mensajes de voz en español  

### Próximos Pasos

1. **Leer completamente este manual** antes del primer uso
2. **Verificar todas las conexiones** según el diagrama
3. **Realizar calibraciones iniciales** (touch, pedal, encoder)
4. **Crear archivos de audio** siguiendo la guía
5. **Hacer pruebas en superficie plana** antes de uso normal
6. **Mantener registro de uso** para seguimiento

### Soporte

Para más información técnica, actualizaciones y soporte:
- **Documentación completa**: Directorio `docs/` del repositorio
- **GitHub**: [https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos](https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos)
- **Issues**: Reportar problemas en GitHub Issues

---

**¡Disfruta de tu vehículo eléctrico inteligente de forma segura y responsable!** 🚗💨

---

**Manual creado por**: Equipo de desarrollo FULL-FIRMWARE-Coche-Marcos  
**Última actualización**: Enero 2026  
**Versión del manual**: 1.0  
**Compatible con firmware**: v2.17.1 y superiores  
**Licencia**: Open Source (ver LICENSE en repositorio)

---

## Nota Final

Este manual ha sido diseñado para ser completo y detallado, cubriendo desde el montaje inicial hasta el mantenimiento avanzado. Se recomienda:

- 📖 Leer el manual completo antes del primer uso
- 🔖 Consultar las secciones específicas según necesidad
- 📝 Mantener un registro de uso y mantenimiento
- 🔄 Revisar periódicamente por actualizaciones
- 🆘 Consultar la sección de solución de problemas ante cualquier incidencia

**¡Buen viaje y conducción segura!** 🎉
