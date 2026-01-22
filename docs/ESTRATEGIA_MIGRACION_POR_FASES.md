# Estrategia de Migración por Fases: ESP32-S3 → ESP32 (HMI) + STM32 (Control Seguro)

**Fecha:** 2026-01-22  
**Firmware base:** v2.17.1 (PHASE 14)  
**Objetivo:** Migración incremental, segura y reversible desde arquitectura monolítica ESP32-S3 hacia arquitectura distribuida ESP32 (HMI) + STM32G474RE (Control + Seguridad) con comunicación CAN.

---

## 1. PRINCIPIOS GENERALES DE MIGRACIÓN POR FASES

### 1.1 ¿Por qué la migración "todo de golpe" es imposible?

**Razones técnicas irrefutables:**

1. **Riesgo de bootloop catastrófico**  
   Cambiar 15+ módulos simultáneamente multiplica los puntos de fallo. Un solo error en CAN, I²C o PWM puede impedir el arranque completo. El sistema actual tiene protección `BootGuard` que detecta bootloops, pero un cambio masivo puede romper la secuencia de inicialización de forma irreversible.

2. **Pérdida de capacidad de diagnóstico**  
   Si todo cambia a la vez, es imposible identificar qué módulo específico causa el fallo. La depuración se vuelve exponencialmente compleja. Ejemplo: si fallan simultáneamente PWM, CAN y encoders, ¿cuál es la causa raíz?

3. **Imposibilidad de rollback controlado**  
   Sin fases intermedias funcionales, no existe punto de retorno. Un fallo en producción obliga a descartar todo el trabajo o depurar en frío sin firmware funcional de respaldo.

4. **Validación funcional imposible**  
   No se puede validar que el nuevo sistema cumple los mismos requisitos funcionales que el anterior si no hay puntos de comparación intermedios donde ambos sistemas coexistan.

5. **Dependencias cruzadas no resueltas**  
   Los módulos del firmware actual tienen dependencias estrictas (ver `ARCHITECTURE.md`, sección "Dependencias entre Managers"). Romper todas las dependencias a la vez genera un estado inconsistente irrecuperable.

### 1.2 ¿Por qué las interfaces y contratos deben separarse primero?

**Concepto clave:** Separar **qué hace** un módulo de **cómo lo hace**.

**Ejemplo concreto del firmware actual:**

```
ESTADO ACTUAL (monolítico):
  ControlManager::update() 
    → lee Pedal::get() 
    → escribe directamente pca9685.setPWM(MOTOR_FL, throttle)

PROBLEMA: 
  ControlManager está acoplado a la implementación local (PCA9685 en ESP32).
  No se puede cambiar el backend sin reescribir ControlManager.

SOLUCIÓN POR INTERFACES:
  ControlManager::update()
    → lee Pedal::get()
    → llama IControlBackend::setTraction(throttle)
    
  Implementaciones de IControlBackend:
    - LocalESP32Backend::setTraction() → pca9685.setPWM(...)
    - CANBackend::setTraction() → envía CAN 0x100 con throttle
```

**Consecuencia:**  
Sin esta separación, cada módulo debe reescribirse completamente. Con interfaces, solo se añade una nueva implementación del backend manteniendo intacta la lógica de negocio.

**Beneficios concretos:**
- Permite ejecutar backend local y CAN **en paralelo** (modo sombra).
- Facilita pruebas comparativas.
- Rollback inmediato cambiando un flag en `Storage`.

### 1.3 ¿Por qué la transferencia de autoridad ocurre al final, no al principio?

**Definición de "autoridad":**  
Autoridad = capacidad de tomar decisiones ejecutivas finales y actuar sobre hardware de potencia (relés, PWM).

**Secuencia correcta de transferencia:**

```
INCORRECTO (transferencia prematura):
  Fase 1: STM32 controla relés + PWM + seguridad
  Fase 2: ESP32 se vuelve HMI
  
  PROBLEMA: Si STM32 falla, no hay fallback. Sistema inoperativo.

CORRECTO (transferencia gradual):
  Fase 1: STM32 solo escucha (sin autoridad)
  Fase 2: STM32 calcula y envía, ESP32 ejecuta (ESP32 conserva autoridad)
  Fase 3: STM32 calcula y ejecuta, ESP32 supervisa (STM32 toma autoridad)
  Fase 4: STM32 tiene autoridad total, ESP32 solo visualiza
  
  BENEFICIO: En cualquier fase hay un sistema operativo con autoridad clara.
```

**Analogía de sistemas críticos (aviación):**  
Un piloto automático nuevo NO toma autoridad desde el primer vuelo. Primero se ejecuta en modo observador, luego en modo asistido, finalmente en modo autónomo. En cada fase, el piloto humano conserva override.

**Aplicado al firmware:**
- **Fase 1-2:** ESP32 conserva override (puede ignorar STM32).
- **Fase 3:** STM32 ejecuta, pero ESP32 puede forzar safe mode.
- **Fase 4:** STM32 tiene autoridad final, ESP32 solo solicita.

---

## 2. DESCRIPCIÓN DETALLADA DE CADA FASE

### FASE 0: Preparación (Refactorización sin cambio funcional)

#### Objetivo técnico
Aislar módulos de control/sensores en interfaces abstractas sin cambiar comportamiento del firmware actual. Sistema sigue siendo 100% ESP32.

#### Módulos ESP32 utilizados
- **Todos los existentes** en v2.17.1 sin modificación funcional.
- Se añaden capas de abstracción (interfaces) sobre `traction.cpp`, `steering_motor.cpp`, `relays.cpp`.

#### Módulos NO utilizados aún
- Ningún módulo se desactiva.
- STM32 no existe aún en esta fase.

#### Módulos añadidos/simulados
- **IControlBackend** (interfaz abstracta)
- **LocalESP32Backend : IControlBackend** (wrapper sobre código actual)
- **CANBackend : IControlBackend** (implementación vacía/stub)

#### Rol de STM32
- **No participa.** STM32 puede no estar presente físicamente.

#### Rol de CAN
- **No utilizado.** CAN no se inicializa.

#### Autoridad
- **ESP32:** 100% autoridad sobre todo (HMI + sensores + control + seguridad).
- **STM32:** No existe.

#### Riesgos evitados
- Evita cambios funcionales simultáneos con refactorización de interfaces.
- Permite validar que la capa de abstracción no introduce regresiones antes de conectar STM32.

#### Necesidad de esta fase
**Sin Fase 0:**  
No se puede introducir CAN sin reescribir todos los managers → alto riesgo de bootloop.

**Con Fase 0:**  
La refactorización se valida de forma aislada. Cualquier bug es detectable inmediatamente sin mezclar con problemas de CAN.

#### Tabla de responsabilidades - Fase 0

| Módulo | Ubicación | Estado | Autoridad | Razón |
|--------|-----------|--------|-----------|-------|
| HUD/Menús/Touch | ESP32 | Activo | ESP32 | Sin cambios |
| TelemetryManager | ESP32 | Activo | ESP32 | Sin cambios |
| SensorManager | ESP32 | Activo | ESP32 | Lectura local de sensores |
| SafetyManager | ESP32 | Activo | ESP32 | Validaciones locales ABS/TCS |
| ControlManager | ESP32 | Activo (refactorizado) | ESP32 | Usa IControlBackend → LocalESP32Backend |
| PowerManager | ESP32 | Activo | ESP32 | Gestión de relés local |
| PCA9685 (PWM) | ESP32 | Activo | ESP32 | Control directo de tracción/dirección |
| MCP23017 (GPIO) | ESP32 | Activo | ESP32 | Control de relés IN1/IN2 |
| INA226 (corriente) | ESP32 | Activo | ESP32 | I²C local |
| DS18B20 (temperatura) | ESP32 | Activo | ESP32 | OneWire local |
| Encoder dirección | ESP32 | Activo | ESP32 | GPIO + EXTI local |
| Sensores rueda | ESP32 | Activo | ESP32 | GPIO local |
| Pedal Hall | ESP32 | Activo | ESP32 | ADC local |
| CAN | – | No usado | – | No inicializado |
| STM32 | – | No existe | – | Fase sin hardware STM32 |

#### Validación de Fase 0
```
✓ Firmware compila sin errores
✓ Firmware arranca sin bootloop
✓ HUD muestra datos correctos
✓ Control de motores funciona idéntico a v2.17.1
✓ ABS/TCS responden correctamente
✓ No hay regresiones de funcionalidad
```

---

### FASE 1: Telemetría CAN en sombra (STM32 solo escucha)

#### Objetivo técnico
Validar comunicación CAN ESP32 ↔ STM32 sin afectar funcionalidad del vehículo. STM32 lee sensores y envía datos por CAN, pero ESP32 **ignora** esos datos y sigue usando sus sensores locales.

#### Módulos ESP32 utilizados
- **Todos los de Fase 0** sin cambios en autoridad.
- Se añade módulo `CANInterface` para recibir mensajes CAN.
- Se añade tarea de diagnóstico que **compara** datos CAN vs. locales.

#### Módulos NO utilizados aún
- **Control por CAN:** ESP32 no envía comandos de control a STM32.
- **Decisiones basadas en CAN:** SafetyManager ignora datos de CAN.

#### Módulos añadidos/simulados
- **STM32 firmware básico:** Lee sensores (encoders, ruedas, corriente, temperatura) y envía mensajes CAN `STATE_FEEDBACK` (0x110), `SAFETY_STATUS` (0x111).
- **ESP32 CANInterface:** Recibe mensajes CAN pero **no los usa para control**, solo para logging comparativo.

#### Rol de STM32
- **Observador pasivo:** Lee sensores conectados a él, envía telemetría por CAN.
- **Sin autoridad:** No controla relés ni PWM. STM32 puede estar desconectado sin afectar el funcionamiento.

#### Rol de CAN
- **Unidireccional:** STM32 → ESP32.
- **Telemetría pasiva:** Mensajes `STATE_FEEDBACK`, `SAFETY_STATUS`.
- **Velocidad:** 500 kbps (seguro para cables >3m con derivaciones).

#### Autoridad
- **ESP32:** 100% sobre sensores, control, seguridad.
- **STM32:** 0% (solo observador).

#### Riesgos evitados
- **Fallo de CAN no afecta operación:** Si CAN se cae, ESP32 continúa normal.
- **Divergencia de sensores detectable:** Se pueden detectar discrepancias entre sensores ESP32 vs. STM32 antes de confiar en ellos.
- **Validación de integridad CAN:** Se verifica que los mensajes llegan sin pérdida ni corrupción.

#### Necesidad de esta fase
**Sin Fase 1:**  
No se conoce si los sensores conectados a STM32 leen igual que los de ESP32. Pasar directamente a control con datos no validados puede generar decisiones erróneas.

**Con Fase 1:**  
Se validan sensores STM32 contra ESP32 (ground truth) antes de confiar en ellos para control.

#### Tabla de responsabilidades - Fase 1

| Módulo | Ubicación | Estado | Autoridad | Razón |
|--------|-----------|--------|-----------|-------|
| HUD/Menús/Touch | ESP32 | Activo | ESP32 | Sin cambios |
| TelemetryManager | ESP32 | Activo | ESP32 | Añade logging comparativo CAN |
| SensorManager | ESP32 | Activo | ESP32 | Sigue siendo fuente de verdad |
| SafetyManager | ESP32 | Activo | ESP32 | Usa sensores locales (ignora CAN) |
| ControlManager | ESP32 | Activo | ESP32 | Usa LocalESP32Backend |
| PowerManager | ESP32 | Activo | ESP32 | Sin cambios |
| PCA9685 (PWM) | ESP32 | Activo | ESP32 | Sin cambios |
| MCP23017 (GPIO) | ESP32 | Activo | ESP32 | Sin cambios |
| INA226 (corriente) | ESP32 + STM32 | Ambos activos | ESP32 | STM32 lee y envía, ESP32 decide |
| DS18B20 (temp) | ESP32 + STM32 | Ambos activos | ESP32 | STM32 lee y envía, ESP32 decide |
| Encoder dirección | ESP32 + STM32 | Ambos activos | ESP32 | STM32 lee y envía, ESP32 decide |
| Sensores rueda | ESP32 + STM32 | Ambos activos | ESP32 | STM32 lee y envía, ESP32 decide |
| Pedal Hall | ESP32 + STM32 | Ambos activos | ESP32 | STM32 lee y envía, ESP32 decide |
| CAN | ESP32 + STM32 | Activo (RX en ESP32) | – | Solo telemetría STM32→ESP32 |
| STM32 firmware | STM32 | Activo (pasivo) | Ninguna | Solo lectura de sensores + TX CAN |

#### Validación de Fase 1
```
✓ CAN inicializa sin errores en ESP32 y STM32
✓ Mensajes CAN STATE_FEEDBACK recibidos cada 20ms sin pérdida
✓ Divergencia sensor ESP32 vs. STM32 < 5% (valores aceptables)
✓ Sistema funciona idéntico con STM32 conectado o desconectado
✓ Logging comparativo muestra coherencia de datos
✓ No se observan errores CAN (CRC, ACK lost, bus-off)
```

---

### FASE 2: Control en sombra (STM32 calcula, ESP32 ejecuta)

#### Objetivo técnico
STM32 calcula salidas de control (PWM tracción, dirección, decisiones ABS/TCS) y envía comandos por CAN, pero **ESP32 conserva autoridad ejecutiva**. ESP32 compara comandos locales vs. CAN y registra discrepancias.

#### Módulos ESP32 utilizados
- **Todos de Fase 1.**
- **ControlManager** ahora recibe comandos de `CANBackend` pero **los compara** con `LocalESP32Backend` sin ejecutarlos directamente.
- Modo de operación: **dual backend con arbitraje ESP32**.

#### Módulos NO utilizados aún
- **Autoridad STM32:** STM32 calcula, pero ESP32 **no ejecuta automáticamente** comandos CAN. ESP32 decide qué backend usar.

#### Módulos añadidos/simulados
- **STM32 firmware de control completo:**
  - Lee sensores locales.
  - Ejecuta algoritmos ABS, TCS, control de tracción/dirección.
  - Envía comandos `CMD_TRACTION_PWM`, `CMD_STEERING_PWM` por CAN.
  - **NO cierra relés ni genera PWM físico** (aún).
  
- **ESP32 CANBackend:**
  - Implementación completa de `IControlBackend::setTraction/setSteering()`.
  - Recibe comandos CAN de STM32.
  - Los compara con backend local.

#### Rol de STM32
- **Calculador activo sin autoridad ejecutiva.**
- Procesa sensores, ejecuta lógica de control, envía comandos.
- **No actúa sobre hardware** (relés/PWM siguen en ESP32).

#### Rol de CAN
- **Bidireccional:**
  - ESP32 → STM32: `CMD_SETPOINTS` (pedal, steering, shifter).
  - STM32 → ESP32: `STATE_FEEDBACK`, `CMD_TRACTION_PWM`, `CMD_STEERING_PWM`, `SAFETY_STATUS`.
- **Frecuencia:** 20 ms (50 Hz) para comandos críticos.

#### Autoridad
- **ESP32:** 100% autoridad ejecutiva (ESP32 decide si ejecutar comandos CAN o locales).
- **STM32:** 0% autoridad ejecutiva (solo calculador).

#### Riesgos evitados
- **Validación de algoritmos STM32:** Se verifica que ABS/TCS de STM32 generan comandos coherentes antes de ejecutarlos.
- **Detección de divergencias:** Si STM32 calcula mal, se detecta antes de actuar sobre hardware.
- **Rollback inmediato:** Si comandos CAN son incoherentes, ESP32 ignora CAN y usa backend local.

#### Necesidad de esta fase
**Sin Fase 2:**  
No se sabe si los algoritmos de control en STM32 generan salidas correctas. Pasar directamente a ejecución puede causar movimientos inesperados.

**Con Fase 2:**  
Se validan algoritmos STM32 con hardware real sin riesgo de actuación incorrecta.

#### Tabla de responsabilidades - Fase 2

| Módulo | Ubicación | Estado | Autoridad | Razón |
|--------|-----------|--------|-----------|-------|
| HUD/Menús/Touch | ESP32 | Activo | ESP32 | Sin cambios |
| TelemetryManager | ESP32 | Activo | ESP32 | Logging comparativo backend local vs. CAN |
| SensorManager | ESP32 + STM32 | Ambos activos | ESP32 | ESP32 es fuente de verdad |
| SafetyManager | ESP32 + STM32 | Ambos activos | ESP32 | STM32 calcula, ESP32 decide |
| ControlManager | ESP32 | Activo (dual backend) | ESP32 | Compara LocalESP32 vs. CAN, ejecuta local |
| PowerManager | ESP32 | Activo | ESP32 | Relés controlados por ESP32 |
| PCA9685 (PWM) | ESP32 | Activo | ESP32 | PWM generado por ESP32 según backend local |
| MCP23017 (GPIO) | ESP32 | Activo | ESP32 | IN1/IN2 controlados por ESP32 |
| INA226 (corriente) | STM32 | Activo (lectura) | STM32 | STM32 lee, envía, ESP32 muestra |
| DS18B20 (temp) | STM32 | Activo (lectura) | STM32 | STM32 lee, envía, ESP32 muestra |
| Encoder dirección | STM32 | Activo (lectura) | STM32 | STM32 lee, calcula ángulo, envía |
| Sensores rueda | STM32 | Activo (lectura) | STM32 | STM32 lee, calcula velocidad, envía |
| Pedal Hall | STM32 | Activo (lectura) | STM32 | STM32 lee ADC, normaliza, envía |
| ABS/TCS (lógica) | STM32 | Activo (cálculo) | Ninguna | STM32 calcula, envía comandos, ESP32 no ejecuta |
| CAN | ESP32 + STM32 | Activo (TX+RX) | – | Comandos bidireccionales |
| Relés TRAC/DIR | ESP32 | Activo | ESP32 | Aún controlados por ESP32 |
| STM32 firmware | STM32 | Activo (calculador) | Ninguna | Calcula control, no ejecuta |

#### Validación de Fase 2
```
✓ Comandos CAN recibidos cada 20ms sin pérdida
✓ Divergencia comando local vs. CAN < 10% en condiciones normales
✓ ABS/TCS de STM32 generan comandos coherentes en frenadas/aceleraciones
✓ Sistema funciona con backend local si CAN falla
✓ Rollback a LocalESP32Backend funcional en tiempo real
✓ No se observan oscilaciones ni inestabilidad en comandos CAN
```

---

### FASE 3: Transferencia de autoridad (STM32 ejecuta, ESP32 supervisa)

#### Objetivo técnico
STM32 **toma autoridad ejecutiva** sobre relés de potencia (TRAC/DIR) y PWM (tracción + dirección). ESP32 **conserva capacidad de override** (puede forzar safe mode) pero ya no controla PWM directamente.

#### Módulos ESP32 utilizados
- **HUD, Telemetry, Audio, LEDs:** Sin cambios.
- **SensorManager:** Recibe datos de STM32 por CAN (ya no lee sensores críticos localmente, excepto backup).
- **SafetyManager:** Supervisa estado de STM32 (heartbeat, faults). Puede ordenar safe mode.
- **ControlManager:** Envía setpoints por CAN, **no ejecuta PWM localmente**.
- **PowerManager:** Solo controla relay MAIN (power-hold), **no controla TRAC/DIR**.

#### Módulos NO utilizados en ESP32
- **PCA9685 (PWM tracción/dirección):** Desconectado de ESP32, conectado a STM32.
- **MCP23017 (IN1/IN2):** Desconectado de ESP32, conectado a STM32.
- **LocalESP32Backend:** Desactivado o en modo backup.

#### Módulos añadidos/simulados
- **STM32 con autoridad total:**
  - Controla relés TRAC/DIR.
  - Genera PWM para BTS7960 (tracción + dirección).
  - Ejecuta ABS/TCS con autoridad.
  - **Timeout de heartbeat:** Si no recibe heartbeat de ESP32 en 250 ms → safe mode (relés off, PWM 0).

#### Rol de STM32
- **Controlador activo con autoridad ejecutiva.**
- Toma decisiones finales de seguridad (corte de potencia si overcurrent/overtemp).
- Responde a setpoints de ESP32 pero valida coherencia con sensores locales.

#### Rol de CAN
- **Crítico:**
  - ESP32 → STM32: `CMD_SETPOINTS`, `HEARTBEAT` (100 ms).
  - STM32 → ESP32: `STATE_FEEDBACK`, `SAFETY_STATUS`, `HEARTBEAT` (100 ms).
- **Pérdida de CAN = safe mode automático en ambos MCUs.**

#### Autoridad
- **ESP32:** Autoridad de supervisión (puede ordenar safe mode, no controla PWM).
- **STM32:** Autoridad ejecutiva total sobre potencia y movimiento.

#### Riesgos evitados
- **Validación de seguridad CAN:** Heartbeat detecta pérdida de comunicación antes de fallo catastrófico.
- **Fallback automático:** Si STM32 falla, ESP32 detecta ausencia de heartbeat y entra en safe mode.
- **Override de emergencia:** ESP32 puede forzar corte por CAN si detecta condición crítica.

#### Necesidad de esta fase
**Sin Fase 3:**  
No se puede validar que la transferencia de autoridad es segura. Si se pasa directamente a Fase 4, no hay mecanismo de override.

**Con Fase 3:**  
ESP32 conserva capacidad de detener el sistema si STM32 se comporta incorrectamente.

#### Tabla de responsabilidades - Fase 3

| Módulo | Ubicación | Estado | Autoridad | Razón |
|--------|-----------|--------|-----------|-------|
| HUD/Menús/Touch | ESP32 | Activo | ESP32 | Sin cambios |
| TelemetryManager | ESP32 | Activo | ESP32 | Logging completo CAN + estado STM32 |
| SensorManager | ESP32 | Pasivo (backup) | STM32 | Recibe datos por CAN, sensores locales en backup |
| SafetyManager | ESP32 | Activo (supervisor) | ESP32 | Supervisa heartbeat STM32, puede forzar safe |
| ControlManager | ESP32 | Activo (solicitante) | Ninguna | Envía setpoints, no ejecuta |
| PowerManager | ESP32 | Activo (limitado) | ESP32 | Solo relay MAIN (power-hold) |
| PCA9685 (PWM) | STM32 | Activo | STM32 | PWM generado por STM32 |
| MCP23017 (GPIO) | STM32 | Activo | STM32 | IN1/IN2 controlados por STM32 |
| INA226 (corriente) | STM32 | Activo | STM32 | Lectura, validación, actuación |
| DS18B20 (temp) | STM32 | Activo | STM32 | Lectura, validación, actuación |
| Encoder dirección | STM32 | Activo | STM32 | Control de lazo cerrado |
| Sensores rueda | STM32 | Activo | STM32 | Control de lazo cerrado |
| Pedal Hall | STM32 | Activo | STM32 | Adquisición y normalización |
| ABS/TCS (lógica) | STM32 | Activo | STM32 | Decisión y ejecución |
| CAN | ESP32 + STM32 | Crítico | – | Pérdida = safe mode |
| Relés TRAC/DIR | STM32 | Activo | STM32 | Control de potencia por STM32 |
| STM32 firmware | STM32 | Activo (ejecutor) | STM32 | Autoridad total de control |
| Heartbeat | ESP32 ↔ STM32 | Activo (100 ms) | Ambos | Timeout 250 ms → safe mode |

#### Validación de Fase 3
```
✓ STM32 controla relés TRAC/DIR correctamente
✓ PWM generado por STM32 controla motores sin oscilaciones
✓ Timeout de heartbeat activa safe mode en 250 ms
✓ ESP32 puede forzar safe mode por CAN (comando de emergencia)
✓ Sistema responde a setpoints de ESP32 con latencia < 50 ms
✓ ABS/TCS de STM32 actúan correctamente ante deslizamientos
✓ Overcurrent/overtemp cortan relés en STM32 sin intervención ESP32
```

---

### FASE 4: Migración completa (STM32 autónomo, ESP32 HMI puro)

#### Objetivo técnico
STM32 opera de forma **autónoma** como controlador de vehículo. ESP32 se reduce a **terminal HMI** (visualización, menús, audio, LEDs) sin capacidad de control directo sobre movimiento.

#### Módulos ESP32 utilizados
- **HUD/Menús/Touch:** Activos, sin cambios.
- **Audio/LEDs:** Activos.
- **TelemetryManager:** Activo, recibe telemetría de STM32.
- **SafetyManager:** Modo observador (muestra alertas, no decide).
- **ControlManager:** Solo envía setpoints solicitados por usuario (vía menús).
- **PowerManager:** Solo relay MAIN (opcional).

#### Módulos NO utilizados en ESP32
- **Sensores críticos:** ESP32 no lee encoders, ruedas, corriente, temperatura (excepto para diagnóstico comparativo opcional).
- **Actuadores:** ESP32 no controla relés TRAC/DIR ni PWM.
- **Decisiones de seguridad:** ABS/TCS ejecutados 100% en STM32.

#### Módulos añadidos/simulados
- **STM32 completamente autónomo:**
  - Puede operar **sin ESP32** en modo de emergencia (si pierde CAN, entra en safe mode pero conserva capacidad de diagnóstico local).
  - Todos los sensores/actuadores críticos conectados a STM32.
  - Logging local en STM32 (opcional).

#### Rol de STM32
- **Controlador principal autónomo.**
- Toma todas las decisiones de control y seguridad.
- ESP32 es solo interfaz de usuario.

#### Rol de CAN
- **Unidireccional prioritario STM32 → ESP32.**
- ESP32 → STM32: Setpoints de usuario (cambio de modo, velocidad objetivo).
- STM32 → ESP32: Telemetría completa (estado, sensores, faults).
- **Pérdida de CAN en ESP32:** STM32 continúa operando en modo degradado (sin HMI).

#### Autoridad
- **ESP32:** 0% autoridad de control. Solo visualización y solicitudes.
- **STM32:** 100% autoridad de control y seguridad.

#### Riesgos evitados
- **Sistema validado completo:** Todas las fases anteriores garantizan que STM32 es confiable.
- **ESP32 puede fallar sin afectar seguridad:** Si ESP32 entra en bootloop, STM32 detecta pérdida de heartbeat y entra en safe mode ordenado.

#### Necesidad de esta fase
**Objetivo final:** Sistema distribuido profesional con separación clara HMI/Control.

#### Tabla de responsabilidades - Fase 4

| Módulo | Ubicación | Estado | Autoridad | Razón |
|--------|-----------|--------|-----------|-------|
| HUD/Menús/Touch | ESP32 | Activo | ESP32 | HMI puro |
| Audio/LEDs | ESP32 | Activo | ESP32 | Feedback de usuario |
| TelemetryManager | ESP32 | Activo (visualizador) | Ninguna | Muestra datos de STM32 |
| SafetyManager | ESP32 | Pasivo (visualizador) | Ninguna | Muestra alertas, no decide |
| ControlManager | ESP32 | Pasivo (solicitante) | Ninguna | Envía setpoints usuario, no ejecuta |
| PowerManager | ESP32 | Activo (mínimo) | ESP32 | Solo relay MAIN (opcional) |
| SensorManager | ESP32 | No usado | – | Sensores críticos en STM32 |
| PCA9685 (PWM) | STM32 | Activo | STM32 | Control total tracción/dirección |
| MCP23017 (GPIO) | STM32 | Activo | STM32 | Control total IN1/IN2 |
| INA226 (corriente) | STM32 | Activo | STM32 | Decisión y actuación |
| DS18B20 (temp) | STM32 | Activo | STM32 | Decisión y actuación |
| Encoder dirección | STM32 | Activo | STM32 | Lazo cerrado |
| Sensores rueda | STM32 | Activo | STM32 | Lazo cerrado |
| Pedal Hall | STM32 | Activo | STM32 | Input crítico |
| ABS/TCS/Regen | STM32 | Activo | STM32 | Lógica completa |
| Relés TRAC/DIR | STM32 | Activo | STM32 | Potencia controlada por STM32 |
| CAN | ESP32 + STM32 | Activo | STM32 | STM32 autónomo, ESP32 opcional |
| STM32 firmware | STM32 | Autónomo | STM32 | Controlador principal |

#### Validación de Fase 4
```
✓ STM32 opera sin ESP32 conectado (modo degradado, sin HMI)
✓ Pérdida de ESP32 no afecta seguridad del vehículo
✓ ESP32 bootloop no causa movimiento inesperado
✓ Todas las funciones de seguridad activas en STM32
✓ HMI en ESP32 muestra datos precisos de STM32
✓ Sistema cumple requisitos de seguridad funcional
```

---

### FASE OPCIONAL: Fase 5 – Redundancia y diagnóstico avanzado

#### Objetivo técnico (opcional)
ESP32 conserva capacidad de lectura de sensores críticos **en modo redundante** para diagnóstico cruzado y detección de fallos en STM32.

#### Implementación
- ESP32 lee sensores críticos en paralelo (INA226, encoders, etc.).
- Compara lecturas ESP32 vs. STM32 (recibidas por CAN).
- Si divergencia > umbral → alerta de fallo en STM32.
- STM32 conserva autoridad ejecutiva total.

**Beneficio:** Detección temprana de fallos en sensores/firmware STM32.

---

## 3. MÓDULOS ESP32: EVOLUCIÓN DE ROLES

### 3.1 Módulos que dejan de actuar directamente sobre hardware

| Módulo | Fase de transición | Estado final |
|--------|-------------------|--------------|
| `traction.cpp` | Fase 3 | Solo solicitante (envía setpoints por CAN) |
| `steering_motor.cpp` | Fase 3 | Solo solicitante |
| `relays.cpp` (TRAC/DIR) | Fase 3 | Migrados a STM32 |
| `abs_system.cpp` | Fase 2-3 | Lógica migrada a STM32 |
| `tcs_system.cpp` | Fase 2-3 | Lógica migrada a STM32 |
| `regen_ai.cpp` | Fase 2-3 | Lógica migrada a STM32 |

### 3.2 Módulos que se vuelven "solicitantes" o "visualizadores"

| Módulo | Rol original | Rol final |
|--------|--------------|-----------|
| `ControlManager` | Actuador directo (PWM) | Solicitante (setpoints por CAN) |
| `SensorManager` | Lector de sensores I²C/ADC | Visualizador (recibe por CAN) |
| `SafetyManager` | Decisor de seguridad | Supervisor (muestra alertas STM32) |
| `TelemetryManager` | Logger de datos locales | Logger de datos CAN |

### 3.3 Módulos que permanecen en ESP32 hasta el final

| Módulo | Razón de permanencia |
|--------|---------------------|
| `HUDManager` | HMI es responsabilidad ESP32 |
| `hud/*` | Display + touch solo en ESP32 |
| `menu/*` | Interfaz de usuario |
| `audio/*` | DFPlayer conectado a ESP32 |
| `lighting/led_controller.*` | WS2812B conectados a ESP32 |
| `PowerManager` (relay MAIN) | Power-hold del sistema |
| `BootGuard` | Detección de bootloop ESP32 |
| `Storage` | Configuración local ESP32 |

---

## 4. MÓDULOS STM32: EVOLUCIÓN DE ROLES

### 4.1 Módulos que aparecen primero como pasivos (Fase 1)

| Módulo STM32 | Estado Fase 1 | Función |
|--------------|--------------|---------|
| `can_interface` | Pasivo (solo TX) | Envía telemetría |
| `sensor_acquisition` | Pasivo (lectura) | Lee sensores, no decide |
| `heartbeat` | Pasivo | Envía heartbeat |

### 4.2 Módulos que se vuelven calculadores (Fase 2)

| Módulo STM32 | Estado Fase 2 | Función |
|--------------|--------------|---------|
| `control_logic` | Calculador | Calcula PWM, no ejecuta |
| `abs_tcs_logic` | Calculador | Calcula correcciones, no ejecuta |
| `safety_monitor` | Calculador | Evalúa condiciones, no corta potencia |

### 4.3 Módulos que se vuelven ejecutores con autoridad (Fase 3+)

| Módulo STM32 | Estado Fase 3+ | Función |
|--------------|---------------|---------|
| `relay_control` | Ejecutor | Control de relés TRAC/DIR |
| `pwm_generator` | Ejecutor | Generación de PWM tracción/dirección |
| `abs_tcs_logic` | Ejecutor | Correcciones ABS/TCS con autoridad |
| `safety_monitor` | Ejecutor | Corte de potencia por overcurrent/temp |
| `heartbeat_monitor` | Ejecutor | Safe mode si pierde heartbeat ESP32 |

---

## 5. JUSTIFICACIÓN DE LA ESTRATEGIA

### 5.1 Minimización de riesgo de bootloop

**Problema del bootloop:**  
Cambios masivos en `main.cpp` pueden romper secuencia de inicialización (`BootGuard` detecta >3 resets → safe mode permanente).

**Mitigación por fases:**
- **Fase 0:** Solo refactorización, sin cambios funcionales → bootloop descartado.
- **Fase 1:** CAN añadido, pero sistema funciona sin él → bootloop descartado.
- **Fase 2-3:** Backends alternativos validados antes de cambiar autoridad → rollback posible.

**Resultado:**  
En cualquier fase hay un firmware funcional operativo. No existe "fase rota".

### 5.2 Evitación de regresiones funcionales

**Regresión típica:**  
"El vehículo ya no responde al pedal después del cambio."

**Prevención por fases:**
- **Fase 1:** Sistema funciona idéntico a v2.17.1 con STM32 conectado → regresión descartada.
- **Fase 2:** Comandos CAN comparados con backend local → divergencias detectadas antes de ejecución.
- **Fase 3:** Autoridad transferida solo tras validación de Fase 2 → regresión controlada.

### 5.3 Validación incremental

**Cada fase tiene criterios de éxito verificables:**

| Fase | Criterio de éxito | Método de validación |
|------|-------------------|---------------------|
| 0 | Comportamiento idéntico a v2.17.1 | Pruebas funcionales completas |
| 1 | Datos CAN coherentes con sensores ESP32 | Logging comparativo, divergencia < 5% |
| 2 | Comandos CAN coherentes con backend local | Logging comparativo, divergencia < 10% |
| 3 | STM32 controla hardware sin fallos | Pruebas de movimiento, ABS/TCS, timeouts |
| 4 | Sistema autónomo STM32 operativo | Pruebas sin ESP32, safe mode validado |

### 5.4 Capacidad de rollback en cualquier punto

**Implementación de rollback:**

```cpp
// En Storage (EEPROM)
enum ControlBackend {
    LOCAL_ESP32 = 0,  // Fase 0-1
    CAN_SHADOW = 1,   // Fase 2
    CAN_EXEC = 2,     // Fase 3
    CAN_FULL = 3      // Fase 4
};

// En ControlManager::init()
ControlBackend backend = Storage::getControlBackend();
switch (backend) {
    case LOCAL_ESP32:
        controlBackend = new LocalESP32Backend();
        break;
    case CAN_SHADOW:
        controlBackend = new DualBackend(); // Compara local vs CAN
        break;
    case CAN_EXEC:
    case CAN_FULL:
        controlBackend = new CANBackend();
        break;
}
```

**Rollback en campo:**  
Cambio de configuración en menú ESP32 → reinicio → sistema vuelve a backend anterior sin recompilación.

---

## 6. CRONOGRAMA ESTIMADO

| Fase | Duración estimada | Esfuerzo principal |
|------|------------------|-------------------|
| Fase 0 | 1-2 semanas | Refactorización de interfaces |
| Fase 1 | 2-3 semanas | Firmware STM32 básico + integración CAN |
| Fase 2 | 3-4 semanas | Algoritmos control STM32 + validación |
| Fase 3 | 2-3 semanas | Transferencia autoridad + pruebas seguridad |
| Fase 4 | 1-2 semanas | Pruebas sistema completo + documentación |
| **Total** | **9-14 semanas** | **~3 meses** |

---

## 7. CONCLUSIÓN TÉCNICA

La migración por fases **NO es opcional**, es la **única estrategia viable** para un sistema embebido crítico de esta complejidad.

**Principios inamovibles:**
1. Interfaces antes que implementación.
2. Observación antes que actuación.
3. Validación antes que autoridad.
4. Rollback siempre disponible.

**Resultado final:**  
Sistema distribuido profesional ESP32 (HMI) + STM32 (Control) con separación clara de responsabilidades, seguridad validada y capacidad de diagnóstico avanzado.

---

**Autor:** Arquitecto de sistemas embebidos automotrices  
**Versión documento:** 1.0  
**Fecha:** 2026-01-22
