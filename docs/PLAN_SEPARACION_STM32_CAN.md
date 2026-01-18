# Plan de separación y upgrade de arquitectura (ESP32 HMI + STM32 Control)

**Proyecto:** FULL-FIRMWARE-Coche-Marcos  
**Alcance:** Planificación y documentación (sin cambios de hardware ni código)  
**Objetivo:** Separar HMI y control seguro en dos firmware dentro de la misma repo, usando CAN con TJA1051T/3.

---

## 0) Decisiones fijas (no negociables)
- **ESP32-S3-N16R8** permanece como **controlador HMI**.
- **STM32G474RE** asume **control seguro y potencia crítica**.
- El ESP32 **nunca** tiene control final directo de potencia o movimiento.
- **Comunicación dedicada** vía **CAN** con TJA1051T/3 (UART solo como respaldo técnico justificado).
- **Un solo repositorio**, organizado y mantenible.
- **Fase inicial:** solo estudio, arquitectura y documentación.

---

## 1) Documentación previa (resumen técnico)

### 1.1 STM32G474RE (NUCLEO-G474RE)
- **FDCAN interno:** soporte para **Classic CAN** y **CAN FD** (bit-rate switching disponible). Para este proyecto se prioriza **Classic CAN** por compatibilidad y simplicidad.
- **Filtros:** soporta filtros **estándar (11-bit)** y **extendidos (29-bit)** con ruteo a FIFOs/colas.
- **Buffers:** TX FIFO/cola, RX FIFO0/1, con mecanismos de priorización y timestamp.
- **Reloj/temporización:** ajuste fino de prescaler y segmentos de bit-time para robustez en ambientes ruidosos.
- **Watchdog:** **IWDG (independiente)** y **WWDG (window)** para garantizar recuperación ante bloqueos.
- **Interrupciones:** FDCAN con interrupciones por RX/TX/errores para baja latencia.

### 1.2 TJA1051T/3 (transceptor CAN High-Speed)
- **Alimentación:** típicamente **5V** (ver hoja de datos exacta y rangos). 
- **Lógica TXD/RXD:** referida a VCC; validar compatibilidad con 3.3V del STM32/ESP32. Si es necesario, usar **nivelación** o transceptor con pin **VIO**.
- **Standby / Silent:** línea de control para ahorro energético y aislamiento del bus (usar como modo seguro).
- **Terminación:** 120Ω en los extremos del bus; para bus dedicado entre dos nodos, **terminación en ambos extremos**.
- **Protecciones:** tolerancia a EMI/ESD, protección ante dominancia prolongada y desconexiones.

### 1.3 Buenas prácticas HMI + Safety (automotriz)
- **Separación fuerte de responsabilidades:** HMI solicita, **control seguro decide**.
- **Fail-safe por defecto:** ante pérdida de comunicación o error, **movimiento inhibido**.
- **Determinismo temporal:** loops de control con prioridad alta y tiempos acotados.
- **Supervisión cruzada:** heartbeat mutuo y diagnóstico de latencias.
- **Degradación controlada:** modos degradados documentados y verificables.

---

## 2) Análisis arquitectónico del firmware actual (ESP32)

### 2.1 HMI
- **Display/Touch:** `src/hud`, `src/menu`, `include/hud_manager.h`, TFT + XPT2046.
- **Audio:** `src/audio` (DFPlayer Mini).
- **Iluminación:** `src/lighting` (WS2812B).
- **UI y diagnóstico:** menús, visualización de errores y estados.
- **Obstáculos:** `safety/obstacle_safety` con lógica visual y avisos.

### 2.2 Control
- **ControlManager:** integra `traction`, `steering_motor`, `relays`, `shifter`.
- **PowerManager:** `power_mgmt` (relés y power-hold).
- **SensorManager:** `sensors`, `wheels`, `steering`, `pedal`, `current`, `temperature`.

### 2.3 Seguridad
- **SafetyManager:** ABS/TCS + lógica de obstáculos.
- **Watchdog + BootGuard:** protección de arranque y recuperación.

### 2.4 Dependencias cruzadas
- `ControlManager` depende de sensores y seguridad (loop principal).
- `HUDManager` muestra estado de sensores/control/errores.
- `SafetyManager` lee sensores y afecta control (inhibición/limitación).

### 2.5 Carga temporal y riesgos
- Loop principal ~10–20 ms (50–100 Hz).
- Render + sensores + seguridad + control en un solo MCU genera **carga cercana al límite**, aumentando riesgo de latencias e interacciones no deterministas.

---

## 3) Separación lógica en dos firmware (misma repo)

### 3.1 firmware-esp32-hmi
- UI/menús/diagnóstico
- Display + Touch + SPI hub
- Audio, LEDs, backlight
- Detección de obstáculos **solo visual/aviso**
- Power-hold mínimo para arranque y visualización

### 3.2 firmware-stm32-control
- Control de tracción y dirección
- Encoder A/B/Z
- Sensores de rueda
- Sensores críticos de corriente y temperatura
- Relés de potencia críticos
- Lógica de seguridad y decisión final

---

## 4) Módulos: retener vs migrar (justificación)

### 4.1 Mantener en ESP32 (HMI)
| Módulo | Razón |
|---|---|
| Display + Touch (ST7796S/XPT2046) | Interacción directa con usuario, alto acoplamiento UI/SPI |
| Menús/UI/diagnóstico | Flujo HMI, no crítico para seguridad |
| Audio (DFPlayer) | Feedback de usuario, no crítico |
| LEDs WS2812B | Indicadores visuales, no críticos |
| Detección de obstáculos (visual/avisos) | Función de percepción/aviso; decisión final de movimiento se queda en STM32 |
| Relés de arranque/power-hold mínimos | Necesarios para boot y mostrar estado |

### 4.2 Migrar a STM32 (control seguro)
| Módulo | Razón |
|---|---|
| Tracción y dirección | Tiempo real, determinismo, seguridad funcional |
| Encoder A/B/Z + ruedas | Entradas críticas para control |
| Sensores de corriente (INA226 + TCA9548A) | Protección eléctrica crítica |
| Sensores térmicos críticos | Protección y derating |
| Relés de potencia | Deben estar bajo control seguro |
| ABS/TCS y lógica final de inhibición | Debe residir en MCU de seguridad |

---

## 5) Propuesta de estructura de repositorio

```
/firmware
  /esp32-hmi
    /src  (código actual ESP32, con separación lógica)
    /include
    platformio.ini
  /stm32-control
    /src
    /include
    /cube  (si aplica)
/common
  /protocol  (frames CAN, IDs, enums)
  /docs
/docs
  PLAN_SEPARACION_STM32_CAN.md
  ARCHITECTURE.md
  ...
```

**Principios:**
- Separación clara por firmware.
- **/common/protocol** como fuente única del contrato CAN.
- Documentación centralizada en `/docs`.

---

## 6) Contrato de comunicación ESP32 ↔ STM32 (CAN)

### 6.1 Roles
- **ESP32 (HMI):** solicita acciones, configura preferencias, muestra estados.
- **STM32 (Control):** decide, ejecuta y **tiene autoridad final**.

### 6.2 Tipos de mensajes
- **Heartbeat:** bidireccional (liveness, versión, uptime).
- **HMI → STM32 (comandos solicitados):**
  - set_mode, request_traction, request_steering, request_relay
  - parámetros no críticos (p.ej. límites de UI)
- **STM32 → ESP32 (estado y decisión):**
  - estado de control (velocidades, estados relés)
  - estado de seguridad (inhibición, fallos)
  - métricas críticas (corriente/temperatura)
- **Errores/Faults:** códigos estructurados, severidad y acción recomendada.

### 6.3 Reglas de autoridad
- El STM32 **puede rechazar** cualquier comando del ESP32.
- La falta de heartbeat o CAN inválido **inhibe movimiento**.
- ESP32 **nunca** conmuta potencia crítica directamente.

### 6.4 Esquema funcional convalidado (estado final)

```
                 ┌───────────────────────────────────────────────┐
                 │                ESP32 HMI                       │
                 │  HUD/UI + Touch + Audio + LEDs + Obstacle      │
                 │  Power-hold mínimo + Diagnóstico               │
                 └───────────────┬───────────────────────────────┘
                                 │  CAN (TJA1051T/3)
                                 ▼
                 ┌───────────────────────────────────────────────┐
                 │              STM32 Control                     │
                 │  Traction + Steering (Ackermann) + Safety      │
                 │  Encoder + Wheels + INA226 + Temps + Relays    │
                 └───────────────────────────────────────────────┘
```

**ESP32 (HMI) mantiene funcionando:**
- `hud_manager`, `menu`, `audio`, `lighting`, backlight.
- Touch + botones de UI (solicitudes al STM32).
- Detección de obstáculos **local** con envío de alertas por CAN.
- Relés de arranque/power-hold mínimos para mostrar estado.

**STM32 (Control seguro) mantiene funcionando:**
- Pedal, shifter y entradas críticas de control.
- Encoder A/B/Z + cálculo de dirección **Ackermann**.
- Sensores de rueda, corrientes INA226 (TCA9548A) y térmicos críticos.
- Relés de potencia (tracción/dirección) y lógica ABS/TCS.

**Rutas de datos (sin pérdida funcional):**
- **Pedal/encoder/ruedas → STM32 → control tracción/dirección → relés.**
- **INA226/temperaturas → STM32 → safety → estado CAN → HUD.**
- **Obstáculos (sensores ESP32) → ESP32 → alerta CAN → STM32 inhibe.**
- **UI (touch/menús) → ESP32 → comandos CAN → STM32 decide/ejecuta.**

---

## 7) Plan de migración por fases

### Fase 0 — Estudio y documentación (actual)
- Inventario de módulos y dependencias.
- Definición de arquitectura objetivo y contrato CAN.
- Plan de repo y estrategia de pruebas.

### Fase 1 — Separación lógica + abstracciones
- Crear límites claros entre HMI y control en el código actual.
- Interfaces/abstracciones para comandos y estados (sin hardware nuevo).
- Preparar carpetas de repo (sin mover hardware todavía).

### Fase 2 — STM32 en modo dummy/simulado
- STM32 recibe CAN y responde con estados simulados.
- ESP32 deja de ejecutar control real y consume estado remoto.
- Validación de protocolo y latencias.

### Fase 3 — Migración real de control
- Migrar control, sensores críticos y relés al STM32.
- Validar safety e inhibición final.
- ESP32 se limita a HMI + avisos.

### Nota de compatibilidad (no romper firmware actual)
- **Fase 0–1:** no se cambia la funcionalidad actual; el ESP32 mantiene la lógica existente de obstáculos, INA226 y encoder/dirección Ackermann.
- **Abstracciones:** se introducen interfaces para que la fuente de datos/actuación pueda ser **local o CAN**, sin modificar el comportamiento visible.
- **Fase 2 (shadow mode):** STM32 calcula en paralelo y el ESP32 compara resultados sin actuar, registrando discrepancias antes del traspaso.
- **Corte final:** solo cuando la equivalencia y latencias estén validadas, la autoridad de potencia se mueve al STM32 (Fase 3).

---

## 8) Riesgos técnicos y errores comunes
- **Latencias CAN** mal dimensionadas → control inestable.
- **Nivel lógico incorrecto** entre MCU y transceptor → fallos silenciosos.
- **Duplicidad de autoridad** (ESP32 aún controla relés) → riesgo crítico.
- **No definir fallback seguro** → movimiento ante pérdida de comunicación.
- **Filtros CAN mal configurados** → tráfico basura o pérdida de mensajes.
- **Migración sin fases** → difícil diagnóstico y regresiones ocultas.

---

## Resultado esperado
Un plan claro y ejecutable para separar **HMI** (ESP32) y **control seguro** (STM32) con CAN, manteniendo estabilidad del firmware actual y reduciendo riesgos en la transición.
