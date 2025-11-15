# Mapeo Completo Módulos HY-M158 Optoacopladores

## Configuración Hardware
- 2x Módulos HY-M158 (8 canales c/u)
- Total: 16 canales disponibles
- Función: Aislar señales 12V/5V → 3.3V ESP32-S3

---

## HY-M158 Módulo #1 - Sensores y Encoder

| Canal | GPIO ESP32 | Señal              | Tipo    | Descripción                          |
|-------|------------|--------------------|---------|------------------------------------|
| CH1   | GPIO 35    | WHEEL_FL           | 5V      | Sensor rueda frontal izquierda     |
| CH2   | GPIO 36    | WHEEL_FR           | 5V      | Sensor rueda frontal derecha       |
| CH3   | GPIO 41    | WHEEL_RL           | 5V      | Sensor rueda trasera izquierda     |
| CH4   | GPIO 40    | WHEEL_RR           | 5V      | Sensor rueda trasera derecha       |
| CH5   | GPIO 37    | ENCODER_A          | 5V      | Encoder dirección canal A          |
| CH6   | GPIO 38    | ENCODER_B          | 5V      | Encoder dirección canal B          |
| CH7   | GPIO 46    | ENCODER_Z          | 5V      | Encoder dirección señal Z (centro) |
| CH8   | GPIO 39    | BTN_MULTIMEDIA     | 12V     | Botón multimedia                   |

---

## HY-M158 Módulo #2 - Shifter (Palanca de Cambios)

| Canal | GPIO ESP32 | Señal              | Tipo    | Descripción                          |
|-------|------------|--------------------|---------|------------------------------------|
| CH1   | GPIO 13    | SHIFTER_P          | 12V     | Posición P (Park)                  |
| CH2   | GPIO 14    | SHIFTER_D2         | 12V     | Posición D2 (Drive 2)              |
| CH3   | GPIO 15    | SHIFTER_D1         | 12V     | Posición D1 (Drive 1)              |
| CH4   | GPIO 16    | SHIFTER_N          | 12V     | Posición N (Neutral)               |
| CH5   | GPIO 17    | SHIFTER_R          | 12V     | Posición R (Reverse)               |
| CH6   | —          | RESERVA            | —       | Disponible                         |
| CH7   | —          | RESERVA            | —       | Disponible                         |
| CH8   | —          | RESERVA            | —       | Disponible                         |

---

## Resumen de Uso

**Total canales usados:** 13 de 16
**Canales libres:** 3

**Distribución por tipo de señal:**
- Sensores inductivos ruedas (LJ12A3-4-Z/BX): 4 canales
- Encoder dirección (E6B2-CWZ6C): 3 canales (A, B, Z)
- Shifter 12V: 5 canales
- Botón multimedia 12V: 1 canal

---

## Notas Importantes

### Sensores LJ12A3-4-Z/BX (Ruedas)
- Alimentación: 5V (convertida desde 12V)
- Salida: NPN normalmente abierta
- 6 tornillos por rueda = 6 pulsos/revolución

### Encoder E6B2-CWZ6C 1200PR (Dirección)
- Alimentación: 5V (convertida desde 12V)
- Resolución: 1200 pulsos/revolución
- Ratio: 1:1 al eje del volante
- Señal Z: Marca punto cero (centro)

### Shifter (Palanca de Cambios)
- Señales: 12V (5 posiciones exclusivas)
- Lógica: LOW activo (pull-up interno ESP32)
- Prioridad lectura: P > D2 > D1 > N > R

### Botón Multimedia
- Señal: 12V
- Lógica: LOW activo (pull-up interno ESP32)
- Función: Toggle estado multimedia

---

## Conexión Física HY-M158

### Lado de Entrada (12V/5V)
```
VCC   → +12V o +5V (según sensor)
GND   → GND común
IN1-8 → Señal del sensor/botón
```

### Lado de Salida (3.3V ESP32)
```
VCC   → +3.3V ESP32
GND   → GND ESP32
OUT1-8 → GPIO ESP32-S3
```

---

## Ventajas del Sistema con Optoacopladores

✅ **Aislamiento galvánico:** Protege ESP32 de sobrevoltajes  
✅ **Compatibilidad:** Acepta señales 5V y 12V  
✅ **Protección:** Evita daños por cortocircuitos en sensores  
✅ **Ruido:** Reduce interferencias eléctricas  
✅ **Seguridad:** Aísla circuito de potencia del control  

---

## Expansión Futura

Si se necesitan más canales para sensores adicionales:
- Añadir módulos HY-M158 adicionales
- Cada módulo añade 8 canales más
- GPIOs libres disponibles en ESP32-S3: varios (verificar pins.h)

---

**Documento generado:** 2025-01-07  
**Firmware versión:** ESP32-S3-DevKitC-1 compatible
