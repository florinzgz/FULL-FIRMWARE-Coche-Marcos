#include "steering.h"
#include "pins.h"
#include "logger.h"
#include "steering_model.h"
#include "storage.h"
#include "settings.h"
#include "system.h"   // para logError()

extern Storage::Config cfg;

static volatile long ticks = 0;
static long zeroOffset = 0;
static long ticksPerTurn = 1024;  // ajustar a tu encoder
static bool zSeen = false;

// Flag de inicialización
static bool initialized = false;

static Steering::State s;

void IRAM_ATTR isrEncA() {
    int a = digitalRead(PIN_ENCODER_A);
    int b = digitalRead(PIN_ENCODER_B);
    if(a == HIGH) {
        ticks += (b == HIGH) ? +1 : -1;
    } else {
        ticks += (b == HIGH) ? -1 : +1;
    }
}

void IRAM_ATTR isrEncZ() {
    zSeen = true;
}

static float ticksToDegrees(long t) {
    return (float)(t - zeroOffset) * 360.0f / (float)ticksPerTurn;
}

void Steering::init() {
    pinMode(PIN_ENCODER_A, INPUT);
    pinMode(PIN_ENCODER_B, INPUT);
    pinMode(PIN_ENCODER_Z, INPUT);

    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), isrEncA, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_Z), isrEncZ, RISING);

    s = {0, 0.0f, 0.0f, 0.0f, false, false};

    SteeringModel::setGeometry(0.95f, 0.70f, 54.0f);

    Logger::info("Steering init");
    initialized = true;
}

void Steering::update() {
    if(!cfg.steeringEnabled) {
        // Guard: si está desactivado → estado neutro
        s.ticks = 0;
        s.angleDeg = 0.0f;
        s.angleFL = 0.0f;
        s.angleFR = 0.0f;
        s.centered = false;
        s.valid = false;
        return;
    }

    long t = ticks;
    s.ticks = t;
    s.angleDeg = ticksToDegrees(t);

    // Clamp de ángulo global
    if(s.angleDeg > 54.0f) s.angleDeg = 54.0f;
    if(s.angleDeg < -54.0f) s.angleDeg = -54.0f;

    // Validación básica
    if(!s.centered && !zSeen) {
        // Encoder activo pero nunca centrado → warning persistente
        Logger::warn("Steering not centered yet");
        System::logError(210); // código reservado: steering sin centrado
        s.valid = false;
    } else {
        s.valid = true;
    }

    if(zSeen && !s.centered) {
        zeroOffset = t;
        s.centered = true;
        zSeen = false;
        Logger::info("Steering centered at Z");
    }

    auto ack = SteeringModel::compute(s.angleDeg);

    if(s.angleDeg >= 0) {
        s.angleFR = constrain(ack.innerDeg, -60.0f, 60.0f);
        s.angleFL = constrain(ack.outerDeg, -60.0f, 60.0f);
    } else {
        s.angleFL = constrain(ack.innerDeg, -60.0f, 60.0f);
        s.angleFR = constrain(ack.outerDeg, -60.0f, 60.0f);
    }
}

void Steering::center() {
    if(s.centered) return;
    if(zSeen) {
        zeroOffset = ticks;
        s.centered = true;
        zSeen = false;
        Logger::info("Steering center via Z");
    } else {
        Logger::warn("Steering center fallback, Z not detected");
        System::logError(211); // código reservado: fallo de centrado
    }
}

void Steering::setTicksPerTurn(long tpt) { 
    ticksPerTurn = (tpt > 0) ? tpt : 1024; // guard
    Logger::infof("Steering ticksPerTurn set: %ld", ticksPerTurn);
}

long Steering::getTicksPerTurn() { return ticksPerTurn; }

void Steering::setZeroOffset(long offset) { 
    zeroOffset = offset; 
    s.centered = true; 
    Logger::infof("Steering zeroOffset set: %ld", zeroOffset);
}

long Steering::getZeroOffset() { return zeroOffset; }

const Steering::State& Steering::get() { return s; }

bool Steering::initOK() {
    return initialized;
}