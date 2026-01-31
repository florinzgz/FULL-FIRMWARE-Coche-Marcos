# ARCHITECTURE_MIGRATION.md

**Proyecto:** FULL-FIRMWARE-Coche-Marcos  
**Versión del Documento:** 1.0  
**Fecha:** 2026-01-29  
**Estado:** ✅ OFICIAL - Referencia Técnica Definitiva  
**Autor:** Arquitectura Principal de Firmware Embebido

---

## 📄 RESUMEN EJECUTIVO

### Visión General

Este documento define la arquitectura de migración de un sistema monolítico basado en ESP32-S3 hacia una arquitectura distribuida dual-MCU que incorpora un STM32G474RE como controlador de seguridad dedicado. La migración está diseñada para mejorar el determinismo del sistema, la seguridad funcional y la escalabilidad del firmware del vehículo eléctrico, manteniendo la compatibilidad total con el firmware existente durante todas las fases.

### Arquitectura Objetivo: División de Responsabilidades

**ESP32-S3 N16R8 (Subsistema de Interfaz Humano-Máquina):**
- Gestión completa de la interfaz de usuario: Display TFT 480×320, touchscreen capacitivo XPT2046, reproducción de audio mediante DFPlayer Mini, y control de 44 LEDs WS2812B para realimentación visual
- Funciones de supervisión del sistema: detección de obstáculos mediante sensores ultrasónicos, diagnóstico en tiempo real, gestión de configuración persistente en memoria NVS
- Comunicación inalámbrica: capacidad WiFi/BLE (reservada para fases futuras)
- Rol en la arquitectura: **Periférico de visualización y configuración**, sin autoridad sobre funciones críticas de seguridad

**STM32G474RE (Subsistema de Control y Seguridad):**
- Control en tiempo real estricto de cinco motores: cuatro motores de tracción independientes con control FOC (Field-Oriented Control) y un motor de dirección con retroalimentación encoder (360 PPR)
- Adquisición continua de sensores críticos: cuatro sensores de velocidad de ruedas, cuatro canales de medición de corriente mediante INA226, cuatro sensores de temperatura DS18B20, sensor analógico de pedal Hall, selector de marcha (F/N/R)
- Sistemas de seguridad activa: ABS (Anti-lock Braking System) con modulación individual de ruedas, TCS (Traction Control System) con limitación de deslizamiento
- Protecciones de hardware: gestión de tres relés de potencia, límites de sobrecorriente, límites de sobretemperatura, watchdog independiente
- Rol en la arquitectura: **Controlador de seguridad con autoridad final**, garantiza operación determinista y failsafe

### Protocolo de Comunicación

La comunicación entre ambos microcontroladores se realiza mediante bus CAN (Controller Area Network) a 500 kbps, utilizando transceptores TJA1051T/3. El protocolo implementa:

- **Heartbeat bidireccional:** Mensajes periódicos de supervisión (10 Hz) que garantizan la operatividad de ambos controladores, con timeouts de 500 ms que disparan estados de seguridad
- **Arquitectura comando-telemetría:** El ESP32 solicita acciones mediante comandos CAN; el STM32 valida, ejecuta y reporta el estado real mediante telemetría periódica
- **Sistema ACK con códigos de rechazo:** Cada comando recibe confirmación explícita, incluyendo razones detalladas de rechazo (fuera de rango, estado no listo, condiciones de seguridad, relés inactivos)
- **Priorización de mensajes:** Heartbeats y alertas de seguridad utilizan IDs de alta prioridad CAN para garantizar latencia mínima

### Estrategia de Migración en Cinco Fases (0-4)

**FASE 0 - Shadow Mode (Sin Cambios en Producción):**
El STM32 opera en paralelo al ESP32, recibiendo copia de todos los comandos y sensores, ejecutando toda la lógica de control pero **sin accionar hardware real**. Esta fase valida la correctitud funcional del firmware STM32 sin riesgo para el sistema en producción. El ESP32 mantiene control completo del vehículo.

**FASE 1 - Control Compartido de Motores:**
Transferencia gradual del control de motores al STM32, comenzando por los motores de tracción, seguido por el motor de dirección. El ESP32 envía comandos de velocidad y ángulo; el STM32 ejecuta el control PWM, la retroalimentación PID y la adquisición de encoders. Ambos sistemas mantienen capacidad de emergency stop.

**FASE 2 - Transferencia de Sensores Críticos:**
Migración de la lectura de sensores de corriente (INA226), temperatura (DS18B20), y velocidad de ruedas del ESP32 al STM32. El STM32 pasa a ser la fuente autoritativa de telemetría crítica, garantizando lecturas deterministas sin interferencia del rendering de UI.

**FASE 3 - Sistemas de Seguridad Activa:**
Activación de los sistemas ABS y TCS en el STM32, que pasan a modular el control de motores en tiempo real sin intervención del ESP32. Implementación de límites de sobrecorriente y sobretemperatura con acción autónoma del STM32.

**FASE 4 - Arquitectura Completa con Failover:**
El STM32 opera de forma completamente autónoma, capaz de mantener el vehículo en estado seguro (SAFE_STOP) incluso ante fallo total del ESP32. El ESP32 queda reducido a rol de HMI pura, sin participación en decisiones críticas de seguridad.

### Garantías de Seguridad y No-Regresión

**Principio de Autoridad Única:** En toda situación de conflicto entre ESP32 y STM32, el STM32 tiene autoridad final. Puede rechazar comandos, aplicar limitaciones de seguridad, o forzar SAFE_STOP sin consultar al ESP32.

**Validación Multi-Nivel:** Cada comando recibido por el STM32 atraviesa cuatro capas de validación: (1) rango de valores aceptables, (2) compatibilidad con estado actual del sistema, (3) verificación de condiciones de seguridad (corriente, temperatura), (4) confirmación de prerrequisitos de hardware (relés activos). Cualquier fallo rechaza el comando con código de error específico.

**Dead Man Switch:** Timeout de heartbeat de 500 ms activa automáticamente el estado SAFE_STOP en el STM32, deteniendo motores, desconectando relés de tracción y manteniendo dirección en posición segura. No requiere intervención del ESP32.

**Rollback Garantizado:** Cada fase de migración incluye capacidad de rollback al comportamiento de fase anterior mediante flags de configuración, sin necesidad de recompilar firmware. Permite revertir instantáneamente ante cualquier regresión funcional.

**Trazabilidad Completa:** Todos los eventos de seguridad (SAFE_STOP, rechazos de comandos, timeouts de heartbeat, alertas de sobrecorriente/temperatura) quedan registrados con timestamp en logs persistentes tanto en STM32 como en ESP32, permitiendo análisis forense post-incidente.

---

## 📋 TABLA DE CONTENIDOS

1. [Contexto del Sistema](#1-contexto-del-sistema)
2. [Objetivo del Documento](#2-objetivo-del-documento)
3. [Principios Arquitectónicos Obligatorios](#3-principios-arquitectónicos-obligatorios)
4. [Arquitectura Objetivo](#4-arquitectura-objetivo)
5. [Estrategia de Migración por Fases](#5-estrategia-de-migración-por-fases)
6. [Mapeo de Módulos Existentes](#6-mapeo-de-módulos-existentes)
7. [Protocolo de Comunicación CAN](#7-protocolo-de-comunicación-can)
8. [Reglas de Autoridad](#8-reglas-de-autoridad)
9. [Seguridad Funcional](#9-seguridad-funcional)
10. [Gestión de Configuración](#10-gestión-de-configuración)
11. [Matriz de Gestión de Fallos](#11-matriz-de-gestión-de-fallos)
12. [Criterios de Validación](#12-criterios-de-validación)
13. [Riesgos y Mitigaciones](#13-riesgos-y-mitigaciones)
14. [Referencias](#14-referencias)

---

## 1. CONTEXTO DEL SISTEMA

### 1.1 Sistema Actual (Baseline)

El firmware actual es un **sistema monolítico** ejecutándose completamente en un ESP32-S3 N16R8:

```
┌─────────────────────────────────────────────────────────────────┐
│                     ESP32-S3 N16R8                              │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ HMI: Display TFT, Touch, Audio, LEDs, Menús             │   │
│  ├──────────────────────────────────────────────────────────┤   │
│  │ Control: Tracción (4 motores), Dirección, PWM, Relés    │   │
│  ├──────────────────────────────────────────────────────────┤   │
│  │ Sensores: INA226, DS18B20, Encoder, Ruedas, Pedal       │   │
│  ├──────────────────────────────────────────────────────────┤   │
│  │ Seguridad: ABS, TCS, Obstáculos, Watchdog               │   │
│  ├──────────────────────────────────────────────────────────┤   │
│  │ Configuración: NVS, Calibración, PID, Límites           │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

**Características del sistema actual:**
- ✅ Firmware **estable** y en **producción**
- ✅ Versión: v2.17.1 (PHASE 14)
- ✅ Hardware: ESP32-S3 N16R8 (16MB Flash QIO + 8MB PSRAM OPI @ 3.3V)
- ✅ Funcionalidades completas: HMI, control, sensores, seguridad
- ⚠️ **Carga CPU cercana al límite** (loop principal ~10-20ms)
- ⚠️ **Riesgo de latencias no deterministas** por múltiples responsabilidades

### 1.2 Sistema Objetivo (Arquitectura Distribuida)

La arquitectura objetivo separa claramente las responsabilidades en **dos microcontroladores** especializados:

```
┌───────────────────────────────────┐         ┌───────────────────────────────────┐
│       ESP32-S3 N16R8              │         │      STM32G474RE                  │
│          (HMI)                    │         │    (CONTROL SEGURO)               │
├───────────────────────────────────┤         ├───────────────────────────────────┤
│ • Display TFT ST7796S 480×320     │         │ • Control Motores (FOC, PWM)      │
│ • Touch XPT2046                   │         │ • 4× Tracción + 1× Dirección      │
│ • Audio DFPlayer Mini             │         │ • Encoder A/B/Z (360 PPR)         │
│ • LEDs WS2812B (44 LEDs)          │   CAN   │ • Sensores Ruedas ×4              │
│ • Menús y Diagnóstico             │ <────>  │ • Sensores Corriente (INA226)     │
│ • Detección Obstáculos (visual)   │  500    │ • Sensores Temperatura (DS18B20)  │
│ • Configuración (NVS)             │  kbps   │ • Pedal Hall (analógico)          │
│ • Supervisión del Sistema         │         │ • Shifter (F/N/R)                 │
│ • WiFi/BLE (futuro)               │         │ • Relés Potencia ×3               │
│                                   │         │ • ABS / TCS                       │
│ Periférico: TWAI                  │         │ • Seguridad Funcional             │
│ Transceptor: TJA1051T/3 #2        │         │ • Tiempo Real Estricto            │
└───────────────────────────────────┘         │                                   │
                                              │ Periférico: FDCAN1                │
                                              │ Transceptor: TJA1051T/3 #1        │
                                              └───────────────────────────────────┘
```

**Motivación de la separación:**

| Aspecto | Problema Actual | Solución con STM32 |
|---------|----------------|-------------------|
| **Determinismo** | Loop de control compite con rendering UI | STM32 garantiza tiempos de control predecibles |
| **Seguridad Funcional** | Falta separación entre HMI y control crítico | STM32 como controlador de seguridad independiente |
| **Escalabilidad** | Carga CPU cercana al límite | Distribución de carga entre dos MCUs |
| **Tiempo Real** | Control de motores sin garantías hard real-time | STM32 con RTOS y prioridades garantizadas |
| **Resiliencia** | Fallo del ESP32 afecta todo el sistema | Fallo del ESP32 solo afecta HMI, control se mantiene seguro |

### 1.3 Especificaciones de Hardware

#### ESP32-S3 N16R8 (HMI)

| Componente | Especificación |
|-----------|----------------|
| MCU | Dual-core Xtensa LX7 @ 240 MHz |
| Flash | 16 MB QIO @ 80 MHz, 3.3V |
| PSRAM | 8 MB OPI @ 80 MHz, 3.3V |
| Periférico CAN | TWAI (Two-Wire Automotive Interface) |
| Pines CAN | GPIO 20 (TX), GPIO 21 (RX) - propuestos |
| Transceptor | TJA1051T/3 #2 |
| Voltage | 3.3V |

**Documentación:** [HARDWARE.md](HARDWARE.md), [PHASE14_N16R8_BOOT_CERTIFICATION.md](PHASE14_N16R8_BOOT_CERTIFICATION.md)

#### STM32G474RE (Control Seguro)

| Componente | Especificación |
|-----------|----------------|
| MCU | ARM Cortex-M4F @ 170 MHz |
| Flash | 512 KB |
| SRAM | 128 KB |
| Periférico CAN | FDCAN1 (compatible con CAN 2.0 y CAN FD) |
| Pines CAN | PB8 (RX), PB9 (TX) |
| Transceptor | TJA1051T/3 #1 |
| Voltage | 3.3V |

**Documentación:** [docs/STM32G474RE_PINOUT_DEFINITIVO.md](docs/STM32G474RE_PINOUT_DEFINITIVO.md)

#### Bus CAN

| Parámetro | Especificación |
|-----------|----------------|
| Protocolo | CAN 2.0A/B (Classic CAN) |
| Velocidad | 500 kbps |
| Topología | Par trenzado punto a punto |
| Terminación | 120Ω en ambos extremos |
| Transceptores | TJA1051T/3 (×2) |
| Alimentación | 5V (lógica compatible 3.3V) |
| Temperatura | -40°C a +125°C |

**Documentación:** [docs/MANUAL_TRANSRECEPTORES_STM32_ESP32.md](docs/MANUAL_TRANSRECEPTORES_STM32_ESP32.md)

---

## 2. OBJETIVO DEL DOCUMENTO

Este documento define de forma **precisa y ejecutable** la migración arquitectónica del firmware desde un sistema monolítico hacia una arquitectura distribuida de dos microcontroladores.

### 2.1 Alcance

El documento establece:

✅ **Arquitectura final** ESP32-S3 (HMI) + STM32G474RE (Control)  
✅ **Reglas de autoridad** entre microcontroladores  
✅ **Mecanismos de seguridad funcional** (heartbeat, dead man switch, fail-safe)  
✅ **Estrategia de migración por fases** sin romper el firmware actual  
✅ **Clasificación de módulos** (permanecen, migran, proxies CAN)  
✅ **Protocolo de comunicación CAN** (IDs, formatos, prioridades)  
✅ **Criterios de validación** para cada fase  

### 2.2 Audiencia

Este documento está dirigido a:

- Ingenieros de firmware que implementarán la migración
- Ingenieros de hardware que validarán la integración
- Arquitectos de sistema que supervisarán la transición
- Ingenieros de pruebas que certificarán cada fase

### 2.3 Principio Fundamental

**PRIORIDAD ABSOLUTA:**

> Durante toda la migración, el firmware actual **DEBE permanecer estable y operativo**. Bajo ningún concepto se permitirá una regresión funcional o pérdida de comportamiento validado.

La migración se realiza por **fases incrementales**, cada una con:
- Objetivo técnico claro
- Criterios de validación específicos
- Posibilidad de rollback

---

## 3. PRINCIPIOS ARQUITECTÓNICOS OBLIGATORIOS

Estos principios son **OBLIGATORIOS** y NO NEGOCIABLES en toda la arquitectura y migración.

### 3.1 PRINCIPIO DE SEGREGACIÓN (Roles Claros)

**Definición:**

El STM32 es la **"Source of Truth"** del estado físico real del sistema. El ESP32 **NUNCA** decide sobre el estado físico directamente.

**Reglas:**

| Acción | ESP32 (HMI) | STM32 (Control) |
|--------|-------------|-----------------|
| **Solicitar cambio** | ✅ Envía comando CAN | ❌ No aplica |
| **Decidir si ejecutar** | ❌ NO decide | ✅ Decide y valida |
| **Ejecutar acción física** | ❌ NO ejecuta | ✅ Ejecuta |
| **Reportar estado real** | ❌ NO reporta | ✅ Reporta estado |
| **Visualizar estado** | ✅ Muestra estado recibido | ❌ No visualiza |

**Consecuencia:**

Ningún cambio visible en HMI (pantalla, LEDs) se refleja hasta recibir **confirmación explícita** desde el STM32 (ACK / Status).

**Ejemplo:**

```
Usuario presiona botón "Encender Tracción" en pantalla
    ↓
ESP32 envía: CAN ID 0x100: CMD_SET_RELAY_TRACTION, value=ON
    ↓
STM32 recibe comando
    ↓
STM32 valida condiciones de seguridad
    ↓
STM32 conmuta relé de tracción (o lo rechaza)
    ↓
STM32 envía: CAN ID 0x200: STATUS_RELAY_TRACTION, value=ON (o OFF si rechazó)
    ↓
ESP32 recibe estado confirmado
    ↓
ESP32 actualiza icono en pantalla según estado recibido
```

### 3.2 PRINCIPIO DE SEGURIDAD FUNCIONAL

**Definición:**

El sistema implementa mecanismos de seguridad que garantizan un **estado seguro** ante cualquier fallo de comunicación o lógica.

#### 3.2.1 Dead Man Switch (Heartbeat)

**Implementación:**

```
ESP32 → STM32: HEARTBEAT cada 100 ms (ID 0x010)
STM32 → ESP32: HEARTBEAT cada 100 ms (ID 0x011)
```

**Regla de Seguridad:**

```
SI (tiempo_desde_ultimo_heartbeat_ESP32 > 500 ms)
ENTONCES
    STM32 entra en SAFE_STOP:
        - Deshabilitar PWM motores
        - Activar fren regenerativo suave
        - Desconectar relés de tracción
        - Mantener dirección en posición actual
        - Activar LED de error
FIN SI
```

**Recuperación:**

```
SI (heartbeat_ESP32 se recupera)
Y (estado_sistema == SAFE_STOP)
ENTONCES
    Esperar confirmación explícita del usuario en HMI
    Requerir secuencia de reactivación
    Validar sensores antes de salir de SAFE_STOP
FIN SI
```

#### 3.2.2 Gestión de Transceptores TJA1051T/3

**Estados del Transceptor:**

| Modo | Pin STBY | Comportamiento | Uso |
|------|----------|----------------|-----|
| **Normal** | LOW | Transmisión y recepción activas | Operación normal |
| **Standby** | HIGH | Modo bajo consumo, bus desconectado | Ahorro energía |
| **Silent** | (configuración interna) | Solo escucha, no transmite | Monitoreo pasivo |

**Manejo de Bus-Off:**

```
SI (contador_errores_TX > 255)
ENTONCES
    Transceptor entra en estado BUS-OFF
    STM32 detecta BUS-OFF
    STM32 entra en SAFE_STOP
    STM32 intenta recuperación automática:
        - Esperar 128 ocurrencias de 11 bits recesivos
        - Reinicializar periférico FDCAN
        - Intentar reenvío de heartbeat
    SI (fallan 3 intentos de recuperación)
    ENTONCES
        Requerir reset de sistema
    FIN SI
FIN SI
```

#### 3.2.3 Estados Seguros ante Pérdida de Comunicación

**Matriz de Fallos:**

| Fallo | Acción STM32 | Acción ESP32 | Estado Sistema |
|-------|--------------|--------------|----------------|
| ESP32 sin heartbeat >500ms | SAFE_STOP | (caído) | Seguro, sin movimiento |
| STM32 sin heartbeat >500ms | (caído) | Mostrar error crítico | Seguro, sin movimiento |
| Bus CAN saturado | Priorizar mensajes SAFETY | Reducir telemetría | Degradado, control mantiene |
| Transceptor fallo | Intentar recuperación | Mostrar error HW | Seguro, sin movimiento |
| Mensaje CAN corrupto | Ignorar, incrementar contador | Ignorar, incrementar contador | Continúa si heartbeat OK |

### 3.3 PRINCIPIO DE GESTIÓN DE CONFIGURACIÓN

**Definición:**

La configuración del sistema (PID, límites, calibraciones) **reside permanentemente en NVS del ESP32**. El STM32 no persiste configuración crítica.

**Flujo de Configuración:**

```
Arranque del sistema:
    1. ESP32 inicia y lee NVS
    2. ESP32 valida configuración (checksums, rangos)
    3. ESP32 espera heartbeat de STM32
    4. ESP32 inyecta configuración al STM32 vía CAN:
        - Parámetros PID tracción
        - Parámetros PID dirección
        - Límites de corriente
        - Límites de temperatura
        - Calibraciones de sensores
        - Configuración ABS/TCS
    5. STM32 recibe y almacena en RAM
    6. STM32 confirma recepción (ACK)
    7. Sistema entra en modo READY
```

**Mensajes de Configuración:**

| ID CAN | Nombre | Contenido | Dirección |
|--------|--------|-----------|-----------|
| 0x300 | CFG_PID_TRACTION | Kp, Ki, Kd (3×float) | ESP32 → STM32 |
| 0x301 | CFG_PID_STEERING | Kp, Ki, Kd (3×float) | ESP32 → STM32 |
| 0x302 | CFG_CURRENT_LIMITS | Max_motor (4×uint16) | ESP32 → STM32 |
| 0x303 | CFG_TEMP_LIMITS | Max_temp, Warning_temp | ESP32 → STM32 |
| 0x304 | CFG_ABS_PARAMS | Threshold, Kp, Ki | ESP32 → STM32 |
| 0x305 | CFG_TCS_PARAMS | Slip_limit, Kp, Ki | ESP32 → STM32 |
| 0x3FF | CFG_ACK | Config_ID, Status | STM32 → ESP32 |

**Ventajas:**

✅ **Reemplazo de STM32 sin pérdida de configuración:** Si se reemplaza el STM32, la configuración se inyecta automáticamente desde el ESP32.  
✅ **Configuración centralizada:** Un único punto de gestión (NVS ESP32).  
✅ **Recuperación ante reset STM32:** Si el STM32 se resetea, el ESP32 reinyecta la configuración.  

### 3.4 PRINCIPIO DE TELEMETRÍA

**Definición:**

El STM32 realiza **downsampling** de datos de alta frecuencia para evitar saturación del bus CAN.

**Frecuencias de Telemetría:**

| Tipo de Dato | Frecuencia Generación | Frecuencia Envío CAN | Justificación |
|--------------|----------------------|---------------------|---------------|
| Velocidades ruedas | 1 kHz (sensores Hall) | 20 Hz | UI actualiza a 20 Hz |
| Corrientes motores | 1 kHz (ADC) | 20 Hz | UI actualiza a 20 Hz |
| Temperaturas | 10 Hz (DS18B20) | 5 Hz | Cambio lento |
| Posición encoder | 1 kHz (A/B/Z) | 50 Hz | Control dirección necesita precisión |
| Estado pedal | 100 Hz (ADC) | 50 Hz | Control tracción necesita respuesta rápida |
| **Errores** | (evento) | **Inmediato** | Prioridad máxima |
| **Safety (ABS/TCS)** | (evento) | **Inmediato** | Prioridad máxima |

**Prioridades CAN:**

CAN utiliza IDs para arbitraje (ID más bajo = mayor prioridad).

| Rango ID | Prioridad | Tipo de Mensaje |
|----------|-----------|-----------------|
| 0x000 - 0x0FF | **Alta** | Heartbeat, Emergency, Safety |
| 0x100 - 0x1FF | Media-Alta | Comandos de control |
| 0x200 - 0x2FF | Media | Estados y ACKs |
| 0x300 - 0x3FF | Media-Baja | Configuración |
| 0x400 - 0x6FF | Baja | Telemetría regular |
| 0x700 - 0x7FF | Muy Baja | Diagnóstico y debug |

**Cálculo de Carga del Bus:**

```
Mensaje CAN (500 kbps):
    - Overhead: ~47 bits (start, ID, CRC, ACK, EOF, IFS)
    - Payload: 0-64 bits (0-8 bytes)
    - Total típico: ~100 bits/mensaje = 200 µs @ 500 kbps

Carga estimada:
    - Heartbeat (2×10 Hz): 20 msg/s × 200 µs = 4 ms/s (0.4%)
    - Telemetría (10 msg × 20 Hz): 200 msg/s × 200 µs = 40 ms/s (4%)
    - Comandos (esporádicos): ~10 msg/s × 200 µs = 2 ms/s (0.2%)
    - Configuración (arranque): despreciable
    
TOTAL: ~5% de carga del bus → MUY SEGURO (objetivo <30%)
```

---

## 4. ARQUITECTURA OBJETIVO

### 4.1 Diagrama de Bloques Detallado

**Secuencia de Arranque e Inicialización del Sistema:**

1. **Fase de Boot del ESP32:** El ESP32 ejecuta su secuencia de arranque completa, inicializa el periférico TWAI (CAN nativo del ESP32) configurado a 500 kbps, y entra en estado de espera activa para detectar el heartbeat del STM32
2. **Detección de STM32:** El ESP32 espera hasta 5 segundos para recibir el primer mensaje de heartbeat del STM32. Si transcurre el timeout sin recepción, el sistema entra en modo degradado mostrando error "STM32 no disponible" en pantalla
3. **Fase de Boot del STM32:** De forma independiente, el STM32 ejecuta su secuencia de arranque, inicializa el periférico FDCAN1 (CAN-FD compatible con CAN clásico) a 500 kbps, y comienza transmisión periódica de heartbeat a 10 Hz
4. **Establecimiento de Comunicación:** Al recibir el heartbeat del STM32, el ESP32 confirma la disponibilidad del controlador de seguridad y procede a la secuencia de configuración inicial
5. **Secuencia de Configuración Multi-Parámetro:** El ESP32 transmite secuencialmente seis grupos de parámetros de configuración, esperando confirmación ACK del STM32 tras cada envío:
   - Parámetros PID de tracción (Kp, Ki, Kd para los cuatro motores) mediante mensajes CAN ID 0x300-0x301
   - Parámetros PID de dirección (Kp, Ki, Kd para el servomotor de dirección) mediante mensajes CAN ID 0x302-0x303
   - Límites de corriente máxima por motor y corriente total del sistema mediante mensajes CAN ID 0x310-0x311
   - Límites de temperatura crítica y temperatura de advertencia mediante mensaje CAN ID 0x320
   - Parámetros del sistema ABS (umbral de deslizamiento, frecuencia de modulación) mediante mensaje CAN ID 0x330
   - Parámetros del sistema TCS (umbral de tracción, ganancia de control) mediante mensaje CAN ID 0x331
6. **Validación de Configuración Completa:** Si todos los mensajes de configuración reciben ACK_OK del STM32, el sistema transiciona al estado READY y habilita el control operativo
7. **Gestión de Fallos de Configuración:** Si algún parámetro es rechazado por el STM32 (valores fuera de rango, incompatibilidad de configuración), el ESP32 reintenta el envío hasta tres veces. Si persiste el fallo, el sistema entra en estado de error seguro y requiere intervención del usuario

---

## 8. REGLAS DE AUTORIDAD

### 8.1 Matriz de Decisión

| Acción | Solicita | Decide | Ejecuta | Reporta |
|--------|----------|--------|---------|---------|
| **Cambiar velocidad tracción** | ESP32 (usuario) | STM32 (valida límites) | STM32 (PWM) | STM32 → ESP32 |
| **Cambiar ángulo dirección** | ESP32 (usuario) | STM32 (valida Ackermann) | STM32 (PWM) | STM32 → ESP32 |
| **Activar relé tracción** | ESP32 (usuario) | STM32 (valida seguridad) | STM32 (GPIO) | STM32 → ESP32 |
| **Activar ABS** | ESP32 (preferencia) | STM32 (autoridad) | STM32 (modulación) | STM32 → ESP32 |
| **Emergency Stop** | ESP32 o STM32 | STM32 (inmediato) | STM32 (PWM=0, relés OFF) | STM32 → ESP32 |
| **Configurar PID** | ESP32 (NVS) | ESP32 (gestión) | STM32 (almacena RAM) | STM32 → ESP32 (ACK) |
| **Mostrar datos en HMI** | STM32 (telemetría) | ESP32 (render) | ESP32 (TFT) | N/A |
| **Reproducir audio** | ESP32 (eventos) | ESP32 | ESP32 (DFPlayer) | N/A |
| **Obstáculo detectado** | ESP32 (sensor) | ESP32 (alerta) | STM32 (limita velocidad) | STM32 → ESP32 |

### 8.2 Validación de Comandos en STM32

El STM32 **SIEMPRE** valida comandos recibidos antes de ejecutarlos mediante un proceso de validación multi-capa:

**Arquitectura de Validación de Comandos:**

El módulo de recepción CAN del STM32 implementa una cadena de validación secuencial para cada comando recibido del ESP32. La validación sigue una arquitectura de cuatro niveles obligatorios:

1. **Validación de Rango de Valores:** El STM32 verifica que el valor solicitado se encuentre dentro de los límites configurables del sistema. Por ejemplo, para un comando de velocidad de tracción, se valida que la velocidad solicitada esté entre -MAX_SPEED y +MAX_SPEED (tanto en avance como en retroceso). Si el valor excede los límites, se envía respuesta ACK_REJECTED_OUT_OF_RANGE y el comando se descarta sin ejecución

2. **Verificación de Estado del Sistema:** Se comprueba que el sistema se encuentra en un estado compatible con la ejecución del comando. Comandos de movimiento solo son aceptados si el sistema está en estado STATE_READY. Comandos recibidos durante estados STATE_INITIALIZING, STATE_ERROR o STATE_SAFE_STOP son rechazados con código ACK_REJECTED_SYSTEM_NOT_READY

3. **Evaluación de Condiciones de Seguridad:** El STM32 consulta los sensores críticos para verificar que no existan condiciones de peligro activas. Se rechazan comandos de movimiento si: (a) la temperatura de cualquier motor supera el umbral de advertencia, (b) la corriente total del sistema supera el 90% del límite máximo, (c) existe una alerta activa de sobrecorriente en cualquier canal. El rechazo se notifica con código ACK_REJECTED_SAFETY

4. **Confirmación de Prerrequisitos de Hardware:** Se verifica que todos los componentes de hardware necesarios para ejecutar el comando están activos y operativos. Por ejemplo, comandos de tracción requieren que el relé de potencia de tracción esté energizado. Si los relés necesarios están desactivados, se rechaza con código ACK_REJECTED_RELAY_OFF

**Flujo de Decisión:** Si cualquiera de las cuatro validaciones falla, el comando se rechaza inmediatamente y se envía un mensaje ACK con el código de error específico al ESP32. Solo si las cuatro validaciones son exitosas, el comando se ejecuta y se envía ACK_OK. Esta arquitectura garantiza que ningún comando inseguro o inválido alcance los actuadores del sistema.

### 8.3 Rechazo de Comandos

Cuando el STM32 rechaza un comando, envía un ACK con código de error:

```
CAN ID 0x220: ACK_COMMAND
    Byte 0-1: cmd_id (uint16) - ID del comando original
    Byte 2: status (uint8)
        0x00 = OK (aceptado y ejecutado)
        0x01 = REJECTED_OUT_OF_RANGE
        0x02 = REJECTED_SYSTEM_NOT_READY
        0x03 = REJECTED_SAFETY
        0x04 = REJECTED_RELAY_OFF
        0x05 = REJECTED_HEARTBEAT_TIMEOUT
        0x06 = REJECTED_INVALID_PAYLOAD
        0xFF = REJECTED_UNKNOWN_ERROR
```

El ESP32 debe gestionar los rechazos de comandos y proporcionar retroalimentación apropiada al usuario:

**Arquitectura de Manejo de Respuestas ACK en ESP32:**

El módulo de recepción CAN del ESP32 implementa un handler de mensajes ACK que procesa las respuestas del STM32 tras cada comando enviado. La arquitectura distingue dos flujos principales:

**Flujo de Comando Rechazado (status ≠ ACK_OK):**
1. Extracción del código de rechazo desde el byte de estado del mensaje ACK
2. Traducción del código numérico a cadena descriptiva legible para el usuario (mediante tabla de lookup de códigos de error)
3. Presentación visual de error en la interfaz HMI: mensaje modal con descripción específica del rechazo
4. Notificación auditiva mediante reproducción de tono de error en DFPlayer Mini
5. **Crítico:** La interfaz NO actualiza los controles visuales con el valor solicitado; mantiene la representación del último estado confirmado por telemetría del STM32
6. Registro del evento de rechazo en log local del ESP32 para análisis posterior

**Flujo de Comando Aceptado (status = ACK_OK):**
1. Confirmación interna de que el comando fue aceptado por el STM32
2. La interfaz entra en modo de espera de telemetría: NO actualiza inmediatamente los indicadores visuales
3. Los indicadores de estado solo se actualizan cuando llega el mensaje de telemetría periódica del STM32 confirmando el nuevo estado real del sistema
4. Esta arquitectura garantiza que la UI siempre refleja el estado real del hardware, nunca el estado solicitado pero no confirmado

Esta separación entre confirmación de comando (ACK) y actualización de UI (telemetría) previene condiciones de race y garantiza que la interfaz refleja la verdad del sistema físico.

### 8.4 Conflictos de Autoridad

**Escenario de Ejemplo: Limitación Dinámica por Seguridad**

Consideremos la situación donde el usuario solicita una velocidad de tracción de 50 km/h, pero el STM32 detecta una condición de sobrecorriente que hace insegura esa velocidad.

**Flujo de Resolución de Conflicto:**

1. **Solicitud del Usuario:** El ESP32 transmite mensaje CAN CMD_SET_TRACTION_SPEED con payload conteniendo el valor 50 km/h solicitado por el usuario
2. **Detección de Condición Limitante:** El STM32, durante el proceso de validación del comando, detecta que la corriente total del sistema está cerca del límite máximo permitido (por ejemplo, 85% del límite de 40A)
3. **Aplicación de Limitación de Seguridad:** El STM32 acepta el comando pero aplica internamente una limitación dinámica, reduciendo la velocidad objetivo a 30 km/h para mantener la corriente dentro de márgenes seguros
4. **Confirmación de Aceptación:** El STM32 envía mensaje ACK_OK al ESP32, indicando que el comando fue aceptado (aunque con limitación interna aplicada)
5. **Reporte de Estado Real:** El STM32 envía mensaje de telemetría STATUS_TRACTION con el valor real aplicado de 30 km/h (no los 50 km/h solicitados)
6. **Notificación de Alerta:** El STM32 transmite mensaje SAFETY_ALERT con código OVER_CURRENT, informando al ESP32 de la razón de la limitación
7. **Presentación en HMI:** El ESP32 actualiza la interfaz mostrando:
   - Indicador de velocidad actual: 30 km/h (valor real del sistema)
   - Banner de alerta: "Corriente elevada - Velocidad limitada automáticamente"
   - Indicador visual (LED/color) de advertencia de sobrecorriente

**Principio Arquitectónico:** El STM32 mantiene autoridad final sobre todas las acciones de control. Puede aceptar comandos del ESP32 pero aplicar limitaciones de seguridad de forma autónoma y transparente. El ESP32 debe presentar siempre el estado real del sistema (según telemetría), nunca el estado solicitado pero no confirmado.

---

## 9. SEGURIDAD FUNCIONAL

### 9.1 Dead Man Switch (Heartbeat Monitor)

**Arquitectura del Monitor de Heartbeat en STM32:**

El STM32 implementa un sistema de monitorización continua de heartbeat (dead man switch) para detectar fallos del ESP32. El sistema opera con las siguientes características arquitectónicas:

**Parámetros del Sistema:**
- Timeout de heartbeat configurado a 500 milisegundos (máximo tiempo permitido sin recibir heartbeat del ESP32)
- Periodo de verificación de 50 milisegundos (frecuencia de comprobación del estado del heartbeat)

**Componentes Arquitectónicos:**

1. **Registro de Última Recepción:** El STM32 mantiene un timestamp de la última recepción de heartbeat del ESP32, actualizado atómicamente en cada mensaje HEARTBEAT_ESP32 recibido por el handler de interrupciones CAN

2. **Tarea de Monitorización Continua:** Una tarea de tiempo real ejecuta cada 50 ms el cálculo del tiempo transcurrido desde la última recepción de heartbeat. Si el tiempo transcurrido supera 500 ms, se activa inmediatamente la secuencia de SAFE_STOP con razón HEARTBEAT_TIMEOUT

3. **Handler de Recepción CAN:** La rutina de interrupción de recepción CAN identifica mensajes con ID HEARTBEAT_ESP32 y actualiza el timestamp de última recepción, reiniciando efectivamente el contador de timeout

**Arquitectura del Transmisor de Heartbeat en ESP32:**

El ESP32 ejecuta una tarea periódica dedicada exclusivamente a la transmisión de heartbeat al STM32:

**Características del Transmisor:**
- Frecuencia de transmisión: 10 Hz (un mensaje cada 100 milisegundos)
- Payload del mensaje: 5 bytes conteniendo (a) timestamp de uptime del ESP32 (4 bytes, uint32), (b) byte de estado operacional del ESP32 (1 byte)
- Prioridad de tarea: Alta, para garantizar transmisión consistente incluso bajo carga de rendering

**Arquitectura del Monitor de Heartbeat del STM32 en ESP32:**

El ESP32 también monitoriza el heartbeat del STM32 con timeout de 500 ms. Cuando detecta pérdida de heartbeat del STM32:

1. **Presentación de Error Crítico:** La interfaz HMI muestra mensaje modal de error crítico "STM32 NO RESPONDE" que no puede ser descartado hasta recuperación
2. **Notificación Auditiva:** Activación de alarma sonora continua mediante DFPlayer Mini
3. **Indicador Visual de Hardware:** Activación de patrón de error en LEDs WS2812B (parpadeo rojo rápido)
4. **Bloqueo de Comandos:** El ESP32 detiene el envío de todos los comandos de control, asumiendo que el vehículo ha entrado en SAFE_STOP automáticamente
5. **Espera de Recuperación:** El ESP32 continúa monitorizando el bus CAN esperando la recuperación del heartbeat del STM32

### 9.2 Estado SAFE_STOP

**Arquitectura del Modo de Parada Segura (SAFE_STOP):**

Cuando el STM32 detecta una condición que requiere detención inmediata del vehículo (timeout de heartbeat, sobrecorriente crítica, sobretemperatura crítica, comando de emergency stop), ejecuta la siguiente secuencia de acciones en orden estricto:

**Secuencia de Entrada a SAFE_STOP:**

1. **Desactivación Inmediata de Actuadores de Tracción:** Todos los canales PWM de los cuatro motores de tracción se configuran a ciclo de trabajo 0%, deteniendo la aplicación de potencia de forma inmediata

2. **Activación de Frenado Regenerativo Controlado:** Se activa el modo de frenado regenerativo en modalidad suave (soft mode), permitiendo que la energía cinética del vehículo se disipe de forma controlada y gradual, evitando frenado brusco que podría causar pérdida de control

3. **Desconexión de Relés de Potencia:** Los relés de tracción se desactivan, cortando físicamente la alimentación a los controladores de motor, proporcionando redundancia de seguridad a nivel de hardware

4. **Bloqueo de Dirección en Posición Actual:** El servomotor de dirección se mantiene energizado y bloqueado en su posición actual para evitar giro descontrolado de las ruedas directrices que podría causar desestabilización del vehículo

5. **Activación de Indicador Visual de Error en Hardware:** Se activa un LED de error directamente controlado por GPIO del STM32 (independiente del ESP32), proporcionando indicación visual física del estado de error

6. **Transmisión de Alerta de Seguridad al ESP32:** Se envía mensaje CAN de alerta ALERT_SAFE_STOP incluyendo el código de razón que causó la activación del modo seguro (para display en HMI)

7. **Transición de Máquina de Estados:** La variable de estado del sistema se actualiza a STATE_SAFE_STOP, bloqueando la aceptación de cualquier comando de control excepto CMD_EXIT_SAFE_STOP

8. **Registro en Log Persistente:** El evento de SAFE_STOP se registra con timestamp y código de razón en el log de eventos del STM32, permitiendo análisis forense posterior

**Procedimiento de Recuperación de SAFE_STOP:**

La salida del estado SAFE_STOP requiere intervención explícita del usuario y validación del STM32:

1. **Detección de Recuperación de Condición:** El STM32 detecta que la condición que causó SAFE_STOP se ha resuelto (por ejemplo, heartbeat del ESP32 se ha restablecido)
2. **No-Recuperación Automática:** El STM32 NO transiciona automáticamente a estado operativo; mantiene SAFE_STOP pero cambia a subestado STATE_SAFE_STOP_RECOVERABLE
3. **Notificación de Disponibilidad de Recuperación:** El STM32 transmite mensaje de estado indicando que el sistema puede ser reactivado
4. **Solicitud de Usuario:** El ESP32 presenta al usuario mensaje modal "Sistema en modo seguro. Presione OK para reactivar", requiriendo confirmación explícita
5. **Envío de Comando de Salida:** El usuario confirma, el ESP32 envía comando CMD_EXIT_SAFE_STOP al STM32
6. **Validación Multi-Punto del STM32:** El STM32 ejecuta verificación completa antes de aceptar la salida:
   - Todos los sensores de corriente reportan valores normales (< 10% del límite)
   - Todos los sensores de temperatura reportan valores normales (< umbral de advertencia)
   - No existen alertas activas en el sistema
   - El heartbeat del ESP32 es estable (sin interrupciones en los últimos 5 segundos)
7. **Aceptación o Rechazo:** Si todas las validaciones pasan, el STM32 transiciona a STATE_READY y envía ACK_OK. Si alguna validación falla, rechaza con código de error específico y mantiene SAFE_STOP
    - Temperatura normal
    - Corriente normal
8. Si validación OK:
    - STM32 sale de SAFE_STOP
    - STM32 envía: STATUS_SYSTEM = STATE_READY
    - ESP32 muestra: "Sistema reactivado"
9. Si validación falla:
    - STM32 permanece en SAFE_STOP
    - STM32 envía: ACK_REJECTED con razón
    - ESP32 muestra error específico

### 9.3 Protecciones de Hardware

#### 9.3.1 Sobrecorriente

**Arquitectura de Protección de Sobrecorriente:**

El STM32 implementa un sistema de protección de corriente de tres niveles para cada uno de los seis motores del sistema (cuatro de tracción, uno de dirección, más motor auxiliar):

**Componentes del Sistema:**
- Frecuencia de muestreo: 100 Hz (lectura cada 10 ms mediante sensores INA226 con comunicación I2C de alta velocidad)
- Tres umbrales configurables por motor: corriente nominal, corriente de advertencia (warning), corriente máxima crítica (1.5× máxima permitida)

**Niveles de Respuesta Graduada:**

1. **Nivel de Advertencia (Current Warning):** Cuando la corriente de un motor supera el umbral de advertencia pero permanece bajo el límite máximo:
   - El STM32 transmite alerta ALERT_CURRENT_WARNING al ESP32 con identificación del motor afectado
   - NO se aplica acción de protección sobre el motor
   - El ESP32 presenta indicador visual de advertencia al usuario (icono de alerta amarillo)
   - Permite operación continua con monitorización aumentada

2. **Nivel de Sobrecorriente (Overcurrent):** Cuando la corriente supera el límite máximo configurado pero permanece bajo 1.5× el límite:
   - El canal PWM del motor afectado se desactiva inmediatamente (PWM = 0%)
   - Se transmite alerta ALERT_OVERCURRENT al ESP32 con identificación del motor
   - El motor queda deshabilitado hasta que la corriente descienda bajo el umbral de advertencia
   - Los demás motores continúan operando normalmente (fallo aislado)
   - El sistema NO entra en SAFE_STOP, permitiendo operación degradada

3. **Nivel Crítico (Overcurrent Critical):** Cuando la corriente supera 1.5× el límite máximo (indicando posible cortocircuito o fallo catastrófico de motor):
   - Se activa inmediatamente la secuencia completa de SAFE_STOP con razón REASON_OVERCURRENT_CRITICAL
   - Todos los motores se desactivan y relés de potencia se desconectan
   - El vehículo entra en modo de parada segura completa
   - Requiere intervención del usuario para recuperación

**Arquitectura de Muestreo Continuo:** La tarea de protección de corriente ejecuta en bucle continuo con prioridad de tiempo real, leyendo secuencialmente los seis canales INA226 y evaluando los tres niveles de umbral para cada canal en cada iteración.

#### 9.3.2 Sobretemperatura

**Arquitectura de Protección Térmica:**

El STM32 monitoriza cuatro sensores de temperatura DS18B20 distribuidos estratégicamente en los componentes críticos del sistema (motores de tracción, controladores de potencia):

**Características del Sistema:**
- Frecuencia de muestreo: 10 Hz (lectura cada 100 ms, limitada por el tiempo de conversión del DS18B20)
- Dos umbrales configurables: temperatura de advertencia y temperatura máxima crítica

**Niveles de Respuesta Térmica:**

1. **Nivel de Advertencia Térmica (Temperature Warning):** Cuando algún sensor reporta temperatura superior al umbral de advertencia pero inferior al máximo crítico:
   - Se activa función de derating térmico (thermal derating): el STM32 reduce gradualmente la potencia máxima permitida a los motores en proporción a la temperatura
   - La reducción de potencia es progresiva: a mayor temperatura, mayor limitación (curva de derating lineal o exponencial configurable)
   - Se transmite alerta ALERT_TEMP_WARNING al ESP32 con identificación del sensor
   - El usuario ve indicador de advertencia de temperatura y reducción de potencia disponible
   - El sistema continúa operativo pero con capacidad reducida

2. **Nivel Crítico de Temperatura (Overtemperature Critical):** Cuando algún sensor supera el umbral de temperatura máxima crítica:
   - Se activa inmediatamente SAFE_STOP con razón REASON_OVERTEMPERATURE
   - Detención completa del sistema para prevenir daño térmico permanente a componentes
   - Se transmite alerta ALERT_OVERTEMP_CRITICAL con identificación del sensor afectado
   - El sistema NO puede salir de SAFE_STOP hasta que todas las temperaturas desciendan bajo el umbral de advertencia (con histéresis térmica)
   - Registro del evento para análisis de sobrecarga o fallo de ventilación

**Arquitectura de Muestreo Lento:** Dado el tiempo de conversión de los sensores DS18B20 (hasta 750 ms para resolución de 12 bits), el sistema utiliza modo de polling a 10 Hz con lecturas secuenciales y buffering de resultados anteriores para garantizar respuesta continua.

### 9.4 Watchdog

**Arquitectura de Watchdog en STM32 (IWDG - Independent Watchdog):**

El STM32 implementa un watchdog de hardware completamente independiente del núcleo principal y del reloj del sistema, garantizando reset del sistema en caso de bloqueo software:

**Configuración del IWDG:**
- Timeout configurado a 1 segundo (periodo máximo permitido entre refrescos del watchdog)
- Reloj independiente de 32 kHz LSI (Low Speed Internal oscillator), inmune a fallos del reloj principal
- Prescaler de 256 aplicado al reloj base, con valor de recarga de 1250, resultando en timeout de aproximadamente 1 segundo
- Activación durante la fase de inicialización del sistema, antes de entrar al loop principal de control

**Arquitectura de Refresh del Watchdog:**
El loop principal de control del STM32 ejecuta en ciclo continuo con periodo de 1 ms, realizando las siguientes operaciones en cada iteración:
1. Lectura de todos los sensores críticos (corriente, temperatura, velocidad de ruedas, posición de encoder)
2. Cálculo de algoritmos de control FOC (Field-Oriented Control) para los cinco motores
3. Aplicación de nuevos valores de PWM a los drivers de motor
4. Procesamiento de mensajes entrantes del bus CAN y envío de telemetría
5. Refresco del watchdog mediante escritura del valor de reset al registro IWDG

Si el loop de control se bloquea y no puede refrescar el watchdog en menos de 1 segundo (indicando fallo crítico de software), el IWDG fuerza automáticamente un reset completo del STM32, llevando el sistema a estado seguro inicial.

**Arquitectura de Watchdog en ESP32 (Task Watchdog Timer):**

El ESP32 utiliza el Task Watchdog Timer de ESP-IDF, un watchdog software integrado con FreeRTOS que monitoriza tareas específicas:

**Configuración del Task WDT:**
- Timeout configurado a 5 segundos (más permisivo que el STM32 debido a la naturaleza no-determinista de las tareas de UI)
- Modo panic activado: si el watchdog expira, el ESP32 genera un core dump y se reinicia automáticamente
- Asociación de la tarea principal (main task) al watchdog durante la fase de setup

**Operación del Task WDT:**
El loop principal del ESP32 ejecuta con periodo nominal de 10 ms, realizando:
1. Procesamiento de eventos de interfaz de usuario (touch, actualización de menús)
2. Manejo de mensajes CAN recibidos del STM32 (telemetría, alertas)
3. Actualización del framebuffer del display TFT con nuevos datos
4. Reset explícito del watchdog mediante llamada a la función de reset del Task WDT

Si la tarea principal se bloquea (por ejemplo, en un deadlock o loop infinito) durante más de 5 segundos, el Task WDT fuerza panic y reboot del ESP32. Dado que el STM32 tiene autoridad de control, el vehículo entra automáticamente en SAFE_STOP por timeout de heartbeat del ESP32, manteniendo seguridad del sistema.

---

## 10. GESTIÓN DE CONFIGURACIÓN

### 10.1 Estructura de Configuración (NVS ESP32)

**Arquitectura de Almacenamiento de Configuración:**

El ESP32 mantiene la configuración completa del sistema en memoria no volátil (NVS - Non-Volatile Storage), actuando como repositorio autoritativo de todos los parámetros operacionales. La estructura de configuración incluye los siguientes grupos de parámetros:

**Parámetros de Control PID:**
- Tracción: Tres coeficientes (Kp, Ki, Kd) y límite de saturación de salida para control de los cuatro motores de tracción en paralelo
- Dirección: Tres coeficientes (Kp, Ki, Kd) y límite de saturación para control del servomotor de dirección con retroalimentación de encoder

**Límites de Protección de Corriente:**
- Seis umbrales de corriente máxima (uno por motor: cuatro de tracción, uno de dirección, uno auxiliar)
- Seis umbrales de advertencia de corriente (típicamente 80-90% del máximo)
- Valores almacenados en miliamperios (uint16_t) para rango de 0-65.5 A

**Límites de Protección Térmica:**
- Temperatura máxima crítica (umbral de SAFE_STOP)
- Temperatura de advertencia (umbral de derating térmico)
- Valores almacenados en grados Celsius con signo (int16_t) para rango -40°C a +125°C

**Parámetros de Sistemas de Seguridad Activa:**
- ABS: Umbral de deslizamiento en porcentaje, coeficientes Kp y Ki para modulación de frenado
- TCS: Límite de slip en porcentaje, coeficientes Kp y Ki para control de tracción

**Calibraciones de Sensores:**
- Offset del encoder de dirección (compensación de posición de referencia)
- Calibración de diámetro de las cuatro ruedas (para cálculo preciso de velocidad)
- Valores mínimo y máximo del sensor analógico del pedal Hall (para normalización 0-100%)

**Integridad de Datos:**
- Checksum CRC32 calculado sobre todos los parámetros anteriores, almacenado al final de la estructura
- Validación de CRC32 en cada lectura para detectar corrupción de datos en NVS

### 10.2 Persistencia en NVS

**Arquitectura de Almacenamiento y Recuperación:**

El ESP32 implementa operaciones de almacenamiento y recuperación de configuración mediante la API NVS de ESP-IDF:

**Operación de Guardado:**
1. Apertura del namespace "config" en NVS con permiso de lectura/escritura
2. Cálculo del checksum CRC32 sobre todos los bytes de la estructura de configuración excepto el campo CRC32 final
3. Almacenamiento del CRC32 calculado en el campo correspondiente de la estructura
4. Escritura completa de la estructura como blob binario en NVS bajo la clave "system_config"
5. Commit de cambios y cierre del handle de NVS

**Operación de Carga:**
1. Apertura del namespace "config" en NVS con permiso de lectura
2. Lectura del blob "system_config" completo a la estructura en RAM
3. Validación de integridad: cálculo de CRC32 sobre los datos leídos y comparación con el CRC32 almacenado
4. Si el CRC32 coincide: configuración válida, se utiliza para operación del sistema
5. Si el CRC32 difiere o la lectura falla: se detecta corrupción o primera ejecución, se cargan valores por defecto de fábrica
6. Cierre del handle de NVS

**Valores por Defecto:** Si la configuración no puede ser recuperada de NVS (primera ejecución o corrupción), el sistema carga automáticamente valores conservadores predefinidos que garantizan operación segura pero potencialmente subóptima, permitiendo al usuario recalibrar posteriormente.

### 10.3 Inyección de Configuración (ESP32 → STM32)

**Arquitectura de Transferencia de Configuración:**

Durante la secuencia de arranque del sistema (tras establecimiento de comunicación CAN), el ESP32 ejecuta un protocolo de inyección completa de configuración al STM32. El proceso sigue una arquitectura secuencial con confirmación por mensaje:

**Secuencia de Inyección:**
1. **Grupo PID de Tracción:** Envío de dos mensajes CAN (CFG_PID_TRACTION y CFG_PID_TRACTION_2) conteniendo los cuatro parámetros del controlador PID de tracción. El ESP32 espera hasta 1 segundo por mensaje ACK del STM32 confirmando recepción y validación correcta
2. **Grupo PID de Dirección:** Envío de dos mensajes CAN (CFG_PID_STEERING y CFG_PID_STEERING_2) con parámetros PID de dirección, con espera de ACK de 1 segundo por mensaje
3. **Límites de Corriente:** Envío de mensajes con los seis umbrales de corriente máxima y advertencia, con confirmación ACK
4. **Límites de Temperatura:** Envío de temperaturas crítica y de advertencia, con confirmación
5. **Parámetros de ABS:** Envío de umbral y coeficientes de control ABS, con confirmación
6. **Parámetros de TCS:** Envío de límite y coeficientes de control TCS, con confirmación

**Gestión de Timeouts:** Si algún mensaje ACK no es recibido dentro del timeout de 1 segundo:
- El ESP32 reintenta el envío del mensaje hasta tres veces
- Si tras tres reintentos no hay ACK, se muestra error al usuario "Fallo de configuración de STM32"
- El sistema no puede entrar en estado READY hasta completar inyección exitosa

**Confirmación de Finalización:** Al completar exitosamente todos los envíos con ACK confirmados, el ESP32 presenta mensaje "Configuración inyectada a STM32" y el sistema transiciona a estado READY.

### 10.4 Almacenamiento en RAM (STM32)

**Arquitectura de Configuración Volátil en STM32:**

El STM32 NO almacena configuración en memoria Flash persistente; todos los parámetros de configuración se mantienen exclusivamente en RAM (variables globales estáticas). Esta decisión arquitectónica tiene implicaciones importantes:

**Estructura de Almacenamiento:**
El STM32 declara una estructura global SystemConfig en RAM que contiene todos los parámetros recibidos del ESP32. Esta estructura se inicializa con valores seguros mínimos en el arranque.

**Manejo de Mensajes de Configuración:**
El handler de recepción CAN del STM32 procesa mensajes con IDs de configuración (0x300-0x33F):
1. Identificación del ID de mensaje recibido
2. Copia directa de los bytes del payload CAN a los campos correspondientes de la estructura de configuración en RAM
3. Envío de mensaje ACK_OK al ESP32 confirmando almacenamiento exitoso
4. Los parámetros quedan inmediatamente disponibles para los módulos de control (PID, protecciones, ABS, TCS)

**Validación de Parámetros:** El STM32 valida que los valores recibidos están dentro de rangos sensatos antes de almacenarlos. Si detecta valores fuera de rango (por ejemplo, Kp negativo, corriente máxima > 100A), rechaza el parámetro con ACK_REJECTED_OUT_OF_RANGE.

**Ventaja de Configuración Volátil:** Si el STM32 se reemplaza por hardware nuevo o se reinicia por cualquier razón, el ESP32 detecta automáticamente la pérdida de configuración (al recibir valores por defecto en telemetría) y reinyecta la configuración completa desde NVS, garantizando restauración automática sin intervención manual. Esta arquitectura simplifica el reemplazo de hardware y la recuperación de fallos.

---
        err = nvs_set_blob(nvs_handle, "system_config", 
                          &system_config, sizeof(SystemConfig));
        
        if (err == ESP_OK) {
---

## 11. MATRIZ DE GESTIÓN DE FALLOS

### 11.1 Tabla Unificada de Respuesta a Fallos

Esta matriz define la respuesta arquitectónica del sistema ante cada tipo de fallo, especificando las acciones autónomas de cada microcontrolador, el estado final del sistema y los tiempos de detección garantizados.

| Tipo de Fallo | Acción STM32 | Acción ESP32 | Estado Final del Sistema | Tiempo de Detección |
|---------------|--------------|--------------|--------------------------|---------------------|
| **Pérdida de comunicación CAN** | Mantiene último comando válido durante 500 ms, luego entra en SAFE_STOP con razón CAN_TIMEOUT. Desactiva motores, desconecta relés, mantiene dirección bloqueada. Continúa intentando transmitir heartbeat y alertas. | Detecta pérdida de heartbeat del STM32. Muestra error crítico "STM32 NO RESPONDE", activa alarma sonora, patrón LED de error. Detiene envío de comandos. Asume vehículo en SAFE_STOP. | Sistema en SAFE_STOP. Vehículo detenido, relés desconectados. HMI muestra error crítico. Sistema no operativo hasta recuperación de CAN. | 500 ms (timeout de heartbeat) |
| **Pérdida de heartbeat ESP32** | Timeout de heartbeat detectado a 500 ms. Ejecuta secuencia SAFE_STOP con razón HEARTBEAT_TIMEOUT. Desactiva todos los motores (PWM=0%), activa frenado regenerativo suave, desconecta relés de tracción, bloquea dirección en posición actual, activa LED de error de hardware. Envía alerta SAFE_STOP al bus CAN (aunque ESP32 no la reciba). | N/A - El ESP32 está no operativo (reset, hang, fallo de alimentación). El sistema NO depende del ESP32 para entrar en estado seguro. | Sistema en SAFE_STOP autónomo. Vehículo detenido de forma segura por STM32 sin asistencia del ESP32. No hay interfaz HMI disponible. LED de error físico activado. | 500 ms (timeout de heartbeat en STM32) |
| **Pérdida de heartbeat STM32** | N/A - El STM32 está no operativo (reset, hang, fallo de alimentación). No puede enviar heartbeat ni responder a comandos. | Detecta falta de heartbeat del STM32 a 500 ms. Presenta error crítico "STM32 NO RESPONDE" con modal no-descartable, activa alarma de alerta continua, patrón LED rojo parpadeante rápido. Bloquea completamente envío de comandos de control. Permanece en estado de error hasta reinicio completo. | Sistema no operativo. ESP32 no puede controlar actuadores (no tiene conexión a motores/relés). Estado del vehículo indeterminado - depende del último estado del STM32 antes del fallo. Requiere reinicio manual del sistema completo. | 500 ms (timeout de heartbeat en ESP32) |
| **Sobrecorriente (Overcurrent)** | Detección a 100 Hz mediante lectura de INA226. Si corriente > límite máximo: desactiva PWM del motor afectado inmediatamente, envía alerta ALERT_OVERCURRENT. Si corriente > 1.5× límite: activa SAFE_STOP con razón OVERCURRENT_CRITICAL, desactiva todos los motores y relés. | Recibe alerta de sobrecorriente del STM32. Presenta banner de advertencia "Sobrecorriente en motor X" con icono de alerta. Si SAFE_STOP por overcurrent crítica: presenta error modal "Sobrecorriente crítica detectada", reproduce tono de error, requiere confirmación del usuario para intentar recuperación. | Sobrecorriente normal: Motor afectado deshabilitado, resto operativo (degradación graceful). Sobrecorriente crítica: Sistema en SAFE_STOP completo, todos los motores desactivados, relés OFF, requiere intervención del usuario. | 10 ms (periodo de muestreo a 100 Hz) |
| **Sobretemperatura (Overtemperature)** | Detección a 10 Hz mediante lectura de DS18B20. Si temperatura > umbral warning: aplica derating térmico (reducción progresiva de potencia máxima), envía ALERT_TEMP_WARNING. Si temperatura > umbral crítico: activa SAFE_STOP con razón OVERTEMPERATURE, desactiva motores y relés, envía ALERT_OVERTEMP_CRITICAL. | Recibe alerta térmica del STM32. Temperatura warning: muestra icono de advertencia térmica, indica potencia reducida en UI. Temperatura crítica: presenta error modal "Temperatura crítica alcanzada", alarma sonora, patrón LED de alerta. Sistema no puede salir de SAFE_STOP hasta que temperatura < umbral warning. | Temperatura warning: Sistema operativo con potencia reducida automáticamente (derating activo). Temperatura crítica: Sistema en SAFE_STOP completo, no recuperable hasta enfriamiento. Display muestra lectura de temperatura en tiempo real. | 100 ms (periodo de muestreo a 10 Hz) |
| **Reset de ESP32** | No detecta inicialmente (ESP32 deja de enviar heartbeat). A 500 ms de timeout de heartbeat: entra en SAFE_STOP automático con razón HEARTBEAT_TIMEOUT. Mantiene SAFE_STOP hasta recibir nuevo heartbeat del ESP32 post-reset y completar nueva secuencia de configuración completa. | Ejecuta secuencia de boot completa (~2-3 segundos). Inicializa TWAI, espera heartbeat de STM32 (detecta STM32 en SAFE_STOP), recibe estado del sistema. Reinyecta configuración completa desde NVS. Presenta al usuario opción de salir de SAFE_STOP tras validación. | Durante reset del ESP32: Sistema en SAFE_STOP (timeout de heartbeat). Post-reset: Sistema permanece en SAFE_STOP hasta intervención del usuario para reactivación. Configuración restaurada automáticamente desde NVS del ESP32. | 500 ms hasta SAFE_STOP, 2-3 segundos hasta recuperación de HMI |
| **Reset de STM32** | Ejecuta secuencia de boot completa (~500 ms). Inicializa FDCAN, comienza transmisión de heartbeat, carga configuración por defecto en RAM (valores conservadores). Sistema arranca en estado INITIALIZING, esperando inyección de configuración del ESP32. | Detecta heartbeat del STM32 con estado INITIALIZING (nuevo boot detectado). Automáticamente reinyecta configuración completa desde NVS mediante protocolo de inyección estándar. Valida ACK de cada parámetro. Al completar: sistema transiciona a READY. | Durante reset del STM32: Sistema no operativo (STM32 en boot). Post-reset: Sistema retorna a READY automáticamente tras reinyección de configuración (transparente para el usuario si ESP32 está operativo). Configuración restaurada desde NVS. | 500-700 ms hasta recuperación completa |
| **Bus CAN saturado** | Detecta saturación mediante conteo de errores de transmisión del periférico FDCAN. Incrementa backoff de mensajes de telemetría no-críticos. Mantiene prioridad máxima de heartbeat y alertas de seguridad. Si saturación persiste >5 segundos: registra evento pero mantiene operación (priorizando mensajes críticos). | Detecta latencias elevadas en recepción de telemetría. Reduce frecuencia de envío de comandos de UI (permite buffering). Mantiene transmisión de heartbeat a 10 Hz (prioridad máxima). Presenta advertencia al usuario "Bus CAN congestionado - respuesta lenta". | Sistema operativo con latencia aumentada en telemetría. Comandos de seguridad (emergency stop, SAFE_STOP) mantienen prioridad y funcionan normalmente. Interfaz puede mostrar datos con retraso pero sistema controlable. | Variable (detección progresiva a 1-2 segundos) |
| **Transceptor en estado Bus-Off** | Periférico FDCAN entra en estado Bus-Off tras exceder límite de errores. STM32 detecta Bus-Off mediante flag de estado del periférico. Ejecuta reset automático del FDCAN y reintento de inicialización. Si falla tras 3 reintentos: entra en SAFE_STOP con razón BUS_OFF_ERROR. Activa LED de error de hardware. | Detecta pérdida total de comunicación (timeout de heartbeat). Presenta error crítico "Fallo de comunicación CAN - Verificar cableado". Activa alarma de error. Asume sistema en SAFE_STOP. Sugiere al usuario verificar conexiones físicas del bus CAN. | Tras reintentos exitosos: Sistema retorna a operación normal. Tras fallos persistentes: Sistema en SAFE_STOP. Indica fallo de hardware de comunicación. Requiere diagnóstico físico del bus CAN (cableado, terminaciones, transceptores). | 100-300 ms hasta detección de Bus-Off, 500 ms adicional hasta SAFE_STOP si no recupera |

### 11.2 Principios de Diseño de Gestión de Fallos

**Failsafe por Defecto:** Ante cualquier fallo ambiguo o no clasificado, el STM32 transiciona a SAFE_STOP, garantizando que el vehículo se detiene de forma segura en lugar de continuar en estado potencialmente peligroso.

**Autoridad del STM32:** El STM32 tiene capacidad de entrar en SAFE_STOP de forma completamente autónoma, sin depender del ESP32. El fallo total del ESP32 NO compromete la seguridad del vehículo.

**Recuperación Gradual:** La salida de SAFE_STOP siempre requiere:
1. Resolución de la condición que causó el fallo
2. Validación multi-punto del estado del sistema
3. Intervención explícita del usuario (excepto en fallos transitorios de comunicación que se auto-recuperan)

**Trazabilidad Total:** Todos los eventos de fallo quedan registrados con timestamp en logs de ambos microcontroladores, permitiendo análisis forense post-incidente.

---

## 12. CRITERIOS DE VALIDACIÓN

### 12.1 Criterios Generales

Cada fase debe cumplir **TODOS** estos criterios antes de avanzar:

| Criterio | Descripción |
|----------|-------------|
| **Funcionalidad completa** | Todas las funciones de la fase operativas |
| **Sin regresión** | Comportamiento idéntico o superior a fase anterior |
| **Pruebas pasadas** | 100% de pruebas específicas de la fase pasan |
| **Estabilidad** | Ejecución 24h sin crashes ni resets |
| **Documentación** | Documentación técnica completa y actualizada |
| **Aprobación equipo** | Revisión y aprobación por equipo de ingeniería |

### 12.2 Criterios por Fase

#### Fase 0 (Baseline)

- ✅ Firmware compila sin warnings
- ✅ Boot exitoso en <5 segundos
- ✅ Todas las funcionalidades operativas
- ✅ Tests de regresión completos pasan
- ✅ Documentación baseline completa

#### Fase 1 (Ping-Pong CAN)

- ✅ Heartbeat bidireccional estable 10 min
- ✅ 1000 ping-pong sin errores
- ✅ Latencia CAN <5 ms (95percentil)
- ✅ Detección bus-off en <500 ms
- ✅ Recuperación bus-off automática
- ✅ Transceptores operativos (normal/standby)

#### Fase 2 (Gateway PWM)

- ✅ Duty cycle aplicado = duty cycle comandado (±1%)
- ✅ Latencia comando→hardware <10 ms
- ✅ Watchdog PWM funcional (<50 ms)
- ✅ Relés remotos operativos
- ✅ Comportamiento motores equivalente a Fase 0
- ✅ Recuperación ante fallo CAN inmediata

#### Fase 3 (Shadow Mode)

- ✅ Valores sensores STM32 ≈ ESP32 (<2% diff)
- ✅ Resultados FOC STM32 ≈ ESP32 (<5% diff)
- ✅ Shadow mode 24h con <1% discrepancias
- ✅ Control sigue ESP32 (NO STM32)
- ✅ PID STM32 converge correctamente

#### Fase 4 (Full Authority)

- ✅ STM32 controla motores autónomamente
- ✅ Heartbeat timeout → SAFE_STOP <500 ms
- ✅ Comandos inseguros rechazados
- ✅ Reinyección config tras reset STM32 OK
- ✅ ABS/TCS autónomo funcional
- ✅ Equivalencia funcional con Fase 0
- ✅ Fallo ESP32 → SAFE_STOP
- ✅ Fallo STM32 → Alerta ESP32
- ✅ Estabilidad 7 días continua

### 12.3 Herramientas de Validación

#### Logging con Timestamps

**Arquitectura de Sistema de Logging en ESP32:**

El ESP32 implementa un sistema de logging multi-nivel con timestamps para trazabilidad completa de eventos:

**Niveles de Logging:**
- INFO (nivel 0): Eventos informativos normales (arranque, transiciones de estado, configuración completada)
- WARN (nivel 1): Advertencias que no afectan operación pero requieren atención (corriente elevada, temperatura warning)
- ERROR (nivel 2): Errores que afectan funcionalidad (timeout de comandos, rechazo de configuración, entrada a SAFE_STOP)

**Características del Sistema:**
- Cada evento se registra con timestamp de millis() desde arranque del ESP32
- Formato de log: "[timestamp_ms] [NIVEL] mensaje_descriptivo"
- Salida inmediata a puerto serial para monitorización en tiempo real durante desarrollo
- Almacenamiento opcional en sistema de archivos SPIFFS para análisis post-operación (análisis forense de incidentes)

**Uso Operacional:** El sistema de logging permite identificar secuencias de eventos que llevaron a un fallo, con resolución temporal de 1 ms, facilitando depuración y validación de comportamiento del sistema.

#### Métricas de Comunicación CAN

**Arquitectura de Monitorización de Bus CAN:**

Ambos microcontroladores mantienen estructuras de métricas de comunicación CAN que rastrean el rendimiento y salud del bus:

**Métricas Capturadas:**
- Contador de mensajes transmitidos (tx_count): Total de mensajes enviados exitosamente
- Contador de mensajes recibidos (rx_count): Total de mensajes recibidos con CRC válido
- Contador de errores (error_count): Errores de transmisión, CRC, ACK
- Contador de eventos Bus-Off (bus_off_count): Número de veces que el transceptor entró en estado Bus-Off
- Latencia mínima, máxima y promedio (en microsegundos): Tiempo desde envío hasta recepción de ACK

**Actualización de Métricas:**
El sistema actualiza contadores en cada operación CAN (transmisión/recepción) y calcula latencia promedio mediante media móvil exponencial (EWMA) con factor 0.9, proporcionando smoothing de variaciones transitorias.

**Transmisión de Diagnóstico:**
Cada segundo, las métricas se transmiten mediante mensaje de diagnóstico DIAG_CAN_STATS, permitiendo al otro microcontrolador monitorizar la salud del bus desde ambas perspectivas.

**Uso en Validación:** Durante fases de migración, estas métricas permiten validar que la latencia CAN se mantiene bajo 5 ms (95º percentil) y que no hay degradación progresiva del bus (incremento sostenido de error_count).

#### Comparador de Shadow Mode

**Arquitectura de Validación de Shadow Mode:**

Durante la Fase 0 (Shadow Mode), el sistema implementa comparadores que validan la concordancia entre el cálculo del ESP32 (producción) y el STM32 (shadow):

**Estructura de Comparación:**
Para cada variable crítica (velocidad de motores, ángulo de dirección, corriente calculada), se registra:
- Valor calculado por el sistema en producción (local_value)
- Valor calculado por el sistema en shadow (shadow_value)
- Porcentaje de diferencia relativa
- Timestamp de la comparación

**Umbral de Alerta:**
Si la diferencia relativa entre ambos valores supera el 5%, se considera una discrepancia significativa que requiere investigación. El sistema:
1. Registra la discrepancia en log con nivel WARNING
2. Incluye ambos valores y el porcentaje de diferencia para análisis
3. Almacena el evento en log persistente para análisis posterior
4. Incrementa contador de discrepancias shadow

**Criterio de Validación de Fase 0:**
La fase shadow solo puede considerarse exitosa si las discrepancias se mantienen bajo 1% del total de comparaciones durante 24 horas de operación continua. Discrepancias superiores indican bugs en la implementación del STM32 que deben corregirse antes de avanzar a fases posteriores.

---

## 13. RIESGOS Y MITIGACIONES

### 13.1 Riesgos Técnicos

| Riesgo | Probabilidad | Impacto | Mitigación |
|--------|--------------|---------|-----------|
| **Latencia CAN excesiva** | Media | Alto | Fase 1 valida latencias antes de usar CAN para control |
| **Incompatibilidad transceptores** | Baja | Alto | Fase 1 valida hardware físico CAN |
| **Bugs en FOC STM32** | Media | Alto | Fase 3 (Shadow Mode) detecta discrepancias antes de ceder autoridad |
| **Fallo de comunicación CAN** | Media | Crítico | Dead Man Switch + SAFE_STOP automático |
| **Sensores STM32 incorrectos** | Media | Alto | Fase 3 compara sensores STM32 vs ESP32 |
| **Pérdida de configuración** | Baja | Medio | Configuración persistente en NVS ESP32, reinyección automática |
| **Regresión funcional** | Media | Crítico | Tests de equivalencia en cada fase vs Fase 0 |
| **Bus CAN saturado** | Baja | Medio | Downsampling telemetría, prioridades CAN correctas |

### 13.2 Riesgos de Migración

| Riesgo | Probabilidad | Impacto | Mitigación |
|--------|--------------|---------|-----------|
| **Romper firmware actual** | Alta | Crítico | Fases incrementales, rollback posible en cada fase |
| **Tiempo de migración excesivo** | Media | Medio | Plan claro por fases, cada fase independiente |
| **Pérdida de conocimiento** | Media | Alto | Documentación exhaustiva, código comentado |
| **Hardware no disponible** | Baja | Alto | Validar disponibilidad STM32G474RE antes de iniciar |
| **Equipo sin experiencia STM32** | Media | Alto | Capacitación previa, documentación de referencia |

### 13.3 Plan de Rollback

Cada fase permite rollback a la fase anterior:

| Desde Fase | Rollback a | Procedimiento |
|------------|------------|---------------|
| Fase 1 | Fase 0 | Deshabilitar código CAN en ESP32, recompilar |
| Fase 2 | Fase 1 | Restaurar control PWM local en ESP32 |
| Fase 3 | Fase 2 | Deshabilitar shadow comparison, mantener Gateway PWM |
| Fase 4 | Fase 3 | Cambiar autoridad de STM32 a ESP32, ejecutar shadow mode |

**Triggers de Rollback:**

- Inestabilidad del sistema
- Tasa de fallos >1% en 24h
- Discrepancias no resueltas en Shadow Mode
- Decisión de equipo técnico
- Problema crítico de seguridad

---

## 14. REFERENCIAS

### 13.1 Documentación del Proyecto

| Documento | Descripción |
|-----------|-------------|
| [README.md](README.md) | Descripción general del proyecto |
| [HARDWARE.md](HARDWARE.md) | Especificación oficial ESP32-S3 N16R8 |
| [docs/PLAN_SEPARACION_STM32_CAN.md](docs/PLAN_SEPARACION_STM32_CAN.md) | Plan inicial de separación |
| [docs/MANUAL_TRANSRECEPTORES_STM32_ESP32.md](docs/MANUAL_TRANSRECEPTORES_STM32_ESP32.md) | Manual de transceptores CAN |
| [docs/STM32G474RE_PINOUT_DEFINITIVO.md](docs/STM32G474RE_PINOUT_DEFINITIVO.md) | Pinout definitivo STM32 |
| [RESPUESTA_TRANSRECEPTORES.md](RESPUESTA_TRANSRECEPTORES.md) | Resumen rápido transceptores |

### 13.2 Datasheets y Especificaciones

| Componente | Referencia |
|------------|-----------|
| ESP32-S3 | [Espressif ESP32-S3 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf) |
| STM32G474RE | [STMicroelectronics STM32G474xx Datasheet](https://www.st.com/resource/en/datasheet/stm32g474re.pdf) |
| TJA1051T/3 | [NXP TJA1051T/3 CAN Transceiver Datasheet](https://www.nxp.com/docs/en/data-sheet/TJA1051.pdf) |
| CAN 2.0 Specification | [Bosch CAN Specification Version 2.0](http://esd.cs.ucr.edu/webres/can20.pdf) |

### 13.3 Estándares

| Estándar | Aplicación |
|----------|-----------|
| ISO 11898-1 | CAN Protocol - Data Link Layer |
| ISO 11898-2 | CAN Protocol - Physical Layer (High-Speed) |
| ISO 26262 | Functional Safety for Road Vehicles |

### 13.4 Herramientas

| Herramienta | Uso |
|-------------|-----|
| PlatformIO | Build system para ESP32 |
| STM32CubeIDE | IDE y configuración para STM32 |
| CANalyzer / CANoe | Análisis y debug del bus CAN |
| Logic Analyzer | Validación física del bus CAN |

---

## APÉNDICE A: GLOSARIO

| Término | Definición |
|---------|-----------|
| **ABS** | Anti-lock Braking System - Sistema que previene bloqueo de ruedas en frenado |
| **ACK** | Acknowledgment - Confirmación de recepción de mensaje |
| **CAN** | Controller Area Network - Bus de comunicación automotriz |
| **Dead Man Switch** | Mecanismo de seguridad que activa estado seguro ante falta de señal |
| **DLC** | Data Length Code - Longitud del payload en mensaje CAN |
| **FDCAN** | Flexible Data-rate CAN - Periférico CAN de STM32 |
| **FOC** | Field-Oriented Control - Técnica avanzada de control de motores |
| **HMI** | Human-Machine Interface - Interfaz de usuario |
| **NVS** | Non-Volatile Storage - Almacenamiento persistente en ESP32 |
| **OPI** | Octal PSRAM Interface - Interfaz de 8 bits para PSRAM |
| **PID** | Proportional-Integral-Derivative - Controlador de lazo cerrado |
| **PSRAM** | Pseudo-Static RAM - Memoria RAM externa del ESP32 |
| **PWM** | Pulse-Width Modulation - Modulación por ancho de pulso |
| **QIO** | Quad I/O - Modo de Flash de 4 bits |
| **SAFE_STOP** | Estado seguro del sistema con motores detenidos |
| **Shadow Mode** | Modo donde STM32 calcula sin aplicar, para validación |
| **TCS** | Traction Control System - Sistema que previene patinaje |
| **TWAI** | Two-Wire Automotive Interface - Periférico CAN del ESP32 |
| **Watchdog** | Timer que resetea el sistema si no se refresca |

---

## APÉNDICE B: HISTORIAL DE REVISIONES

| Versión | Fecha | Autor | Cambios |
|---------|-------|-------|---------|
| 1.0 | 2026-01-29 | Arquitectura Principal | Documento inicial completo |

---

## CONCLUSIÓN

Este documento establece la **estrategia completa y ejecutable** para migrar el firmware de un sistema monolítico (ESP32) a una arquitectura distribuida (ESP32 + STM32) mediante comunicación CAN.

### Puntos Clave:

1. **Migración por fases incrementales** - Cada fase es validable y permite rollback
2. **Seguridad funcional prioritaria** - Dead Man Switch, SAFE_STOP, validación de comandos
3. **Sin regresión funcional** - Comportamiento idéntico al sistema actual (Fase 0)
4. **Separación clara de roles** - ESP32 (HMI/Supervisor), STM32 (Control Seguro)
5. **Autoridad en STM32** - El STM32 tiene decisión final sobre el estado físico
6. **Configuración centralizada** - NVS del ESP32 como fuente única de configuración

### Próximos Pasos:

1. **Revisión del documento** por equipo técnico
2. **Aprobación de recursos** (hardware STM32, transceptores, tiempo de desarrollo)
3. **Capacitación del equipo** en STM32 y CAN
4. **Inicio de Fase 1** - Validación física y lógica del bus CAN
5. **Iteración y refinamiento** según resultados de cada fase

---

**Documento aprobado para ser la referencia oficial de la migración arquitectónica.**

**Versión:** 1.0  
**Fecha:** 2026-01-29  
**Estado:** ✅ COMPLETO

---

