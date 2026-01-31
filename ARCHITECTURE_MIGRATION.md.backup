# ARCHITECTURE_MIGRATION.md

**Proyecto:** FULL-FIRMWARE-Coche-Marcos  
**Versión del Documento:** 1.0  
**Fecha:** 2026-01-29  
**Estado:** ✅ OFICIAL - Referencia Técnica Definitiva  
**Autor:** Arquitectura Principal de Firmware Embebido

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
11. [Criterios de Validación](#11-criterios-de-validación)
12. [Riesgos y Mitigaciones](#12-riesgos-y-mitigaciones)
13. [Referencias](#13-referencias)

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

```
1. ESP32 arranca
2. ESP32 inicializa periférico TWAI
3. ESP32 espera HEARTBEAT de STM32 (timeout 5s)
4. Si timeout → ESP32 muestra error "STM32 no disponible"
5. STM32 arranca
6. STM32 inicializa periférico FDCAN1
7. STM32 comienza envío HEARTBEAT
8. ESP32 recibe HEARTBEAT de STM32
9. ESP32 inicia secuencia de configuración:
    a. Envía CFG_PID_TRACTION (0x300, 0x301)
    b. Espera CFG_ACK
    c. Envía CFG_PID_STEERING (0x302, 0x303)
    d. Espera CFG_ACK
    e. Envía CFG_CURRENT_LIMITS (0x310, 0x311)
    f. Espera CFG_ACK
    g. Envía CFG_TEMP_LIMITS (0x320)
    h. Espera CFG_ACK
    i. Envía CFG_ABS_PARAMS (0x330)
    j. Espera CFG_ACK
    k. Envía CFG_TCS_PARAMS (0x331)
    l. Espera CFG_ACK
10. Si todas las configuraciones ACK OK → Sistema READY
11. Si alguna falla → Reintentar 3 veces → Error si falla
```

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

El STM32 **SIEMPRE** valida comandos recibidos antes de ejecutarlos:

```c
// Pseudocódigo STM32:
void can_command_handler(CAN_Message* msg) {
    switch(msg->id) {
        case CMD_SET_TRACTION_SPEED:
            float requested_speed = *(float*)msg->data;
            
            // Validación 1: Rango
            if (requested_speed < -MAX_SPEED || requested_speed > MAX_SPEED) {
                send_ack(msg->id, ACK_REJECTED_OUT_OF_RANGE);
                return;
            }
            
            // Validación 2: Estado del sistema
            if (system_state != STATE_READY) {
                send_ack(msg->id, ACK_REJECTED_SYSTEM_NOT_READY);
                return;
            }
            
            // Validación 3: Seguridad
            if (temperature_too_high() || current_too_high()) {
                send_ack(msg->id, ACK_REJECTED_SAFETY);
                return;
            }
            
            // Validación 4: Relés activos
            if (!relay_traction_is_on()) {
                send_ack(msg->id, ACK_REJECTED_RELAY_OFF);
                return;
            }
            
            // Todas las validaciones OK → Ejecutar
            set_traction_speed_internal(requested_speed);
            send_ack(msg->id, ACK_OK);
            break;
    }
}
```

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

El ESP32 debe manejar estos rechazos y mostrarlos al usuario:

```cpp
// En ESP32:
void handle_command_ack(CAN_Message* msg) {
    uint16_t cmd_id = msg->data[0] | (msg->data[1] << 8);
    uint8_t status = msg->data[2];
    
    if (status != ACK_OK) {
        // Comando rechazado
        const char* reason = get_rejection_reason_string(status);
        hud_show_error("Comando rechazado: %s", reason);
        audio_play_error_beep();
        
        // NO actualizar UI con estado solicitado
        // Mantener último estado confirmado
    } else {
        // Comando aceptado
        // Esperar telemetría para actualizar UI
    }
}
```

### 8.4 Conflictos de Autoridad

**Escenario:** Usuario solicita velocidad 50 km/h, pero STM32 detecta sobrecorriente.

```
1. ESP32 envía: CMD_SET_TRACTION_SPEED = 50 km/h
2. STM32 detecta corriente > límite
3. STM32 limita velocidad a 30 km/h (internamente)
4. STM32 envía: ACK_OK (acepta comando pero con limitación)
5. STM32 envía: STATUS_TRACTION = 30 km/h (velocidad real aplicada)
6. STM32 envía: SAFETY_ALERT = OVER_CURRENT
7. ESP32 muestra:
    - Velocidad actual: 30 km/h (no 50)
    - Alerta: "Corriente elevada - Velocidad limitada"
```

**Principio:** STM32 tiene autoridad final. Puede aceptar un comando pero aplicarlo de forma limitada por seguridad.

---

## 9. SEGURIDAD FUNCIONAL

### 9.1 Dead Man Switch (Heartbeat Monitor)

**Implementación en STM32:**

```c
#define HEARTBEAT_TIMEOUT_MS 500

uint32_t last_heartbeat_esp32_time = 0;

void heartbeat_monitor_task() {
    while(1) {
        uint32_t now = millis();
        uint32_t elapsed = now - last_heartbeat_esp32_time;
        
        if (elapsed > HEARTBEAT_TIMEOUT_MS) {
            // ESP32 no responde → SAFE STOP
            enter_safe_stop(REASON_HEARTBEAT_TIMEOUT);
        }
        
        delay(50);  // Check every 50 ms
    }
}

void can_rx_handler(CAN_Message* msg) {
    if (msg->id == HEARTBEAT_ESP32) {
        last_heartbeat_esp32_time = millis();
        heartbeat_esp32_received = true;
    }
}
```

**Implementación en ESP32:**

```cpp
void heartbeat_tx_task() {
    while(1) {
        uint32_t uptime = millis();
        uint8_t status = get_esp32_status_byte();
        
        uint8_t payload[5];
        memcpy(payload, &uptime, 4);
        payload[4] = status;
        
        can_send(HEARTBEAT_ESP32, payload, 5);
        
        delay(100);  // 10 Hz
    }
}

void heartbeat_rx_monitor() {
    static uint32_t last_heartbeat_stm32_time = 0;
    
    if (millis() - last_heartbeat_stm32_time > 500) {
        // STM32 no responde
        hud_show_critical_error("STM32 NO RESPONDE");
        audio_play_alarm();
        led_set_error_pattern();
        
        // Asumir que vehículo está en SAFE_STOP
        // No enviar más comandos de control
    }
}
```

### 9.2 Estado SAFE_STOP

Cuando el STM32 entra en SAFE_STOP:

```c
void enter_safe_stop(uint8_t reason) {
    // 1. Deshabilitar PWM de todos los motores
    set_all_motors_pwm(0);
    
    // 2. Activar freno regenerativo suave
    enable_regenerative_brake(SOFT_MODE);
    
    // 3. Desconectar relés de tracción
    relay_set(RELAY_TRACTION, OFF);
    
    // 4. Mantener dirección en posición actual (no desconectar)
    lock_steering_position();
    
    // 5. Activar LED de error en hardware
    gpio_set(GPIO_ERROR_LED, HIGH);
    
    // 6. Enviar alerta al ESP32
    send_safety_alert(ALERT_SAFE_STOP, reason);
    
    // 7. Cambiar estado del sistema
    system_state = STATE_SAFE_STOP;
    
    // 8. Logear evento
    log_event(EVENT_SAFE_STOP, reason);
}
```

**Recuperación de SAFE_STOP:**

```
1. STM32 detecta que heartbeat ESP32 se recuperó
2. STM32 NO sale automáticamente de SAFE_STOP
3. STM32 envía: STATUS_SYSTEM = STATE_SAFE_STOP_RECOVERABLE
4. ESP32 muestra al usuario: "Sistema en modo seguro. Presione OK para reactivar"
5. Usuario presiona OK
6. ESP32 envía: CMD_EXIT_SAFE_STOP
7. STM32 valida:
    - Sensores OK
    - Sin errores activos
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
```

### 9.3 Protecciones de Hardware

#### 9.3.1 Sobrecorriente

```c
void current_protection_task() {
    while(1) {
        for (int motor = 0; motor < 6; motor++) {
            uint16_t current_ma = read_current_ina226(motor);
            
            if (current_ma > config.current_max[motor]) {
                // Sobrecorriente detectada
                set_motor_pwm(motor, 0);
                send_safety_alert(ALERT_OVERCURRENT, motor);
                
                if (current_ma > config.current_max[motor] * 1.5) {
                    // Sobrecorriente crítica → SAFE_STOP
                    enter_safe_stop(REASON_OVERCURRENT_CRITICAL);
                }
            }
            
            if (current_ma > config.current_warning[motor]) {
                // Warning nivel
                send_safety_alert(ALERT_CURRENT_WARNING, motor);
            }
        }
        
        delay(10);  // 100 Hz
    }
}
```

#### 9.3.2 Sobretemperatura

```c
void temperature_protection_task() {
    while(1) {
        for (int sensor = 0; sensor < 4; sensor++) {
            int16_t temp_c = read_temperature_ds18b20(sensor);
            
            if (temp_c > config.temp_max) {
                // Temperatura crítica → SAFE_STOP
                enter_safe_stop(REASON_OVERTEMPERATURE);
                send_safety_alert(ALERT_OVERTEMP_CRITICAL, sensor);
            }
            
            if (temp_c > config.temp_warning) {
                // Warning → Reducir potencia
                apply_temperature_derating(temp_c);
                send_safety_alert(ALERT_TEMP_WARNING, sensor);
            }
        }
        
        delay(100);  // 10 Hz (DS18B20 es lento)
    }
}
```

### 9.4 Watchdog

**STM32 - IWDG (Independent Watchdog):**

```c
void iwdg_init() {
    // Configurar IWDG para timeout de 1 segundo
    IWDG->KR = 0x5555;  // Enable write access
    IWDG->PR = 6;       // Prescaler = 256
    IWDG->RLR = 1250;   // Reload value → ~1s timeout
    IWDG->KR = 0xCCCC;  // Start watchdog
}

void iwdg_refresh() {
    IWDG->KR = 0xAAAA;  // Refresh watchdog
}

void main_control_loop() {
    while(1) {
        // Loop de control crítico
        read_sensors();
        calculate_foc();
        apply_pwm();
        process_can();
        
        iwdg_refresh();  // Refresh cada loop (<1s)
        
        delay(1);  // 1 ms loop
    }
}
```

**ESP32 - Task Watchdog Timer:**

```cpp
void setup() {
    // Habilitar watchdog para task principal
    esp_task_wdt_init(5, true);  // 5 segundos, panic on timeout
    esp_task_wdt_add(NULL);      // Añadir task actual
}

void loop() {
    // Loop principal
    handle_ui();
    handle_can();
    update_display();
    
    esp_task_wdt_reset();  // Reset watchdog
    
    delay(10);
}
```

---

## 10. GESTIÓN DE CONFIGURACIÓN

### 10.1 Estructura de Configuración (NVS ESP32)

```cpp
struct SystemConfig {
    // PID Tracción
    float pid_traction_kp;
    float pid_traction_ki;
    float pid_traction_kd;
    float pid_traction_limit;
    
    // PID Dirección
    float pid_steering_kp;
    float pid_steering_ki;
    float pid_steering_kd;
    float pid_steering_limit;
    
    // Límites de Corriente (mA)
    uint16_t current_max[6];     // 4× tracción + 1× dirección + 1× auxiliar
    uint16_t current_warning[6];
    
    // Límites de Temperatura (°C)
    int16_t temp_max;
    int16_t temp_warning;
    
    // ABS Parameters
    uint16_t abs_threshold;      // % slip
    float abs_kp;
    float abs_ki;
    
    // TCS Parameters
    uint16_t tcs_slip_limit;     // % slip
    float tcs_kp;
    float tcs_ki;
    
    // Calibraciones Sensores
    float encoder_offset;
    float wheel_calibration[4];
    float pedal_min;
    float pedal_max;
    
    // Checksum
    uint32_t crc32;
};
```

### 10.2 Persistencia en NVS

```cpp
void config_save_to_nvs() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("config", NVS_READWRITE, &nvs_handle);
    
    if (err == ESP_OK) {
        // Calcular CRC32
        system_config.crc32 = calculate_crc32(&system_config, 
                                               sizeof(SystemConfig) - sizeof(uint32_t));
        
        // Guardar estructura completa
        err = nvs_set_blob(nvs_handle, "system_config", 
                          &system_config, sizeof(SystemConfig));
        
        if (err == ESP_OK) {
            nvs_commit(nvs_handle);
        }
        
        nvs_close(nvs_handle);
    }
}

bool config_load_from_nvs() {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open("config", NVS_READONLY, &nvs_handle);
    
    if (err == ESP_OK) {
        size_t size = sizeof(SystemConfig);
        err = nvs_get_blob(nvs_handle, "system_config", &system_config, &size);
        nvs_close(nvs_handle);
        
        if (err == ESP_OK) {
            // Validar CRC32
            uint32_t calculated_crc = calculate_crc32(&system_config, 
                                                       sizeof(SystemConfig) - sizeof(uint32_t));
            if (calculated_crc == system_config.crc32) {
                return true;  // Config válida
            }
        }
    }
    
    // Si falla, cargar defaults
    config_load_defaults();
    return false;
}
```

### 10.3 Inyección de Configuración (ESP32 → STM32)

```cpp
void config_inject_to_stm32() {
    // Enviar PID Tracción
    can_send_config_pid_traction();
    wait_ack(CFG_PID_TRACTION, 1000);
    
    can_send_config_pid_traction_2();
    wait_ack(CFG_PID_TRACTION_2, 1000);
    
    // Enviar PID Dirección
    can_send_config_pid_steering();
    wait_ack(CFG_PID_STEERING, 1000);
    
    can_send_config_pid_steering_2();
    wait_ack(CFG_PID_STEERING_2, 1000);
    
    // Enviar límites de corriente
    can_send_config_current_limits();
    wait_ack(CFG_CURRENT_LIMITS, 1000);
    
    // ... (resto de configuración)
    
    // Al finalizar
    hud_show_message("Configuración inyectada a STM32");
}
```

### 10.4 Almacenamiento en RAM (STM32)

```c
// En STM32 - Solo en RAM, NO en flash
struct SystemConfig config;  // Variable global

void config_receive_handler(CAN_Message* msg) {
    switch(msg->id) {
        case CFG_PID_TRACTION:
            memcpy(&config.pid_traction_kp, &msg->data[0], 4);
            memcpy(&config.pid_traction_ki, &msg->data[4], 4);
            send_config_ack(CFG_PID_TRACTION, ACK_OK);
            break;
            
        case CFG_PID_TRACTION_2:
            memcpy(&config.pid_traction_kd, &msg->data[0], 4);
            memcpy(&config.pid_traction_limit, &msg->data[4], 4);
            send_config_ack(CFG_PID_TRACTION_2, ACK_OK);
            break;
            
        // ... (resto de mensajes de configuración)
    }
}
```

**Ventaja:** Si STM32 se reemplaza o resetea, ESP32 reinyecta automáticamente la configuración.

---

## 11. CRITERIOS DE VALIDACIÓN

### 11.1 Criterios Generales

Cada fase debe cumplir **TODOS** estos criterios antes de avanzar:

| Criterio | Descripción |
|----------|-------------|
| **Funcionalidad completa** | Todas las funciones de la fase operativas |
| **Sin regresión** | Comportamiento idéntico o superior a fase anterior |
| **Pruebas pasadas** | 100% de pruebas específicas de la fase pasan |
| **Estabilidad** | Ejecución 24h sin crashes ni resets |
| **Documentación** | Documentación técnica completa y actualizada |
| **Aprobación equipo** | Revisión y aprobación por equipo de ingeniería |

### 11.2 Criterios por Fase

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

### 11.3 Herramientas de Validación

#### Logging

```cpp
// ESP32 - Logging con timestamps
#define LOG_LEVEL_INFO  0
#define LOG_LEVEL_WARN  1
#define LOG_LEVEL_ERROR 2

void log_event(uint8_t level, const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    uint32_t timestamp = millis();
    printf("[%lu] [%s] %s\n", timestamp, level_str[level], buffer);
    
    // Opcional: Guardar en SPIFFS para análisis posterior
    log_to_file(timestamp, level, buffer);
}
```

#### Métricas CAN

```cpp
struct CANMetrics {
    uint32_t tx_count;
    uint32_t rx_count;
    uint32_t error_count;
    uint32_t bus_off_count;
    uint32_t latency_min_us;
    uint32_t latency_max_us;
    uint32_t latency_avg_us;
};

void can_update_metrics() {
    // Actualizar métricas en cada envío/recepción
    can_metrics.tx_count++;
    can_metrics.latency_avg_us = (can_metrics.latency_avg_us * 0.9) + (latency * 0.1);
    
    // Enviar métricas cada segundo
    if (millis() - last_metrics_time > 1000) {
        can_send_diagnostics(DIAG_CAN_STATS, &can_metrics, sizeof(can_metrics));
        last_metrics_time = millis();
    }
}
```

#### Comparador Shadow Mode

```cpp
struct ShadowComparison {
    float local_value;
    float shadow_value;
    float difference_percent;
    uint32_t timestamp;
};

void shadow_compare_and_log(float local, float shadow, const char* name) {
    float diff_percent = fabs((local - shadow) / local) * 100.0f;
    
    if (diff_percent > 5.0f) {
        ShadowComparison comp = {
            .local_value = local,
            .shadow_value = shadow,
            .difference_percent = diff_percent,
            .timestamp = millis()
        };
        
        log_warning("SHADOW MISMATCH: %s - Local=%.2f, Shadow=%.2f, Diff=%.1f%%",
                    name, local, shadow, diff_percent);
        
        // Guardar para análisis
        shadow_log_save(&comp);
    }
}
```

---

## 12. RIESGOS Y MITIGACIONES

### 12.1 Riesgos Técnicos

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

### 12.2 Riesgos de Migración

| Riesgo | Probabilidad | Impacto | Mitigación |
|--------|--------------|---------|-----------|
| **Romper firmware actual** | Alta | Crítico | Fases incrementales, rollback posible en cada fase |
| **Tiempo de migración excesivo** | Media | Medio | Plan claro por fases, cada fase independiente |
| **Pérdida de conocimiento** | Media | Alto | Documentación exhaustiva, código comentado |
| **Hardware no disponible** | Baja | Alto | Validar disponibilidad STM32G474RE antes de iniciar |
| **Equipo sin experiencia STM32** | Media | Alto | Capacitación previa, documentación de referencia |

### 12.3 Plan de Rollback

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

## 13. REFERENCIAS

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

