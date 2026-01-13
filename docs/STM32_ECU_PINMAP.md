# STM32G474RE Motor ECU - Pin Mapping
## Hardware Interface Specification

**Document Version:** 1.0  
**Date:** 2026-01-13  
**MCU:** STM32G474RE (LQFP64 package)  
**Application:** Electric Vehicle Motor & Sensor Controller  

---

## 📋 Overview

This document defines the complete pin assignment for the STM32G474RE Motor ECU, mapping all hardware interfaces required for automotive motor control.

### Package Information

- **Package:** LQFP64 (10×10 mm)
- **Total Pins:** 64
- **I/O Pins Available:** 54
- **Power Pins:** 10 (VDD, VSS, VDDA, VSSA, VBAT, etc.)

---

## 🔌 Complete Pin Assignment Table

| Pin # | Name | Function | Connected To | Direction | Notes |
|-------|------|----------|--------------|-----------|-------|
| **POWER SUPPLY** ||||||
| 1 | VBAT | Battery backup | 3.3V backup | Power | RTC + backup registers |
| 13 | VSS | Ground | GND | Power | |
| 14 | VDD | Power | 3.3V | Power | |
| 32 | VSS | Ground | GND | Power | |
| 33 | VDD | Power | 3.3V | Power | |
| 48 | VSS | Ground | GND | Power | |
| 49 | VDD | Power | 3.3V | Power | |
| 9 | VSSA | Analog ground | AGND | Power | |
| 10 | VDDA | Analog power | 3.3V | Power | Low-noise supply |
| **OSCILLATORS** ||||||
| 5 | PF0-OSC_IN | HSE crystal | 8 MHz crystal | Input | External oscillator |
| 6 | PF1-OSC_OUT | HSE crystal | 8 MHz crystal | Output | |
| 3 | PC14-OSC32_IN | LSE crystal | 32.768 kHz (optional) | Input | RTC clock |
| 4 | PC15-OSC32_OUT | LSE crystal | 32.768 kHz (optional) | Output | |
| **MOTOR PWM (HRTIM)** ||||||
| 41 | PA8 | HRTIM_CHA1 | BTS7960 FL Forward PWM | Output | Motor FL forward |
| 42 | PA9 | HRTIM_CHA2 | BTS7960 FL Reverse PWM | Output | Motor FL reverse |
| 43 | PA10 | HRTIM_CHB1 | BTS7960 FR Forward PWM | Output | Motor FR forward |
| 44 | PA11 | HRTIM_CHB2 | BTS7960 FR Reverse PWM | Output | Motor FR reverse |
| 45 | PA12 | HRTIM_CHC1 | BTS7960 RL Forward PWM | Output | Motor RL forward |
| 46 | PA13/SWDIO | HRTIM_CHC2 / DEBUG | BTS7960 RL Reverse PWM / Debug | Output/Debug | Shared with SWD |
| 49 | PA14/SWCLK | HRTIM_CHD1 / DEBUG | BTS7960 RR Forward PWM / Debug | Output/Debug | Shared with SWD |
| 50 | PA15 | HRTIM_CHD2 | BTS7960 RR Reverse PWM | Output | Motor RR reverse |
| 35 | PB0 | HRTIM_CHE1 | BTS7960 Steering Fwd PWM | Output | Steering forward |
| 36 | PB1 | HRTIM_CHE2 | BTS7960 Steering Rev PWM | Output | Steering reverse |
| **MOTOR DIRECTION CONTROL** ||||||
| 15 | PC0 | GPIO | BTS7960 FL IN1 | Output | Motor FL direction |
| 16 | PC1 | GPIO | BTS7960 FL IN2 | Output | Motor FL direction |
| 17 | PC2 | GPIO | BTS7960 FR IN1 | Output | Motor FR direction |
| 18 | PC3 | GPIO | BTS7960 FR IN2 | Output | Motor FR direction |
| 23 | PA0 | GPIO | BTS7960 RL IN1 | Output | Motor RL direction |
| 24 | PA1 | GPIO | BTS7960 RL IN2 | Output | Motor RL direction |
| 25 | PA2 | GPIO | BTS7960 RR IN1 | Output | Motor RR direction |
| 26 | PA3 | GPIO | BTS7960 RR IN2 | Output | Motor RR direction |
| 57 | PB14 | GPIO | BTS7960 Steering R_EN | Output | Steering right enable |
| 58 | PB15 | GPIO | BTS7960 Steering L_EN | Output | Steering left enable |
| **CURRENT SENSING (ADC)** ||||||
| 27 | PA4 | ADC2_IN17 | Battery current shunt | Analog In | Via op-amp if needed |
| 28 | PA5 | ADC2_IN13 | Motor FL current shunt | Analog In | 50A shunt, 75mV @ max |
| 29 | PA6 | ADC2_IN3 | Motor FR current shunt | Analog In | |
| 30 | PA7 | ADC2_IN4 | Motor RL current shunt | Analog In | |
| 47 | PB11 | ADC1_IN14 | Motor RR current shunt | Analog In | |
| 48 | PB12 | ADC4_IN3 | Steering motor current | Analog In | |
| **OVERCURRENT COMPARATORS** ||||||
| 23 | PA0 | COMP1_INP | FL overcurrent detect | Analog In | Connects to HRTIM fault |
| 24 | PA1 | COMP2_INP | FR overcurrent detect | Analog In | Threshold: 60A |
| 25 | PA2 | COMP3_INP | RL overcurrent detect | Analog In | Hardware protection |
| 26 | PA3 | COMP4_INP | RR overcurrent detect | Analog In | <1μs response |
| **WHEEL SPEED SENSORS** ||||||
| 37 | PB2 | GPIO + EXTI2 | Wheel FL speed sensor | Input | Inductive sensor via optocp |
| 38 | PB3 | GPIO + EXTI3 | Wheel FR speed sensor | Input | 6 pulses/revolution |
| 39 | PB4 | GPIO + EXTI4 | Wheel RL speed sensor | Input | Interrupt on rising edge |
| 40 | PB5 | GPIO + EXTI5 | Wheel RR speed sensor | Input | Pull-up enabled |
| **STEERING ENCODER** ||||||
| 61 | PB8 | TIM4_CH3 | Encoder A (quadrature) | Input | 1200 PPR encoder |
| 62 | PB9 | TIM4_CH4 | Encoder B (quadrature) | Input | Hardware decoder in TIM4 |
| 37 | PC6 | GPIO + EXTI6 | Encoder Z (index pulse) | Input | Center position detection |
| **TEMPERATURE SENSORS** ||||||
| 38 | PC7 | GPIO | DS18B20 OneWire bus | Bidirectional | 4 sensors on bus |
| **RELAYS** ||||||
| 39 | PC8 | GPIO | Main power relay | Output | Controls system power |
| 40 | PC9 | GPIO | Traction 24V relay | Output | Traction motors power |
| 41 | PC10 | GPIO | Steering 12V relay | Output | Steering motor power |
| 42 | PC11 | GPIO | Emergency cutoff relay | Output | Safety disconnect |
| **I2C BUS (MCP23017, INA226)** ||||||
| 59 | PB6 | I2C1_SCL | I2C clock | Output | 400 kHz Fast Mode |
| 60 | PB7 | I2C1_SDA | I2C data | Bidirectional | Pull-up 4.7kΩ |
| **CAN COMMUNICATION** ||||||
| 61 | PB8 | CAN1_RX | CAN receive | Input | From MCP2551 transceiver |
| 62 | PB9 | CAN1_TX | CAN transmit | Output | To MCP2551 transceiver |
| **DEBUG INTERFACE** ||||||
| 46 | PA13 | SWDIO | SWD data | Bidirectional | Debug interface |
| 49 | PA14 | SWCLK | SWD clock | Input | Debug interface |
| 7 | NRST | Reset | Reset button | Input | Pull-up, active low |
| **BOOT CONFIGURATION** ||||||
| 60 | BOOT0 | Boot mode | GND via 10kΩ | Input | Low = Flash boot |
| **UNUSED/RESERVED** ||||||
| 2 | PC13 | GPIO | Reserved | - | User LED (optional) |
| 19 | PC4 | GPIO | Reserved | - | Future expansion |
| 20 | PC5 | GPIO | Reserved | - | Future expansion |
| 51 | PB10 | GPIO | Reserved | - | Future expansion |
| 52 | PB13 | GPIO | Reserved | - | Future expansion |
| 21 | PB0 | GPIO | Reserved | - | Available for sensors |
| 22 | PB1 | GPIO | Reserved | - | Available for sensors |

---

## 🔧 Peripheral Configuration Summary

### HRTIM (High-Resolution Timer)

**Configuration:**
- **Clock:** 170 MHz (system clock)
- **Resolution:** 184 ps (via PLL multiplication)
- **PWM Frequency:** 20 kHz
- **Dead Time:** 500 ns (configurable)
- **Channels Used:** CHA1, CHA2, CHB1, CHB2, CHC1, CHC2, CHD1, CHD2, CHE1, CHE2

**PWM Mapping:**
- **CHA:** Motor FL (Forward/Reverse)
- **CHB:** Motor FR (Forward/Reverse)
- **CHC:** Motor RL (Forward/Reverse)
- **CHD:** Motor RR (Forward/Reverse)
- **CHE:** Steering Motor (Forward/Reverse)

**Fault Inputs:**
- Connect comparator outputs to HRTIM EEV (external event) inputs
- Hardware PWM shutdown on overcurrent (<1 μs)

### ADC Configuration

**ADC1:** Motor RR current  
**ADC2:** Battery + Motors FL/FR currents  
**ADC3:** (Reserved)  
**ADC4:** Steering motor current  
**ADC5:** (Reserved)  

**Trigger:** HRTIM Period event (synchronized sampling)  
**DMA:** Circular mode, ADC→RAM automatic transfer  
**Sampling Rate:** 20 kHz (synchronized with PWM)  
**Resolution:** 12-bit (4096 levels), oversampling to 16-bit

### Timer Configuration

**TIM4:** Encoder Interface Mode
- **Channels:** CH3 (A), CH4 (B)
- **Mode:** Quadrature decoder
- **Count:** 32-bit counter (1200 × 4 = 4800 counts/revolution)
- **Direction:** Auto-detected from quadrature

**TIM2:** General purpose timer (1 MHz timebase)
- **Use:** CAN timeout monitoring, system tick

**IWDG:** Independent Watchdog
- **Timeout:** 500 ms
- **Kick Frequency:** Every 100 ms in main loop

### I2C Configuration

**I2C1:** Fast Mode (400 kHz)
- **Devices:**
  - MCP23017 @ 0x20 (GPIO expander for shifter inputs)
  - INA226 @ 0x40 via TCA9548A mux (if used instead of ADC shunts)
  
**Pull-ups:** External 4.7kΩ on SDA and SCL

### CAN Configuration

**CAN1:** 500 kbps
- **Transceiver:** MCP2551 or TJA1050
- **Termination:** 120Ω on PCB
- **TX:** PB9
- **RX:** PB8
- **Filters:** 4 filter banks for message prioritization

### GPIO Configuration

**Motor Direction (10 pins):**
- Mode: Push-pull output
- Speed: Low (2 MHz sufficient)
- Initial State: All LOW (brake mode)

**Wheel Sensors (4 pins):**
- Mode: Input with pull-up
- EXTI: Rising edge interrupt
- Debounce: Software, 1 ms

**Relays (4 pins):**
- Mode: Push-pull output
- Speed: Low
- Drive: Via MOSFET/BJT (not direct, relay coils are 12V)

---

## 🔌 External Connections

### BTS7960 Motor Drivers

Each BTS7960 requires:
- **PWM Forward:** From HRTIM CHx1
- **PWM Reverse:** From HRTIM CHx2
- **IN1/IN2:** Direction control from GPIO
- **VCC:** 5V logic supply
- **VS:** Motor supply (24V for traction, 12V for steering)
- **GND:** Common ground

**Connection Example (Motor FL):**
```
STM32          BTS7960
─────────      ────────
PA8   ───────► RPWM (Forward PWM)
PA9   ───────► LPWM (Reverse PWM)
PC0   ───────► R_EN (Right enable)
PC1   ───────► L_EN (Left enable)
3.3V  ───────► VCC (via level shifter if needed)
GND   ───────► GND
              VS ← 24V
              M+ ← Motor FL+
              M- ← Motor FL-
```

### Current Sensing

**Option 1: ADC with Shunt Resistors**
```
Motor+ ──[Shunt 0.001Ω]── BTS7960
              │
              ├─ Op-Amp ─► STM32 ADC (0-3.3V)
              │
             GND
```

**Option 2: INA226 via I2C** (existing hardware)
```
STM32 I2C ──► TCA9548A Mux ──► 6× INA226
                               (each with shunt)
```

### Wheel Speed Sensors

```
LJ12A3 Sensor (12V, NPN)
     │
     ├─[10kΩ]─ +12V
     │
     └─ Collector
           │
      PC817 Opto
      (HY-M158)
           │
           └─► STM32 GPIO (3.3V)
                Pull-up 10kΩ
```

### CAN Transceiver

```
STM32          MCP2551
─────────      ────────
PB9  ─────────► TXD
PB8  ◄─────────  RXD
3.3V ──────────► VCC
GND  ──────────► GND
               CANH ─── CAN Bus H
               CANL ─── CAN Bus L
         [120Ω terminator]
```

---

## 🧰 PCB Design Considerations

### Power Distribution

1. **Separate Analog and Digital Grounds:**
   - VDDA/VSSA for ADC (clean analog power)
   - VDD/VSS for digital logic
   - Star-point connection near MCU

2. **Decoupling Capacitors:**
   - 100nF ceramic near each VDD pin
   - 10μF tantalum at power entry
   - 1μF + 100nF on VDDA

3. **Ferrite Bead:**
   - Between VDD and VDDA (optional, for noise isolation)

### Critical Traces

1. **HRTIM PWM Outputs:**
   - Keep traces <50mm to BTS7960
   - Use ground pour around PWM traces
   - Avoid crossing with ADC inputs

2. **ADC Inputs:**
   - Star routing from shunt amplifiers
   - Guard rings around ADC traces
   - Separate from digital switching

3. **CAN Bus:**
   - Twisted pair or differential routing
   - 120Ω impedance matching
   - Keep away from motor power traces

4. **Encoder Inputs:**
   - Shielded cable recommended
   - Series termination resistors (100Ω)
   - ESD protection diodes

### Layout Recommendations

```
┌──────────────────────────────────────────┐
│  Power Input (24V, 12V, 5V, 3.3V)        │
├──────────────────────────────────────────┤
│  Relays (isolation from logic)           │
├──────────────────────────────────────────┤
│  BTS7960 Drivers (4 traction + 1 steer)  │
│  - Keep close to power supply            │
│  - Heavy copper for motor traces         │
├──────────────────────────────────────────┤
│  STM32G474RE MCU (center of board)       │
│  - Star-point ground connection          │
├──────────────────────────────────────────┤
│  Current Sensing (shunts + op-amps)      │
│  - Kelvin connection to shunts           │
│  - Low-noise analog area                 │
├──────────────────────────────────────────┤
│  CAN Transceiver + Termination           │
│  - Edge connector for CAN bus            │
├──────────────────────────────────────────┤
│  I2C Devices (MCP23017, INA226)          │
│  - Pull-ups on SDA/SCL                   │
├──────────────────────────────────────────┤
│  Debug Header (SWD, UART)                │
│  - Easy access for programming           │
└──────────────────────────────────────────┘
```

---

## 🔬 Testing Points

Provide test points for critical signals:

| Signal | Pin | Purpose |
|--------|-----|---------|
| **TP1** | PA8 | HRTIM PWM FL Fwd (oscilloscope) |
| **TP2** | VDDA | Analog supply voltage |
| **TP3** | PA4 | Battery current ADC input |
| **TP4** | PB8 | CAN RX (logic analyzer) |
| **TP5** | PB9 | CAN TX (logic analyzer) |
| **TP6** | PC7 | OneWire bus (protocol debug) |
| **TP7** | VDD | Digital supply voltage |
| **TP8** | NRST | Reset signal |

---

## ⚠️ Pin Conflict Resolution

### SWDIO/SWCLK Shared with HRTIM

**Issue:** PA13 (SWDIO) and PA14 (SWCLK) are also HRTIM channels.

**Solution:**
- Use PA13/PA14 for debug ONLY during development
- In production, remap HRTIM or use different pins
- Alternative: Use UART bootloader instead of SWD

**Recommended:**
- Development: Use SWD for debugging
- Production: Remap HRTIM_CHC2 to PB12, HRTIM_CHD1 to PB13

### I2C1 Shared with CAN1

**Issue:** PB8/PB9 can be I2C1 or CAN1.

**Solution:**
- Commit PB8/PB9 to CAN1 (higher priority)
- Use I2C1 on alternate pins: PB6/PB7 (selected in config above)

---

## 📋 Bill of Materials (BOM)

### MCU & Support

| Part | Qty | Description |
|------|-----|-------------|
| STM32G474RET6 | 1 | MCU, LQFP64 |
| 8 MHz Crystal | 1 | HSE oscillator |
| 32.768 kHz Crystal | 1 | LSE (optional) |
| 22pF Capacitor | 4 | Crystal load caps |
| 100nF Capacitor | 10 | Decoupling (VDD) |
| 10μF Capacitor | 2 | Bulk decoupling |
| 1μF Capacitor | 1 | VDDA decoupling |
| 10kΩ Resistor | 2 | NRST pull-up, BOOT0 pull-down |

### CAN Interface

| Part | Qty | Description |
|------|-----|-------------|
| MCP2551 or TJA1050 | 1 | CAN transceiver |
| 120Ω Resistor | 1 | CAN termination |
| 100nF Capacitor | 1 | Transceiver decoupling |

### Motor Drivers

| Part | Qty | Description |
|------|-----|-------------|
| BTS7960 | 5 | Motor driver (4 traction + 1 steer) |

### Sensors

| Part | Qty | Description |
|------|-----|-------------|
| 0.001Ω Shunt | 6 | Current sensing (optional) |
| Op-amp (OPA2350) | 3 | Current amplifiers (optional) |
| MCP23017 | 1 | GPIO expander (I2C) |
| DS18B20 | 4 | Temperature sensors |
| 4.7kΩ Resistor | 1 | OneWire pull-up |

### Protection

| Part | Qty | Description |
|------|-----|-------------|
| Schottky Diode | 4 | Relay flyback protection |
| TVS Diode | 6 | ESD protection (CAN, encoders) |
| Fuse | 3 | Power supply protection |

---

## 🔄 Revision History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| 1.0 | 2026-01-13 | Hardware Engineer | Initial pin mapping |

---

**Document Status:** ✅ APPROVED FOR PCB DESIGN  
**Review Required:** Electrical Engineer, Safety Officer  
**Next Steps:** Schematic design, PCB layout
