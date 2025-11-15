# Sistemas Avanzados de Seguridad y Eficiencia

Este firmware incluye tres sistemas avanzados para mejorar la seguridad, tracción y eficiencia energética del vehículo eléctrico inteligente.

## 🛡️ Sistema ABS (Anti-lock Braking System)

### Descripción
El sistema ABS evita el bloqueo de las ruedas durante el frenado, manteniendo la capacidad de dirección y acortando las distancias de frenado.

### Características
- **Detección de deslizamiento**: Monitorea cada rueda individualmente
- **Umbral configurable**: 20% de deslizamiento por defecto
- **Ciclos de modulación**: 100ms de frecuencia (10 Hz)
- **Reducción de presión**: 30% por pulso
- **Velocidad mínima**: Activo por encima de 5 km/h

### Funcionamiento
```
Si deslizamiento de rueda > 20%:
   → Reduce presión de freno 30%
   → Espera 100ms
   → Reevalúa condiciones
   → Recupera presión gradualmente
```

### Configuración
Edita `src/control/abs_system.cpp`:
```cpp
static Config config = {
    .enabled = true,              // Activar/desactivar
    .slipThreshold = 20.0f,       // Umbral de deslizamiento (%)
    .minSpeedKmh = 5.0f,          // Velocidad mínima de activación
    .cycleMs = 100,               // Frecuencia de ciclo
    .pressureReduction = 0.3f     // Factor de reducción de presión
};
```

### API de Código
```cpp
#include "abs_system.h"

// Verificar si ABS está activo
if (ABSSystem::isActive()) {
    // ABS está modulando los frenos
}

// Obtener estado completo
auto state = ABSSystem::getState();
Serial.printf("ABS: %s, Rueda 0 deslizando: %.1f%%\n", 
    state.active ? "ACTIVO" : "INACTIVO",
    state.wheelSlip[0]
);

// Activar/desactivar
ABSSystem::setEnabled(true);

// Obtener factor de reducción de freno para una rueda
float factor = ABSSystem::getBrakeReduction(0); // 0 = sin reducción, 1 = reducción máxima
```

### Monitoreo
El sistema registra:
- Estado de activación por rueda
- Porcentaje de deslizamiento de cada rueda
- Número de intervenciones
- Velocidad de referencia del vehículo

---

## 🏎️ Sistema TCS (Traction Control System)

### Descripción
El sistema TCS evita que las ruedas patinen durante la aceleración, optimizando la tracción y estabilidad.

### Características
- **Control diferencial 4x4**: Gestiona independientemente cada motor
- **Umbral de deslizamiento**: 15% por defecto
- **Reducción agresiva**: 40% de potencia inicial
- **Recuperación gradual**: 25%/s de retorno de potencia
- **Distribución inteligente**: Transfiere potencia a ruedas con agarre

### Funcionamiento
```
Si deslizamiento de rueda > 15%:
   → Reducción inmediata de 40% de potencia
   → Transferencia a rueda opuesta (si 4WD)
   → Reducción gradual adicional de 5%/ciclo
   → Recuperación suave al normalizar
```

### Configuración
Edita `src/control/tcs_system.cpp`:
```cpp
static Config config = {
    .enabled = true,
    .slipThreshold = 15.0f,        // Umbral de patinaje (%)
    .minSpeedKmh = 3.0f,            // Velocidad mínima
    .aggressiveReduction = 40.0f,   // Reducción inicial (%)
    .smoothReduction = 5.0f,        // Reducción gradual (%)
    .recoveryRatePerSec = 25.0f,    // Velocidad de recuperación (%/s)
    .enabled4WD = true              // Control diferencial 4WD
};
```

### API de Código
```cpp
#include "tcs_system.h"

// Verificar si TCS está activo
if (TCSSystem::isActive()) {
    // TCS está limitando potencia
}

// Obtener factores de reducción de potencia
float reductions[4];
TCSSystem::getPowerReductions(reductions);

// Aplicar al control de tracción
for (int i = 0; i < 4; i++) {
    float finalPower = requestedPower * (1.0f - reductions[i]);
    Motor::setPower(i, finalPower);
}

// Obtener estado
auto state = TCSSystem::getState();
Serial.printf("TCS: %s, Intervenciones: %d\n",
    state.active ? "ACTIVO" : "INACTIVO",
    state.interventionCount
);
```

### Integración con Traction.cpp
El TCS debe integrarse en el sistema de tracción existente:
```cpp
// En Traction::update()
float powerReductions[4];
TCSSystem::getPowerReductions(powerReductions);

for (int i = 0; i < 4; i++) {
    float basePower = calculateMotorPower(i);
    float finalPower = basePower * (1.0f - powerReductions[i]);
    Motor::set(i, finalPower);
}
```

---

## 🔋 Sistema de Frenado Regenerativo con IA

### Descripción
Sistema inteligente que optimiza la recuperación de energía durante el frenado mediante IA basada en tablas de decisión.

### Características
- **IA basada en lookup tables**: Optimizada para velocidad y desaceleración
- **Adaptación dinámica**: Ajusta según SOC, temperatura y condiciones
- **Máximo 80% de potencia regenerativa**: Configurable
- **Eficiencia del 40-82%**: Según condiciones de operación
- **Estadísticas de energía**: Tracking de Wh recuperados

### Funcionamiento
```
Inputs del sistema:
   → Velocidad actual (km/h)
   → Aceleración/desaceleración (m/s²)
   → SOC de batería (%)
   → Temperatura de batería (°C)
   → Corriente de batería (A)

IA evalúa condiciones:
   → Lookup table según velocidad/desaceleración
   → Ajuste por SOC (reduce si >80%)
   → Ajuste por temperatura (reduce si >35°C)
   → Aplica factor de agresividad (70% default)

Output:
   → Potencia regenerativa óptima (0-80%)
   → Eficiencia esperada (40-82%)
   → Nivel de confianza (0-1)
```

### Configuración
Edita `src/safety/regen_ai.cpp`:
```cpp
static Config config = {
    .enabled = true,
    .maxRegenPower = 80.0f,       // Potencia regen máxima (%)
    .batterySOCMin = 20.0f,       // SOC mínimo para regen
    .batterySOCMax = 95.0f,       // SOC máximo para regen
    .tempMax = 45.0f,             // Temperatura máxima batería (°C)
    .aggressiveness = 0.7f        // Agresividad (0-1)
};
```

### Tabla de Decisión IA
| Velocidad (km/h) | Decel 0-1 m/s² | Decel 1-2 m/s² | Decel 2-3 m/s² | Decel >3 m/s² |
|-----------------|----------------|----------------|----------------|---------------|
| 0-10            | 10%            | 20%            | 30%            | 40%           |
| 10-30           | 20%            | 35%            | 50%            | 65%           |
| 30-50           | 30%            | 50%            | 70%            | 85%           |
| 50-70           | 40%            | 60%            | 80%            | 95%           |
| 70+             | 45%            | 65%            | 85%            | 100%          |

### API de Código
```cpp
#include "regen_ai.h"

// Verificar si regen está activo
if (RegenAI::isActive()) {
    float regenPower = RegenAI::getOptimalRegenPower();
    // Aplicar potencia regenerativa
}

// Evaluar condiciones actuales
RegenAI::evaluateRegen();

// Obtener estado completo
auto state = RegenAI::getState();
Serial.printf("Regen: %.1f%%, Eficiencia: %.1f%%, Energía: %.2f Wh\n",
    state.actualRegenPower,
    state.prediction.efficiency,
    state.energyRecovered
);

// Configuración runtime
auto& cfg = RegenAI::getConfig();
cfg.aggressiveness = 0.9f;  // Más agresivo
RegenAI::setEnabled(true);
```

### Integración con Traction.cpp
```cpp
// En Traction::update() cuando está frenando
if (pedal < 0 || shifter == REVERSE) {  // Frenando
    RegenAI::evaluateRegen();
    
    if (RegenAI::isActive()) {
        float regenPower = RegenAI::getOptimalRegenPower();
        
        // Aplicar frenado regenerativo a todos los motores
        for (int i = 0; i < 4; i++) {
            float regenTorque = calculateRegenTorque(regenPower);
            Motor::setRegen(i, regenTorque);
        }
    }
}
```

---

## 🔗 Integración Completa

### Ejemplo de Loop Principal Integrado
```cpp
void loop() {
    // 1. Leer entradas
    Pedal::update();
    Steering::update();
    Buttons::update();
    Shifter::update();
    
    // 2. Actualizar sensores
    Sensors::updateWheels();
    Sensors::updateCurrent();
    Sensors::updateTemperature();
    
    // 3. Control de tracción base
    float basePower = Pedal::get().percent;
    Traction::setDemand(basePower);
    
    // 4. Sistemas de seguridad avanzados
    ABSSystem::update();         // Modula frenos si detecta bloqueo
    TCSSystem::update();         // Reduce potencia si detecta patinaje
    RegenAI::update();          // Evalúa regen óptimo
    
    // 5. Aplicar correcciones
    float powerReductions[4];
    TCSSystem::getPowerReductions(powerReductions);
    
    for (int i = 0; i < 4; i++) {
        float power = basePower;
        
        // Aplicar TCS
        power *= (1.0f - powerReductions[i]);
        
        // Aplicar ABS si está frenando
        if (isBraking) {
            float brakeReduction = ABSSystem::getBrakeReduction(i);
            power *= (1.0f - brakeReduction);
        }
        
        Motor::set(i, power);
    }
    
    // Aplicar regen si está disponible
    if (RegenAI::isActive()) {
        float regenPower = RegenAI::getOptimalRegenPower();
        applyRegenerativeBraking(regenPower);
    }
    
    // 6. HUD y audio
    HUD::update();
    Audio::AudioQueue::update();
    
    // 7. WiFi y sistema
    WiFiManager::update();
    System::update();
    
    delay(20);  // 50 Hz
}
```

---

## 📊 Monitoreo y Telemetría

### Datos Disponibles para WiFi Telemetry
```cpp
// Preparar paquete de telemetría
struct TelemetryPacket {
    // ABS
    bool absActive;
    float wheelSlip[4];
    uint32_t absInterventions;
    
    // TCS
    bool tcsActive;
    float powerReductions[4];
    uint32_t tcsInterventions;
    
    // Regen AI
    bool regenActive;
    float regenPower;
    float regenEfficiency;
    float energyRecovered;
    
    // Sensores
    float speeds[4];
    float batterySOC;
    float batteryTemp;
    float batteryCurrent;
};

// Enviar vía WiFi/MQTT
void sendTelemetry() {
    TelemetryPacket packet;
    
    packet.absActive = ABSSystem::isActive();
    auto absState = ABSSystem::getState();
    memcpy(packet.wheelSlip, absState.wheelSlip, sizeof(packet.wheelSlip));
    packet.absInterventions = absState.interventionCount;
    
    packet.tcsActive = TCSSystem::isActive();
    TCSSystem::getPowerReductions(packet.powerReductions);
    packet.tcsInterventions = TCSSystem::getState().interventionCount;
    
    packet.regenActive = RegenAI::isActive();
    auto regenState = RegenAI::getState();
    packet.regenPower = regenState.actualRegenPower;
    packet.regenEfficiency = regenState.prediction.efficiency;
    packet.energyRecovered = regenState.energyRecovered;
    
    // Enviar packet vía MQTT, WebSocket, etc.
    mqtt.publish("car/telemetry", &packet, sizeof(packet));
}
```

---

## ⚙️ Configuración Recomendada

### Para Uso Normal (Balance seguridad/eficiencia)
```cpp
// ABS: Moderado
ABSSystem::getConfig().slipThreshold = 20.0f;

// TCS: Moderado
TCSSystem::getConfig().slipThreshold = 15.0f;
TCSSystem::getConfig().aggressiveReduction = 40.0f;

// Regen: Moderado
RegenAI::getConfig().aggressiveness = 0.7f;
RegenAI::getConfig().maxRegenPower = 80.0f;
```

### Para Máxima Seguridad (Pista mojada/nieve)
```cpp
// ABS: Agresivo
ABSSystem::getConfig().slipThreshold = 15.0f;
ABSSystem::getConfig().pressureReduction = 0.4f;

// TCS: Muy agresivo
TCSSystem::getConfig().slipThreshold = 10.0f;
TCSSystem::getConfig().aggressiveReduction = 50.0f;

// Regen: Suave
RegenAI::getConfig().aggressiveness = 0.5f;
RegenAI::getConfig().maxRegenPower = 60.0f;
```

### Para Máxima Eficiencia (Condiciones ideales)
```cpp
// ABS: Permisivo
ABSSystem::getConfig().slipThreshold = 25.0f;

// TCS: Permisivo
TCSSystem::getConfig().slipThreshold = 20.0f;
TCSSystem::getConfig().aggressiveReduction = 30.0f;

// Regen: Agresivo
RegenAI::getConfig().aggressiveness = 0.9f;
RegenAI::getConfig().maxRegenPower = 90.0f;
```

---

## 🐛 Resolución de Problemas

### ABS no se activa
- Verificar sensores de velocidad: `Sensors::isWheelSensorOk(i)`
- Verificar velocidad mínima: Debe ser >5 km/h
- Comprobar que `ABSSystem::isEnabled()` es `true`
- Revisar logs de deslizamiento de ruedas

### TCS interviene demasiado
- Aumentar `slipThreshold` (ej: 20.0f)
- Reducir `aggressiveReduction` (ej: 30.0f)
- Verificar calibración de sensores de velocidad
- Comprobar superficies de agarre

### Regen AI no recupera energía
- Verificar SOC: Debe estar entre 20-95%
- Verificar temperatura: Debe ser <45°C
- Comprobar que hay desaceleración: accel < -0.1 m/s²
- Verificar velocidad: Debe ser >5 km/h
- Revisar `RegenAI::isActive()` y logs

### Conflictos entre sistemas
Los sistemas están diseñados para cooperar:
1. **ABS tiene prioridad** sobre TCS durante frenado
2. **TCS desactiva regen** si detecta patinaje excesivo
3. **Regen se desactiva** automáticamente si ABS interviene

---

## 📈 Próximas Mejoras

### V2.0 - Machine Learning Real
- Modelo TensorFlow Lite Micro entrenado
- Aprendizaje online con datos de conducción
- Predicción de agarre basada en condiciones

### V2.0 - Sensores Adicionales
- IMU para detección de pendiente real
- Sensor de presión de freno físico
- Sensor de temperatura de frenos
- Sensor de humedad/lluvia

### V2.0 - ESP (Electronic Stability Program)
- Combinación ABS + TCS + Control de dirección
- Detección de sobreviraje/subviraje
- Corrección automática con frenado selectivo

---

## 📚 Referencias

- **ABS Theory**: ISO 21994 Road Vehicles - ABS Performance Requirements
- **TCS Design**: SAE J2564 - Traction Control System Performance Requirements
- **Regen Braking**: IEEE Paper "Optimal Regenerative Braking for Electric Vehicles"

---

## 📝 Notas de Desarrollo

**Versión**: 1.0.0  
**Fecha**: 2025-01-03  
**Compatible con**: ESP32-S3, PlatformIO  
**Licencia**: MIT  

**Autores**:
- Florinzgz (Hardware & Integration)
- GitHub Copilot (Advanced Systems Implementation)

---

**¿Preguntas o problemas?**
Abre un issue en GitHub: https://github.com/florinzgz/coche-2-inteligente/issues
