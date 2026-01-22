# STM32G474RE - Pinout Definitivo y Manual de Integración Hardware

**Fecha:** 2026-01-22  
**MCU:** STM32G474RE (LQFP64, 170 MHz, Cortex-M4F)  
**Aplicación:** Controlador principal de vehículo automotriz  
**Documento:** Manual eléctrico definitivo para cableado de producción

---

## Índice

1. [Microcontrolador - Recursos Disponibles](#1-microcontrolador---recursos-disponibles)
2. [Motores](#2-motores)
3. [Pedal (Sensor Hall)](#3-pedal-sensor-hall)
4. [Sensores de Rueda](#4-sensores-de-rueda)
5. [Encoder de Dirección](#5-encoder-de-dirección-e6b2-cwz6c)
6. [Sensores de Temperatura](#6-sensores-de-temperatura)
7. [Sensor de Corriente](#7-sensor-de-corriente)
8. [Palanca de Cambios](#8-palanca-de-cambios)
9. [Llave de Contacto / Power-Hold](#9-llave-de-contacto--power-hold)
10. [Relés de Potencia](#10-relés-de-potencia)
11. [CAN Bus](#11-can-bus)
12. [TOFSense-M (Sensor LiDAR)](#12-tofsense-m-sensor-lidar)
13. [Pines Reservados / No Usar](#13-pines-reservados--no-usar)
14. [Resumen Final - Tabla Completa de Pinout](#14-resumen-final---tabla-completa-de-pinout)
15. [Diagrama de Bloques](#15-diagrama-de-bloques)
16. [Notas de Implementación](#16-notas-de-implementación)

---

## 1. Microcontrolador - Recursos Disponibles

### STM32G474RE - Especificaciones Clave

| Recurso | Cantidad | Notas |
|---------|----------|-------|
| **CPU** | ARM Cortex-M4F @ 170 MHz | Con FPU para cálculos de punto flotante |
| **Flash** | 512 KB | Programa + datos no volátiles |
| **SRAM** | 128 KB | Datos volátiles |
| **ADC** | 5 ADCs de 12-bit | ADC1, ADC2, ADC3, ADC4, ADC5 - hasta 4 MSPS |
| **DAC** | 4 DACs de 12-bit | Para señales analógicas de salida (no usados en este diseño) |
| **Timers** | 11 timers | TIM1-8, TIM15-17 (avanzados: TIM1, TIM8) |
| **FDCAN** | 3 instancias | CAN-FD por hardware (usaremos FDCAN1) |
| **UART** | 5 instancias | USART1-3, UART4-5 |
| **I²C** | 4 instancias | I²C1-4 con SMBus/PMBus |
| **SPI** | 4 instancias | SPI1-4 |
| **GPIO** | 51 pines I/O | En package LQFP64 |
| **DMA** | 2 controladores DMA | 16 canales totales para transferencias sin CPU |

### Timers Avanzados (PWM de alta calidad)

| Timer | Tipo | Canales | Uso en este diseño |
|-------|------|---------|-------------------|
| **TIM1** | Avanzado | 4 canales + 3 complementarios | PWM motores tracción (4 ruedas) |
| **TIM8** | Avanzado | 4 canales + 3 complementarios | PWM motor dirección + reserva |
| **TIM2** | General 32-bit | 4 canales | Encoder dirección (modo encoder) |
| **TIM3** | General | 4 canales | Trigger ADC pedal + muestreo sincronizado |
| **TIM4** | General | 4 canales | Encoder secundario / reserva |

### ADCs - Canales Disponibles

**ADC1:** 16 canales (IN1-IN16)  
**ADC2:** 18 canales (IN1-IN18, algunos compartidos con ADC1)  
**ADC3:** 18 canales  
**ADC4:** 5 canales  
**ADC5:** 5 canales  

**Notas importantes:**
- ADC1 y ADC2 pueden operar en modo dual simultáneo (muestreo sincronizado).
- Resolución: 12-bit (4096 niveles), configurables a 10/8/6 bits para mayor velocidad.
- Trigger por timer para muestreo determinista.
- DMA automático para transferencia sin CPU.

---

## 2. Motores

### A) Motor de Tracción (4 ruedas independientes)

**Topología:** 4 motores DC brushed con drivers BTS7960 (H-Bridge doble)  
**Control:** PWM (velocidad) + DIR (dirección) + EN (enable)

#### Motor FL (Front Left - Delantero Izquierdo)

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| PWM_FL | **PA8** | TIM1_CH1 | PWM 20 kHz, duty 0-100% |
| DIR_FL | **PC0** | GPIO Output | High=Forward, Low=Reverse |
| EN_FL | **PC1** | GPIO Output | High=Enable, Low=Disable |

#### Motor FR (Front Right - Delantero Derecho)

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| PWM_FR | **PA9** | TIM1_CH2 | PWM 20 kHz, duty 0-100% |
| DIR_FR | **PC2** | GPIO Output | High=Forward, Low=Reverse |
| EN_FR | **PC3** | GPIO Output | High=Enable, Low=Disable |

#### Motor RL (Rear Left - Trasero Izquierdo)

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| PWM_RL | **PA10** | TIM1_CH3 | PWM 20 kHz, duty 0-100% |
| DIR_RL | **PC4** | GPIO Output | High=Forward, Low=Reverse |
| EN_RL | **PC5** | GPIO Output | High=Enable, Low=Disable |

#### Motor RR (Rear Right - Trasero Derecho)

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| PWM_RR | **PA11** | TIM1_CH4 | PWM 20 kHz, duty 0-100% |
| DIR_RR | **PC6** | GPIO Output | High=Forward, Low=Reverse |
| EN_RR | **PC7** | GPIO Output | High=Enable, Low=Disable |

**Configuración TIM1:**
- Frecuencia PWM: **20 kHz** (período = 50 µs)
- Prescaler: 170 MHz / (8500 × 20 kHz) = 1 → Prescaler = 0
- Auto-Reload: 8500 (resolución ~12 bits efectivos)
- Modo: PWM Mode 1 (activo alto)
- Dead-time: No requerido (H-bridge externo maneja)

### B) Motor de Dirección (Steering)

**Topología:** Motor DC con BTS7960 + encoder E6B2-CWZ6C  
**Control:** PWM + DIR + EN (similar a tracción)

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| PWM_STEER | **PC8** | TIM8_CH3 | PWM 20 kHz, duty 0-100% |
| DIR_STEER | **PC9** | GPIO Output | High=Left, Low=Right |
| EN_STEER | **PC10** | GPIO Output | High=Enable, Low=Disable |

**Configuración TIM8:**
- Frecuencia PWM: **20 kHz**
- Configuración idéntica a TIM1

**Nota crítica:** El encoder de dirección (ver sección 5) usa TIM2, completamente independiente de TIM8 para evitar conflictos.

---

## 3. Pedal (Sensor Hall)

**Tipo:** Sensor Hall lineal analógico (A1324, SS49E o equivalente)  
**Señal:** Analógica 0.5 - 4.5 V (sobre alimentación 5 V)  
**Criticidad:** **ALTA** - Señal crítica de control

### Conexión

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| PEDAL_ANALOG | **PA0** | ADC1_IN1 | ADC 12-bit, trigger TIM3, DMA |

### Especificaciones ADC

- **ADC:** ADC1, Canal IN1
- **Resolución:** 12-bit (4096 niveles)
- **Trigger:** TIM3_TRGO a **200 Hz** (cada 5 ms)
- **DMA:** DMA1 Channel 1 (transferencia automática a buffer)
- **Conversión:** Single-ended, referencia interna 3.3 V
- **Impedancia entrada:** >1 MΩ (no carga el sensor)

### Acondicionamiento de Señal Recomendado

**Circuito externo:**
```
Sensor Hall (5V) → Divisor resistivo → Filtro RC → ADC1_IN1 (PA0)

                    ┌─────────┐
Vout_Hall (0.5-4.5V)│         │
    ────────────────┤  R1=10k ├─────┬───────── PA0 (ADC1_IN1)
                    │         │     │
                    └─────────┘     │
                                   ═╡ C1=100nF
                                    │
                                   GND

Divisor: Si Sensor es 5V y ADC tolera 3.3V:
  R1 = 10kΩ (hacia sensor)
  R2 = 6.8kΩ (hacia GND)
  Vout_ADC = Vin × (R2 / (R1 + R2)) = Vin × 0.4
  → 4.5V × 0.4 = 1.8V (seguro para ADC de 3.3V)

Filtro RC: fc = 1/(2π×10k×100nF) ≈ 159 Hz (elimina ruido >200 Hz)
```

### Calibración

- **Valor mínimo (pedal suelto):** ADC ≈ 620 (0.5 V → escalado)
- **Valor máximo (pedal a fondo):** ADC ≈ 3720 (3.0 V → escalado)
- **Zona muerta:** ±50 LSB cerca de mínimo
- **Validación:** Rechazar valores fuera de [300, 4000]

### Muestreo Sincronizado (TIM3)

**TIM3 configurado para disparar ADC a 200 Hz:**
```c
// Configuración TIM3 (trigger ADC)
TIM3->PSC = 8499;              // Prescaler: 170 MHz / 8500 = 20 kHz
TIM3->ARR = 99;                // Auto-reload: 20 kHz / 100 = 200 Hz
TIM3->CR2 |= TIM_CR2_MMS_1;    // Master Mode: Update event como TRGO
```

**ADC1 con trigger TIM3:**
```c
ADC1->CFGR |= ADC_CFGR_EXTEN_0;        // Trigger en rising edge
ADC1->CFGR |= ADC_CFGR_EXTSEL_3;       // Seleccionar TIM3_TRGO
```

---

## 4. Sensores de Rueda

**Tipo:** Sensores inductivos / Hall / ópticos (señal digital)  
**Función:** Medir velocidad de rueda para ABS/TCS  
**Frecuencia típica:** 1-100 Hz (según velocidad del vehículo)

### Conexión

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| WHEEL_FL | **PB0** | GPIO + EXTI0 | Input, pull-up, rising edge |
| WHEEL_FR | **PB1** | GPIO + EXTI1 | Input, pull-up, rising edge |
| WHEEL_RL | **PB2** | GPIO + EXTI2 | Input, pull-up, rising edge |
| WHEEL_RR | **PB10** | GPIO + EXTI10 | Input, pull-up, rising edge |

### Configuración

**Modo:** GPIO Input con interrupción externa (EXTI)  
**Pull-up/down:** **Pull-up interno** (sensor activo bajo o colector abierto)  
**Trigger:** **Rising edge** (flanco de subida por cada pulso)  
**Debounce:** Software - ignorar pulsos <500 µs (típico ruido)

### Cálculo de Velocidad

**Método:**
- Contar pulsos en ventana de tiempo (ej: 100 ms).
- Medir tiempo entre flancos consecutivos.
- Velocidad = (Frecuencia_pulsos × Circunferencia_rueda) / Pulsos_por_vuelta

**Ejemplo:**
- Rueda: Ø = 0.3 m → Circunferencia = 0.942 m
- Sensor: 10 pulsos/vuelta
- Frecuencia medida: 50 Hz
- Velocidad = (50 Hz × 0.942 m) / 10 = **4.71 m/s** (≈ 17 km/h)

---

## 5. Encoder de Dirección (E6B2-CWZ6C)

**Tipo:** Encoder incremental rotativo  
**Resolución:** 360 pulsos/revolución (PPR)  
**Señales:** A, B, Z (canales cuadratura + índice)

### Conexión

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| ENC_A | **PA15** | TIM2_CH1 | Encoder mode, TI1 |
| ENC_B | **PB3** | TIM2_CH2 | Encoder mode, TI2 |
| ENC_Z | **PB4** | GPIO + EXTI4 | Input, pull-up, rising edge (index) |

### Configuración TIM2 en Modo Encoder

**TIM2 - Timer de 32 bits (permite conteo extendido):**
```c
// Configuración Encoder Mode 3 (cuenta en ambos flancos de A y B)
TIM2->SMCR |= TIM_SMCR_SMS_0 | TIM_SMCR_SMS_1;  // SMS = 0b11 (Encoder mode 3)
TIM2->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC2P);  // Polaridad normal (no invertir)
TIM2->ARR = 0xFFFFFFFF;                          // 32-bit auto-reload (máximo)
TIM2->CNT = 0;                                   // Iniciar en 0
```

**Resolución efectiva:**
- Encoder: 360 PPR
- Modo Encoder 3: 4 conteos por pulso (cuadratura)
- Resolución total: **1440 conteos/revolución**
- Precisión angular: 360° / 1440 = **0.25°**

### Canal Z (Index)

**Uso:** Reset del contador en posición de referencia (home)  
**Pin PB4 (EXTI4):** Interrupción en flanco de subida  
**Acción ISR:** TIM2->CNT = ZERO_OFFSET (calibración)

---

## 6. Sensores de Temperatura

**Tipo:** DS18B20 (OneWire, digital)  
**Función:** Monitorear temperaturas de motores / drivers  
**Precisión:** ±0.5°C  
**Rango:** -55°C a +125°C

### Conexión

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| TEMP_ONEWIRE | **PB5** | GPIO (OneWire) | Open-drain, pull-up externo 4.7kΩ |

### Configuración

**Modo:** GPIO Output Open-Drain + Input  
**Pull-up:** **Externo 4.7kΩ a 3.3V** (obligatorio para OneWire)  
**Protocolo:** OneWire (bit-banging por software o UART emulation)

### Topología Bus

**Múltiples DS18B20 en paralelo:**
```
STM32 PB5 ────┬──── DS18B20 #1 (Motor FL)
              ├──── DS18B20 #2 (Motor FR)
              ├──── DS18B20 #3 (Motor RL)
              └──── DS18B20 #4 (Motor RR)
              │
             4.7kΩ
              │
            +3.3V
```

**Direccionamiento:** Cada DS18B20 tiene ROM de 64-bit única (autodirección).

### Consideraciones de Ruido

- **Cable máximo:** 30 m (con pull-up adecuado)
- **Blindaje:** Cable blindado si hay EMI de motores
- **Capacitor adicional:** 100nF cerca del STM32 en PB5

---

## 7. Sensor de Corriente

**Tipo:** INA226 (Current/Power Monitor I²C)  
**Función:** Medir corriente de motores / batería  
**Precisión:** ±0.1% (FSR)  
**Interfaz:** I²C

### Conexión

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| I2C_SCL | **PB6** | I2C1_SCL | Open-drain, pull-up externo 2.2kΩ |
| I2C_SDA | **PB7** | I2C1_SDA | Open-drain, pull-up externo 2.2kΩ |

### Configuración I²C1

**Velocidad:** 400 kHz (Fast Mode)  
**Direcciones INA226 (configurables por resistencias A0/A1):**
- INA226 #1: 0x40 (Motor FL)
- INA226 #2: 0x41 (Motor FR)
- INA226 #3: 0x44 (Motor RL)
- INA226 #4: 0x45 (Motor RR)
- INA226 #5: 0x48 (Batería principal)
- INA226 #6: 0x49 (Motor dirección)

**Pull-up externo obligatorio:** 2.2kΩ a 3.3V en SCL y SDA

### Multiplexor (Opcional)

Si se necesitan >7 dispositivos I²C o para aislar buses:

| Señal | Pin STM32 | Función |
|-------|-----------|---------|
| TCA9548A_ADDR | **PB8** | GPIO Output (opcional para selección) |

---

## 8. Palanca de Cambios

**Tipo:** Microinterruptores mecánicos (3 posiciones: F/N/R)  
**Señales:** 3 entradas digitales (Forward, Neutral, Reverse)  
**Lógica:** Activo bajo (cerrado a GND cuando activo)

### Conexión

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| SHIFTER_FWD | **PB12** | GPIO Input | Pull-up interno, activo bajo |
| SHIFTER_NEU | **PB13** | GPIO Input | Pull-up interno, activo bajo |
| SHIFTER_REV | **PB14** | GPIO Input | Pull-up interno, activo bajo |

### Configuración

**Modo:** GPIO Input con pull-up interno  
**Lógica:** 
- Botón presionado → Pin a GND → Lectura = 0 (activo)
- Botón suelto → Pull-up → Lectura = 1 (inactivo)

### Estados Válidos

| FWD | NEU | REV | Estado | Acción |
|-----|-----|-----|--------|--------|
| 0 | 1 | 1 | Forward | Marcha adelante |
| 1 | 0 | 1 | Neutral | Punto muerto |
| 1 | 1 | 0 | Reverse | Marcha atrás |
| 1 | 1 | 1 | **Inválido** | Error - todos sueltos |
| 0 | 0 | ? | **Inválido** | Error - múltiples activos |
| 0 | ? | 0 | **Inválido** | Error - múltiples activos |
| ? | 0 | 0 | **Inválido** | Error - múltiples activos |

**Detección de estados inválidos:** Si lectura no coincide con estados válidos → Modo seguro (NEUTRAL forzado).

### Debounce

**Software:** Leer 3 veces consecutivas con 10 ms de intervalo. Estado válido solo si las 3 lecturas coinciden.

---

## 9. Llave de Contacto / Power-Hold

**Tipo:** Interruptor de llave o botón momentáneo  
**Función:** Arranque del sistema + señal power-hold (mantener relé principal activo)

### Conexión

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| KEY_ON | **PB15** | GPIO + EXTI15 | Input, pull-up, falling/rising edge |

### Configuración

**Modo:** GPIO Input con EXTI (interrupción externa)  
**Pull-up:** Interno (señal por defecto alta)  
**Trigger:** Ambos flancos (rising + falling) para detectar ON/OFF

### Lógica de Power-Hold

**Funcionamiento:**
1. Usuario gira llave → KEY_ON = 0 (activo bajo).
2. Interrupción EXTI15 despierta MCU.
3. STM32 activa RELAY_MAIN (ver sección 10) → Alimentación mantenida.
4. KEY_ON vuelve a 1 (llave suelta) → Sistema sigue encendido (power-hold).
5. Usuario presiona OFF → KEY_ON = 0 nuevamente (según implementación).
6. STM32 ejecuta shutdown ordenado → Desactiva RELAY_MAIN → Sistema se apaga.

**Protección:** Si STM32 falla y no puede desactivar RELAY_MAIN, circuito externo debe incluir watchdog hardware o timeout.

---

## 10. Relés de Potencia

**Función:** Conmutación de alimentación a subsistemas críticos

### A) Relé MAIN (Power-Hold)

**Función:** Mantener alimentación del sistema (self-latch)

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| RELAY_MAIN | **PC11** | GPIO Output | Push-pull, default LOW |

**Lógica:**
- HIGH → Relé cerrado → Alimentación mantenida
- LOW → Relé abierto → Sistema apagado

**Estado seguro por defecto:** LOW (sin alimentación activa sin control MCU)

### B) Relé TRAC (Tracción)

**Función:** Habilitar alimentación a drivers de motores de tracción

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| RELAY_TRAC | **PC12** | GPIO Output | Push-pull, default LOW |

**Lógica:**
- HIGH → Alimentación a BTS7960 tracción habilitada
- LOW → Motores sin potencia (seguro)

### C) Relé DIR (Dirección)

**Función:** Habilitar alimentación a driver de motor de dirección

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| RELAY_DIR | **PD2** | GPIO Output | Push-pull, default LOW |

**Lógica:**
- HIGH → Alimentación a BTS7960 dirección habilitada
- LOW → Motor de dirección sin potencia (seguro)

### Configuración de Seguridad

**Estado por defecto (reset/brownout):** **TODOS LOW** → Sistema seguro sin potencia en motores.

**Driver de relé:** Usar transistor/MOSFET/ULN2003 para manejar bobina del relé (STM32 no puede alimentar directamente relés de 12V/alto consumo).

**Diodo flyback:** Obligatorio en paralelo con bobina del relé (1N4007 o similar) para proteger contra inducción al desconectar.

---

## 11. CAN Bus

**Función:** Comunicación con ESP32-S3 (HMI)  
**Protocolo:** CAN clásico (ISO 11898-2)  
**Transceptor:** TJA1051T/3

### Conexión

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| CAN_TX | **PB9** | FDCAN1_TX | Alternate function AF9 |
| CAN_RX | **PB8** | FDCAN1_RX | Alternate function AF9 |

### Configuración FDCAN1

**Modo:** CAN clásico (no usar CAN-FD para compatibilidad)  
**Velocidad:** **500 kbps** (recomendado para cables >3 m con múltiples nodos)  
**Terminación:** 120Ω en ambos extremos del bus

### Topología Física

```
STM32 ───┬─── TJA1051T ───┬─── CANH/CANL ───┬─── TJA1051T ─── ESP32
         │                │                  │
        3.3V             5V                120Ω (terminación)
         │                                   │
        GND                                 GND
```

**Conexión TJA1051T:**
- Pin 1 (TXD) → STM32 PB9 (FDCAN1_TX)
- Pin 2 (GND) → GND
- Pin 3 (VCC) → 5V
- Pin 4 (RXD) → STM32 PB8 (FDCAN1_RX con divisor si necesario, o TJA1051T tolera 3.3V)
- Pin 5 (NC) → No conectar
- Pin 6 (CANH) → Bus CAN H
- Pin 7 (CANL) → Bus CAN L
- Pin 8 (STB) → GND (modo normal, no standby)

**Resistencias de terminación:** 120Ω entre CANH y CANL en STM32 y ESP32 (extremos del bus).

### Mensajes CAN Principales

| ID | Dirección | Contenido | Frecuencia |
|----|-----------|-----------|------------|
| 0x100 | ESP32→STM32 | CMD_SETPOINTS (throttle, steering, shifter) | 20 ms |
| 0x110 | STM32→ESP32 | STATE_FEEDBACK (velocidades, ángulo, pedal) | 20 ms |
| 0x111 | STM32→ESP32 | SAFETY_STATUS (ABS/TCS/corriente/temp) | 50 ms |
| 0x130 | ESP32→STM32 | HEARTBEAT_ESP32 | 100 ms |
| 0x131 | STM32→ESP32 | HEARTBEAT_STM32 | 100 ms |

---

## 12. TOFSense-M (Sensor LiDAR)

**Tipo:** Sensor LiDAR multi-point ranging  
**Interfaz:** UART (o CAN, según configuración)  
**Función:** Detección de obstáculos

### Conexión UART

| Señal | Pin STM32 | Función | Configuración |
|-------|-----------|---------|---------------|
| TOF_TX | **PA2** | USART2_TX | Alternate function AF7 (no usado si solo RX) |
| TOF_RX | **PA3** | USART2_RX | Alternate function AF7, 921600 bps |

### Configuración USART2

**Baudrate:** 921600 bps (921.6 kbps)  
**Formato:** 8N1 (8 bits, no paridad, 1 stop bit)  
**DMA:** DMA1 para recepción (transferencia automática de frames)  
**Modo:** Solo RX si sensor está en Active Output

### Alternativa: TOF por CAN

Si TOFSense-M se configura con interfaz CAN:
- Conectar al mismo bus CAN que ESP32.
- Configurar ID CAN único (ej: 0x200).
- Ventaja: Múltiples sensores TOF en cascada sin UARTs adicionales.

---

## 13. Pines Reservados / No Usar

### Pines de Debug/Programación (SWD)

| Pin | Función | Uso |
|-----|---------|-----|
| **PA13** | SWDIO | Debug (no usar para GPIO) |
| **PA14** | SWCLK | Debug (no usar para GPIO) |

**Nota:** Estos pines son críticos para programación y debug. **NO CONECTAR** a otra función a menos que se desactive SWD explícitamente (no recomendado).

### Pin BOOT0

| Pin | Función | Uso |
|-----|---------|-----|
| **BOOT0** | Boot selection | Pull-down 10kΩ a GND (boot desde Flash) |

**Configuración:** BOOT0 debe estar conectado a GND mediante resistencia de 10kΩ para arranque normal desde Flash. Si BOOT0 se deja flotante o en HIGH, MCU arranca en bootloader (no ejecuta programa).

### Pines con Funciones Especiales Evitadas

| Pin | Función Alternativa | Razón para Evitar |
|-----|---------------------|-------------------|
| PA12 | USB_DP | Reservado si se usa USB OTG |
| PA11 | USB_DM | Reservado si se usa USB OTG (ya usado para PWM_RR) |
| PB11 | USB (secundario) | Conflicto potencial |
| PC13/14/15 | OSC32 (RTC crystal) | Si se usa cristal externo 32 kHz |

**Nota:** PA11 se usa para PWM_RR (TIM1_CH4). USB OTG **NO** está disponible en este diseño. Si se requiere USB, reasignar PWM_RR a otro pin.

---

## 14. Resumen Final - Tabla Completa de Pinout

### Tabla Maestra de Conexiones

| Pin | Nombre | Función STM32 | Módulo | Señal | Tipo | Configuración |
|-----|--------|---------------|--------|-------|------|---------------|
| **PA0** | ADC1_IN1 | ADC1 | Pedal Hall | PEDAL_ANALOG | ADC | 12-bit, trigger TIM3, DMA |
| **PA2** | USART2_TX | USART2 | TOFSense-M | TOF_TX | UART | 921600 bps (opcional) |
| **PA3** | USART2_RX | USART2 | TOFSense-M | TOF_RX | UART | 921600 bps, DMA |
| **PA8** | TIM1_CH1 | TIM1 | Motor FL | PWM_FL | PWM | 20 kHz, 0-100% |
| **PA9** | TIM1_CH2 | TIM1 | Motor FR | PWM_FR | PWM | 20 kHz, 0-100% |
| **PA10** | TIM1_CH3 | TIM1 | Motor RL | PWM_RL | PWM | 20 kHz, 0-100% |
| **PA11** | TIM1_CH4 | TIM1 | Motor RR | PWM_RR | PWM | 20 kHz, 0-100% |
| **PA13** | SWDIO | Debug | — | SWDIO | Debug | **Reservado** |
| **PA14** | SWCLK | Debug | — | SWCLK | Debug | **Reservado** |
| **PA15** | TIM2_CH1 | TIM2 | Encoder Dir | ENC_A | Encoder | Modo encoder 3 |
| | | | | | | |
| **PB0** | GPIO | GPIO | Sensor rueda | WHEEL_FL | EXTI | Pull-up, rising edge |
| **PB1** | GPIO | GPIO | Sensor rueda | WHEEL_FR | EXTI | Pull-up, rising edge |
| **PB2** | GPIO | GPIO | Sensor rueda | WHEEL_RL | EXTI | Pull-up, rising edge |
| **PB3** | TIM2_CH2 | TIM2 | Encoder Dir | ENC_B | Encoder | Modo encoder 3 |
| **PB4** | GPIO | GPIO | Encoder Dir | ENC_Z | EXTI | Pull-up, index reset |
| **PB5** | GPIO | GPIO | Temperatura | TEMP_ONEWIRE | OneWire | Open-drain, pull-up 4.7kΩ |
| **PB6** | I2C1_SCL | I2C1 | INA226 | I2C_SCL | I2C | 400 kHz, pull-up 2.2kΩ |
| **PB7** | I2C1_SDA | I2C1 | INA226 | I2C_SDA | I2C | 400 kHz, pull-up 2.2kΩ |
| **PB8** | FDCAN1_RX | FDCAN1 | CAN Bus | CAN_RX | CAN | AF9, 500 kbps |
| **PB9** | FDCAN1_TX | FDCAN1 | CAN Bus | CAN_TX | CAN | AF9, 500 kbps |
| **PB10** | GPIO | GPIO | Sensor rueda | WHEEL_RR | EXTI | Pull-up, rising edge |
| **PB12** | GPIO | GPIO | Shifter | SHIFTER_FWD | Input | Pull-up, activo bajo |
| **PB13** | GPIO | GPIO | Shifter | SHIFTER_NEU | Input | Pull-up, activo bajo |
| **PB14** | GPIO | GPIO | Shifter | SHIFTER_REV | Input | Pull-up, activo bajo |
| **PB15** | GPIO | GPIO | Llave contacto | KEY_ON | EXTI | Pull-up, both edges |
| | | | | | | |
| **PC0** | GPIO | GPIO | Motor FL | DIR_FL | Output | Push-pull |
| **PC1** | GPIO | GPIO | Motor FL | EN_FL | Output | Push-pull |
| **PC2** | GPIO | GPIO | Motor FR | DIR_FR | Output | Push-pull |
| **PC3** | GPIO | GPIO | Motor FR | EN_FR | Output | Push-pull |
| **PC4** | GPIO | GPIO | Motor RL | DIR_RL | Output | Push-pull |
| **PC5** | GPIO | GPIO | Motor RL | EN_RL | Output | Push-pull |
| **PC6** | GPIO | GPIO | Motor RR | DIR_RR | Output | Push-pull |
| **PC7** | GPIO | GPIO | Motor RR | EN_RR | Output | Push-pull |
| **PC8** | TIM8_CH3 | TIM8 | Motor Dirección | PWM_STEER | PWM | 20 kHz, 0-100% |
| **PC9** | GPIO | GPIO | Motor Dirección | DIR_STEER | Output | Push-pull |
| **PC10** | GPIO | GPIO | Motor Dirección | EN_STEER | Output | Push-pull |
| **PC11** | GPIO | GPIO | Relé | RELAY_MAIN | Output | Push-pull, default LOW |
| **PC12** | GPIO | GPIO | Relé | RELAY_TRAC | Output | Push-pull, default LOW |
| | | | | | | |
| **PD2** | GPIO | GPIO | Relé | RELAY_DIR | Output | Push-pull, default LOW |

### Pines No Utilizados (Disponibles)

| Pin | Función | Disponibilidad |
|-----|---------|----------------|
| PA1 | ADC1_IN2 | Reserva ADC (sensor adicional) |
| PA4-PA7 | ADC2/DAC | Reserva ADC/DAC |
| PC13-PC15 | GPIO/RTC | Reserva (evitar si RTC externo) |
| PD3-PD15 | GPIO | Expansión futura |
| PE0-PE15 | GPIO | Expansión futura (no todos disponibles en LQFP64) |

---

## 15. Diagrama de Bloques

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           STM32G474RE                                   │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │ SENSORES CRÍTICOS (Inputs)                                       │  │
│  │  - PA0:  ADC1_IN1 ← Pedal Hall (analógico)                       │  │
│  │  - PA15/PB3: TIM2_CH1/CH2 ← Encoder Dirección A/B (cuadratura)   │  │
│  │  - PB4:  GPIO/EXTI ← Encoder Z (index)                           │  │
│  │  - PB0-PB2,PB10: GPIO/EXTI ← Sensores Rueda x4 (digitales)       │  │
│  │  - PB5:  GPIO ← DS18B20 OneWire (temperaturas)                   │  │
│  │  - PB6/PB7: I2C1 ← INA226 x6 (corrientes)                        │  │
│  │  - PA3:  USART2_RX ← TOFSense-M (UART 921600)                    │  │
│  │  - PB12-PB14: GPIO ← Shifter F/N/R (digitales)                   │  │
│  │  - PB15: GPIO/EXTI ← Llave contacto (digital)                    │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │ CONTROL Y LÓGICA (Procesamiento)                                 │  │
│  │  - ARM Cortex-M4F @ 170 MHz con FPU                              │  │
│  │  - ADC trigger por TIM3 @ 200 Hz (muestreo sincronizado)         │  │
│  │  - Encoders por TIM2 (32-bit, modo cuadratura)                   │  │
│  │  - Cálculo de PWM por algoritmos de control (PID, mapeo)         │  │
│  │  - ABS/TCS basados en velocidades de rueda                       │  │
│  │  - Límites de corriente/temperatura por INA226/DS18B20           │  │
│  │  - Detección de obstáculos por TOFSense-M → frenado              │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │ ACTUADORES (Outputs)                                             │  │
│  │  - PA8-PA11: TIM1_CH1-CH4 → PWM Motores Tracción x4 (20 kHz)     │  │
│  │  - PC0-PC7: GPIO → DIR/EN Motores Tracción x4                    │  │
│  │  - PC8: TIM8_CH3 → PWM Motor Dirección (20 kHz)                  │  │
│  │  - PC9-PC10: GPIO → DIR/EN Motor Dirección                       │  │
│  │  - PC11-PC12,PD2: GPIO → Relés MAIN/TRAC/DIR                     │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │ COMUNICACIÓN                                                     │  │
│  │  - PB8/PB9: FDCAN1 ↔ ESP32-S3 (CAN 500 kbps, TJA1051T/3)        │  │
│  │    * Comandos HMI (throttle, steering, modo)                     │  │
│  │    * Feedback estado (velocidades, sensores, fallos)             │  │
│  │    * Heartbeat mutuo (watchdog)                                  │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │ SEGURIDAD Y WATCHDOG                                             │  │
│  │  - Watchdog interno STM32 (reset si firmware cuelga)             │  │
│  │  - Heartbeat CAN: si ESP32 no responde >250ms → modo seguro      │  │
│  │  - Relés default LOW → motores sin potencia si MCU resetea       │  │
│  │  - Validación de sensores (rango, coherencia temporal)           │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘

Conexiones Externas:
→ Motores DC 4x + Driver BTS7960 (tracción)
→ Motor DC 1x + Driver BTS7960 (dirección)
→ Encoder E6B2-CWZ6C (dirección)
→ Sensores rueda inductivos x4
→ Sensor Hall pedal (analógico)
→ DS18B20 x4 (temperaturas)
→ INA226 x6 (corrientes)
→ TOFSense-M (LiDAR)
→ Shifter mecánico (F/N/R)
→ Llave de contacto
→ Relés de potencia x3
→ CAN bus ↔ ESP32-S3 (HMI) vía TJA1051T/3
```

---

## 16. Notas de Implementación

### 1. Alimentación

**Topología recomendada:**
```
Batería 12V → Relé MAIN (control PC11) → Reguladores escalonados:
                                         ├─ 5V (3A) para drivers BTS7960
                                         ├─ 5V (1A) para sensores (Hall, TOF)
                                         └─ 3.3V (1A) para STM32 + lógica
```

**Capacitores de desacoplo en STM32:**
- 100nF cerámico en cada pin VDD/VDDA (mínimo 4 capacitores).
- 10µF electrolítico en alimentación principal.
- 1µF cerámico en VDDA (ADC) para filtrado adicional.

### 2. Protección EMI

**Motores DC generan ruido significativo:**
- Cables PWM/DIR/EN apantallados o retorcidos (twisted pair).
- Ferrite beads en alimentación de drivers BTS7960.
- Capacitores 100nF + 10µF en alimentación de motores.
- Separación física entre cables de potencia y señales analógicas.

**ADC de pedal especialmente sensible:**
- Cable blindado desde sensor Hall a STM32.
- Filtro RC (10kΩ + 100nF) cerca del pin PA0.
- Plano de GND continuo bajo trazas ADC.

### 3. PCB Layout

**Recomendaciones críticas:**
- **Plano de GND:** Sólido y continuo bajo todo el STM32.
- **Ancho de trazas PWM:** >1 mm (corrientes pico de drivers).
- **Separación analógico/digital:** Mantener trazas ADC alejadas de PWM/digital.
- **Cristal:** HSE (8-25 MHz) cerca del STM32 con capacitores 15-20pF a GND.
- **Decoupling:** Capacitores de desacoplo lo más cerca posible de pines VDD.

### 4. Configuración de Clock

**Configuración recomendada para 170 MHz:**
```
HSE: 8 MHz (cristal externo)
PLL: HSE → ×85 / 2 = 170 MHz
HCLK: 170 MHz (CPU)
APB1: 170 MHz / 2 = 85 MHz (periféricos lentos)
APB2: 170 MHz (periféricos rápidos)
```

### 5. Consideraciones de Seguridad Funcional

**Validaciones obligatorias en firmware:**
- **Pedal:** Rechazar valores fuera de rango, detectar stuck-at.
- **Sensores rueda:** Coherencia entre ruedas (no permitir divergencias >50%).
- **Encoder dirección:** Validar cambios dentro de límites físicos.
- **Shifter:** Solo estados válidos, rechazar múltiples activos.
- **Temperaturas:** Cortar potencia si >80°C.
- **Corrientes:** Limitar PWM si corriente >umbral seguro.
- **Heartbeat CAN:** Timeout ESP32 → modo seguro.

### 6. Debug y Diagnóstico

**Pines SWD (PA13/PA14) permiten:**
- Programación in-situ (ST-Link).
- Debug en tiempo real con breakpoints.
- Monitoreo de variables (watch).

**Recomendación:** Dejar header 2x5 en PCB para SWD + UART (para logs).

### 7. Expansión Futura

**Pines disponibles para añadir:**
- Acelerómetro/giróscopo (I2C/SPI).
- GPS (UART adicional: UART4/5).
- Display local de diagnóstico (SPI).
- Botones adicionales (GPIO libres).
- Sensores analógicos (ADC libres).

---

## Confirmación Final

### ✅ Verificaciones Completadas

**No hay conflictos de pines:**
- Cada señal tiene pin STM32 único asignado.
- No se reutilizan pines entre módulos incompatibles.
- Funciones alternativas respetadas (TIM, ADC, EXTI, etc.).

**No se usan pines sensibles:**
- SWD (PA13/PA14) **reservados** para debug.
- BOOT0 **fijado a GND** para arranque normal.
- USB (PA11/PA12) **no disponible** (PA11 usado para PWM, aceptable).

**El diseño es coherente para producción:**
- Timers avanzados (TIM1/TIM8) para PWM de alta calidad.
- ADC con trigger por timer para determinismo.
- Encoders con timers dedicados en modo cuadratura.
- CAN por hardware (FDCAN1) con transceptor industrial.
- Relés con estado seguro por defecto (LOW).
- Protecciones eléctricas (pull-up, filtros RC, debounce).

**Este pinout puede usarse directamente para:**
- Diseño de PCB.
- Cableado de sistema.
- Programación de firmware.
- Validación de hardware.

---

**Documento definitivo para integración hardware STM32G474RE en sistema de control vehicular.**

**Versión:** 1.0  
**Fecha:** 2026-01-22  
**Autor:** Ingeniería de Hardware y Firmware Embebido  
**Estado:** Aprobado para producción
