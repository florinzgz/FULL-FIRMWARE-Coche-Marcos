# 🔬 ANÁLISIS TÉCNICO: Por qué variant_path causa Bootloop

## Flujo de Inicialización Arduino-ESP32

### Secuencia Normal de Arranque

```
1. ROM Bootloader (Stage 1)
   ├─ Inicializa hardware básico
   ├─ Configura reloj del sistema
   └─ Carga Second Stage Bootloader desde flash

2. Second Stage Bootloader (Stage 2)
   ├─ Inicializa flash y PSRAM
   ├─ Lee tabla de particiones
   ├─ Selecciona partición app a arrancar
   └─ Carga firmware principal

3. ESP-IDF Runtime (libesp32.a)
   ├─ Inicializa heap y memoria
   ├─ Configura watchdog timer
   ├─ Inicializa FreeRTOS
   └─ Llama a app_main()

4. Arduino Core (cores/esp32/main.cpp)
   ├─ loopTaskWDTEnabled = true
   ├─ initArduino() ← PUNTO CRÍTICO
   │  ├─ Serial.begin()
   │  ├─ Wire.begin()      ← Necesita pins_arduino.h
   │  ├─ SPI.begin()       ← Necesita pins_arduino.h
   │  └─ USB.begin()       ← Necesita pins_arduino.h
   ├─ startWiFi()          ← ARDUINO_EVENT_RUNNING_CORE
   ├─ setup()              ← Usuario
   └─ loop()               ← Usuario (en ARDUINO_RUNNING_CORE)
```

### ❌ Fallo sin variant_path

En el paso 4 (Arduino Core), cuando se ejecuta `initArduino()`:

```cpp
// cores/esp32/main.cpp
void initArduino() {
    // Intenta incluir pins_arduino.h
    #include "pins_arduino.h"  ← ❌ ARCHIVO NO ENCONTRADO
    
    // Sin las definiciones de pines, estas llamadas FALLAN:
    Serial.begin(115200, SERIAL_8N1, RX, TX);  ← RX y TX no definidos
    Wire.begin(SDA, SCL);                      ← SDA y SCL no definidos
    SPI.begin(SCK, MISO, MOSI, SS);           ← Pines SPI no definidos
    
    // Resultado: Excepción o comportamiento indefinido
}
```

**Consecuencias:**
- Los periféricos no se inicializan
- `setup()` nunca se ejecuta
- Watchdog timer expira (típicamente 5 segundos)
- Sistema se resetea → `rst:0x3 (RTC_SW_SYS_RST)`
- Vuelve al paso 1 → **BOOTLOOP**

## Diagnóstico del Error

### rst:0x3 (RTC_SW_SYS_RST)

```
Códigos de Reset ESP32-S3:
0x1 = POWERON_RESET        ✅ Normal
0x3 = RTC_SW_SYS_RST       ⚠️  Software reset (problema)
0x4 = DEEPSLEEP_RESET      ✅ Normal
0xc = CPU_RESET            ⚠️  CPU crash
0xe = RTCWDT_RESET         ❌ Watchdog timeout
```

**rst:0x3 indica:**
- Reset por software (no hardware)
- Típicamente causado por:
  - Watchdog timer
  - Excepción no manejada
  - `esp_restart()` llamado

### entry 0x403c98b8

```
Mapa de Memoria ESP32-S3:
0x3FC88000 - 0x3FCF0000    SRAM0/SRAM1 (416KB)
0x3D000000 - 0x3E000000    External PSRAM (hasta 32MB)
0x40370000 - 0x403E0000    ROM (448KB)
0x403c98b8 ← ✅ Dentro de ROM
```

**0x403c98b8 es la ROM del ESP32-S3:**
- Punto de entrada del First Stage Bootloader
- El sistema vuelve al inicio
- Indica que el firmware no arrancó correctamente

### Secuencia del Error

```
T+0ms:    ESP32-S3 arranca → ROM Bootloader
T+50ms:   Carga firmware desde flash
T+100ms:  ESP-IDF runtime inicializa
T+200ms:  Arduino Core → initArduino()
T+250ms:  ❌ pins_arduino.h no encontrado
T+300ms:  ❌ Serial/Wire/SPI fallan al inicializar
T+350ms:  ❌ setup() no se ejecuta
T+5000ms: ⏰ Watchdog timer expira
T+5001ms: 🔄 rst:0x3 (RTC_SW_SYS_RST)
T+5002ms: 🔙 entry 0x403c98b8 (vuelve a ROM)
          → VUELVE A T+0ms → BOOTLOOP INFINITO
```

## Resolución de Rutas en PlatformIO

### Sin variant_path (Resolución Automática)

```python
# PlatformIO builder script
def get_variant_path():
    variant = board_config.get("build.variant", "")
    
    # Busca en framework packages
    framework_dir = platform.get_package_dir("framework-arduinoespressif32")
    variant_path = os.path.join(framework_dir, "variants", variant)
    
    if os.path.exists(variant_path):  ← ⚠️ Puede fallar
        return variant_path
    else:
        # ❌ No encuentra → compilación puede fallar o warning ignorado
        return None
```

**Problemas:**
- Depende de la estructura del paquete framework
- Puede variar entre versiones de Arduino-ESP32
- La ruta puede ser incorrecta si el paquete no está instalado
- El error puede no ser evidente hasta runtime

### Con variant_path (Resolución Explícita)

```python
# PlatformIO builder script
def get_variant_path():
    variant_path = board_config.get("build.variant_path", "")
    
    if variant_path:
        # ✅ Usa la ruta del proyecto directamente
        project_variant_path = os.path.join(PROJECT_DIR, variant_path)
        
        if os.path.exists(project_variant_path):
            return project_variant_path  ← ✅ Siempre funciona
```

**Ventajas:**
- Ruta explícita y predecible
- No depende de paquetes externos
- Versionado con el proyecto
- Error claro en compile-time si falta

## Contenido de pins_arduino.h

### Definiciones Críticas

```cpp
// pins_arduino.h

// USB CDC (Serial via USB)
static const uint8_t TX = 43;    ← Sin esto, Serial.begin() falla
static const uint8_t RX = 44;    ← Sin esto, Serial.begin() falla

// I2C (Wire)
static const uint8_t SDA = 8;    ← Sin esto, Wire.begin() falla
static const uint8_t SCL = 9;    ← Sin esto, Wire.begin() falla

// SPI
static const uint8_t SS   = 10;  ← Sin esto, SPI.begin() falla
static const uint8_t MOSI = 11;
static const uint8_t MISO = 13;
static const uint8_t SCK  = 12;
```

### Uso en Arduino Core

```cpp
// cores/esp32/HardwareSerial.cpp
void HardwareSerial::begin(unsigned long baud) {
    // Usa TX y RX de pins_arduino.h
    uart_config_t conf;
    conf.tx_io_num = TX;  ← ❌ Undefined sin pins_arduino.h
    conf.rx_io_num = RX;  ← ❌ Undefined sin pins_arduino.h
    uart_param_config(uart_num, &conf);
}

// libraries/Wire/src/Wire.cpp
bool TwoWire::begin(int sdaPin, int sclPin) {
    if (sdaPin < 0) sdaPin = SDA;  ← ❌ Undefined sin pins_arduino.h
    if (sclPin < 0) sclPin = SCL;  ← ❌ Undefined sin pins_arduino.h
    // ...
}
```

## Por qué `arduino.memory_type = qio_qspi`

### Modos de Acceso a PSRAM

```
DIO (Dual I/O):
├─ 2 líneas de datos
├─ Velocidad: ~40 MB/s
└─ Compatible con todo

QIO (Quad I/O):
├─ 4 líneas de datos
├─ Velocidad: ~80 MB/s
└─ Requiere hardware compatible

QSPI (Quad SPI):
├─ 4 líneas SPI dedicadas
├─ Velocidad: ~80 MB/s
└─ ESP32-S3 N16R8 usa esto

QIO_QSPI (Combinado):
├─ QIO para flash
├─ QSPI para PSRAM
├─ Velocidad óptima
└─ ✅ Configuración recomendada para N16R8
```

**Sin `arduino.memory_type`:**
- PlatformIO usa DIO por defecto
- PSRAM funciona pero MÁS LENTO
- Puede causar timeouts en operaciones intensivas

**Con `arduino.memory_type = qio_qspi`:**
- ✅ Máximo rendimiento de PSRAM
- ✅ Compatible con hardware N16R8
- ✅ Reduce latencia en accesos a memoria

## Por qué ARDUINO_RUNNING_CORE=1

### Arquitectura Dual-Core ESP32-S3

```
ESP32-S3 tiene 2 cores (Xtensa LX7):

Core 0 (APP_CPU):
├─ Tareas de sistema
├─ WiFi/Bluetooth stack
├─ Timers del sistema
└─ Interrupciones de alta prioridad

Core 1 (PRO_CPU):
├─ Tareas de aplicación
├─ Arduino loop()
├─ Tareas de usuario
└─ ✅ RECOMENDADO para Arduino
```

**Sin ARDUINO_RUNNING_CORE definido:**
- Arduino elige core automáticamente
- Puede elegir Core 0 (ocupado con WiFi/BT)
- Rendimiento degradado
- Posibles conflictos con tasks del sistema

**Con ARDUINO_RUNNING_CORE=1:**
- ✅ loop() siempre en Core 1
- ✅ Core 0 libre para WiFi/BT
- ✅ Mejor rendimiento
- ✅ Menos latencia en respuesta

## Conclusión Técnica

### El Bootloop ocurre por una cascada de fallos:

```
1. variant_path ausente
   ↓
2. PlatformIO no resuelve correctamente la ruta
   ↓
3. Compilador no encuentra pins_arduino.h
   ↓
4. Definiciones de pines quedan undefined
   ↓
5. initArduino() falla al inicializar periféricos
   ↓
6. setup() nunca se ejecuta
   ↓
7. Watchdog timer expira
   ↓
8. Sistema resetea → rst:0x3
   ↓
9. Vuelve a ROM → entry 0x403c98b8
   ↓
10. BOOTLOOP INFINITO
```

### La solución es simple pero crítica:

```json
{
  "variant_path": "variants/esp32s3"  ← Ruta explícita
}
```

Esto garantiza que Arduino **SIEMPRE** encuentre `pins_arduino.h`,
evitando el fallo en `initArduino()` y permitiendo que el
firmware arranque correctamente.

---

**Moraleja:** En sistemas embebidos, **explicit is better than implicit**.
Confiar en resolución automática puede funcionar el 99% del tiempo,
pero el 1% restante causa bootloops difíciles de diagnosticar.
