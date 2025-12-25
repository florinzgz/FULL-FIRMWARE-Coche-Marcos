#include "pedal.h"
#include "pins.h"
#include "settings.h"
#include "logger.h"
#include "storage.h"
#include "system.h"   // para logError()

extern Storage::Config cfg;

// ========================================
// Constantes de validación de pedal
// ========================================
namespace PedalValidation {
    constexpr int OUT_OF_RANGE_MARGIN_PERCENT = 10;    // ±10% del rango calibrado
    constexpr uint8_t MAX_STATIC_READS = 50;           // Lecturas estáticas antes de error
    constexpr uint8_t MAX_EXTREME_VALUE_READS = 20;    // Lecturas en extremos antes de error
    constexpr uint16_t ADC_EXTREME_LOW = 10;           // Umbral inferior extremo
    constexpr uint16_t ADC_EXTREME_HIGH = 4085;        // Umbral superior extremo
    constexpr float MAX_PERCENT_CHANGE = 20.0f;        // Cambio máximo por update (%)
    constexpr uint32_t WARN_THROTTLE_MS = 5000;        // Throttle de warnings (5s)
    constexpr uint32_t STATIC_WARN_THROTTLE_MS = 10000; // Throttle error estático (10s)
}

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
    // ========================================
    // Guard: Verificar inicialización PRIMERO
    // ========================================
    if (!initialized) {
        Logger::warn("Pedal::update() llamado sin init");
        s.valid = false;
        s.percent = 0.0f;
        return;
    }

    int raw = analogRead(PIN_PEDAL);
    
    // ========================================
    // VALIDACIÓN 1: Rango de calibración
    // ========================================
    // Si la lectura está muy fuera del rango calibrado, indica problema hardware
    const int MARGIN = (adcMax - adcMin) / 10;  // 10% margen de tolerancia
    
    if (raw < (adcMin - MARGIN) || raw > (adcMax + MARGIN)) {
        // Posible desconexión o calibración incorrecta
        static uint32_t lastOutOfRangeWarn = 0;
        if (millis() - lastOutOfRangeWarn > PedalValidation::WARN_THROTTLE_MS) {  // Throttle: log cada 5s
            Logger::warnf("Pedal: Lectura fuera de rango calibrado: %d (esperado %d-%d ±%d)", 
                         raw, adcMin, adcMax, MARGIN);
            lastOutOfRangeWarn = millis();
        }
        
        // Usar último valor válido como fallback seguro
        s.valid = false;
        s.percent = lastPercent;
        return;
    }
    
    // ========================================
    // VALIDACIÓN 2: Detección de lectura estática
    // ========================================
    // Si el ADC lee el mismo valor muchas veces consecutivas, 
    // probablemente el sensor está desconectado o defectuoso
    static int lastRaw = -1;
    static uint8_t staticReadCount = 0;
    
    if (raw == lastRaw) {
        staticReadCount++;
        if (staticReadCount >= PedalValidation::MAX_STATIC_READS) {
            static uint32_t lastStaticWarn = 0;
            if (millis() - lastStaticWarn > PedalValidation::STATIC_WARN_THROTTLE_MS) {  // Log cada 10s
                Logger::errorf("Pedal: Lectura estática detectada (%d) - posible desconexión", raw);
                lastStaticWarn = millis();
            }
            
            // SAFE: Forzar pedal en reposo (0%) para seguridad
            s.valid = false;
            s.percent = 0.0f;
            s.raw = 0;
            return;
        }
    } else {
        // Lectura cambió, resetear contador
        staticReadCount = 0;
    }
    lastRaw = raw;
    
    // ========================================
    // VALIDACIÓN 3: Detección de valores extremos sostenidos
    // ========================================
    // Si ADC está pegado en 0 o 4095, probablemente hay problema eléctrico
    static uint8_t extremeValueCount = 0;
    
    if (raw <= PedalValidation::ADC_EXTREME_LOW || raw >= PedalValidation::ADC_EXTREME_HIGH) {  // Cerca de límites ADC
        extremeValueCount++;
        if (extremeValueCount >= PedalValidation::MAX_EXTREME_VALUE_READS) {
            static uint32_t lastExtremeWarn = 0;
            if (millis() - lastExtremeWarn > PedalValidation::STATIC_WARN_THROTTLE_MS) {
                Logger::errorf("Pedal: Valor extremo sostenido (%d) - verificar hardware", raw);
                lastExtremeWarn = millis();
            }
            
            // SAFE: Pedal en reposo
            s.valid = false;
            s.percent = 0.0f;
            s.raw = 0;
            return;
        }
    } else {
        extremeValueCount = 0;
    }
    
    // ========================================
    // Filtro EMA (Exponential Moving Average)
    // ========================================
    // Solo aplicar si pasaron todas las validaciones anteriores
    if (rawFiltered == 0.0f) {
        rawFiltered = (float)raw;  // Inicializar en primera lectura
    }
    rawFiltered = EMA_ALPHA * (float)raw + (1.0f - EMA_ALPHA) * rawFiltered;
    
    s.raw = (int)rawFiltered;
    
    // ========================================
    // Mapeo a porcentaje (0-100%)
    // ========================================
    int clamped = constrain(s.raw, adcMin, adcMax);
    // Usar cálculo directo de punto flotante para mejor precisión
    float mapped = (float)(clamped - adcMin) / (float)(adcMax - adcMin) * 100.0f;
    
    // Aplicar deadband (zona muerta inicial)
    if (mapped < deadbandPct) {
        mapped = 0.0f;
    }
    
    // Aplicar curva de respuesta
    float normalized = mapped / 100.0f;
    float curved = applyCurve(normalized);
    s.percent = curved * 100.0f;
    
    // Limitar a rango válido
    s.percent = constrain(s.percent, 0.0f, 100.0f);
    
    // Validación final: detectar cambios bruscos (posible glitch ADC)
    if (fabs(s.percent - lastPercent) > PedalValidation::MAX_PERCENT_CHANGE) {
        static uint32_t lastGlitchWarn = 0;
        if (millis() - lastGlitchWarn > PedalValidation::WARN_THROTTLE_MS) {
            Logger::warnf("Pedal: Cambio brusco detectado: %.1f%% -> %.1f%%", 
                         lastPercent, s.percent);
            lastGlitchWarn = millis();
        }
        
        // Suavizar el cambio
        if (s.percent > lastPercent) {
            s.percent = lastPercent + PedalValidation::MAX_PERCENT_CHANGE;
        } else {
            s.percent = lastPercent - PedalValidation::MAX_PERCENT_CHANGE;
        }
    }
    
    lastPercent = s.percent;
    s.valid = true;
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