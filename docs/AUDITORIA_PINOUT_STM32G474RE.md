# Auditoría Técnica - STM32G474RE Pinout Definitivo

**Fecha de Auditoría:** 2026-01-22  
**Documento Auditado:** `STM32G474RE_PINOUT_DEFINITIVO.md`  
**Auditor:** Ingeniero Senior Hardware/Firmware Embebido Automotriz  
**Fase:** Pre-fabricación PCB / Design Freeze

---

## VEREDICTO GENERAL

**Estado:** ⚠️ **APTO CON CORRECCIONES OBLIGATORIAS**

**Resumen Ejecutivo:**
El documento presenta una base sólida y bien estructurada para la integración hardware del STM32G474RE. Sin embargo, se han detectado **3 errores críticos** y **7 mejoras recomendadas** que deben abordarse antes del design freeze y fabricación de la PCB.

**Principales hallazgos:**
1. **Conflicto crítico:** Pin PB3 usado simultáneamente para JTDO y TIM2_CH2
2. **Error de especificación:** Divisor resistivo del pedal Hall genera pérdida innecesaria de resolución ADC
3. **Riesgo de diseño:** Asignación de EXTI puede causar problemas de latencia
4. **Mejoras necesarias:** Aclaraciones sobre AF y configuraciones específicas

---

## 1. CONFLICTOS DE PINES

### 1.1 CONFLICTO CRÍTICO: PB3

**Problema Detectado:**  
El pin **PB3** está asignado a **TIM2_CH2** (Encoder dirección canal B).

**Análisis:**  
PB3 tiene función alternativa **JTDO** (JTAG Data Out) y **TRACESWO** (Serial Wire Output).  
Si bien SWD no requiere JTDO, este pin puede tener configuración por defecto que cause conflictos.

**Datasheet STM32G474RE (DS12288 Rev 8):**
- PB3: GPIO, TIM2_CH2, **TRACESWO**, I2S3_CK, SPI1_SCK, SPI3_SCK
- Después de reset, PB3/PB4 pueden estar configurados en modo TRACE si TRACE está habilitado en registros de debug

**Riesgo:**
- **ALTO** - El encoder de dirección es crítico para control.
- Si TRACESWO está activo, puede interferir con TIM2_CH2.
- Requiere configuración explícita de AFIO para desactivar TRACE.

**Corrección Obligatoria:**
```c
// En init del firmware, desactivar TRACE explícitamente:
__HAL_AFIO_REMAP_SWJ_NOJTAG();  // Deshabilita JTAG pero mantiene SWD
// O mejor aún, usar pin alternativo para TIM2_CH2
```

**Recomendación:**
- **Opción A:** Mantener PB3 pero documentar claramente la configuración AFIO requerida.
- **Opción B (PREFERIDA):** Reasignar TIM2_CH2 a **PB11** (TIM2_CH4 alternativo) si está libre.

**Impacto en documento:** CRÍTICO - Requiere nota de advertencia y configuración obligatoria.

---

### 1.2 Verificación de Pines Multifunción

**Pines Analizados para Conflictos:**

| Pin | Asignación Documento | Función AF | Conflicto | Estado |
|-----|----------------------|------------|-----------|--------|
| PA0 | ADC1_IN1 | TIM2_CH1_ETR | No | ✅ Correcto (ADC tiene prioridad) |
| PA2 | USART2_TX (opcional) | TIM2_CH3 | No | ✅ OK (UART no interfiere) |
| PA3 | USART2_RX | TIM2_CH4 | No | ✅ OK |
| PA8 | TIM1_CH1 | MCO | No | ✅ OK (PWM tiene prioridad) |
| PA11 | TIM1_CH4 | USB_DM | **SÍ** | ⚠️ USB deshabilitado (documentado) |
| PA15 | TIM2_CH1 | JTDI | No | ✅ OK (JTAG deshabilitado) |
| PB3 | TIM2_CH2 | **TRACESWO** | **SÍ** | ❌ **CRÍTICO** |
| PB4 | GPIO (ENC_Z) | NJTRST | No | ✅ OK (JTAG deshabilitado) |
| PB8 | FDCAN1_RX | TIM4_CH3 | No | ✅ OK (CAN tiene prioridad) |
| PB9 | FDCAN1_TX | TIM4_CH4 | No | ✅ OK |
| PC8 | TIM8_CH3 | N/A | No | ✅ OK |

**Conclusión Sección 1:**  
Un conflicto crítico detectado (PB3). Resto de asignaciones correctas con AF configurables sin conflicto.

---

## 2. PINES SENSIBLES / ESPECIALES

### 2.1 Pines SWD/JTAG

**PA13 (SWDIO) y PA14 (SWCLK):**  
✅ **CORRECTO** - Documentados como reservados.  
✅ No se reutilizan para otra función.  
✅ Permiten programación y debug en producción.

**Recomendación adicional:**  
Añadir en PCB un header 2×5 con pin-out estándar ARM Cortex (10-pin SWD):
```
1: VTref    2: SWDIO (PA13)
3: GND      4: SWCLK (PA14)
5: GND      6: SWO (PB3 - conflicto!)
7: N/C      8: N/C
9: GND     10: nRESET
```

⚠️ **ADVERTENCIA:** SWO típicamente usa PB3 (TRACESWO), que está asignado a encoder. Si se necesita trace en debug, habrá conflicto. Documentar que trace NO está disponible.

---

### 2.2 Pin BOOT0

✅ **CORRECTO** - Documentado con pull-down 10kΩ a GND.  
✅ Configuración adecuada para boot desde Flash.

**Verificación adicional:**  
- Confirmar que pull-down es externo (no confiar solo en resistencia interna débil).
- Añadir capacitor 100nF a GND cerca de BOOT0 para inmunidad a ruido.

---

### 2.3 Pines USB

**PA11 (USB_DM) usado para PWM_RR:**  
✅ **Documentado explícitamente** que USB no está disponible.  
⚠️ Si en futuro se requiere USB, necesita rediseño.

**Recomendación:**  
Añadir nota en PCB silkscreen: "PA11/PA12: No USB".

---

### 2.4 Pines RTC (PC13-PC15)

**Estado:** No utilizados en este diseño.  
✅ **CORRECTO** - Evita conflicto con cristal LSE de 32.768 kHz si se añade en futuro.

**Mejora sugerida:**  
Si se planea RTC en futuro:
- Reservar PC14 (OSC32_IN) y PC15 (OSC32_OUT).
- Documentar que PC13 solo puede ser salida de baja corriente (<3mA).

---

### 2.5 Pines HSE (Cristal Principal)

**Estado:** No documentado en el pinout.

❌ **OMISIÓN IMPORTANTE:**  
No se menciona si se usa cristal externo HSE o reloj interno.

**Análisis:**
- STM32G474RE puede usar HSE (8-48 MHz) o HSI16 interno.
- Para aplicación automotriz con CAN @ 500 kbps, se recomienda **HSE externo** para precisión.
- HSE usa **PF0-OSC_IN** y **PF1-OSC_OUT** (no disponibles en LQFP64).
- En LQFP64, HSE usa **PH0** y **PH1** (no documentados).

**Corrección Obligatoria:**  
Añadir sección explícita sobre configuración de reloj:
- Si usa HSI16: Documentar deriva de ±1% y validar impacto en CAN.
- Si usa HSE: Documentar cristal (ej: 8 MHz, 25 MHz) y capacitores de carga.

**Recomendación para CAN:**  
Usar HSE de **8 MHz** con PLL para alcanzar 170 MHz. Deriva <±50 ppm garantiza CAN confiable.

---

## 3. TIMERS Y PWM

### 3.1 Asignación de Timers Avanzados

**TIM1 - PWM Tracción 4x4:**

| Canal | Pin | Módulo | Estado |
|-------|-----|--------|--------|
| CH1 | PA8 | Motor FL | ✅ Correcto |
| CH2 | PA9 | Motor FR | ✅ Correcto |
| CH3 | PA10 | Motor RL | ✅ Correcto |
| CH4 | PA11 | Motor RR | ✅ Correcto |

**Verificación:**  
✅ Todos los canales de TIM1 en pines correctos (AF6).  
✅ Frecuencia 20 kHz adecuada para BTS7960.  
✅ No hay conflicto entre canales.

**Mejora sugerida:**  
Añadir cálculo explícito de PSC y ARR:
```c
// Para 20 kHz @ 170 MHz:
// TIM_CLK = 170 MHz
// PSC = 0 (sin división)
// ARR = 170,000,000 / 20,000 = 8500
// Resolución efectiva = 13.05 bits (8500 niveles)
```

---

**TIM8 - PWM Dirección:**

| Canal | Pin | Módulo | Estado |
|-------|-----|--------|--------|
| CH3 | PC8 | Motor Dirección | ✅ Correcto (AF4) |

**Verificación:**  
✅ TIM8_CH3 en PC8 es correcto.  
⚠️ Solo usa 1 canal de 4 disponibles en TIM8.

**Oportunidad de expansión:**  
- TIM8_CH1/CH2/CH4 libres para futuras actuaciones PWM de alta calidad.
- Documentar canales reservados para expansión.

---

### 3.2 TIM2 - Encoder Dirección

**Configuración Modo Encoder:**

| Canal | Pin | Señal | Estado |
|-------|-----|-------|--------|
| CH1 | PA15 | ENC_A | ✅ Correcto (AF1) |
| CH2 | **PB3** | ENC_B | ❌ **CONFLICTO TRACESWO** |

**Análisis de Modo Encoder 3:**  
✅ Documentado correctamente (cuenta en ambos flancos de A y B).  
✅ Resolución 4× correcta (360 PPR → 1440 conteos/rev).  
✅ TIM2 es 32-bit (permite conteo extendido sin overflow frecuente).

**Problema:**  
PB3 con TRACESWO puede no funcionar como TIM2_CH2 sin configuración AFIO.

**Corrección:**  
Ver sección 1.1 - Requiere deshabilitar TRACE o reasignar pin.

---

### 3.3 TIM3 - Trigger ADC

**Uso:** Generar trigger para ADC1 @ 200 Hz.

✅ **CORRECTO** - TIM3 no genera PWM, solo evento TRGO.  
✅ Frecuencia 200 Hz adecuada para pedal Hall.  
✅ No hay conflicto de pines (TIM3 sin salida física).

**Mejora sugerida:**  
Documentar configuración TIM3:
```c
TIM3->PSC = 8499;      // 170 MHz / 8500 = 20 kHz
TIM3->ARR = 99;        // 20 kHz / 100 = 200 Hz
TIM3->CR2 |= TIM_CR2_MMS_1;  // Update event → TRGO
```

---

## 4. ADC Y SEÑALES ANALÓGICAS

### 4.1 Pedal Hall - ADC1_IN1 (PA0)

**Configuración Documentada:**
- ADC: ADC1, Canal IN1
- Resolución: 12-bit
- Trigger: TIM3 @ 200 Hz
- DMA: Sí

✅ **CORRECTO** - Configuración adecuada para señal crítica.

---

### 4.2 ERROR CRÍTICO: Divisor Resistivo

**Problema Detectado en Sección 3:**

El documento propone un divisor resistivo:
```
Sensor Hall (5V) → Divisor → ADC (3.3V)
R1 = 10kΩ
R2 = 6.8kΩ
Vout_ADC = Vin × (R2 / (R1 + R2)) = Vin × 0.4
→ 4.5V × 0.4 = 1.8V
```

**Análisis del Error:**

| Parámetro | Sensor Hall | Con Divisor | Pérdida |
|-----------|-------------|-------------|---------|
| Rango de salida | 0.5 - 4.5 V (4.0 V útil) | 0.2 - 1.8 V (1.6 V útil) | 60% |
| Resolución ADC (12-bit) | 4096 niveles | 4096 niveles | 0% |
| Resolución efectiva | 4.0 V / 4096 = 0.98 mV/LSB | 1.6 V / 4096 = **0.39 mV/LSB** | **60%** |
| Niveles útiles | ~4000 LSB | ~1600 LSB | 60% desperdiciado |

**Impacto:**
- ❌ Se desperdicia 60% de la resolución del ADC.
- ❌ Ruido relativo aumenta (menor señal, mismo ruido absoluto).
- ❌ Peor relación señal/ruido (SNR).

**Corrección Obligatoria:**

**Opción A (RECOMENDADA): Alimentar sensor Hall a 3.3V**
```
Sensor Hall alimentado a 3.3V:
- Vout = 0.33 - 3.0 V (aprox, según sensor)
- Rango útil: ~2.7 V
- Resolución: 2.7 V / 4096 = 0.66 mV/LSB
- Niveles útiles: ~4000 LSB (100% ADC)
- NO requiere divisor resistivo
```

**Opción B: Divisor optimizado**
```
Si sensor DEBE ser 5V:
R1 = 3.3kΩ
R2 = 5.6kΩ
Factor = 5.6 / (3.3 + 5.6) = 0.629
4.5V × 0.629 = 2.83V (dentro de 3.3V)
Rango útil: 0.31 - 2.83 V (2.52 V)
Resolución: 2.52 V / 4096 = 0.62 mV/LSB
Niveles útiles: ~3700 LSB (90% ADC)
```

**Opción C (IDEAL): ADC tolerante a 5V**

STM32G474RE tiene **ADC 5V-tolerant** en algunos pines si:
- Pin configurado como entrada analógica.
- VDDA = 3.3V.
- Usar ADC con **VREF+ externo de 5V** (si disponible).

**Verificación Datasheet:**  
PA0 NO está marcado como 5V-tolerant en modo analógico.  
Por tanto, **Opción A o B son obligatorias**.

**Recomendación Final:**  
**Opción A** - Alimentar sensor Hall a 3.3V. Simplifica hardware, maximiza resolución.

---

### 4.3 Filtro RC

**Filtro documentado:**
- R = 10kΩ
- C = 100nF
- fc = 1 / (2π × 10k × 100nF) = **159 Hz**

**Análisis:**
✅ Frecuencia de corte 159 Hz adecuada para señal de pedal (cambios lentos).  
✅ Atenúa ruido >200 Hz (PWM @ 20 kHz muy atenuado).

**Mejora sugerida:**  
Añadir capacitor adicional de 10nF en paralelo con 100nF para mejor supresión de picos de alta frecuencia (EMI).

---

### 4.4 Calibración ADC

**Valores documentados:**
- Mínimo: ADC ≈ 620 (0.5 V escalado)
- Máximo: ADC ≈ 3720 (3.0 V escalado)

⚠️ **INCOHERENCIA:**  
Si se usa divisor ×0.4:
- 0.5 V → 0.2 V → ADC = 0.2/3.3 × 4096 = **248** (no 620)
- 3.0 V → 1.2 V → ADC = 1.2/3.3 × 4096 = **1489** (no 3720)

**Conclusión:**  
Los valores de calibración son **incorrectos** si se usa el divisor propuesto.  
Confirma que el divisor resistivo es un error de diseño.

**Corrección:**  
Actualizar valores de calibración según opción elegida (A o B).

---

## 5. INTERRUPCIONES (EXTI)

### 5.1 Asignación de Líneas EXTI

**EXTI Utilizadas:**

| EXTI Line | Pin | Módulo | Prioridad |
|-----------|-----|--------|-----------|
| EXTI0 | PB0 | WHEEL_FL | Media |
| EXTI1 | PB1 | WHEEL_FR | Media |
| EXTI2 | PB2 | WHEEL_RL | Media |
| EXTI4 | PB4 | ENC_Z (index) | Alta |
| EXTI10 | PB10 | WHEEL_RR | Media |
| EXTI15 | PB15 | KEY_ON | Baja |

**Verificación:**  
✅ No hay conflictos (cada línea EXTI puede conectarse a un solo pin a la vez).  
✅ Distribución razonable de interrupciones.

---

### 5.2 Agrupación de EXTI

**Handlers en STM32G4:**
- **EXTI0_IRQHandler** → EXTI0 (PB0 - WHEEL_FL)
- **EXTI1_IRQHandler** → EXTI1 (PB1 - WHEEL_FR)
- **EXTI2_IRQHandler** → EXTI2 (PB2 - WHEEL_RL)
- **EXTI4_IRQHandler** → EXTI4 (PB4 - ENC_Z)
- **EXTI15_10_IRQHandler** → EXTI10 (PB10 - WHEEL_RR) y EXTI15 (PB15 - KEY_ON)

⚠️ **ADVERTENCIA:**  
EXTI10 y EXTI15 **comparten el mismo handler** (EXTI15_10_IRQHandler).  
Si ambas interrupciones ocurren simultáneamente, el handler debe discriminar cuál disparó.

**Código típico:**
```c
void EXTI15_10_IRQHandler(void) {
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_10)) {
        // WHEEL_RR
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_10);
        handleWheelRR();
    }
    if (__HAL_GPIO_EXTI_GET_IT(GPIO_PIN_15)) {
        // KEY_ON
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_15);
        handleKeyOn();
    }
}
```

✅ **ACEPTABLE** - KEY_ON es evento raro (arranque/apagado), WHEEL_RR es periódico pero no crítico si comparte handler.

---

### 5.3 Latencia de Interrupciones

**Frecuencia esperada de sensores de rueda:**
- Velocidad máxima: 10 km/h = 2.78 m/s
- Rueda Ø = 0.3 m → Circunferencia = 0.942 m
- Revoluciones/s = 2.78 / 0.942 = 2.95 rps
- Con 10 pulsos/rev → **29.5 Hz por rueda**
- 4 ruedas → **118 interrupciones/s total**

**Análisis:**  
✅ Frecuencia baja (118 Hz) - STM32 @ 170 MHz puede manejar sin problema.  
✅ Latencia de ISR típica <10 µs.

**Mejora sugerida:**  
Implementar debounce en software (ignorar pulsos <500 µs) para eliminar ruido.

---

## 6. CAN BUS

### 6.1 Pines CAN

**Asignación:**
- CAN_TX: PB9 (FDCAN1_TX, AF9)
- CAN_RX: PB8 (FDCAN1_RX, AF9)

✅ **CORRECTO** - Pines válidos para FDCAN1.  
✅ AF9 correcta para FDCAN1.

**Verificación Datasheet:**  
Alternativas FDCAN1:
- PA11/PA12 (usado para PWM/USB)
- **PB8/PB9** ← Opción elegida ✅
- PD0/PD1 (no disponibles en LQFP64)

---

### 6.2 Velocidad CAN

**Documentado:** 500 kbps

✅ **CORRECTO** para cables >3 m y múltiples nodos.  
✅ Permite comunicación robusta con ESP32.

**Validación de Timing:**

Para 500 kbps con PCLK = 170 MHz:
```
Bit Time = 1 / 500k = 2 µs
Prescaler típico = 17 → TQ = 170 MHz / 17 = 10 MHz → TQ = 100 ns
Bit Time = 20 TQ = 20 × 100 ns = 2 µs ✅

Segmentos típicos:
SYNC_SEG = 1 TQ
PROP_SEG = 5 TQ
PHASE_SEG1 = 8 TQ
PHASE_SEG2 = 6 TQ
Total = 20 TQ = 2 µs
```

✅ Timing coherente con 500 kbps.

---

### 6.3 Terminación CAN

**Documentado:** 120Ω en ambos extremos del bus.

✅ **CORRECTO** - Estándar ISO 11898-2.

**Mejora sugerida:**  
Añadir capacitor de 100pF en paralelo con terminación para suprimir reflexiones de alta frecuencia (opcional pero recomendado en entornos con EMI).

---

### 6.4 Mensajes CAN y Ancho de Banda

**Mensajes principales:**

| ID | Dirección | Frecuencia | Bytes | Throughput |
|----|-----------|------------|-------|------------|
| 0x100 | ESP32→STM32 | 20 ms (50 Hz) | 8 | 400 bytes/s |
| 0x110 | STM32→ESP32 | 20 ms (50 Hz) | 8 | 400 bytes/s |
| 0x111 | STM32→ESP32 | 50 ms (20 Hz) | 8 | 160 bytes/s |
| 0x130 | ESP32→STM32 | 100 ms (10 Hz) | 2 | 20 bytes/s |
| 0x131 | STM32→ESP32 | 100 ms (10 Hz) | 2 | 20 bytes/s |
| **Total** | | | | **~1000 bytes/s** |

**Cálculo de utilización del bus:**

CAN frame overhead (aproximado):
- Datos: 8 bytes
- Header + CRC + ACK + EOF: ~14 bytes
- Total por frame: ~22 bytes = 176 bits

Utilización @ 500 kbps:
- 1000 bytes/s datos → ~2750 bytes/s con overhead → **22000 bits/s**
- Utilización: 22000 / 500000 = **4.4%**

✅ **EXCELENTE** - Bus CAN con <5% utilización tiene margen enorme para expansión.

---

## 7. I²C Y ONE-WIRE

### 7.1 I²C1 - INA226

**Pines:**
- SCL: PB6 (I2C1_SCL, AF4)
- SDA: PB7 (I2C1_SDA, AF4)

✅ **CORRECTO** - Pines válidos para I2C1.

**Pull-up documentado:** 2.2kΩ

✅ **ADECUADO** para 400 kHz (Fast Mode).

**Cálculo de rise time:**
```
tr_max = 300 ns (Fast Mode I²C)
C_bus ≈ 100 pF (estimado, 4 sensores + trazas)
R_pullup = 2.2kΩ
tr ≈ 2.2 × C_bus × R_pullup = 2.2 × 100pF × 2.2kΩ = 484 ns
```

⚠️ **ADVERTENCIA:** Rise time de 484 ns excede 300 ns de Fast Mode.

**Corrección:**  
Reducir pull-up a **1.5kΩ** para tr ≈ 330 ns (dentro de margen).

---

### 7.2 Número de Dispositivos I²C

**Documentado:** 6 dispositivos INA226 en I2C1.

✅ **ACEPTABLE** - I²C soporta hasta 112 dispositivos teóricamente.  
⚠️ Capacitancia total puede ser límite.

**Análisis:**
- 6 INA226 × ~10pF cada uno = 60pF
- Trazas PCB: ~50pF
- Total estimado: **110pF**

Con pull-up 1.5kΩ → tr ≈ 363 ns → Aceptable.

**Mejora sugerida:**  
Si en futuro se añaden más dispositivos I²C, considerar multiplexor TCA9548A.

---

### 7.3 OneWire - DS18B20

**Pin:** PB5 (GPIO open-drain)  
**Pull-up:** 4.7kΩ (externo)

✅ **CORRECTO** - Configuración estándar OneWire.

**Verificación:**  
PB5 no tiene funciones alternativas conflictivas importantes.  
✅ Adecuado para OneWire.

**Mejora sugerida:**  
Añadir capacitor 100nF cerca del pin PB5 para filtrar ruido EMI de motores.

---

## 8. SEÑALES DE SEGURIDAD CRÍTICAS

### 8.1 Pedal Hall

**Criticidad:** **MUY ALTA**  
**Análisis:** Ver sección 4 (ADC).

**Validaciones documentadas:**
✅ Rechazo fuera de rango  
✅ Detección stuck-at  
✅ Coherencia temporal

**Mejora sugerida:**  
Añadir validación de **tasa de cambio máxima**:
```c
// Pedal físico no puede cambiar >50% en 100ms
if (abs(pedal_new - pedal_old) > 2048 && dt < 100ms) {
    // Error: Cambio imposible → Fallo de sensor
    return PEDAL_FAULT;
}
```

---

### 8.2 Palanca de Cambios

**Estados válidos documentados:**  
✅ Tabla de estados correcta.  
✅ Detección de estados inválidos.

**Debounce documentado:** 3 lecturas × 10 ms

✅ **ADECUADO** para switches mecánicos.

**Mejora sugerida:**  
Añadir timeout de transición:
```c
// Si estado inválido persiste >100ms → Fallo mecánico
if (invalid_state_duration > 100ms) {
    // Forzar NEUTRAL y generar código de fallo
}
```

---

### 8.3 Llave de Contacto

**Pin:** PB15 (EXTI15)  
**Trigger:** Both edges

✅ **CORRECTO** - Detecta encendido y apagado.

**Lógica power-hold documentada:**  
✅ Correcta - MCU mantiene RELAY_MAIN activo.

⚠️ **RIESGO:**  
Si STM32 falla y RELAY_MAIN queda en HIGH, sistema no se puede apagar.

**Mitigación obligatoria:**  
Añadir en PCB:
- **Watchdog hardware externo** (ej: MAX6369) que resetee RELAY_MAIN si STM32 no responde.
- O **timeout RC** que desactive RELAY_MAIN tras ~10 segundos sin refresh.

---

### 8.4 Relés de Potencia

**Estados por defecto documentados:** Todos LOW

✅ **CORRECTO** - Fail-safe por diseño.

**Verificación:**  
GPIO en reset están en modo input pull-down → LOW por defecto.  
✅ Coherente con seguridad.

**Mejora sugerida:**  
Añadir resistencias pull-down externas (10kΩ) en PC11, PC12, PD2 para garantizar LOW incluso si pin queda flotante.

---

## 9. COHERENCIA ARQUITECTÓNICA

### 9.1 Separación STM32 (Control) vs ESP32 (HMI)

✅ **EXCELENTE** - El diseño respeta completamente la arquitectura dual:
- STM32: Todos los sensores críticos, actuadores, seguridad.
- ESP32: Solo HMI por CAN (no documentado en este pinout, correcto).

✅ No hay "fugas" de funciones críticas a ESP32.

---

### 9.2 Sensores Críticos en STM32

**Sensores críticos correctamente asignados a STM32:**
- ✅ Pedal Hall (ADC1)
- ✅ Encoder dirección (TIM2)
- ✅ Sensores rueda (EXTI)
- ✅ Corrientes (I²C)
- ✅ Temperaturas (OneWire)
- ✅ TOFSense-M (UART)

✅ **CORRECTO** - Decisión arquitectónica coherente.

---

### 9.3 Determinismo Temporal

**Elementos que garantizan determinismo:**
- ✅ ADC con trigger por timer (TIM3)
- ✅ Encoder por timer hardware (TIM2)
- ✅ PWM por timers avanzados (TIM1, TIM8)
- ✅ CAN por hardware (FDCAN1)

✅ **EXCELENTE** - No depende de polling ni timing variable.

---

## 10. LISTA DE CORRECCIONES

### 10.1 ERRORES CRÍTICOS (Obligatorio Corregir)

#### **ERROR 1: Conflicto PB3 (TRACESWO vs TIM2_CH2)**

**Gravedad:** 🔴 **CRÍTICA**

**Problema:**  
PB3 usado para encoder dirección (TIM2_CH2) puede conflictuar con TRACESWO.

**Impacto:**  
Encoder de dirección puede no funcionar correctamente.

**Corrección:**
- **Opción A:** Añadir configuración AFIO obligatoria en firmware para deshabilitar TRACE.
- **Opción B (RECOMENDADA):** Reasignar TIM2_CH2 a pin alternativo (ej: PB11 como TIM2_CH4 si no se requiere cuadratura exacta, o usar otro timer).

**Acción:** Actualizar documento con nota de advertencia y configuración AFIO, O reasignar pin.

---

#### **ERROR 2: Divisor Resistivo Pedal Hall**

**Gravedad:** 🔴 **CRÍTICA**

**Problema:**  
Divisor ×0.4 desperdicia 60% de resolución ADC.

**Impacto:**  
Degradación de precisión del pedal (señal crítica de control).

**Corrección:**
- **Recomendada:** Alimentar sensor Hall a 3.3V (sin divisor).
- **Alternativa:** Divisor optimizado (R1=3.3k, R2=5.6k) para factor ~0.63.

**Acción:** Eliminar divisor ×0.4 del documento y recalcular calibración.

---

#### **ERROR 3: Valores de Calibración ADC Incorrectos**

**Gravedad:** 🟡 **MODERADA**

**Problema:**  
Valores de calibración (620, 3720) no coinciden con divisor propuesto.

**Impacto:**  
Confusión en implementación de firmware.

**Corrección:**  
Recalcular valores según opción final de alimentación del sensor (3.3V o 5V con divisor).

**Acción:** Actualizar tabla de calibración con valores correctos.

---

### 10.2 MEJORAS RECOMENDADAS (No Bloqueantes)

#### **MEJORA 1: Reloj HSE**

**Problema:** No documentado si se usa HSE o HSI.

**Recomendación:** Añadir sección sobre configuración de reloj (HSE 8 MHz recomendado para CAN).

---

#### **MEJORA 2: Pull-up I²C**

**Problema:** 2.2kΩ puede dar rise time >300 ns.

**Recomendación:** Reducir a 1.5kΩ para Fast Mode confiable.

---

#### **MEJORA 3: Watchdog Hardware Externo**

**Problema:** RELAY_MAIN puede quedar activo si STM32 falla.

**Recomendación:** Añadir MAX6369 o similar para resetear relé si STM32 no responde.

---

#### **MEJORA 4: Pull-down Externos en Relés**

**Problema:** Confianza en pull-down interno de GPIO.

**Recomendación:** Añadir resistencias 10kΩ externas en PC11, PC12, PD2.

---

#### **MEJORA 5: Capacitor en BOOT0**

**Problema:** BOOT0 sensible a ruido.

**Recomendación:** Añadir 100nF a GND cerca de BOOT0.

---

#### **MEJORA 6: Documentar Cálculos PWM**

**Problema:** PSC y ARR no documentados explícitamente.

**Recomendación:** Añadir fórmulas de cálculo para TIM1/TIM8.

---

#### **MEJORA 7: Debounce Sensores de Rueda**

**Problema:** No documentado en firmware.

**Recomendación:** Añadir nota sobre implementación de debounce (<500 µs).

---

## 11. CONFIRMACIÓN FINAL

### 11.1 ¿Puede Usarse como Manual Eléctrico?

**Respuesta:** ✅ **SÍ, CON CORRECCIONES**

El documento es **muy completo y bien estructurado**, adecuado como base para:
- Diseño de PCB
- Cableado de sistema
- Programación de firmware

**Pero requiere:**
- Resolver conflicto PB3 (CRÍTICO)
- Corregir divisor pedal Hall (CRÍTICO)
- Añadir sección HSE (IMPORTANTE)
- Implementar mejoras recomendadas (OPCIONAL pero aconsejable)

---

### 11.2 Cambios Necesarios Antes del Design Freeze

**Obligatorios:**
1. ✅ Resolver conflicto PB3 (reasignar pin o documentar AFIO)
2. ✅ Eliminar divisor ×0.4 del pedal Hall
3. ✅ Actualizar valores de calibración ADC
4. ✅ Añadir sección de configuración de reloj (HSE/HSI)

**Recomendados:**
5. ⚠️ Reducir pull-up I²C a 1.5kΩ
6. ⚠️ Añadir watchdog hardware externo para RELAY_MAIN
7. ⚠️ Añadir pull-down externos en relés

**Opcionales (mejoran documentación):**
8. ℹ️ Añadir cálculos explícitos de PSC/ARR de timers
9. ℹ️ Documentar debounce de sensores de rueda
10. ℹ️ Añadir capacitor en BOOT0

---

## 12. TABLA RESUMEN DE HALLAZGOS

| # | Tipo | Gravedad | Sección | Problema | Estado |
|---|------|----------|---------|----------|--------|
| 1 | Error | 🔴 Crítica | Pines | PB3 conflicto TRACESWO | ❌ Requiere corrección |
| 2 | Error | 🔴 Crítica | ADC | Divisor resistivo ineficiente | ❌ Requiere corrección |
| 3 | Error | 🟡 Moderada | ADC | Calibración incorrecta | ❌ Requiere corrección |
| 4 | Omisión | 🟡 Moderada | Clock | HSE no documentado | ⚠️ Añadir sección |
| 5 | Mejora | 🟢 Baja | I²C | Pull-up subóptimo | ℹ️ Recomendada |
| 6 | Mejora | 🟡 Moderada | Seguridad | Falta watchdog HW | ⚠️ Recomendada |
| 7 | Mejora | 🟢 Baja | Seguridad | Falta pull-down externo | ℹ️ Recomendada |

---

## CONCLUSIÓN

El documento **STM32G474RE_PINOUT_DEFINITIVO.md** es un excelente punto de partida con arquitectura sólida y bien pensada.

**Fortalezas:**
- ✅ Separación clara STM32 (control) vs ESP32 (HMI)
- ✅ Uso correcto de timers avanzados para PWM
- ✅ Asignación determinista de sensores críticos
- ✅ Documentación estructurada y legible

**Debilidades:**
- ❌ Conflicto PB3 sin resolver
- ❌ Diseño subóptimo del divisor resistivo
- ❌ Omisión de configuración de reloj

**Veredicto Final:**  
**APTO PARA PRODUCCIÓN DESPUÉS DE IMPLEMENTAR LAS 4 CORRECCIONES OBLIGATORIAS.**

Las mejoras recomendadas añaden robustez pero no son bloqueantes.

---

**Firma de Auditoría:**  
Ingeniero Senior Hardware/Firmware Embebido  
Especialista en Sistemas Automotrices y Seguridad Funcional  
Fecha: 2026-01-22

**Próximos Pasos:**
1. Implementar correcciones críticas (1-4)
2. Revisar documento actualizado
3. Aprobación final
4. Design freeze y fabricación PCB
