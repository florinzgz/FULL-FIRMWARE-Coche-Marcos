# Mapeo Completo Módulos HY-M158 Optoacopladores

## Configuración Hardware
- 2x Módulos HY-M158 (8 canales c/u)
- Total: 16 canales disponibles
- Función: Aislar señales 12V/5V → 3.3V para ESP32-S3 y MCP23017

---

## HY-M158 Módulo #1 - Sensores y Encoder

| Canal | Destino | Señal | Tipo Entrada | Descripción |
|-------|---------|-------|--------------|-------------|
| CH1   | GPIO 3  | WHEEL_FL | 5V | Sensor rueda frontal izquierda |
| CH2   | GPIO 36 | WHEEL_FR | 5V | Sensor rueda frontal derecha |
| CH3   | GPIO 17 | WHEEL_RL | 5V | Sensor rueda trasera izquierda |
| CH4   | GPIO 15 | WHEEL_RR | 5V | Sensor rueda trasera derecha |
| CH5   | GPIO 37 | ENCODER_A | 5V | Encoder dirección canal A |
| CH6   | GPIO 38 | ENCODER_B | 5V | Encoder dirección canal B |
| CH7   | GPIO 39 | ENCODER_Z | 5V | Encoder dirección señal Z (centro) |
| CH8   | — | RESERVA | — | Disponible |

---

## HY-M158 Módulo #2 - Shifter (Palanca de Cambios) → MCP23017

⚠️ **IMPORTANTE v2.3.0**: El shifter ahora se conecta al **MCP23017** (I²C 0x20), NO directamente a GPIOs del ESP32.

| Canal | Destino MCP23017 | Señal | Tipo Entrada | Descripción |
|-------|------------------|-------|--------------|-------------|
| CH1   | GPIOB0 (pin 8)   | SHIFTER_P  | **12V** | Posición P (Park) |
| CH2   | GPIOB1 (pin 9)   | SHIFTER_R  | **12V** | Posición R (Reverse) |
| CH3   | GPIOB2 (pin 10)  | SHIFTER_N  | **12V** | Posición N (Neutral) |
| CH4   | GPIOB3 (pin 11)  | SHIFTER_D1 | **12V** | Posición D1 (Drive 1 - baja velocidad) |
| CH5   | GPIOB4 (pin 12)  | SHIFTER_D2 | **12V** | Posición D2 (Drive 2 - alta velocidad) |
| CH6   | — | RESERVA | — | Disponible |
| CH7   | — | RESERVA | — | Disponible |
| CH8   | — | RESERVA | — | Disponible |

---

## 🔧 CÓMO SE CONECTA LA PALANCA DE CAMBIOS (SHIFTER)

### Resumen de Voltajes

| Componente | Voltaje de Operación |
|------------|---------------------|
| Palanca de cambios (entrada) | **12V DC** |
| Optoacoplador HY-M158 (aislamiento) | 12V entrada → 3.3V salida |
| MCP23017 (I²C expander) | **3.3V** |
| ESP32-S3 | **3.3V** |

### Diagrama de Conexión Completo

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        PALANCA DE CAMBIOS (SHIFTER)                         │
│                        ════════════════════════════                         │
│                                                                             │
│   La palanca funciona con 12V DC y tiene 5 contactos (uno por posición)    │
│                                                                             │
│                              ┌───────────────┐                              │
│                              │   PALANCA     │                              │
│                              │   CAMBIOS     │                              │
│                              │    12V DC     │                              │
│                              │               │                              │
│                              │ P  ●──────────┼─── Cable Rojo                │
│                              │ R  ●──────────┼─── Cable Blanco              │
│                              │ N  ●──────────┼─── Cable Verde               │
│                              │ D1 ●──────────┼─── Cable Azul                │
│                              │ D2 ●──────────┼─── Cable Amarillo            │
│                              │               │                              │
│                              │ COM ●─────────┼─── +12V (común)              │
│                              └───────────────┘                              │
│                                     │                                       │
│                                     ▼                                       │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │                     HY-M158 OPTOACOPLADOR #2                        │   │
│   │                     (Módulo PC817 x 8 canales)                      │   │
│   │                                                                     │   │
│   │  LADO ENTRADA (12V)           │          LADO SALIDA (3.3V)        │   │
│   │  ─────────────────            │          ──────────────────        │   │
│   │  VCC ●──── +12V ──────────────┤                                    │   │
│   │  GND ●──── GND común ─────────┤          VCC ●──── +3.3V           │   │
│   │  IN1 ●──── P (Rojo) ──────────┼─────────►OUT1 ●                    │   │
│   │  IN2 ●──── R (Blanco) ────────┼─────────►OUT2 ●                    │   │
│   │  IN3 ●──── N (Verde) ─────────┼─────────►OUT3 ●                    │   │
│   │  IN4 ●──── D1 (Azul) ─────────┼─────────►OUT4 ●                    │   │
│   │  IN5 ●──── D2 (Amarillo) ─────┼─────────►OUT5 ●                    │   │
│   │  IN6-8 ── (Reserva) ──────────┤          OUT6-8 ● (Reserva)        │   │
│   │                               │          GND ●──── GND             │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│                                     │                                       │
│                                     ▼                                       │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │                     MCP23017 EXPANSOR I²C                           │   │
│   │                     (Dirección I²C: 0x20)                           │   │
│   │                                                                     │   │
│   │  GPIOB0 (pin 8)  ●◄──── OUT1 ──── P (Park)                         │   │
│   │  GPIOB1 (pin 9)  ●◄──── OUT2 ──── R (Reverse)                      │   │
│   │  GPIOB2 (pin 10) ●◄──── OUT3 ──── N (Neutral)                      │   │
│   │  GPIOB3 (pin 11) ●◄──── OUT4 ──── D1 (Drive 1)                     │   │
│   │  GPIOB4 (pin 12) ●◄──── OUT5 ──── D2 (Drive 2)                     │   │
│   │  GPIOB5-B7       ●───── (Libres para expansión)                    │   │
│   │                                                                     │   │
│   │  SDA ●──────────────────────────────────────────────►GPIO 8 ESP32  │   │
│   │  SCL ●──────────────────────────────────────────────►GPIO 9 ESP32  │   │
│   │  VCC ●──── 3.3V                                                    │   │
│   │  GND ●──── GND                                                     │   │
│   └─────────────────────────────────────────────────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Funcionamiento

1. **La palanca opera a 12V**: Cuando seleccionas una posición (P, R, N, D1, D2), se conecta +12V al contacto correspondiente.

2. **El optoacoplador HY-M158 aísla y convierte**: 
   - Recibe la señal de 12V en el lado de entrada
   - El LED interno del optoacoplador (PC817) se enciende
   - El fototransistor conduce y conecta la salida a GND
   - La salida pasa de HIGH (3.3V por pull-up) a LOW

3. **El MCP23017 lee la señal**:
   - Tiene pull-ups internos activados
   - Lee LOW cuando la posición está activa (lógica invertida)
   - Comunica el estado al ESP32-S3 vía I²C

4. **El ESP32-S3 procesa**:
   - Lee el MCP23017 por I²C (dirección 0x20)
   - Prioridad de lectura: P > R > N > D1 > D2
   - Implementa debounce de 50ms para evitar rebotes

### ⚠️ Polaridad de Señales (MUY IMPORTANTE)

| Estado Físico | Entrada HY-M158 | Salida HY-M158 | Lectura MCP23017 |
|---------------|-----------------|----------------|------------------|
| Posición NO seleccionada | 0V (abierto) | HIGH (3.3V) | 1 (HIGH) |
| Posición SELECCIONADA | 12V (activo) | LOW (0V) | 0 (LOW) |

**El código ya maneja esto automáticamente**: `readMcpPin()` en `shifter.cpp` devuelve `true` cuando lee LOW.

---

## Resumen de Uso

**Total canales usados:** 12 de 16
**Canales libres:** 4

**Distribución por tipo de señal:**
- Sensores inductivos ruedas (LJ12A3-4-Z/BX): 4 canales
- Encoder dirección (E6B2-CWZ6C): 3 canales (A, B, Z)
- Shifter 12V (vía MCP23017): 5 canales

---

## Notas Importantes

### Sensores LJ12A3-4-Z/BX (Ruedas)
- Alimentación: 5V (o 12V según modelo)
- Salida: NPN normalmente abierta
- 6 tornillos por rueda = 6 pulsos/revolución
- Diámetro de rueda: 110 cm (circunferencia calculada en firmware)
- Distancia máxima detección: 4mm

### Encoder E6B2-CWZ6C 1200PR (Dirección)
- Alimentación: 5V (convertida desde 12V)
- Resolución: 1200 pulsos/revolución
- Ratio: 1:1 al eje del volante
- Señal Z: Marca punto cero (centro)

### Shifter (Palanca de Cambios) ✅ v2.3.0
- **Voltaje de entrada: 12V DC**
- Aislamiento: HY-M158 optoacoplador (12V → 3.3V)
- Destino: MCP23017 I²C (0x20), banco GPIOB, pines 8-12
- Lógica: LOW = activo (por inversión del optoacoplador)
- Prioridad lectura: P > R > N > D1 > D2
- Debounce: 50ms

---

## ⚠️ Componentes que NO Usan HY-M158

Los siguientes componentes se conectan **DIRECTAMENTE** a los GPIOs del ESP32-S3 (sin optoacoplador):

### Botones de Control (LIGHTS, MEDIA, 4X4)
- **Voltaje: 3.3V** (directo a GPIO)
- NO requieren optoacoplador
- Pull-up interno activado en firmware
- Lógica: LOW = pulsado

| Botón | GPIO | Función |
|-------|------|---------|
| LIGHTS | GPIO 2 | Luces ON/OFF |
| MEDIA | GPIO 40 | Multimedia |
| 4X4 | GPIO 41 | Switch 4x4/4x2 |

### Llave de Contacto (KEY_SYSTEM)
- **Voltaje: 3.3V** (directo a GPIO)
- NO requiere optoacoplador
- ⚠️ GPIO 0 es strapping pin - requiere pull-up EXTERNO de 10kΩ

### Pedal Acelerador (A1324LUA-T)
- **Voltaje sensor: 5V** (alimentación)
- NO requiere optoacoplador
- ⚠️ Requiere DIVISOR RESISTIVO (2.7kΩ + 4.7kΩ) para reducir 5V → 3.3V
- Conectado a GPIO 35 (ADC)

---

## Conexión Física HY-M158

### Lado de Entrada (12V/5V)
```
VCC   → +12V o +5V (según sensor)
GND   → GND común
IN1-8 → Señal del sensor/botón
```

### Lado de Salida (3.3V)
```
VCC   → +3.3V
GND   → GND común
OUT1-8 → GPIO ESP32-S3 o MCP23017
```

---

## Ventajas del Sistema con Optoacopladores

✅ **Aislamiento galvánico:** Protege ESP32 y MCP23017 de sobrevoltajes  
✅ **Compatibilidad:** Acepta señales 5V y 12V  
✅ **Protección:** Evita daños por cortocircuitos en sensores  
✅ **Ruido:** Reduce interferencias eléctricas  
✅ **Seguridad:** Aísla circuito de potencia del control  

---

## Expansión Futura

Si se necesitan más canales para sensores adicionales:
- Añadir módulos HY-M158 adicionales
- Cada módulo añade 8 canales más
- GPIOs libres disponibles: GPIO 18, 19, 45, 46 (liberados en v2.3.0)
- MCP23017 GPIOB5-B7 disponibles para más entradas

---

**Documento actualizado:** 2025-12-01  
**Firmware versión:** v2.8.5  
**Placa:** ESP32-S3-DevKitC-1 (44 pines)
