# Integración ESP32-S3 + STM32G474RE por CAN (TJA1051T/3)

**Fecha/Date:** 2026-01-22 (ISO 8601, enero 2026)  
**Versión firmware base analizada:** v2.17.1 (PHASE 14, README.md).  

Este documento cumple el requisito de **estudio previo** del firmware actual y
define una **arquitectura de integración ESP32-S3 + STM32G474RE** basada en los
módulos reales existentes, con migración progresiva y segura.

### Criterios de éxito (objetivos verificables)
- ESP32 debe **arrancar solo** y seguir operando HMI aunque STM32 esté desconectado.
- CAN operativo con `CMD_SETPOINTS` (20 ms) y `STATE_FEEDBACK` (20 ms) sin pérdida.
- STM32 debe **cortar potencia** tras pérdida de heartbeat dentro del timeout definido.
- Migración por fases con conmutación reversible de backend sin cambios en HMI.

---

## 1) Resumen del firmware actual (basado en el código real)

### 1.1 Estructura general del proyecto

**Entry point y orden real de arranque** (ver `src/main.cpp`):
1. `Serial.begin` + backlight/reset TFT.
2. `BootGuard::initBootCounter()` (detección de bootloop).
3. `System::init()` → memoria, heap, PSRAM, estado.
4. `Storage::init()`, `Watchdog::init()`, `I2CRecovery::init()`.
5. Inicialización de managers **en orden estricto**:
   - `PowerManager`
   - `SensorManager`
   - `SafetyManager`
   - `HUDManager`
   - `ControlManager`
   - `TelemetryManager` *(omitido en safe mode)*
   - `ModeManager` *(omitido en safe mode)*

**Loop principal actual**:
`Power → Sensor → Safety → Mode → Control → Telemetry → HUD`  

### 1.2 Managers actuales y responsabilidad real

Basado en `src/managers/*.h`:

| Manager | Responsabilidad real | Módulos principales |
| --- | --- | --- |
| `PowerManager` | Gestión de alimentación y relés base | `power_mgmt.h` |
| `SensorManager` | Adquisición de inputs y sensores | `Pedal`, `Steering`, `Shifter`, `Buttons`, `Sensors` |
| `SafetyManager` | Seguridad activa (ABS/TCS/obstáculos) | `abs_system`, `tcs_system`, `obstacle_safety` |
| `HUDManager` | Display, touch, HUD, menús | `hud/*`, `menu/*` |
| `ControlManager` | Actuación de tracción/dirección/relés | `traction`, `steering_motor`, `relays`, `mcp23017_manager` |
| `TelemetryManager` | Logging/telemetría | `telemetry` |
| `ModeManager` | Modos operativos | `operation_modes` |

### 1.3 Sensores y actuadores conectados hoy al ESP32-S3

**Fuente:** `include/pins.h` + `GPIO_ASSIGNMENT_LIST.md`

**Sensores críticos actuales (feedback duro):**
- Encoder dirección A/B/Z → `GPIO 37/38/39`
- Sensores de rueda inductivos → `GPIO 7/36/15/1`
- Corriente (INA226 x6) vía I²C → `GPIO 8/9` + `TCA9548A`
- Temperatura (DS18B20 x4) → `GPIO 20`
- Pedal (Hall analógico) → `GPIO 4` (ADC)
- TOFSense-M S 8x8 (obstáculos) → UART0 `GPIO 43/44`

**Actuadores actuales (potencia/movimiento):**
- PWM tracción (4 motores) → PCA9685 x2 en I²C
- Dirección (PWM + IN1/IN2) → PCA9685 + MCP23017 vía I²C
- Relés potencia → `GPIO 35/5/6/46`

**HMI y periféricos de usuario:**
- TFT ST7796S + Touch XPT2046 → SPI `GPIO 10-16`, `GPIO 21/47`
- Audio DFPlayer Mini → UART1 `GPIO 18/17`
- LEDs WS2812B → `GPIO 1/48`
- Botones → `GPIO 2` (y MCP23017 para shifter)

### 1.4 Watchdogs y protecciones

- `BootGuard` detecta bootloops y fuerza **safe mode**.
- `Watchdog` alimentado en init y loop.
- `System::init()` verifica heap/PSRAM y falla en seguro si insuficiente.
- Módulos críticos validan I²C/PWM y ejecutan `Relays::emergencyStop()` cuando aplica.

### 1.5 Dependencias cruzadas y puntos críticos

**Dependencias reales entre managers** (ver `docs/ARCHITECTURE.md`):
- `PowerManager` es base para sensores y HUD.
- `SensorManager` alimenta datos a `SafetyManager`, `ControlManager`, `ModeManager`.
- `SafetyManager` valida condiciones antes de `ControlManager`.
- `ControlManager` usa I²C (PCA9685 + MCP23017) y depende de `I2CRecovery`.

**Puntos críticos de hardware:**
- Bus I²C único (`GPIO 8/9`) con PCA9685 + MCP23017 + TCA9548A.
- Relés de potencia en GPIOs dedicados (`35/5/6/46`).

---

## 2) Clasificación de módulos actuales (HMI / Control / Seguridad / Mixto)

**Basado en el código real (`src/`)**

### 2.1 HMI
- `hud/*` (render, gauges, icons, compositor)
- `menu/*` (menús)
- `audio/*` (DFPlayer, alerts)
- `lighting/led_controller.*` (WS2812)
- `input/buttons.*` (botones UI)
- `hud_manager` y `HUDManager`

### 2.2 Control (actuación)
- `control/traction.*`
- `control/steering_motor.*`
- `control/relays.*`
- `control/adaptive_cruise.*`
- `control/tcs_system.*` *(control + seguridad)*
- `input/shifter.*` *(control de modos de marcha)*

### 2.3 Seguridad
- `safety/abs_system.*`
- `safety/obstacle_safety.*`
- `safety/regen_ai.*`
- `core/boot_guard.*`
- `core/watchdog.*`

### 2.4 Mixtos / transversales
- `sensors/*` (suministra datos a control y seguridad)
- `input/pedal.*`, `input/steering.*` (inputs críticos para control)
- `core/system.*`, `core/operation_modes.*` (estado global y modos)
- `managers/*` (orquestación)
- `telemetry/*` (observabilidad + depuración)

---

## 3) Decisión: qué permanece en ESP32 y qué migra a STM32

### 3.1 ESP32-S3 se mantiene como “cabeza del coche” (obligatorio)
**Se queda en ESP32 (sin discusión):**
- HMI completo (TFT, touch, menús, HUD).
- LEDs WS2812B.
- Audio DFPlayer Mini.
- Detección de obstáculos a nivel **percepción/aviso** (TOFSense).
- Relés mínimos de arranque / power-hold (PIN_RELAY_MAIN).
- Telemetría, logging, menús de diagnóstico.

**Componentes actuales que deben quedarse en ESP32:**
- `HUDManager`, `hud/*`, `menu/*`, `audio/*`, `lighting/*`.
- `ObstacleSafety` (pero con **autoridad limitada**: solo avisos).
- `TelemetryManager`, `Logger`, `Config/Storage`.

### 3.2 STM32G474RE asumirá control seguro progresivo
**Se migran al STM32 (por fases):**
- Control de tracción (PWM + dirección motores 4x4).
- Control de dirección (PWM + IN1/IN2 BTS7960).
- Encoder A/B/Z de dirección.
- Sensores de rueda (4 inductivos).
- Corriente crítica (INA226 + TCA9548A) y temperaturas DS18B20.
- Relés de potencia que habilitan movimiento (TRAC, DIR).
- Lógica final de seguridad/decisión (ABS/TCS/regen, límites de corriente/temperatura).

**Módulos actuales a migrar (backend STM32):**
- `control/traction.*`
- `control/steering_motor.*`
- `control/relays.*` (solo relés de potencia, no el power-hold)
- `safety/abs_system.*`
- `control/tcs_system.*`
- `safety/regen_ai.*`
- `sensors/current.*`, `sensors/temperature.*`, `sensors/wheels.*`
- `input/steering.*`, `input/pedal.*`, `input/shifter.*` *(lectura crítica)*

### 3.3 Reglas de autoridad (no negociables)
- **STM32 tiene la autoridad final sobre potencia y movimiento.**
- ESP32 solo **solicita** acciones (setpoints) y muestra estado.
- ESP32 nunca cierra relés de potencia ni genera PWM directo en fase final.

---

## 4) Conexión física propuesta (STM32G474RE)

> **Nota:** Se define la asignación por periféricos STM32 (no por GPIO exacto)
> para permitir ajuste final en placa. El STM32G474RE dispone de ADCs, timers
> avanzados y CAN-FD (FDCAN). Se utilizará modo CAN clásico si se requiere
> compatibilidad simple.

### 4.1 Buses y periféricos STM32

**CAN (FDCAN1 o FDCAN2):**
- Conexión al transceptor **TJA1051T/3**.
- Línea CANH/CANL compartida con ESP32 via transceptor propio.
- Velocidad recomendada inicial: **500 kbps** para cableados largos (≈ >3 m),
  más derivaciones o mayor número de nodos. **1 Mbps** si el bus es corto
  (≈ ≤3 m), con baja carga y pocas derivaciones. Regla práctica de CAN clásica
  (ISO 11898-2): mayor longitud ⇢ menor bit rate para mantener margen de
  integridad de señal.

**GPIO habilitación transceptor (recomendado):**
- `CAN_STB`/`EN` del TJA1051T/3 controlado por STM32 para fail-safe.

**Timers PWM (control motores):**
- Timers avanzados para PWM de tracción y dirección (BTS7960).
- 4 canales para tracción + 2 canales para dirección (FWD/REV).

**GPIO digitales (IN1/IN2 + relés):**
- 8 líneas para IN1/IN2 de BTS7960 tracción.
- 2 líneas para IN1/IN2 de BTS7960 dirección.
- 2 relés de potencia: **TRAC** y **DIR**.

**ADC (sensores críticos):**
- Pedal Hall analógico.
- Corriente/voltaje si se decide migrar INA226 a lectura analógica directa (opcional).

**I²C/OneWire:**
- I²C para INA226 + TCA9548A (si se mantiene digital).
- OneWire para DS18B20 (temperaturas).

**Entradas con interrupción:**
- Encoder A/B/Z (dirección).
- Sensores inductivos de rueda (4).

### 4.2 Conexión física propuesta por componente

| Componente | Interfaz STM32 | Notas |
| --- | --- | --- |
| Encoder A/B/Z | GPIO + EXTI | Lectura en tiempo real |
| Sensores rueda x4 | GPIO + EXTI | 4 entradas con ISR |
| INA226 x6 + TCA9548A | I²C | Mantener topología actual |
| DS18B20 x4 | OneWire | 1 bus |
| BTS7960 tracción x4 | PWM + IN1/IN2 | 8 digitales + 4 PWM |
| BTS7960 dirección | PWM + IN1/IN2 | 2 digitales + 2 PWM |
| Relés TRAC/DIR | GPIO | Relés de potencia |
| CAN transceiver | FDCAN | TJA1051T/3 |

---

## 5) Rol del CAN-Bus y tipo de mensajes

**Objetivo:** Separar HMI (ESP32) de control seguro (STM32) manteniendo
**autoridad final** en STM32.

### 5.1 Mensajes principales (CAN IDs sugeridos)

| ID | Dirección | Contenido | Periodo |
| --- | --- | --- | --- |
| 0x100 | ESP32 → STM32 | `CMD_SETPOINTS` (throttle %, steering deg, shifter, enable) | 20 ms *(loop 50 Hz actual, ver `SYSTEM_TICK_MS`)* |
| 0x101 | ESP32 → STM32 | `CMD_HMI_MODE` (modo UI, flags, override) | 100 ms |
| 0x110 | STM32 → ESP32 | `STATE_FEEDBACK` (velocidad ruedas, dirección, pedal real) | 20 ms |
| 0x111 | STM32 → ESP32 | `SAFETY_STATUS` (ABS/TCS/overcurrent/temp) | 50 ms |
| 0x112 | STM32 → ESP32 | `POWER_RELAYS` (estado TRAC/DIR + fallos) | 100 ms |
| 0x120 | STM32 → ESP32 | `FAULT_CODE` (códigos críticos) | Event-driven |
| 0x130 | ESP32 → STM32 | `HEARTBEAT` | 100 ms |
| 0x131 | STM32 → ESP32 | `HEARTBEAT` | 100 ms |

### 5.2 Política de seguridad CAN
- Si STM32 **pierde heartbeat** del ESP32 → entra en modo seguro tras
  **250 ms** sin heartbeat (≈ 2-3 periodos de 100 ms + margen de proceso).
- ESP32 solo puede solicitar, **nunca forzar**.
- STM32 valida todos los setpoints con sus sensores locales.

### 5.3 Mensajes coherentes con módulos actuales

**Setpoints actuales en ESP32 (referencias reales):**
- `Pedal::get()` → `% pedal`.
- `Steering::get().angleDeg` → ángulo de dirección.
- `Shifter::get()` → P/R/N/D.

**Feedback actual requerido por HMI:**
- `Sensors::getWheelSpeed()` → velocidad por rueda.
- `Sensors::getCurrent()` / `Sensors::getTemperature()` → estado de potencia.

---

## 6) Estrategia de migración por fases (reversible y segura)

### Fase 0 – Preparación
- Crear interfaces de **backend de control** en ESP32 (actual vs CAN).
- Mantener firmware 100% funcional.

### Fase 1 – CAN telemetría (solo lectura)
- STM32 conectado, **solo envía** sensores (ruedas/encoder/temperatura/corriente).
- ESP32 muestra datos y compara con sensores propios (diagnóstico dual).

### Fase 2 – Control en sombra
- ESP32 sigue actuando motores/relés.
- STM32 calcula control y envía setpoints, pero **sin autoridad**.

### Fase 3 – STM32 toma potencia (modo piloto)
- STM32 controla **relés TRAC/DIR** y PWM.
- ESP32 se vuelve HMI + supervisor (solo solicita).

### Fase 4 – Migración completa
- STM32 es controlador final de movimiento y seguridad.
- ESP32 conserva HMI, telemetría, audio, LED, percepción de obstáculos.

Cada fase debe poder **revertirse** cambiando un backend/flag.

### Backend sugerido (para reversibilidad)
- `ControlBackend::LocalESP32` (actual).
- `ControlBackend::CAN_STM32` (nuevo).

---

## 7) Riesgos técnicos y mitigaciones

| Riesgo | Impacto | Mitigación |
| --- | --- | --- |
| Divergencia de sensores ESP32/STM32 | decisiones inconsistentes | Fase 1 con comparación y logs |
| Pérdida de CAN | pérdida de control | STM32 entra en safe + relés off tras **250 ms** sin heartbeat |
| Latencia CAN | control inestable | Periodos ≤20 ms + watchdog |
| Integración relés | apagado incorrecto | STM32 controla TRAC/DIR, ESP32 solo power-hold |
| Bootloop por cambios | sistema no arranca | Mantener init actual + backends selectivos |
| Consumo I²C compartido | lecturas erráticas | Migrar I²C a STM32 y mantener ESP32 solo HMI |

---

## 8) LISTA FINAL CL (Clasificación + destino + autoridad)

**Formato solicitado “CL” — Clasificación + Localización final**

1. **HUD / Menús / Touch** → **HMI** → **ESP32** (autoridad total HMI)
2. **LEDs WS2812B** → **HMI** → **ESP32**
3. **Audio DFPlayer** → **HMI** → **ESP32**
4. **Telemetría / Logger / Storage** → **Mixto** → **ESP32**
5. **Power Hold (RELAY_MAIN)** → **Seguridad** → **ESP32**
6. **Relés TRAC/DIR** → **Seguridad** → **STM32**
7. **Tracción PWM + IN1/IN2** → **Control** → **STM32**
8. **Dirección PWM + IN1/IN2** → **Control** → **STM32**
9. **Encoder A/B/Z** → **Control** → **STM32**
10. **Sensores rueda x4** → **Control** → **STM32**
11. **INA226 corrientes** → **Seguridad** → **STM32**
12. **DS18B20 temperaturas** → **Seguridad** → **STM32**
13. **Pedal Hall** → **Control** → **STM32**
14. **ABS/TCS/Regen** → **Seguridad** → **STM32**
15. **ObstacleDetection (percepción/alertas)** → **HMI** → **ESP32**
