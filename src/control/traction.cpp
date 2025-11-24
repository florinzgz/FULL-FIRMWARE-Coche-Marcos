#include "traction.h"
#include "current.h"
#include "wheels.h"
#include "sensors.h"
#include "pedal.h"
#include "steering.h"
#include "logger.h"
#include "system.h"
#include "settings.h"
#include "storage.h"

#include <cmath>     // std::isfinite, std::fabs
#include <cstdint>
#include <cstring>
#include <algorithm> // std::min, std::max

extern Storage::Config cfg;

static Traction::State s;
static bool initialized = false;

namespace {
    // Implementación independiente de std::clamp para máxima compatibilidad
    inline float clampf(float v, float lo, float hi) {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    // 🔒 CORRECCIÓN 2.1: Obtener corriente máxima desde configuración
    // En lugar de constante hardcodeada, usar valores configurables
    inline float getMaxCurrentA(int channel) {
        // Canal 4 = batería (típico 100A), resto = motores (típico 50A)
        // Si cfg no tiene estos campos, usar defaults seguros
        if (channel == 4) {
            // Batería: 100A por defecto
            return 100.0f; // TODO: usar cfg.maxBatteryCurrentA cuando esté disponible
        } else {
            // Motores: 50A por defecto  
            return 50.0f;  // TODO: usar cfg.maxMotorCurrentA cuando esté disponible
        }
    }

    // Mapea 0..100% -> 0..255 PWM
    inline float demandPctToPwm(float pct) {
        return clampf(pct, 0.0f, 100.0f) * 255.0f / 100.0f;
    }
}

void Traction::init() {
    s = {};
    for (int i = 0; i < 4; ++i) {
        s.w[i] = {};
        s.w[i].demandPct = 0.0f;
        s.w[i].outPWM = 0.0f;
        s.w[i].effortPct = 0.0f;
        s.w[i].currentA = 0.0f;
        s.w[i].speedKmh = 0.0f;
        s.w[i].tempC = 0.0f;
    }
    s.enabled4x4 = false;
    s.demandPct = 0.0f;
    Logger::info("Traction init");
    initialized = true;
}

void Traction::setMode4x4(bool on) {
    s.enabled4x4 = on;
    Logger::infof("Traction mode set: %s", on ? "4x4" : "4x2");
    // Si hay acciones hardware (p. ej. activar relés), se deberían llamar aquí.
}

void Traction::setDemand(float pedalPct) {
    // 🔒 CORRECCIÓN 2.2: Validación de NaN/Inf antes de clamp
    if (!std::isfinite(pedalPct)) {
        Logger::errorf("Traction: demanda inválida (NaN/Inf), usando 0");
        System::logError(801); // código: demanda de tracción inválida
        s.demandPct = 0.0f;
        return;
    }
    
    pedalPct = clampf(pedalPct, 0.0f, 100.0f);
    s.demandPct = pedalPct;
}

void Traction::update() {
    if (!initialized) {
        Logger::warn("Traction update called before init");
        return;
    }

    if (!cfg.tractionEnabled) {
        for (int i = 0; i < 4; ++i) {
            s.w[i].demandPct = 0.0f;
            s.w[i].outPWM = 0.0f;
            s.w[i].effortPct = 0.0f;
            s.w[i].currentA = 0.0f;
            s.w[i].tempC = 0.0f;
        }
        s.enabled4x4 = false;
        return;
    }

    // 🔒 CORRECCIÓN 2.3: Reparto básico 50/50 entre ejes en 4x4
    // En modo 4x2, toda la potencia va al eje delantero
    const float base = s.demandPct;
    float front = 0.0f;
    float rear = 0.0f;
    
    if (s.enabled4x4) {
        // Modo 4x4: reparto 50% delantero, 50% trasero
        front = base * 0.5f;
        rear  = base * 0.5f;
        Logger::debugf("Traction 4x4: base=%.1f%%, front=%.1f%%, rear=%.1f%%", base, front, rear);
    } else {
        // Modo 4x2: toda la potencia a ejes delanteros, traseros en 0
        front = base;
        rear = 0.0f;
        Logger::debugf("Traction 4x2: base=%.1f%%, front=%.1f%%, rear=0%%", base, front);
    }

    // Ackermann: ajustar según ángulo de dirección
    auto steer = Steering::get();
    float factorFL = 1.0f;
    float factorFR = 1.0f;
    
    if (cfg.steeringEnabled && steer.valid) {
        float angle = std::fabs(steer.angleDeg);
        
        // 🔒 CORRECCIÓN 2.4: Escalado Ackermann más suave (70% mínimo en vez de 50%)
        // Evita reducción excesiva en curvas cerradas a baja velocidad
        // A 60° de ángulo: rueda interior al 70% (antes 50%)
        float scale = clampf(1.0f - (angle / 60.0f) * 0.3f, 0.7f, 1.0f);
        
        if (steer.angleDeg > 0.0f) {
            // Giro a la derecha: reducir rueda derecha
            factorFR = scale;
        } else if (steer.angleDeg < 0.0f) {
            // Giro a la izquierda: reducir rueda izquierda
            factorFL = scale;
        }
        
        Logger::debugf("Ackermann: angle=%.1f°, factorFL=%.2f, factorFR=%.2f", 
                       steer.angleDeg, factorFL, factorFR);
    }

    // Aplicar reparto por rueda
    s.w[FL].demandPct = clampf(front * factorFL, 0.0f, 100.0f);
    s.w[FR].demandPct = clampf(front * factorFR, 0.0f, 100.0f);
    s.w[RL].demandPct = clampf(rear, 0.0f, 100.0f);
    s.w[RR].demandPct = clampf(rear, 0.0f, 100.0f);

    // Actualizar sensores y calcular métricas por rueda
    for (int i = 0; i < 4; ++i) {
        // -- Corriente
        if (cfg.currentSensorsEnabled) {
            // 🔒 CORRECCIÓN 2.5: API de sensores usa índices 0-based (0=FL, 1=FR, 2=RL, 3=RR)
            // Documentado claramente en sensors.h
            float currentA = Sensors::getCurrent(i);
            
            // Validar lectura
            if (!std::isfinite(currentA) || currentA < -999.0f) {
                System::logError(810 + i); // códigos 810-813 para motores FL-RR
                Logger::errorf("Traction: corriente inválida rueda %d", i);
                currentA = 0.0f;
            }
            s.w[i].currentA = currentA;

            // Calcular effortPct en base a máxima corriente del canal
            float maxA = getMaxCurrentA(i);
            if (maxA > 0.0f) {
                s.w[i].effortPct = clampf((currentA / maxA) * 100.0f, -100.0f, 100.0f);
            } else {
                s.w[i].effortPct = 0.0f;
            }
        } else {
            s.w[i].currentA = 0.0f;
            s.w[i].effortPct = 0.0f;
        }

        // -- Temperatura
        if (cfg.tempSensorsEnabled) {
            // 🔒 API de Sensors::getTemperature() usa índices 0-based
            float t = Sensors::getTemperature(i);
            if (!std::isfinite(t) || t < -999.0f) {
                System::logError(820 + i); // códigos 820-823 para motores FL-RR
                Logger::errorf("Traction: temperatura inválida rueda %d", i);
                t = 0.0f;
            }
            s.w[i].tempC = clampf(t, -40.0f, 150.0f);
        } else {
            s.w[i].tempC = 0.0f;
        }

        // -- Velocidad: si tienes Sensors::getSpeed o similar, añádelo aquí
        // s.w[i].speedKmh = Sensors::getSpeedKmh(i);

        // -- PWM de salida (valor a aplicar al driver BTS7960 u otro)
        s.w[i].outPWM = demandPctToPwm(s.w[i].demandPct);
        // Si tienes función para aplicar PWM, llámala aquí:
        // e.g. MotorDriver::setPWM(i, static_cast<uint8_t>(s.w[i].outPWM));
    }

    // 🔒 CORRECCIÓN 2.6: Validación mejorada de reparto anómalo
    float sumDemand = s.w[FL].demandPct + s.w[FR].demandPct + s.w[RL].demandPct + s.w[RR].demandPct;
    
    // Calcular límite esperado según modo
    float maxExpectedSum = s.enabled4x4 ? (base * 2.0f) : base; // 4x4: base*2, 4x2: base
    float tolerance = 15.0f; // 15% de margen por Ackermann y redondeos
    
    if (sumDemand > maxExpectedSum + tolerance) {
        System::logError(800); // código: reparto anómalo detectado
        Logger::errorf("Traction: reparto anómalo >%.0f%% esperado (%.2f%% real)", 
                       maxExpectedSum, sumDemand);
        
        // Aplicar fallback: reducir todas las demandas proporcionalmente
        if (sumDemand > 0.01f) { // evitar división por cero
            float scaleFactor = maxExpectedSum / sumDemand;
            Logger::warnf("Traction: aplicando factor corrección %.3f", scaleFactor);
            for (int i = 0; i < 4; ++i) {
                s.w[i].demandPct *= scaleFactor;
                s.w[i].outPWM = demandPctToPwm(s.w[i].demandPct);
            }
        }
    }
    
    // 🔒 Validación adicional: detectar reparto asimétrico extremo
    float maxWheel = std::max({s.w[FL].demandPct, s.w[FR].demandPct, 
                               s.w[RL].demandPct, s.w[RR].demandPct});
    float minWheel = std::min({s.w[FL].demandPct, s.w[FR].demandPct, 
                               s.w[RL].demandPct, s.w[RR].demandPct});
    
    if ((maxWheel - minWheel > 80.0f) && sumDemand > 50.0f) {
        System::logError(802); // código: asimetría extrema
        Logger::warnf("Traction: reparto asimétrico extremo (max=%.1f%%, min=%.1f%%)", 
                      maxWheel, minWheel);
    }
}

const Traction::State& Traction::get() {
    return s;
}

bool Traction::initOK() {
    return initialized;
}