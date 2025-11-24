#include "pedal.h"
#include "pins.h"
#include "settings.h"
#include "logger.h"
#include "storage.h"
#include "system.h"   // para logError()

extern Storage::Config cfg;

static Pedal::State s;
static int adcMin = 200;
static int adcMax = 3800;
static uint8_t curveMode = 0;    // 0 lineal, 1 suave, 2 agresiva
static float deadbandPct = 3.0f; // % muerto inicial
static float lastPercent = 0.0f;
// 🔒 CORRECCIÓN: Filtro EMA para suavizar lecturas ADC
static constexpr float EMA_ALPHA = 0.15f;  // Factor de suavizado
static float rawFiltered = 0.0f;

// Flag de inicialización
static bool initialized = false;

static float applyCurve(float x) {
    // x en [0,1]
    switch(curveMode) {
        case 1: // suave (ease-in-out)
            return x * x * (3 - 2 * x);
        case 2: // agresiva (cuadrática hacia arriba)
            return sqrtf(x);
        default: // lineal
            return x;
    }
}

void Pedal::init() {
    pinMode(PIN_PEDAL, INPUT);
    s = {0, 0.0f, true};
    
    // 🔒 CORRECCIÓN MEDIA: Cargar calibración de configuración
    if (cfg.pedalMin > 0 && cfg.pedalMax > cfg.pedalMin) {
        adcMin = cfg.pedalMin;
        adcMax = cfg.pedalMax;
        Logger::infof("Pedal: Calibración cargada %d-%d", adcMin, adcMax);
    } else {
        Logger::infof("Pedal: Usando calibración por defecto %d-%d", adcMin, adcMax);
    }
    
    Logger::info("Pedal init");
    initialized = true;
}

void Pedal::update() {
    // Note: pedalEnabled was removed from Config, pedal is always active
    // if(!cfg.pedalEnabled) {
    //     // Guard: si está desactivado → neutro
    //     s.raw = 0;
    //     s.percent = 0.0f;
    //     s.valid = false;
    //     return;
    // }

    int raw = analogRead(PIN_PEDAL);
    
    // 🔒 CORRECCIÓN CRÍTICA: analogRead retorna uint16_t (0-4095), no puede ser < 0
    // Validación correcta: solo verificar límite superior y rango válido
    if(raw > 4095) {
        s.valid = false;
        s.percent = lastPercent; // fallback
        System::logError(100);   // código reservado pedal
        Logger::errorf("Pedal lectura fuera de rango: %d", raw);
        return;
    }
    
    // 🔒 CORRECCIÓN: Aplicar filtro EMA para reducir ruido eléctrico
    if (rawFiltered == 0.0f) {
        rawFiltered = (float)raw;  // Inicializar en primera lectura
    } else {
        rawFiltered = rawFiltered + EMA_ALPHA * ((float)raw - rawFiltered);
    }
    
    s.raw = (int)rawFiltered;

    // 🔒 CORRECCIÓN: Validación adicional de hardware
    // Si el pedal está en reposo (esperado ~200) pero lee muy alto o muy bajo,
    // podría indicar problema de hardware
    if (!initialized) {
        Logger::warn("Pedal::update() llamado sin init");
        s.valid = false;
        return;
    }

    // Normalización con valores filtrados
    int clamped = constrain((int)rawFiltered, adcMin, adcMax);
    float norm = (float)(clamped - adcMin) / (float)(adcMax - adcMin);
    norm = constrain(norm, 0.0f, 1.0f);

    // Deadband
    if(norm < (deadbandPct / 100.0f)) norm = 0.0f;

    // Curva
    float curved = applyCurve(norm);

    // Clamp final
    s.percent = constrain(curved * 100.0f, 0.0f, 100.0f);
    s.valid = true;
    lastPercent = s.percent;
}

void Pedal::setCalibration(int minAdc, int maxAdc, uint8_t curve) {
    adcMin = minAdc;
    adcMax = maxAdc;
    curveMode = curve;
    Logger::infof("Pedal calibration updated: min=%d max=%d curve=%u", minAdc, maxAdc, curve);
}

void Pedal::getCalibration(int &minAdc, int &maxAdc, uint8_t &curve) {
    minAdc = adcMin;
    maxAdc = adcMax;
    curve = curveMode;
}

void Pedal::setDeadband(float percent) { 
    deadbandPct = constrain(percent, 0.0f, 20.0f); // clamp de seguridad
    Logger::infof("Pedal deadband set: %.1f%%", deadbandPct);
}

float Pedal::getDeadband() { 
    return deadbandPct; 
}

const Pedal::State& Pedal::get() { 
    return s; 
}

bool Pedal::initOK() {
    return initialized;
}