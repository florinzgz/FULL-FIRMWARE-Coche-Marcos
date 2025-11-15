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

    // Valor por defecto de corriente máxima para cálculo de % de esfuerzo.
    // Si en cfg tienes un campo real (ej. cfg.maxMotorCurrentA), reemplázalo por ese.
    constexpr float DEFAULT_MAX_CURRENT_A = 100.0f;

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

    // Reparto básico: 50/50 entre ejes (puedes ajustar para 4x2 por s.enabled4x4)
    const float base = s.demandPct;
    float front = base * 0.5f;
    float rear  = base * 0.5f;

    // Si el modo es 4x2 (por ejemplo solo ejes delanteros), reasignar:
    if (!s.enabled4x4) {
        // Mantenemos demanda global en ejes delanteros y 0 en traseros
        rear = 0.0f;
        // front = base; // <-- opción si prefieres todo en delantero
    }

    // Ackermann: ajustar según ángulo de dirección
    auto steer = Steering::get();
    float factorFL = 1.0f;
    float factorFR = 1.0f;
    if (cfg.steeringEnabled) {
        float angle = std::fabs(steer.angleDeg);
        float scale = clampf(1.0f - angle / 60.0f, 0.5f, 1.0f);
        if (steer.angleDeg > 0.0f) {
            factorFR = scale;
        } else if (steer.angleDeg < 0.0f) {
            factorFL = scale;
        }
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
            // IMPORTANTE: aquí uso índice 0-based. Si tu API de Sensors usa 1-based,
            // cambia a Sensors::getCurrent(i+1).
            float currentA = Sensors::getCurrent(i);
            if (!std::isfinite(currentA)) {
                System::logError(810 + i);
                Logger::errorf("Traction: corriente inválida rueda %d", i);
                currentA = 0.0f;
            }
            s.w[i].currentA = currentA;

            // Calcular effortPct en base a máxima corriente de referencia.
            float maxA = DEFAULT_MAX_CURRENT_A;
            // Si tienes cfg.maxMotorCurrentA (o similar), reemplaza la línea anterior
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
            // IMPORTANTE: aquí uso índice 0-based. Si tu API usa 1-based, usa (i+1).
            float t = Sensors::getTemperature(i);
            if (!std::isfinite(t)) {
                System::logError(820 + i);
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

    // Validación global
    float sumDemand = s.w[FL].demandPct + s.w[FR].demandPct + s.w[RL].demandPct + s.w[RR].demandPct;
    if (sumDemand > 400.0f + 1e-6f) {
        System::logError(800);
        Logger::errorf("Traction: reparto anómalo >400%% (%.2f%%)", sumDemand);
    }
}

const Traction::State& Traction::get() {
    return s;
}

bool Traction::initOK() {
    return initialized;
}