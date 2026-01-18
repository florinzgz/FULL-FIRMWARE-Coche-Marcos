# Estudio técnico y separación de firmware (ESP32 HMI + STM32 Control)

**Objetivo:** definir una separación limpia en dos firmwares manteniendo una sola repo, sin romper el sistema actual, y preparando la migración del control crítico a STM32G474RE.

**Alcance actual:** solo arquitectura y plan de transición. No hay cambios de cableado ni hardware en esta fase.

---

## 1. Estudio técnico del estado actual

### 1.1 Responsabilidades actuales (ESP32-S3)
El firmware actual concentra **HMI + control + seguridad + sensórica**, lo que genera acoplamiento fuerte y cargas críticas en el mismo MCU:

- **HMI/UI:** `src/hud`, `src/menu`, `src/hud_*`, `src/render_engine.cpp`
- **Entrada de usuario:** `src/input/*`
- **Control de tracción/dirección:** `src/control/*`
- **Seguridad:** `src/safety/*`
- **Sensores:** `src/sensors/*`, `src/core/mcp23017_manager.cpp`, `src/i2c.cpp`
- **Power & relés:** `src/system/power_mgmt.cpp`, `src/managers/PowerManager.h`
- **Telemetría:** `src/core/telemetry.cpp`, `src/managers/TelemetryManager.h`
- **Audio:** `src/audio/*`
- **LEDs:** `src/lighting/*`

### 1.2 Carga y riesgos identificados
- **Riesgo de prioridad inversa:** el render/UI comparte tiempo con lógica de control en el mismo loop.
- **Riesgo de bloqueo I2C/SPI:** sensores críticos y periféricos HMI comparten buses.
- **Seguridad no aislada:** decisiones de cut-off y movimiento dependen de la misma CPU que la UI.
- **Complejidad de arranque:** `PowerManager`, sensores y HUD comparten secuencia crítica.
- **Observabilidad limitada:** logs y UI compiten con tareas de control.

### 1.3 Estado arquitectónico
La arquitectura por Managers (`PowerManager`, `SensorManager`, `SafetyManager`, `ControlManager`, `HUDManager`, `ModeManager`, `TelemetryManager`) está bien organizada, pero **no separa dominios de seguridad** y mezcla responsabilidades en el mismo firmware.

---

## 2. Propuesta de separación lógica (dos firmwares)

### Firmware A: **firmware-esp32-hmi**
Responsable exclusivo de HMI y arranque básico:
- Display ST7796S + touch XPT2046 y todo el hub SPI asociado
- UI, menús, diagnóstico y visualización
- Audio DFPlayer Mini
- LEDs WS2812B
- Detección de obstáculos (sensores + lógica visual)
- Relés mínimos de arranque / power-hold
- Comunicación con STM32 y presentación de estados/alertas

### Firmware B: **firmware-stm32-control**
Responsable del control crítico y seguridad:
- Control de motores y dirección
- Encoder y sensores de rueda
- INA226 + TCA9548A (corriente) y sensores térmicos críticos
- Relés de potencia no esenciales para el arranque (tracción, dirección, etc.)
- Decisiones finales de seguridad: cut-off, inhibición, fallos
- Gestión determinista de tiempo real y watchdogs del dominio de control

---

## 3. Mapeo de módulos actuales → destino

> **Principio:** separar por **dominio de riesgo**, no por conveniencia.

### 3.1 Permanecen en ESP32 (HMI)
- **HUD/UI/Render:** `src/hud/*`, `src/menu/*`, `src/render_engine.cpp`, `src/hud_compositor.cpp`
- **Touch & calibración:** `src/touch_*`
- **Audio:** `src/audio/*`
- **LEDs:** `src/lighting/led_controller.cpp`
- **Obstacle visual + lógica:** `src/sensors/obstacle_detection.cpp`, `src/hud/obstacle_display.cpp`, `src/safety/obstacle_safety.cpp`
- **Input de usuario:** `src/input/*` (se convierte en comandos hacia STM32)
- **Power-hold mínimo:** `src/system/power_mgmt.cpp` (solo arranque/retención)

### 3.2 Migran o se abstraen hacia STM32 (Control)
- **Control tracción/dirección:** `src/control/traction.cpp`, `steering_motor.cpp`, `steering_model.cpp`, `tcs_system.cpp`, `adaptive_cruise.cpp`
- **Sensores críticos:** `src/sensors/current.cpp`, `temperature.cpp`, `wheels.cpp`, `car_sensors.cpp`
- **ABS y regen:** `src/safety/abs_system.cpp`, `regen_ai.cpp`
- **Relés de potencia:** `src/control/relays.cpp` (solo tracción/dirección)
- **Safety Manager y Mode Manager:** migrar lógica decisoria al STM32; el ESP32 solo refleja estado.

### 3.3 Compartido (definido por contrato)
- **Modelo de estado del vehículo (telemetría y flags de seguridad)**
- **Setpoints de control (acelerador, dirección, modos)**
- **Diagnóstico y eventos críticos**

---

## 4. Estructura propuesta de repo (única repo, dos firmwares)

```
/
├─ firmware/
│  ├─ esp32-hmi/
│  │  ├─ platformio.ini
│  │  └─ src/            (HMI, UI, audio, LEDs, obstacle)
│  └─ stm32-control/
│     ├─ CMakeLists.txt o STM32CubeMX/
│     └─ src/            (control, safety, sensores críticos)
├─ shared/
│  ├─ protocol/          (contrato, IDs, estados, CRC)
│  └─ types/             (estructuras comunes, enums)
├─ docs/
│  └─ DUAL_FIRMWARE_ARCHITECTURE_STUDY.md
└─ tools/
```

**Nota:** mientras dure la transición, el firmware actual permanece en `src/` y se va moviendo a `firmware/esp32-hmi` de forma incremental.

---

## 5. Protocolo de comunicación (concepto, no implementación)

### 5.1 Canal físico
- **Primario:** UART con framing y CRC (simple, robusto)
- **Alternativo:** SPI si se requiere mayor throughput

### 5.2 Principios
- **STM32 es autoridad de seguridad.**
- **ESP32 solo solicita y visualiza.**
- **Heartbeat obligatorio y timeouts con fail-safe.**

### 5.3 Tipos de mensajes (ejemplo)
**HMI → Control**
1. `CMD_SETPOINT` (throttle, brake, steering)
2. `CMD_MODE_REQUEST` (modo conducción, perfil)
3. `CMD_ACTUATOR_TEST` (diagnósticos controlados)
4. `CMD_HMI_ACK` (confirmación de recepción de eventos)

**Control → HMI**
1. `TELEMETRY_STATE` (velocidad, corriente, temperaturas, wheel speed)
2. `SAFETY_EVENT` (cut-off, inhibición, fault level)
3. `CONTROL_STATUS` (modo efectivo, límites activos)
4. `DIAG_SNAPSHOT` (estado sensores críticos, flags)

**Mensajes de sistema**
1. `HELLO/HELLO_ACK` (versiones, compatibilidad)
2. `HEARTBEAT` (temporización y watchdog cruzado)
3. `ERROR_FRAME` (CRC, framing, timeout)

### 5.4 Flujo mínimo recomendado
1. **Boot ESP32 → HELLO**
2. **STM32 responde con versión + estado seguro**
3. **ESP32 habilita UI y muestra “Ready”**
4. **Setpoints solo aceptados si STM32 está “armed”**
5. **Si se pierde heartbeat → STM32 inhibe movimiento**

---

## 6. Transición incremental (sin romper el sistema)

1. **Definir contrato** (`shared/protocol`) y documentarlo.
2. **Crear interfaces abstractas** en el ESP32 (`IControlLink`, `ISafetyStatus`) con stubs locales.
3. **Inicialmente, ESP32 sigue actuando** como control real (stubs usan la implementación actual).
4. **Mover módulos de control al STM32** uno por uno:
   - Primero sensores críticos
   - Luego control de tracción/dirección
   - Finalmente decisiones de seguridad y relés de potencia
5. **En cada etapa:** mantener fallback en ESP32 y comparar telemetría (shadow mode).
6. **Habilitar “autoridad STM32”** cuando el enlace es estable y validado.

---

## 7. Errores típicos a evitar

- **No definir el contrato temprano** (se crean dependencias implícitas difíciles de romper).
- **Mover UI y control en el mismo paso** (rompe validación incremental).
- **No aislar decisiones de seguridad** (STM32 debe tener lógica final).
- **Ignorar timeouts y fail-safe** entre MCUs.
- **Acoplar mensajes a detalles de implementación** (el protocolo debe ser estable).
- **Subestimar el arranque**: power-hold y UI deben estar disponibles aunque el STM32 falle.

---

## 8. Conclusión

La separación propuesta mantiene el **ESP32-S3 como HMI** y libera el **STM32G474RE** para control determinista y seguridad. El diseño se apoya en la arquitectura modular existente, pero introduce un **contrato explícito** y una transición incremental que preserva el funcionamiento actual.

El siguiente paso es definir el protocolo y crear las capas de abstracción en el firmware actual, sin tocar aún el hardware.
