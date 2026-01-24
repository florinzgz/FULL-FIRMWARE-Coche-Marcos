# Respuesta: Transreceptores y Conexión ESP32-S3 ↔ STM32G474RE

**Fecha:** 2026-01-24  
**Pregunta Original:** *"¿Cuántos transreceptores has añadido para la implementación con el STM32G474RE? ¿Has añadido un manual? Estúdialo y dime cómo has hecho la conexión entre el ESP32-S3 y el STM32G474RE"*

---

## Respuesta Rápida

### 1. ¿Cuántos transreceptores se han añadido?

**RESPUESTA: DOS (2) TRANSRECEPTORES CAN**

- **Transreceptor #1:** TJA1051T/3 para **STM32G474RE** (Control seguro)
- **Transreceptor #2:** TJA1051T/3 para **ESP32-S3** (HMI)

### 2. ¿Hay un manual?

**SÍ ✅** - Se ha creado un manual técnico completo:

📄 **[docs/MANUAL_TRANSRECEPTORES_STM32_ESP32.md](docs/MANUAL_TRANSRECEPTORES_STM32_ESP32.md)**

Este manual incluye:
- ✅ Especificaciones detalladas de los transreceptores TJA1051T/3
- ✅ Diagramas esquemáticos de conexión completos
- ✅ Configuración de pines para ambos microcontroladores
- ✅ Código de ejemplo (STM32 FDCAN y ESP32 TWAI)
- ✅ Especificaciones del bus CAN @ 500 kbps
- ✅ Guía de validación y troubleshooting
- ✅ Referencias técnicas y datasheets

### 3. ¿Cómo se hace la conexión ESP32-S3 ↔ STM32G474RE?

**Resumen de la Conexión:**

```
ESP32-S3 (HMI)                         STM32G474RE (Control)
     │                                        │
     │ GPIO 20 (TX) ──┐                      │
     │ GPIO 21 (RX) ──┤                      │
     │                │                      │
     ▼                ▼                      ▼
┌─────────────┐  ┌─────────────┐      ┌─────────────┐
│ TJA1051T #2 │  │  BUS CAN    │      │ TJA1051T #1 │
│             │  │             │      │             │
│ CANH ───────┼──┤ CANH ────── │──────┼─── CANH     │
│ CANL ───────┼──┤ CANL ────── │──────┼─── CANL     │
│             │  │   500 kbps  │      │             │
│             │  │   120Ω Term │      │             │
└─────────────┘  └─────────────┘      └─────────────┘
                                            │
                                            │ PB9 (TX)
                                            │ PB8 (RX)
                                            ▼
                                     STM32G474RE
                                       FDCAN1
```

**Detalles Clave:**

1. **Protocolo:** CAN (Controller Area Network) @ 500 kbps
2. **Transreceptores:** TJA1051T/3 (compatible con 3.3V y 5V)
3. **Pines STM32:** PB8 (FDCAN1_RX), PB9 (FDCAN1_TX)
4. **Pines ESP32:** GPIO 20 (TWAI_TX), GPIO 21 (TWAI_RX) - propuestos
5. **Bus físico:** Par trenzado con terminaciones de 120Ω en ambos extremos
6. **Alimentación transreceptores:** +5V (lógica compatible con 3.3V)

**Separación de Responsabilidades:**

| Microcontrolador | Rol | Funciones Principales |
|------------------|-----|----------------------|
| **ESP32-S3** | HMI (Interfaz Humano-Máquina) | Display TFT, Touch, Audio, LEDs, Menús, Diagnóstico visual |
| **STM32G474RE** | Control Seguro | Motores (4× tracción + dirección), Sensores críticos, ABS/TCS, Relés de potencia |

---

## Documentación Completa

Para información detallada, consultar:

### Manual Principal
📄 **[docs/MANUAL_TRANSRECEPTORES_STM32_ESP32.md](docs/MANUAL_TRANSRECEPTORES_STM32_ESP32.md)**  
Manual técnico completo de 36KB con:
- Especificaciones de hardware
- Diagramas de conexión detallados
- Configuración de software
- Código de ejemplo
- Validación y pruebas
- Troubleshooting

### Documentación de Referencia

1. **[docs/STM32G474RE_PINOUT_DEFINITIVO.md](docs/STM32G474RE_PINOUT_DEFINITIVO.md)**  
   Pinout completo del STM32G474RE con asignación de pines FDCAN1

2. **[docs/PLAN_SEPARACION_STM32_CAN.md](docs/PLAN_SEPARACION_STM32_CAN.md)**  
   Plan de arquitectura dual ESP32 HMI + STM32 Control

3. **[docs/DESIGN_FREEZE_STM32G474RE.md](docs/DESIGN_FREEZE_STM32G474RE.md)**  
   Design freeze del pinout con correcciones aplicadas

4. **[docs/AUDITORIA_PINOUT_STM32G474RE.md](docs/AUDITORIA_PINOUT_STM32G474RE.md)**  
   Auditoría técnica del diseño hardware

5. **[docs/STM32_CAN_MIGRATION_STUDY.md](docs/STM32_CAN_MIGRATION_STUDY.md)**  
   Estudio de integración completo

---

## Diagrama de Conexión Simplificado

```
┌──────────────────────────────────────────────────────────────┐
│                      ESP32-S3 N16R8 (HMI)                    │
│  • Display TFT ST7796S 480×320 con touch XPT2046             │
│  • Audio DFPlayer Mini                                       │
│  • LEDs WS2812B (28 frontales + 16 traseros)                 │
│  • Detección de obstáculos TOFSense                          │
│  • Menús, diagnóstico y visualización                        │
└────────────────────┬─────────────────────────────────────────┘
                     │
                     │ GPIO 20/21 → TJA1051T/3 #2
                     │
                     ▼
            ┌────────────────┐
            │   CAN BUS      │ ← 500 kbps, Classic CAN
            │  CANH / CANL   │    Par trenzado, 120Ω terminación
            └────────────────┘
                     │
                     │ PB8/PB9 ← TJA1051T/3 #1
                     ▼
┌──────────────────────────────────────────────────────────────┐
│                   STM32G474RE (CONTROL SEGURO)               │
│  • Motores: 4× tracción (BTS7960) + 1× dirección            │
│  • Encoder dirección E6B2-CWZ6C (360 PPR)                    │
│  • Sensores de rueda × 4 (velocidad)                         │
│  • Sensores corriente INA226 × 6 (vía I2C + TCA9548A)        │
│  • Sensores temperatura DS18B20                              │
│  • Pedal analógico Hall                                      │
│  • Shifter mecánico (Forward/Neutral/Reverse)                │
│  • Relés de potencia × 3 (Main, Tracción, Dirección)         │
│  • Lógica ABS/TCS y seguridad                                │
└──────────────────────────────────────────────────────────────┘
```

---

## Estado del Proyecto

**Firmware ESP32-S3:** ✅ Operativo (v2.17.1 PHASE 14)  
**Pinout STM32G474RE:** ✅ Definido y congelado  
**Transreceptores CAN:** ✅ Especificados (TJA1051T/3 × 2)  
**Documentación:** ✅ Completa  
**Implementación física:** ⏳ Pendiente (fase de planificación)

### Próximos Pasos

1. ⏳ Validar pines GPIO finales para TWAI en ESP32-S3
2. ⏳ Fabricar PCB prototipo con transreceptores
3. ⏳ Implementar stack CAN en firmware de ambos MCUs
4. ⏳ Pruebas de integración hardware
5. ⏳ Migración progresiva según plan de fases

---

## Referencias Rápidas

### Especificaciones TJA1051T/3

| Parámetro | Valor |
|-----------|-------|
| Tipo | High-Speed CAN Transceiver |
| Estándar | ISO 11898-2 |
| Velocidad | Hasta 1 Mbps (configurado a 500 kbps) |
| Alimentación | 5V (lógica compatible con 3.3V) |
| Temperatura | -40°C a +125°C |
| Encapsulado | SO-8 |

### Pines de Conexión

**STM32G474RE (FDCAN1):**
- PB8: FDCAN1_RX (Alternate Function 9)
- PB9: FDCAN1_TX (Alternate Function 9)

**ESP32-S3 (TWAI - propuesto):**
- GPIO 20: TWAI_TX (configurable)
- GPIO 21: TWAI_RX (configurable)

### Parámetros CAN

- **Protocolo:** CAN 2.0A/B (Classic CAN)
- **Velocidad:** 500 kbps
- **Sample Point:** ~81-87%
- **Formato ID:** Standard 11-bit (0x000 a 0x7FF)
- **DLC máximo:** 8 bytes
- **Terminación:** 120Ω en ambos extremos

---

## Contacto y Soporte

Para preguntas o aclaraciones sobre esta implementación:

- **Repositorio:** [florinzgz/FULL-FIRMWARE-Coche-Marcos](https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos)
- **Documentación completa:** `docs/` en el repositorio
- **Issues:** GitHub Issues del proyecto

---

**Última actualización:** 2026-01-24  
**Versión del documento:** 1.0  
**Autor:** Documentación Técnica del Proyecto
