# 🚧 Sistema Detector de Obstáculos VL53L5CX

## Descripción General

El sistema de detección de obstáculos está basado en sensores VL53L5CX de STMicroelectronics, que son sensores ToF (Time-of-Flight) con matriz de medición 8x8 zonas. Este sistema proporciona detección precisa de obstáculos en las direcciones **frontal y trasera** del vehículo para asistencia en aparcamiento, evitación de colisiones y control de crucero adaptativo.

> **Nota histórica**: Hasta la versión v2.11.0, el sistema incluía 4 sensores (FRONT, REAR, LEFT, RIGHT). A partir de **v2.11.1**, los sensores laterales fueron eliminados para liberar GPIOs necesarios para las tiras LED WS2812B.

---

## 🔧 Configuración de Hardware

### Sensores VL53L5CX

| Sensor | Posición | Pin XSHUT  | Canal I²C | Función |
|--------|----------|------------|-----------|---------|
| FRONT  | Frontal  | GPIO 46    | Canal 0   | Detección frontal, frenado automático |
| REAR   | Trasero  | GPIO 19    | Canal 1   | Asistencia de aparcamiento, marcha atrás |

**Nota:** Los sensores laterales (LEFT/RIGHT) fueron eliminados en v2.11.1 para liberar GPIOs para las tiras LED WS2812B.

### Multiplexor I²C (PCA9548A)

```
Dirección I²C: 0x71
```

El multiplexor PCA9548A permite conectar los 2 sensores VL53L5CX (ambos con dirección por defecto 0x29) en un mismo bus I²C, seleccionando cada sensor a través de canales independientes.

### Diagrama de Conexiones

```
ESP32-S3
   │
   ├─── I²C SDA (GPIO 8) ──────────────────────┐
   ├─── I²C SCL (GPIO 9) ──────────────────────┤
   │                                            │
   │    ┌──────────────────────────────────────┤
   │    │         PCA9548A (0x71)              │
   │    │      I²C Multiplexer                 │
   │    ├──────────────────────────────────────┤
   │    │ Canal 0 ── VL53L5CX FRONT (0x29)     │
   │    │ Canal 1 ── VL53L5CX REAR  (0x29)     │
   │    │ Canales 2-7: Disponibles             │
   │    └──────────────────────────────────────┘
   │
   ├─── GPIO 46 (XSHUT_FRONT) ───> VL53L5CX FRONT
   └─── GPIO 19 (XSHUT_REAR)  ───> VL53L5CX REAR
```

### Notas sobre los Pines XSHUT

Los pines XSHUT controlan el encendido/apagado de cada sensor:
- **LOW (0V)**: Sensor en modo shutdown (desactivado)
- **HIGH (3.3V)**: Sensor activo y listo para operar

> ⚠️ **IMPORTANTE v2.11.1+**: 
> - **GPIO 46 (XSHUT_FRONT)**: Es un strapping pin (Boot mode / ROM log). 
>   - **Recomendación hardware:** Añadir resistencia pull-up **10kΩ a 3.3V** para garantizar estado HIGH durante boot.
>   - El firmware lo configura como OUTPUT después del arranque.
> - **GPIO 19 (XSHUT_REAR)**: GPIO estándar, sin restricciones.
> - **GPIO 45**: Ahora LIBRE (antes usado para XSHUT_LEFT). Reservado para detección de llave (KEY_DETECT).

---

## 📊 Umbrales de Distancia

El sistema define 4 niveles de proximidad basados en la distancia al obstáculo más cercano:

| Nivel | Distancia (mm) | Distancia (cm) | Acción del Sistema |
|-------|---------------|----------------|-------------------|
| `SAFE` | > 1000 | > 100 cm | Sin obstáculos, operación normal |
| `CAUTION` | 500 - 1000 | 50-100 cm | Reducir velocidad |
| `WARNING` | 200 - 500 | 20-50 cm | Activar asistencia de freno |
| `CRITICAL` | < 200 | < 20 cm | **Parada de emergencia** |
| `INVALID` | 8191 | N/A | Sin datos válidos |

### Constantes de Configuración

```cpp
// Archivo: include/obstacle_config.h
namespace ObstacleConfig {
    constexpr uint16_t DISTANCE_CRITICAL = 200;     // 0-20cm: Parada emergencia
    constexpr uint16_t DISTANCE_WARNING = 500;      // 20-50cm: Asistencia freno
    constexpr uint16_t DISTANCE_CAUTION = 1000;     // 50-100cm: Reducir velocidad
    constexpr uint16_t DISTANCE_MAX = 4000;         // Rango máximo detección (4m)
    constexpr uint16_t DISTANCE_INVALID = 8191;     // Marcador fuera de rango
}
```

---

## ⏱️ Parámetros de Tiempo

| Parámetro | Valor | Descripción |
|-----------|-------|-------------|
| `UPDATE_INTERVAL_MS` | 66 ms | Frecuencia de actualización (15 Hz) |
| `MEASUREMENT_TIMEOUT_MS` | 500 ms | Tiempo máximo de medición |
| `INIT_DELAY_MS` | 50 ms | Retardo de estabilización tras power-up |
| `MUX_SWITCH_DELAY_US` | 100 µs | Tiempo de conmutación del multiplexor |
| `RANGING_FREQUENCY_HZ` | 15 Hz | Frecuencia de medición del sensor |
| `INTEGRATION_TIME_MS` | 20 ms | Tiempo de integración por medición |

---

## 🛡️ Sistemas de Seguridad Integrados

### 1. Evitación de Colisiones (Collision Avoidance)

```cpp
// Configuración por defecto
collisionCutoffDistanceMm = 200;  // 20cm
```

- **Activación**: Cuando la distancia frontal o trasera es < 20cm
- **Acción**: Activa freno de emergencia automático
- **Alerta**: Reproduce audio de emergencia (`AUDIO_EMERGENCIA`)
- **Comportamiento bidireccional**:
  - **Marcha adelante**: Responde al sensor FRONT cuando la distancia < 20cm
  - **Marcha atrás**: Responde al sensor REAR cuando la distancia < 20cm
  - Ambas direcciones activan el mismo `emergencyBrakeApplied = true` y alertas de audio

### 2. Asistencia de Aparcamiento (Parking Assist)

```cpp
// Configuración por defecto
parkingBrakeDistanceMm = 500;  // 50cm
```

- **Activación**: Cuando la distancia frontal/trasera es < 50cm
- **Acción**: Reduce la velocidad proporcionalmente a la distancia
- **Factor de reducción**: `speedReductionFactor = distancia / 500mm`

### 3. Advertencia de Punto Ciego (Blind Spot Warning)

```cpp
// Configuración por defecto (deshabilitado en v2.11.1+)
blindSpotDistanceMm = 1000;  // 1 metro
```

> ⚠️ **Nota v2.11.1+**: Esta función está **deshabilitada** desde la eliminación de los sensores laterales (LEFT/RIGHT).
> - **Estado**: Código mantenido para compatibilidad futura
> - **Hardware requerido**: 2 sensores VL53L5CX adicionales en posiciones laterales

### 4. Control de Crucero Adaptativo (Adaptive Cruise)

```cpp
// Configuración por defecto (desactivado)
adaptiveCruiseEnabled = false;
cruiseFollowDistanceMm = 2000;  // 2 metros
minCruiseSpeed = 10.0f;  // 10 km/h
```

> ⚠️ **Nota**: El control de crucero adaptativo está desactivado por defecto y requiere integración con sensores de velocidad adicionales.

---

## 🔄 Flujo de Operación

### Secuencia de Inicialización

```
1. Test de bus I²C
   ├── Verifica salud del bus con I2CRecovery
   └── Si falla → Intenta recuperación
   
2. Reset de sensores vía XSHUT
   ├── Todos los pines XSHUT → LOW
   └── Espera 10ms para shutdown completo

3. Verificar multiplexor PCA9548A
   ├── Intenta seleccionar canal 0
   └── Si falla → Modo simulación (placeholder)

4. Inicializar cada sensor secuencialmente
   ├── XSHUT → HIGH (power up)
   ├── Espera 50ms estabilización
   ├── Selecciona canal MUX
   ├── Lee ID del dispositivo desde registro 0x0000 (valor esperado: 0xF0)
   └── Marca sensor como healthy/unhealthy
```

### Ciclo de Actualización (loop principal)

```
1. ObstacleDetection::update() [cada 66ms / 15Hz]
   ├── Si no inicializado → return
   ├── Verifica intervalo de tiempo
   └── Actualiza timestamps de sensores activos

2. ObstacleSafety::update() [cada 50ms / 20Hz]
   ├── Obtiene estado de todos los sensores
   ├── Evalúa condiciones de seguridad:
   │   ├── Evitación de colisiones (prioridad máxima)
   │   ├── Asistencia de aparcamiento
   │   ├── Advertencia de punto ciego
   │   └── Control de crucero adaptativo
   └── Activa alertas sonoras según nivel
```

---

## 📐 Matriz de Zonas 8x8

Cada sensor VL53L5CX mide distancia en una matriz de 8×8 = **64 zonas**:

```
┌─────────────────────────────────────┐
│  Z0  │  Z1  │  Z2  │ ... │  Z7     │   ← Fila 0 (arriba)
├──────┼──────┼──────┼─────┼─────────┤
│  Z8  │  Z9  │  Z10 │ ... │  Z15    │   ← Fila 1
├──────┼──────┼──────┼─────┼─────────┤
│  ... │  ... │  ... │ ... │  ...    │
├──────┼──────┼──────┼─────┼─────────┤
│ Z56  │ Z57  │ Z58  │ ... │  Z63    │   ← Fila 7 (abajo)
└─────────────────────────────────────┘
       Campo de visión del sensor
```

### Estructura de Datos por Zona

```cpp
struct ObstacleZone {
    uint16_t distanceMm;    // Distancia en milímetros
    uint8_t confidence;     // Confianza de medición (0-100%)
    ObstacleLevel level;    // Nivel de proximidad
    bool valid;             // Flag de validez de datos
};
```

### Confianza Mínima

```cpp
constexpr uint8_t MIN_CONFIDENCE = 50;  // 50% mínimo para considerar válida
```

---

## 🧪 Modo Placeholder (Simulación)

El sistema soporta un **modo placeholder** cuando los sensores VL53L5CX no están físicamente presentes:

```cpp
bool isPlaceholderMode();   // Retorna true si en simulación
bool isHardwarePresent();   // Retorna true si hay hardware real
```

### Comportamiento en Modo Placeholder

- El sistema se inicializa correctamente
- Los sensores se marcan como `healthy = false`
- Los timestamps se actualizan normalmente
- Las distancias permanecen en `DISTANCE_INVALID`
- Útil para desarrollo y pruebas sin hardware

---

## 🔧 API Pública

### Funciones de Control

```cpp
namespace ObstacleDetection {
    void init();                                    // Inicializar sistema
    void update();                                  // Actualizar (llamar en loop)
    bool enableSensor(uint8_t idx, bool enable);    // Habilitar/deshabilitar sensor
    void setDistanceOffset(uint8_t idx, int16_t mm); // Calibrar offset de distancia
}
```

### Funciones de Lectura

```cpp
const ObstacleSensor& getSensor(uint8_t sensorIdx);  // Datos completos del sensor
uint16_t getMinDistance(uint8_t sensorIdx);          // Distancia mínima (todas las zonas)
ObstacleLevel getProximityLevel(uint8_t sensorIdx);  // Nivel de proximidad
bool isHealthy(uint8_t sensorIdx);                   // Estado de salud del sensor
void getStatus(ObstacleStatus& status);              // Estado global del sistema
```

### Funciones de Configuración

```cpp
bool loadConfig();                          // Cargar configuración de almacenamiento
bool saveConfig();                          // Guardar configuración
const ObstacleSettings& getConfig();        // Obtener configuración actual
void setConfig(const ObstacleSettings& cfg); // Aplicar nueva configuración
```

### Funciones de Diagnóstico

```cpp
void resetErrors();      // Resetear contadores de error
bool runDiagnostics();   // Ejecutar test de diagnóstico
```

---

## 📊 Estructura de Estado del Sistema

```cpp
struct ObstacleStatus {
    uint8_t sensorsHealthy;         // Número de sensores funcionando
    uint8_t sensorsEnabled;         // Número de sensores habilitados
    ObstacleLevel overallLevel;     // Nivel de proximidad más crítico
    uint16_t minDistanceFront;      // Distancia mínima frontal
    uint16_t minDistanceRear;       // Distancia mínima trasera
    uint16_t minDistanceLeft;       // Distancia mínima izquierda (sin usar en v2.11.1+)
    uint16_t minDistanceRight;      // Distancia mínima derecha (sin usar en v2.11.1+)
    bool emergencyStopActive;       // Parada de emergencia activada
    bool parkingAssistActive;       // Asistencia de aparcamiento activa
    uint32_t lastUpdateMs;          // Timestamp última actualización
};
```

> **Nota v2.11.1+**: Los campos `minDistanceLeft` y `minDistanceRight` se mantienen en la estructura por compatibilidad binaria, pero siempre contienen `DISTANCE_INVALID` (8191).

---

## ⚡ Fiabilidad del Sistema

### Recuperación I²C

El sistema utiliza el módulo `I2CRecovery` para manejar errores de comunicación:

```cpp
// Selección segura de canal del multiplexor
bool tcaSelectSafe(uint8_t channel, uint8_t muxAddress);

// Lectura con reintentos automáticos
bool readBytesWithRetry(uint8_t addr, uint16_t reg, uint8_t* buf, uint8_t len, uint8_t deviceId);
```

### Manejo de Errores

```cpp
struct ObstacleSensor {
    uint8_t errorCount;  // Contador de errores consecutivos
    // Se incrementa en caso de fallo de comunicación
    // Se resetea cuando hay lectura exitosa
};
```

### Alertas con Throttling

Para evitar alertas repetitivas molestas:

```cpp
static constexpr uint32_t ALERT_INTERVAL_MS = 1000;  // Mínimo 1s entre alertas iguales
```

---

## 🔊 Alertas de Audio

| Evento | Audio | Descripción |
|--------|-------|-------------|
| Colisión inminente | `AUDIO_EMERGENCIA` | Alarma de emergencia |
| Obstáculo cercano | `AUDIO_ERROR_GENERAL` | Beep de proximidad |
| Punto ciego | `AUDIO_ERROR_GENERAL` | Beep de advertencia |

---

## 📝 Ejemplo de Uso

```cpp
#include "obstacle_detection.h"
#include "obstacle_safety.h"

void setup() {
    ObstacleDetection::init();
    ObstacleSafety::init();
}

void loop() {
    ObstacleDetection::update();
    ObstacleSafety::update();
    
    // Obtener estado del sistema
    ObstacleDetection::ObstacleStatus status;
    ObstacleDetection::getStatus(status);
    
    // Verificar emergencia
    if (status.emergencyStopActive) {
        // ¡Activar freno de emergencia!
        emergencyBrake();
    }
    
    // Obtener distancia frontal
    uint16_t frontDist = ObstacleDetection::getMinDistance(ObstacleDetection::SENSOR_FRONT);
    if (frontDist < 500) {
        Serial.printf("¡Obstáculo frontal a %d mm!\n", frontDist);
    }
    
    // Verificar nivel de proximidad
    auto level = ObstacleDetection::getProximityLevel(ObstacleDetection::SENSOR_FRONT);
    switch (level) {
        case ObstacleDetection::LEVEL_CRITICAL:
            Serial.println("¡CRÍTICO! Detener vehículo");
            break;
        case ObstacleDetection::LEVEL_WARNING:
            Serial.println("Advertencia: reducir velocidad");
            break;
        case ObstacleDetection::LEVEL_CAUTION:
            Serial.println("Precaución: obstáculo detectado");
            break;
        case ObstacleDetection::LEVEL_SAFE:
            Serial.println("Vía libre");
            break;
    }
}
```

---

## 🐛 Resolución de Problemas

### Sensor no detectado

1. **Verificar conexiones físicas**:
   - VCC del sensor a 3.3V
   - GND compartido
   - SDA/SCL a través del multiplexor

2. **Verificar pin XSHUT**:
   - Debe estar conectado y en HIGH durante operación
   - Probar continuidad del cable

3. **Verificar multiplexor**:
   ```cpp
   // En Serial Monitor, buscar:
   "Obstacle: PCA9548A (0x71) not found"
   ```

4. **Escanear dirección alternativa del multiplexor**:
   Si el PCA9548A no responde en 0x71, puede tener otra dirección.
   Ejecutar un scanner I²C para detectar dispositivos:
   
   ```cpp
   #include <Wire.h>
   
   void scanI2C() {
       Serial.println("Escaneando bus I2C...");
       for (uint8_t addr = 0x08; addr < 0x78; addr++) {
           Wire.beginTransmission(addr);
           if (Wire.endTransmission() == 0) {
               Serial.printf("Dispositivo encontrado en 0x%02X\n", addr);
           }
       }
   }
   ```
   
   **Direcciones comunes del PCA9548A**:
   - 0x70 (A0=A1=A2=0)
   - 0x71 (A0=1, A1=A2=0) ← Configuración por defecto
   - 0x72-0x77 (otras combinaciones de A0-A2)
   
   Para cambiar la dirección en el firmware:
   ```cpp
   // En include/obstacle_config.h
   constexpr uint8_t PCA9548A_ADDR = 0x71;  // Cambiar según hardware
   ```

### Lecturas inválidas constantes

1. **Verificar canal del multiplexor**:
   ```cpp
   // Debe mostrar selección exitosa:
   "Obstacle: Initializing sensor FRONT (GPIO 18, MUX ch 0)..."
   ```

2. **Verificar ID del sensor**:
   ```cpp
   // Esperado: 0xF0
   "Obstacle: Sensor FRONT detected at 0x29 (ID: 0xF0)"
   ```

### Sistema en modo placeholder

Si ve en logs:
```
"Obstacle detection system ready (placeholder/simulation mode)"
```

Significa que ningún sensor fue detectado. Verificar:
- Multiplexor PCA9548A conectado
- Sensores VL53L5CX alimentados
- Pines XSHUT correctamente conectados

---

## 📚 Referencias

- **VL53L5CX Datasheet**: STMicroelectronics
- **PCA9548A Datasheet**: NXP Semiconductors
- **ESP32-S3 Technical Reference**: Espressif Systems

---

## 📋 Historial de Versiones

| Versión | Fecha | Cambios |
|---------|-------|---------|
| v2.4.1 | 2025-12-01 | Corrección de pines XSHUT (GPIO 18,19,45,46) |
| v2.4.0 | 2025-11-25 | Implementación inicial del sistema |

---

**¿Preguntas o problemas?**
Abre un issue en GitHub: https://github.com/florinzgz/FULL-FIRMWARE-Coche-Marcos/issues
