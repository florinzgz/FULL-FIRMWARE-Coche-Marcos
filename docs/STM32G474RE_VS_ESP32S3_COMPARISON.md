# Comparación: STM32G474RE vs ESP32-S3 N16R8

**Fecha:** 2026-01-13  
**Propósito:** Análisis comparativo para proyecto de control de vehículo eléctrico  
**Hardware Actual:** ESP32-S3 N16R8 (16MB Flash + 8MB PSRAM)  
**Hardware Analizado:** STM32G474RE (512KB Flash + 128KB SRAM)

---

## 📊 Tabla Comparativa General

| Característica | ESP32-S3 N16R8 | STM32G474RE |
|----------------|----------------|-------------|
| **Arquitectura** | Dual-core Xtensa LX7 | ARM Cortex-M4 |
| **Frecuencia** | 240 MHz | 170 MHz |
| **Cores** | 2 | 1 |
| **FPU** | Sí | Sí |
| **Flash** | 16 MB | 512 KB |
| **RAM** | 8 MB PSRAM + 512KB SRAM | 128 KB SRAM |
| **Voltaje** | 3.3V | 1.71V - 3.6V |
| **Conectividad** | WiFi, BLE | Ninguna |
| **Precio aprox.** | $3-4 USD | $4-6 USD |

---

## 🎯 Análisis por Categorías

### 1. Procesamiento y Memoria

#### ESP32-S3 N16R8 ✅
**Ventajas:**
- **Memoria Masiva:** 16 MB Flash, 8 MB PSRAM
- **Dual-Core:** Permite separación de tareas (UI + Control)
- **Ideal para:** Interfaces gráficas complejas, almacenamiento de datos, múltiples tareas

**Desventajas:**
- Mayor consumo energético en operación
- Arquitectura Xtensa menos estándar

#### STM32G474RE ⚠️
**Ventajas:**
- **ARM Estándar:** Cortex-M4 ampliamente soportado
- **Aceleradores:** CORDIC, FMAC para matemáticas específicas
- **Eficiencia:** Bajo consumo en modos sleep
- **Memoria Razonable:** 512 KB Flash, 128 KB RAM

**Desventajas:**
- **Memoria Limitada vs ESP32:** 512 KB Flash vs 16 MB
- **No viable para:** UI gráfica compleja con TFT 480×320
- **Almacenamiento:** Requiere memoria externa para datos grandes

---

### 2. Control de Motores

#### STM32G474RE ⭐ GANADOR
**Hardware Especializado:**
- ✅ **HRTIM:** 184 ps de resolución PWM
- ✅ **5 ADCs:** 12-bit, 4 Msps, conversión sincronizada
- ✅ **7 Comparadores:** Ultra-rápidos para protección
- ✅ **6 Op-Amps:** Acondicionamiento de señales integrado
- ✅ **CORDIC:** Aceleración de transformadas Park/Clarke
- ✅ **FMAC:** Filtros digitales por hardware

**Aplicaciones Ideales:**
- Control vectorial (FOC) de motores BLDC/PMSM
- Frecuencias de PWM >100 kHz con dead-time preciso
- Bucles de control >20 kHz
- Medición sincronizada de corrientes trifásicas

#### ESP32-S3 N16R8
**Capacidades:**
- ✅ PWM por MCPWM y LEDC
- ✅ ADCs integrados (2 × 13-bit)
- ⚠️ **Sin hardware especializado** para control de motores
- ⚠️ **PWM de resolución estándar** (~1 µs típica)
- ⚠️ **ADCs más lentos** comparado con STM32G4

**Aplicaciones Adecuadas:**
- Control básico de motores DC
- Control de servos con PCA9685 (externo)
- PWM para iluminación LED
- Control de velocidad con BTS7960

**Limitaciones:**
- No apto para FOC de alto rendimiento
- Latencia mayor en bucles de control
- Sin sincronización hardware ADC-PWM

---

### 3. Interfaz Gráfica de Usuario (TFT Display)

#### ESP32-S3 N16R8 ⭐ GANADOR
**Ventajas Críticas:**
- ✅ **16 MB Flash:** Suficiente para assets gráficos, fuentes
- ✅ **8 MB PSRAM:** Framebuffers, caché de pantalla
- ✅ **Dual-Core:** Core 0 para UI, Core 1 para control
- ✅ **DMA para SPI:** Transferencias eficientes al TFT
- ✅ **Librerías maduras:** TFT_eSPI, LVGL

**Configuración Actual:**
- Display ST7796S 480×320 (16-bit color)
- Touch XPT2046
- HSPI @ 40 MHz
- Fuentes múltiples, gráficos, iconos

#### STM32G474RE ❌ NO VIABLE
**Problemas Fundamentales:**
- ⚠️ **512 KB Flash:** Limitado comparado con ESP32, pero para framebuffer + código
  - Framebuffer 480×320×16-bit = 307 KB (2.4× la Flash total!)
- ❌ **128 KB RAM:** No puede alojar framebuffer completo
- ❌ **Sin controlador de display:** Requiere bit-banging o controlador externo
- ❌ **Memoria externa requerida:** Quad-SPI para Flash/RAM externa

**Posible con:**
- Display mucho más pequeño (128×64 monocromo)
- Memoria externa (Quad-SPI Flash + SRAM)
- Controlador de display externo
- Mayor complejidad y costo

---

### 4. Conectividad

#### ESP32-S3 N16R8 ⭐ GANADOR
**Integrado:**
- ✅ WiFi 802.11 b/g/n (2.4 GHz)
- ✅ Bluetooth 5.0 LE
- ✅ USB OTG (dispositivo y host)

**Notas del Proyecto:**
- WiFi/BLE **deshabilitados** por seguridad (v2.11.0+)
- Firmware 100% standalone
- Solo USB para programación/debug

#### STM32G474RE
**Integrado:**
- ✅ USB Device (Full-speed 2.0)
- ✅ USB Type-C / Power Delivery (UCPD)
- ❌ Sin WiFi
- ❌ Sin Bluetooth

**Comunicación Industrial:**
- ✅ **3 × CAN FD:** Ideal para automotive/industrial
- ✅ **4 × I2C, 4 × SPI**
- ✅ **5 × USART/UART**

---

### 5. Periféricos del Proyecto Actual

#### Análisis de Compatibilidad

| Periférico | ESP32-S3 | STM32G474RE | Notas |
|------------|----------|-------------|-------|
| **Display ST7796S** | ✅ Nativo SPI | ⚠️ Posible con SPI | Requiere memoria externa |
| **Touch XPT2046** | ✅ Compartido SPI | ✅ Compatible SPI | OK |
| **PCA9685 (PWM)** | ✅ I2C | ✅ I2C | OK |
| **BTS7960 (Motor)** | ✅ PWM + GPIO | ✅ PWM + GPIO | STM32 mejor PWM |
| **INA226 (Corriente)** | ✅ I2C | ✅ I2C | STM32 ADCs internos mejores |
| **DS18B20 (Temp)** | ✅ OneWire | ✅ GPIO (bitbang) | OK |
| **WS2812B (LEDs)** | ✅ RMT | ⚠️ SPI/Timer | ESP32 mejor |
| **DFPlayer Mini** | ✅ UART | ✅ UART | OK |
| **Encoder Magnético** | ✅ GPIO + Interrupts | ✅ GPIO + Interrupts | OK |
| **MCP23017 (I2C)** | ✅ I2C | ✅ I2C | OK |

**Problemas Críticos con STM32G474RE:**
1. ❌ **Display 480×320:** No viable sin memoria externa
2. ⚠️ **WS2812B (44 LEDs):** Más complejo sin periférico RMT
3. ⚠️ **Memoria:** Código actual no cabe en 128 KB

---

### 6. Desarrollo y Ecosistema

#### ESP32-S3 N16R8
**Framework:** Arduino-ESP32 + ESP-IDF
- ✅ **PlatformIO:** Excelente integración
- ✅ **Librerías abundantes:** TFT_eSPI, FastLED, etc.
- ✅ **Comunidad grande:** Mucha documentación
- ✅ **Ejemplos:** Miles de proyectos opensource

**Toolchain:**
- GCC para Xtensa
- Documentación en español disponible
- Debugging via JTAG/USB

#### STM32G474RE
**Framework:** STM32Cube (HAL/LL) + Arduino
- ✅ **STM32CubeIDE:** IDE gráfico profesional
- ✅ **STM32CubeMX:** Configuración visual
- ✅ **HAL/LL:** APIs bien documentadas
- ✅ **ARM estándar:** Debuggers compatibles

**Toolchain:**
- GCC ARM Embedded
- ST-LINK debugger
- Motor Control Workbench para motores

---

### 7. Consumo de Energía

#### STM32G474RE ⭐ GANADOR
**Modos de Bajo Consumo:**
- **Run:** ~100 µA/MHz
- **Sleep:** ~50 µA/MHz
- **Stop:** ~5-10 µA
- **Standby:** ~1-2 µA
- **Shutdown:** ~30 nA

**Ideal para:** Aplicaciones con baterías

#### ESP32-S3 N16R8
**Consumo:**
- **Active (CPU):** ~40-50 mA
- **Modem-sleep:** ~20-30 mA (WiFi off)
- **Light-sleep:** ~800 µA - 5 mA
- **Deep-sleep:** ~10-150 µA

**Notas:** Mayor consumo debido a dual-core y periféricos

---

### 8. Costo y Disponibilidad

#### ESP32-S3 N16R8
- **Precio módulo:** $3-4 USD
- **DevKit:** $8-12 USD
- **Disponibilidad:** Excelente
- **Proveedores:** Múltiples (Espressif, third-party)

#### STM32G474RE
- **Precio chip:** $4-6 USD
- **Núcleo mínimo:** Requiere cristal, caps, regulador
- **Disponibilidad:** Buena (2026)
- **Proveedores:** ST, distribuidores globales

**Nota:** Precio similar, pero ESP32-S3 más fácil de usar (módulo completo)

---

## 🎯 Análisis de Viabilidad para este Proyecto

### Requerimientos del Sistema Actual

1. ✅ **Display TFT 480×320** con touch
2. ✅ **Control de 4 motores DC** con BTS7960
3. ✅ **Sensores múltiples:** INA226, DS18B20, encoders
4. ✅ **Iluminación:** 44 LEDs WS2812B
5. ✅ **Audio:** DFPlayer Mini
6. ✅ **Sistemas de seguridad:** ABS, TCS (software)
7. ✅ **Interfaz de usuario compleja**

### Veredicto por Plataforma

#### ESP32-S3 N16R8: ✅ **EXCELENTE FIT**

**Fortalezas para este proyecto:**
- ✅ Memoria suficiente para UI gráfica compleja
- ✅ Dual-core permite UI fluida + control en paralelo
- ✅ PSRAM para framebuffers y caché
- ✅ RMT para WS2812B eficiente
- ✅ Suficiente I/O (GPIOs) para todos los periféricos
- ✅ Ecosistema maduro con librerías necesarias
- ✅ Desarrollo rápido con Arduino framework

**Limitaciones aceptadas:**
- ⚠️ Control de motores básico (no FOC)
- ⚠️ PWM de resolución estándar (suficiente para DC)
- ⚠️ Mayor consumo (no crítico con batería grande)

#### STM32G474RE: ❌ **NO VIABLE**

**Problemas bloqueantes:**
1. ⚠️ **Memoria limitada vs ESP32:** 512 KB Flash << 16 MB Flash actual
2. ❌ **Display imposible:** Sin espacio para framebuffer
3. ❌ **Requiere rediseño completo** con memoria externa
4. ❌ **Mayor complejidad** y costo final
5. ❌ **WS2812B** más difícil sin RMT

**Ventajas no utilizadas:**
- 🔸 Control avanzado de motores (no requerido aquí)
- 🔸 ADCs ultra-rápidos (INA226 externo suficiente)
- 🔸 HRTIM (BTS7960 con PWM estándar OK)

---

## 💡 Conclusiones y Recomendaciones

### Para el Proyecto Actual (Vehículo Eléctrico con Display)

**MANTENER ESP32-S3 N16R8** ✅

**Razones:**
1. ✅ **Memoria abundante:** Permite UI rica y evolutiva
2. ✅ **Dual-core:** Separación limpia UI/Control
3. ✅ **Ecosistema:** Desarrollo rápido, librerías probadas
4. ✅ **Periféricos:** RMT para LEDs, SPI rápido para TFT
5. ✅ **Futuro:** Espacio para nuevas features

### Casos donde STM32G474RE sería Superior

#### Proyecto de Control Puro de Motor (sin UI)
**Requerimientos:**
- Motor BLDC/PMSM de alto rendimiento
- Control FOC a >20 kHz
- Sin display gráfico (solo LEDs/LCD básico)
- Eficiencia energética crítica
- Comunicación CAN FD

**Ejemplo:**
- Controlador ESC (Electronic Speed Controller)
- Inversor de potencia
- Servo-drive industrial

#### Características que Aprovecharían STM32G474:
- ✅ HRTIM para PWM de 184 ps
- ✅ ADCs sincronizados para corrientes trifásicas
- ✅ CORDIC para transformadas rápidas
- ✅ Comparadores para protección instantánea
- ✅ CAN FD para comunicación industrial

---

## 📋 Tabla de Decisión

| Criterio | Peso | ESP32-S3 | STM32G474RE |
|----------|------|----------|-------------|
| **Memoria para UI** | 30% | 10/10 ✅ | 1/10 ❌ |
| **Control de Motores** | 15% | 6/10 ⚠️ | 10/10 ✅ |
| **Periféricos del proyecto** | 20% | 9/10 ✅ | 5/10 ⚠️ |
| **Facilidad de desarrollo** | 15% | 9/10 ✅ | 7/10 ⚠️ |
| **Costo total** | 10% | 9/10 ✅ | 7/10 ⚠️ |
| **Consumo energético** | 5% | 5/10 ⚠️ | 9/10 ✅ |
| **Conectividad (futuro)** | 5% | 10/10 ✅ | 3/10 ❌ |

### Puntuación Ponderada

- **ESP32-S3 N16R8:** 8.4/10 ✅
- **STM32G474RE:** 5.7/10 ⚠️

---

## 🚀 Recomendación Final

### Para este Proyecto: **ESP32-S3 N16R8** 🏆

El ESP32-S3 N16R8 es la plataforma correcta para este sistema de control de vehículo con interfaz gráfica. La memoria abundante, dual-core, y periféricos versátiles lo hacen ideal para la aplicación actual.

### STM32G474RE: Excelente, pero para Otro Proyecto

El STM32G474RE es un microcontrolador excepcional para **control especializado de motores** y **electrónica de potencia**, pero no es adecuado para sistemas con interfaces gráficas complejas debido a sus limitaciones de memoria.

### Uso Potencial Futuro

Si en el futuro se requiere un **controlador dedicado de motores de alto rendimiento** (separado del sistema principal), el STM32G474RE sería una excelente opción para ese sub-sistema específico, comunicándose con el ESP32-S3 principal vía CAN, I2C, o UART.

**Arquitectura sugerida (solo si se requiere FOC):**
```
┌─────────────────────┐
│   ESP32-S3 N16R8    │ ← Main controller
│   - UI (TFT)        │
│   - Telemetry       │
│   - Sensors         │
│   - Decision logic  │
└──────────┬──────────┘
           │ CAN/UART
           ▼
┌─────────────────────┐
│  STM32G474RE        │ ← Motor controller
│   - FOC algorithm   │
│   - Current sensing │
│   - PWM generation  │
│   - Protection      │
└─────────────────────┘
```

Pero para el alcance actual del proyecto, el ESP32-S3 solo es suficiente y más eficiente. ✅

---

**Documento creado:** 2026-01-13  
**Autor:** Análisis comparativo técnico  
**Versión:** 1.0  
**Proyecto:** FULL-FIRMWARE-Coche-Marcos v2.17.1
