#include "traction.h"
#include "logger.h"
#include "pedal.h"
#include "shifter.h"  // For reverse gear detection
#include "steering.h"
#include "config.h"
#include "system.h"
#include <algorithm>
#include <cmath>

// Utilidad de clamp
static inline float clampf(float v, float lo, float hi) {
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

namespace Traction {

// Estado interno
static State s = {};
static bool initialized = false;

// Constantes de seguridad para PWM
constexpr float PWM_MAX_SAFE = 255.0f;
constexpr float PWM_MIN = 0.0f;

// Mapea 0..100% -> 0..255 PWM con validación de límites
static inline float demandPctToPwm(float pct) {
    float pwm = clampf(pct, 0.0f, 100.0f) * 255.0f / 100.0f;
    return clampf(pwm, PWM_MIN, PWM_MAX_SAFE);
}

void init() {
    Logger::info("Traction: Initializing");
    
    // Inicializar estado
    s = State{};
    s.enabled4x4 = false;  // Por defecto 4x2
    s.demandPct = 0.0f;
    s.axisRotation = false;
    
    for (int i = 0; i < 4; ++i) {
        s.w[i] = WheelState{};
        s.w[i].reverse = false;
        s.w[i].demandPct = 0.0f;
        s.w[i].outPWM = 0.0f;
    }
    
    initialized = true;
    Logger::info("Traction: Init OK");
}

void setMode4x4(bool on) {
    s.enabled4x4 = on;
    Logger::infof("Traction: Mode switched to %s", on ? "4x4" : "4x2");
}

void setDemand(float pedalPct) {
    if (!std::isfinite(pedalPct)) {
        Logger::errorf("Traction: demanda inválida (NaN/Inf), usando 0");
        System::logError(801);
        s.demandPct = 0.0f;
        return;
    }

    pedalPct = clampf(pedalPct, 0.0f, 100.0f);
    
    // Smooth power ramping to prevent abrupt changes
    static float rampedDemand = 0.0f;
    static uint32_t lastRampTime = 0;
    
    uint32_t now = millis();
    float deltaTime = (now - lastRampTime) / 1000.0f;  // seconds
    lastRampTime = now;
    
    const float RAMP_RATE = 100.0f;  // 100% per second (fast but smooth)
    float maxDelta = RAMP_RATE * deltaTime;
    
    if (pedalPct > rampedDemand) {
        rampedDemand = std::min(rampedDemand + maxDelta, pedalPct);
    } else {
        rampedDemand = std::max(rampedDemand - maxDelta, pedalPct);
    }
    
    s.demandPct = rampedDemand;
    
    if (std::abs(pedalPct - rampedDemand) > 1.0f) {
        Logger::debugf("Traction: Ramping demand %.1f%% -> %.1f%%", pedalPct, rampedDemand);
    }
}

void setAxisRotation(bool enabled, float speedPct) {
    s.axisRotation = enabled;
    if (enabled) {
        Logger::infof("Traction: Axis rotation ON at %.1f%%", speedPct);
        // En modo axis rotation, todas las ruedas del lado izquierdo van en reversa
        // y todas las del lado derecho van hacia adelante (o viceversa)
        for (int i = 0; i < 4; ++i) {
            bool isLeftSide = (i == FL || i == RL);
            s.w[i].reverse = isLeftSide;
            s.w[i].demandPct = speedPct;
            s.w[i].outPWM = demandPctToPwm(speedPct);
        }
    } else {
        Logger::info("Traction: Axis rotation OFF");
    }
}

void update() {
    if (!initialized) {
        Logger::warn("Traction: update() called before init()");
        return;
    }
    
    // Si está en modo axis rotation, no hacemos nada más
    // (el modo axis rotation se configura explícitamente via setAxisRotation)
    if (s.axisRotation) {
        return;
    }
    
    // Obtener entrada del pedal
    float pedalPct = Pedal::get().percent;
    setDemand(pedalPct);
    
    const float base = s.demandPct;
    
    // Detect reverse gear from shifter (except in axis rotation mode)
    bool reverseGear = (Shifter::get().gear == Shifter::R);
    for (int i = 0; i < 4; ++i) {
        s.w[i].reverse = reverseGear;
    }
    if (reverseGear) {
        Logger::debug("Traction: Reverse gear active");
    }
    
    // Apply gear-based power limiting
    float gearMultiplier = 1.0f;
    auto currentGear = Shifter::get().gear;
    
    if (currentGear == Shifter::D1) {
        gearMultiplier = 0.70f;  // 70% in D1 for difficult terrain
        Logger::debugf("Traction: D1 mode - limiting to 70%% power");
    } else if (currentGear == Shifter::D2) {
        gearMultiplier = 1.0f;   // 100% in D2 for speed
    } else if (currentGear == Shifter::R) {
        gearMultiplier = 0.80f;  // 80% in reverse for safety
        Logger::debugf("Traction: Reverse mode - limiting to 80%% power");
    }
    
    const float adjustedBase = base * gearMultiplier;
    
    // Distribución de potencia entre ejes
    float front = 0.0f;
    float rear = 0.0f;
    
    if (s.enabled4x4) {
        front = adjustedBase * 0.5f;
        rear = adjustedBase * 0.5f;
        Logger::debugf("Traction 4x4: base=%.1f%%, gear=%.0f%%, front=%.1f%%, rear=%.1f%%", 
                       base, gearMultiplier*100, front, rear);
    } else {
        front = adjustedBase;
        rear = 0.0f;
        Logger::debugf("Traction 4x2: base=%.1f%%, gear=%.0f%%, front=%.1f%%, rear=0%%", 
                       base, gearMultiplier*100, front);
    }
    
    // Obtener ángulo de dirección para compensación Ackermann
    float steeringAngle = Steering::get().angleDeg;
    
    // Aplicar compensación de dirección (Ackermann simplificado)
    // Las ruedas interiores reciben menos potencia, las exteriores más
    float steerFactor = steeringAngle / 45.0f; // Normalizar a +/- 1.0 at 45 grados
    steerFactor = clampf(steerFactor, -1.0f, 1.0f);
    
    if (std::abs(steerFactor) < 0.1f) {
        // Sin giro o giro mínimo - potencia uniforme
        s.w[FL].demandPct = front;
        s.w[FR].demandPct = front;
        s.w[RL].demandPct = rear;
        s.w[RR].demandPct = rear;
    } else if (steerFactor > 0) {
        // Giro a la derecha - ruedas izquierdas exteriores
        s.w[FL].demandPct = front * (1.0f + steerFactor * 0.3f);  // exterior
        s.w[FR].demandPct = front * (1.0f - steerFactor * 0.5f);  // interior
        s.w[RL].demandPct = rear * (1.0f + steerFactor * 0.3f);   // exterior
        s.w[RR].demandPct = rear * (1.0f - steerFactor * 0.5f);   // interior
    } else {
        // Giro a la izquierda - ruedas derechas exteriores
        float absSteer = -steerFactor;
        s.w[FL].demandPct = front * (1.0f - absSteer * 0.5f);     // interior
        s.w[FR].demandPct = front * (1.0f + absSteer * 0.3f);     // exterior
        s.w[RL].demandPct = rear * (1.0f - absSteer * 0.5f);      // interior
        s.w[RR].demandPct = rear * (1.0f + absSteer * 0.3f);      // exterior
    }
    
    // Convertir demandas a PWM
    for (int i = 0; i < 4; ++i) {
        s.w[i].outPWM = demandPctToPwm(s.w[i].demandPct);
    }
    
    // TODO: Aplicar PWM a los BTS7960 vía PCA9685
    // TODO: Leer sensores de corriente (INA226)
    // TODO: Leer sensores de temperatura (DS18B20)
    // TODO: Calcular velocidad de ruedas desde encoders
}

const State& get() {
    return s;
}

bool initOK() {
    return initialized;
}

} // namespace Traction
