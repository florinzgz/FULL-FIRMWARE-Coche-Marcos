# Arquitectura del Firmware - ESP32-S3 Vehicle Control

## Versión
v2.11.5+

## Índice
1. [Visión General](#visión-general)
2. [Diagrama de Componentes](#diagrama-de-componentes)
3. [Dependencias entre Managers](#dependencias-entre-managers)
4. [Secuencia de Inicialización](#secuencia-de-inicialización)
5. [Flujo del Loop Principal](#flujo-del-loop-principal)
6. [Thread Safety](#thread-safety)
7. [Modos de Operación](#modos-de-operación)
8. [Gestión de Errores](#gestión-de-errores)
9. [Memoria](#memoria)
10. [Referencias](#referencias)

---

## Visión General

El firmware está diseñado con arquitectura modular basada en **Managers** que encapsulan funcionalidades específicas.

**Principios de diseño:**
- ✅ Separación de responsabilidades
- ✅ Bajo acoplamiento entre módulos
- ✅ Alto cohesión dentro de cada módulo
- ✅ Tolerancia a fallos (degradación gradual)
- ✅ Auto-recuperación cuando es posible

---

## Diagrama de Componentes

```
┌─────────────────────────────────────────────────────────┐
│                     MAIN LOOP                           │
│  (src/main.cpp)                                         │
└─────────────────────────────────────────────────────────┘
                        │
        ┌───────────────┼───────────────┐
        ▼               ▼               ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│ Power        │ │ Sensor       │ │ Safety       │
│ Manager      │ │ Manager      │ │ Manager      │
└──────────────┘ └──────────────┘ └──────────────┘
        │               │               │
        └───────────────┼───────────────┘
                        ▼
        ┌───────────────┴───────────────┐
        ▼                               ▼
┌──────────────┐                 ┌──────────────┐
│ Control      │                 │ HUD          │
│ Manager      │                 │ Manager      │
└──────────────┘                 └──────────────┘
        │                               │
        ▼                               ▼
┌──────────────┐                 ┌──────────────┐
│ Traction     │                 │ Display      │
│ System       │                 │ (TFT+Touch)  │
└──────────────┘                 └──────────────┘
```

---

## Dependencias entre Managers

### **Nivel 0 (Sin dependencias)**
- `PowerManager` - Gestión de alimentación y relés

### **Nivel 1 (Dependen solo de Nivel 0)**
- `SensorManager` → `PowerManager`
  - Requiere sensores alimentados

### **Nivel 2 (Dependen de Nivel 0-1)**
- `SafetyManager` → `SensorManager`
  - Lee estado de sensores para validar seguridad
  
- `HUDManager` → `PowerManager`
  - Requiere display alimentado

### **Nivel 3 (Dependen de Nivel 0-2)**
- `ControlManager` → `SensorManager`, `SafetyManager`
  - Lee sensores para control
  - Valida condiciones de seguridad antes de actuar
  
- `ModeManager` → `SensorManager`, `SafetyManager`
  - Determina modo operativo según estado de sensores/seguridad

### **Nivel 4 (Dependen de todos)**
- `TelemetryManager` → Todos los managers
  - Recopila datos de todos los módulos

**Regla de oro:** Un manager **NUNCA** debe depender de managers de nivel superior.

---

## Secuencia de Inicialización

```cpp
void setup() {
    // 1. CORE SYSTEM (orden crítico)
    Serial.begin(115200);
    System::init();           // Estado del sistema
    Storage::init();          // Configuración EEPROM
    Watchdog::init();         // Watchdog (30s timeout)
    Watchdog::feed();
    Logger::init();           // Sistema de logging
    Watchdog::feed();
    
    // 2. MANAGERS (orden por niveles de dependencia)
    initializeSystem();
}

void initializeSystem() {
    // Nivel 0
    PowerManager::init();     // Alimentación
    Watchdog::feed();
    
    // Nivel 1
    SensorManager::init();    // Sensores (depende de Power)
    Watchdog::feed();
    
    // Nivel 2
    SafetyManager::init();    // Seguridad (depende de Sensors)
    Watchdog::feed();
    HUDManager::init();       // Display (depende de Power)
    Watchdog::feed();
    
    // Nivel 3
    ControlManager::init();   // Control (depende de Sensors+Safety)
    Watchdog::feed();
    ModeManager::init();      // Modos (depende de Sensors+Safety)
    Watchdog::feed();
    
    // Nivel 4
    TelemetryManager::init(); // Telemetría (depende de todos)
    Watchdog::feed();
}
```

**Tiempo total de inicialización:** ~2-5 segundos  
**Watchdog feeds:** 7 feeds mínimo (uno entre cada manager)

---

## Flujo del Loop Principal

```cpp
void loop() {
    // 1. WATCHDOG (SIEMPRE PRIMERO)
    Watchdog::feed();  // Reset watchdog timer
    
    // 2. POWER & SENSORS (inputs del sistema)
    PowerManager::update();   // Monitorear voltaje/corriente
    SensorManager::update();  // Leer todos los sensores
    
    // 3. SAFETY (validaciones críticas)
    SafetyManager::update();  // Validar condiciones seguras
    
    // 4. MODE MANAGEMENT (determinar comportamiento)
    VehicleMode mode = ModeManager::getCurrentMode();
    ModeManager::update();
    
    // 5. CONTROL (actuación)
    ControlManager::update(); // Aplicar comandos a motores
    
    // 6. OUTPUT (telemetría y HUD)
    TelemetryManager::update();  // Registrar datos
    HUDManager::update();        // Actualizar display
    
    // 7. DELAY (prevenir CPU hogging)
    delay(SYSTEM_TICK_MS);  // Típicamente 10-20ms
}
```

**Frecuencia del loop:** ~50-100 Hz (10-20ms por ciclo)

---

## Thread Safety

### **Sensores con acceso concurrente**

| Módulo | Mutex | Razón |
|--------|-------|-------|
| `Temperature` | ✅ `tempMutex` | Lectura desde loop + actualización desde interrupciones |
| `Current` | ❌ No | Solo acceso desde loop (single-thread) |
| `Wheels` | ❌ No | Solo acceso desde loop |

### **Reglas de thread safety**

1. **Loop principal:** Single-threaded, no requiere mutexes entre managers
2. **Interrupciones:** Si un sensor usa ISR, debe proteger variables compartidas con mutex
3. **FreeRTOS tasks:** Si se crean tasks adicionales, añadir mutexes

### **Ejemplo: Temperature con mutex**

```cpp
// Declaración
static SemaphoreHandle_t tempMutex = nullptr;

// Init
void Temperature::init() {
    tempMutex = xSemaphoreCreateMutex();
}

// Escritura (desde ISR o task)
void update() {
    if (xSemaphoreTake(tempMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        lastTemp[i] = newValue;  // Escritura protegida
        xSemaphoreGive(tempMutex);
    }
}

// Lectura (desde loop)
float getTemperature(int ch) {
    float temp;
    if (xSemaphoreTake(tempMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        temp = lastTemp[ch];  // Lectura protegida
        xSemaphoreGive(tempMutex);
    }
    return temp;
}
```

---

## Modos de Operación

El sistema implementa **degradación gradual** mediante modos:

| Modo | Sensores requeridos | Funcionalidad |
|------|---------------------|---------------|
| `FULL` | Todos OK | Funcionalidad completa |
| `MINIMAL` | Batería + 2 ruedas | Movimiento básico sin telemetría |
| `EMERGENCY` | Solo batería | Solo seguridad activa |
| `SAFE` | Cualquiera | Sistema detenido, solo monitoring |

**Transiciones automáticas:**
```
FULL → MINIMAL → EMERGENCY → SAFE
```

---

## Gestión de Errores

### **Errores recuperables**
- Sensor I2C timeout → Retry 3 veces → Modo MINIMAL
- Lectura ADC inválida → Usar último valor válido
- Touch no calibrado → Usar calibración por defecto

### **Errores críticos (v2.11.5+)**
- Power Manager init failed → Retry 3 veces → Watchdog reset
- Heap insuficiente → Abort init → Watchdog reset
- Stack overflow → Panic → Watchdog reset

### **Auto-recovery (v2.11.5+)**
```cpp
void handleCriticalError(const char* errorMsg) {
    static uint8_t retryCount = 0;
    
    if (retryCount >= MAX_RETRIES) {
        // Permitir watchdog reset después de 3 reintentos
        while (true) {
            delay(1000);
            // No alimentar watchdog - reset automático
        }
    }
    
    // Esperar 5 segundos e intentar restart
    delay(5000);
    ESP.restart();
}
```

---

## Memoria

### **Stack sizes (ESP32-S3)**
```ini
IPC Task:    4096 bytes (4KB)
Loop Task:  32768 bytes (32KB)
Main Task:  20480 bytes (20KB)
```

### **Heap usage**
- **Boot:** ~312 KB libre
- **Post-init:** ~280 KB libre
- **Runtime:** ~250-280 KB libre
- **Mínimo seguro:** 50 KB

---

## Protecciones de Seguridad (v2.11.5+)

### **I2C Timeout Protection**
```cpp
// Prevenir hang del sistema por sensores I2C no respondiendo
namespace I2CConstants {
    constexpr uint32_t READ_TIMEOUT_MS = 100;   // Timeout lectura
    constexpr uint32_t WRITE_TIMEOUT_MS = 50;   // Timeout escritura
}

// Implementación con timeout manual
while (Wire.available() < 2 && (millis() - start) < READ_TIMEOUT_MS) {
    delayMicroseconds(10);
}
```

### **PWM Channel Validation**
```cpp
// Prevenir crashes por canales PWM inválidos
static inline bool validatePWMChannel(uint8_t channel, const char* context) {
    if (!pwm_channel_valid(channel)) {
        Logger::errorf("PWM: Invalid channel %d in %s", channel, context);
        return false;
    }
    return true;
}

// Validar antes de escribir
if (validatePWMChannel(channel, "MOTOR_FL")) {
    pca9685.setPWM(channel, 0, value);
}
```

### **Watchdog Protection**
- **Timeout:** 30 segundos
- **Feed interval:** Cada ciclo del loop (~10-20ms)
- **Auto-reset:** Si el sistema se cuelga, reset automático

---

## Comunicación I2C

### **Bus I2C Principal**
- **SDA:** GPIO 21
- **SCL:** GPIO 22
- **Frecuencia:** 400 kHz (Fast Mode)

### **Dispositivos I2C**

| Dispositivo | Dirección | Descripción |
|-------------|-----------|-------------|
| TCA9548A | 0x70 | Multiplexor I2C (8 canales) |
| PCA9685 Front | 0x40 | PWM front motors |
| PCA9685 Rear | 0x41 | PWM rear motors |
| PCA9685 Steering | 0x42 | PWM steering |
| MCP23017 | 0x20 | GPIO expander |
| INA226 (x5) | 0x40-0x44 | Current sensors (via TCA9548A) |

### **Timeouts I2C (v2.11.5+)**
- **Read:** 100ms
- **Write:** 50ms
- **Recovery:** Log error y continuar

---

## Referencias

- **main.cpp:** Punto de entrada y loop principal
- **watchdog.h:** Implementación del watchdog
- **pins.h:** Definiciones de pines GPIO
- **platformio.ini:** Configuración de compilación
- **i2c.cpp:** Helpers I2C con timeout protection
- **traction.cpp:** Control de motores con validación PWM

---

**Última actualización:** 2025-12-24  
**Versión firmware:** v2.11.5+  
**Autor:** Sistema de control vehicular ESP32-S3
