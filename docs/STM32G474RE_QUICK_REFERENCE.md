# STM32G474RE - Guía Rápida de Referencia

**Microcontrolador especializado en control de motores**

---

## 🎯 ¿Cuándo usar STM32G474RE?

### ✅ Ideal Para:

- Control FOC (Field Oriented Control) de motores BLDC/PMSM
- Inversores y convertidores de potencia
- Aplicaciones con PWM ultra-preciso (184 ps de resolución)
- Sistemas con comunicación CAN FD
- Medición analógica de alta velocidad y precisión
- Control industrial con bajo consumo

### ❌ NO Recomendado Para:

- Interfaces gráficas complejas (displays TFT grandes)
- Aplicaciones que requieren mucha memoria (>128 KB)
- Proyectos con conectividad wireless nativa
- Sistemas con almacenamiento de datos extenso

---

## 📊 Especificaciones Clave

```
CPU:        ARM Cortex-M4 @ 170 MHz (213 DMIPS)
Flash:      512 KB (con ECC)
RAM:        128 KB SRAM (con parity check)
FPU:        Sí (Single precision)
DSP:        Sí (DSP instructions)
Aceleradores: CORDIC + FMAC
```

---

## 🚀 Periféricos Destacados

### Control de Motores

| Periférico | Características |
|------------|-----------------|
| **HRTIM** | 6×16-bit, 184 ps, 12 PWM channels |
| **ADC** | 5×12-bit, 4 Msps, 42 canales |
| **Comparadores** | 7× ultra-rápidos, rail-to-rail |
| **Op-Amps** | 6× con modo PGA |
| **DAC** | 7×12-bit (3 ext + 4 int) |

### Comunicación

- **I2C:** 4 ports
- **SPI:** 4 ports (2 con I2S)
- **UART/USART:** 6 total (1 LPUART)
- **CAN FD:** 3 ports
- **USB:** Device + Type-C PD

### Temporizadores

- **17 timers** en total
- **HRTIM:** Alta resolución (184 ps)
- **Motor control timers**
- **General purpose timers**
- **Watchdog timers**

---

## 💾 Mapa de Memoria

```
┌────────────────────┐ 0x20000000 + 128KB
│    SRAM (128 KB)   │
├────────────────────┤ 0x20000000
│                    │
│  CCM SRAM (fast)   │
│                    │
├────────────────────┤ 0x08000000 + 128KB
│   Flash (128 KB)   │
├────────────────────┤ 0x08000000
│                    │
│   Boot Loader      │
│                    │
└────────────────────┘
```

---

## ⚡ Consumo de Energía

| Modo | Consumo Típico |
|------|---------------|
| Run (170 MHz) | ~17 mA |
| Sleep | CPU off, periféricos on |
| Stop | ~5-10 µA |
| Standby | ~1-2 µA |
| Shutdown | ~30 nA |

---

## 🔌 Pinout Típico (UFQFPN48)

```
         ┌─────────┐
  VBAT ──┤1     48├── VDD
   PC13 ──┤2     47├── GND
   PC14 ──┤3     46├── VDDA
   PC15 ──┤4     45├── PA0
    PF0 ──┤5     44├── PA1
    PF1 ──┤6     43├── PA2
  NRST ──┤7     42├── PA3
        ──┤...   ..├──
         └─────────┘
         
10×10 mm, 54 I/Os útiles
```

---

## 🛠️ Herramientas de Desarrollo

### Software

- **STM32CubeIDE:** IDE gráfico gratuito
- **STM32CubeMX:** Configurador visual
- **Motor Control Workbench:** Para aplicaciones de motores
- **STM32CubeProgrammer:** Flash y debug

### Hardware

- **NUCLEO-G474RE:** Placa de desarrollo (~$15)
- **ST-LINK/V2:** Debugger/programador
- **Power Shields:** Para evaluación de potencia

### Frameworks

- **STM32 HAL:** Hardware Abstraction Layer
- **LL (Low-Layer):** APIs de bajo nivel
- **Arduino:** Soporte via STM32duino
- **FreeRTOS:** Incluido en STM32Cube

---

## 🎓 Ejemplos de Aplicaciones

### 1. Control FOC de Motor BLDC

```c
// Pseudocódigo simplificado
void motor_control_task() {
    // Leer corrientes (ADC sincronizado con HRTIM)
    read_phase_currents(&Ia, &Ib, &Ic);
    
    // Transformada de Clarke (acelerada con CORDIC)
    clarke_transform(Ia, Ib, Ic, &Ialpha, &Ibeta);
    
    // Transformada de Park
    park_transform(Ialpha, Ibeta, theta, &Id, &Iq);
    
    // Control PI
    Vd = PI_controller_d(Id_ref, Id);
    Vq = PI_controller_q(Iq_ref, Iq);
    
    // Transformadas inversas
    inv_park_transform(Vd, Vq, theta, &Valpha, &Vbeta);
    
    // Modulación SVM
    space_vector_modulation(Valpha, Vbeta, &duty_A, &duty_B, &duty_C);
    
    // Actualizar PWM (HRTIM)
    update_hrtim_duty(duty_A, duty_B, duty_C);
}
```

### 2. Convertidor DC-DC Buck

```c
// Control de convertidor Buck con protección
void buck_converter_control() {
    // Leer tensión de salida (ADC)
    float Vout = read_output_voltage();
    
    // Control PI
    float duty = PI_controller(Vref, Vout);
    
    // Limitar duty cycle
    duty = constrain(duty, 0.1, 0.9);
    
    // Actualizar PWM (HRTIM con dead-time automático)
    set_hrtim_duty(HRTIM_TIMER_A, duty);
    
    // Protección por hardware (comparador + HRTIM fault input)
    // Si comparador detecta sobrecorriente, HRTIM desactiva PWM
    // automáticamente sin intervención de software
}
```

### 3. Medición Multi-Canal

```c
// ADC multi-canal con DMA
void adc_multi_channel_init() {
    // Configurar 5 ADCs para conversión simultánea
    // ADC1, ADC2: Corrientes de motor (Phase A, B)
    // ADC3: Tensión de bus DC
    // ADC4: Temperatura
    // ADC5: Corriente total
    
    // Trigger desde HRTIM (sincronizado con PWM)
    // DMA transferencia automática a buffer
    // Callback cuando todos los ADCs completan
}
```

---

## 📋 Checklist de Diseño Hardware

### Alimentación

- [ ] Regulador 3.3V de bajo ruido para VDDA
- [ ] Capacitor 100nF cerca de cada pin VDD
- [ ] Capacitor 1µF + 100nF en VDDA
- [ ] Ferrite bead entre VDD y VDDA (opcional)

### Clock

- [ ] Cristal 8-24 MHz (típico: 16 MHz HSE)
- [ ] Capacitores de carga 20pF (ajustar según cristal)
- [ ] Trazas cortas y simétricas
- [ ] 32.768 kHz para RTC (opcional)

### Reset

- [ ] Resistor pull-up 10K en NRST
- [ ] Capacitor 100nF a GND (opcional)
- [ ] Botón de reset (opcional)

### Debug

- [ ] Conector SWD (SWDIO, SWCLK, GND, VDD)
- [ ] Resistor 10K pull-up en SWDIO
- [ ] Header 1.27mm pitch o 2.54mm

### Boot Mode

- [ ] Jumper/switch en BOOT0
- [ ] Resistor pull-down 10K en BOOT0

---

## 🔧 Configuración Típica

### Reloj Sistema

```
HSE: 16 MHz (cristal externo)
PLL: ×85 / 2 = 170 MHz
HCLK: 170 MHz (CPU)
APB1: 170 MHz (Periféricos)
APB2: 170 MHz (Periféricos rápidos)
```

### HRTIM para Motor Control

```
Frecuencia PWM: 20-100 kHz típico
Resolución: 184 ps
Dead-time: 100-500 ns típico
Fault inputs: Conectados a comparadores
ADC trigger: En center/peak de PWM
```

### ADC para Lectura de Corrientes

```
Resolución: 12-bit (4096 niveles)
Oversampling: 16× → 16-bit efectivo
Velocidad: 4 Msps (cada ADC)
Trigger: HRTIM (sincronizado)
DMA: Transferencia automática
```

---

## 💡 Tips y Trucos

### Optimización de Rendimiento

1. **Usar CORDIC para trigonometría:** 10-20× más rápido que software
2. **FMAC para filtros:** Libera CPU para otras tareas
3. **CCM SRAM:** Usar para variables críticas (0 wait states)
4. **LL APIs:** Para código time-critical (menor overhead que HAL)

### Reducción de Consumo

1. **Clock gating:** Desactivar relojes de periféricos no usados
2. **DMA:** Transferencias sin CPU
3. **Stop mode:** Para delays largos
4. **LPUART:** Comunicación en modos low-power

### Debugging

1. **SWV (Serial Wire Viewer):** Printf por SWD
2. **ETM:** Trace detallado de ejecución
3. **Comparadores:** Detectar fallas sin debugger
4. **IWDG:** Watchdog independiente para producción

---

## 📚 Recursos Adicionales

### Documentación ST

- [STM32G474RE Product Page](https://www.st.com/en/microcontrollers-microprocessors/stm32g474cb.html)
- [Datasheet PDF](https://www.st.com/resource/en/datasheet/stm32g474cb.pdf)
- [Reference Manual RM0440](https://www.st.com/resource/en/reference_manual/rm0440-stm32g4-series-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
- [STM32G4 Series Documentation](https://www.st.com/en/microcontrollers-microprocessors/stm32g4-series/documentation.html)

### Application Notes Importantes

- **AN5048:** Motor control with STM32G4
- **AN4946:** Position and speed control
- **AN5301:** Sensorless FOC for PMSM
- **AN5195:** HRTIM cookbook

### Comunidad y Soporte

- [STM32 Community Forums](https://community.st.com/)
- [GitHub - STM32Examples](https://github.com/STMicroelectronics)
- [X-CUBE packages](https://www.st.com/en/embedded-software/x-cube-mcsdk.html) - Motor Control SDK

---

## ⚠️ Limitaciones Importantes

### NO usar STM32G474RE si necesitas:

- ❌ Display gráfico grande (>128×64)
- ❌ Más de 128 KB de código
- ❌ WiFi/Bluetooth integrado
- ❌ Almacenamiento masivo de datos
- ❌ Procesamiento de imagen/video
- ❌ USB Host complejo

### Soluciones a limitaciones:

- **Poca memoria:** Usar memoria externa vía Quad-SPI
- **Sin display controller:** Usar controlador externo (SSD1963, ILI9341)
- **Sin WiFi:** Añadir módulo ESP32 como co-procesador
- **Almacenamiento:** SD card vía SPI/SDIO

---

## 🎯 Conclusión

El **STM32G474RE** es un microcontrolador **altamente especializado** y **extremadamente capaz** para:

✅ Control de motores de alto rendimiento
✅ Electrónica de potencia
✅ Medición y adquisición de datos
✅ Control industrial

Pero requiere **experiencia en control de motores** y **diseño de sistemas embebidos** para aprovecharlo completamente.

**No es un MCU de propósito general** - es una herramienta especializada que brilla en su dominio específico.

---

**Documento:** Guía Rápida STM32G474RE  
**Versión:** 1.0  
**Fecha:** 2026-01-13  
**Autor:** Análisis técnico basado en documentación oficial
