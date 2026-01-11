#include "pedal.h"
#include "logger.h"
#include "pins.h"
#include "settings.h"
#include "storage.h"
#include "system.h" // para logError()

extern Storage::Config cfg;

// ========================================
// Constantes de validación de pedal
// ========================================
namespace PedalValidation {
constexpr int OUT_OF_RANGE_MARGIN_PERCENT = 10; // ±10% del rango calibrado
constexpr uint8_t MAX_STATIC_READS = 50;        // Lecturas estáticas antes de error
constexpr uint8_t MAX_EXTREME_VALUE_READS = 20; // Lecturas en extremos antes de error
constexpr uint16_t ADC_EXTREME_LOW = 10;        // Umbral inferior extremo
constexpr uint16_t ADC_EXTREME_HIGH = 4085;     // Umbral superior extremo
constexpr float MAX_PERCENT_CHANGE = 20.0f;    // Cambio máximo por update (%)
constexpr uint32_t WARN_THROTTLE_MS = 5000;     // Throttle de warnings (5s)
constexpr uint32_t STATIC_WARN_THROTTLE_MS = 10000; // Throttle error estático (10s)
} // namespace PedalValidation

static Pedal::State s;
static int adcMin = 200;
static int adcMax = 3800;
static uint8_t curveMode = 0;    // 0 lineal, 1 suave, 2 agresiva
static float deadbandPct = 3.0f; // % muerto inicial
static float lastPercent = 0.0f;

// 🔒 EMA filter
static constexpr float EMA_ALPHA = 0.15f;
static float rawFiltered = 0.0f;
static bool emaInitialized = false;   // 🔒 FIX

static bool initialized = false;

static float applyCurve(float x) {
  switch (curveMode) {
  case 1: return x * x * (3 - 2 * x); // suave
  case 2: return sqrtf(x);           // agresiva
  default: return x;                // lineal
  }
}

void Pedal::init() {
  pinMode(PIN_PEDAL, INPUT);
  s = {0, 0.0f, true};

  if (cfg.pedalMin > 0 && cfg.pedalMax > cfg.pedalMin) {
    adcMin = cfg.pedalMin;
    adcMax = cfg.pedalMax;
    Logger::infof("Pedal: Calibración cargada %d-%d", adcMin, adcMax);
  } else {
    Logger::infof("Pedal: Usando calibración por defecto %d-%d", adcMin, adcMax);
  }

  rawFiltered = 0.0f;
  emaInitialized = false;

  Logger::info("Pedal init");
  initialized = true;
}

void Pedal::update() {
  if (!initialized) {
    Logger::warn("Pedal::update() llamado sin init");
    s.valid = false;
    s.percent = 0.0f;
    return;
  }

  int raw = analogRead(PIN_PEDAL);

  // ==============================
  // VALIDACIÓN 1: Rango
  // ==============================
  const int MARGIN = (adcMax - adcMin) * PedalValidation::OUT_OF_RANGE_MARGIN_PERCENT / 100;

  if (raw < (adcMin - MARGIN) || raw > (adcMax + MARGIN)) {
    static uint32_t lastWarn = 0;
    if (millis() - lastWarn > PedalValidation::WARN_THROTTLE_MS) {
      Logger::warnf("Pedal: fuera de rango %d (esperado %d-%d)", raw, adcMin, adcMax);
      lastWarn = millis();
    }
    s.valid = false;
    s.percent = lastPercent;
    return;
  }

  // ==============================
  // VALIDACIÓN 2: ADC congelado
  // ==============================
  static int lastRaw = -1;
  static uint8_t staticCount = 0;

  if (raw == lastRaw) {
    if (++staticCount >= PedalValidation::MAX_STATIC_READS) {
      static uint32_t lastWarn = 0;
      if (millis() - lastWarn > PedalValidation::STATIC_WARN_THROTTLE_MS) {
        Logger::errorf("Pedal: ADC congelado (%d)", raw);
        lastWarn = millis();
      }
      s.valid = false;
      s.percent = 0.0f;
      s.raw = 0;
      return;
    }
  } else {
    staticCount = 0;
  }
  lastRaw = raw;

  // ==============================
  // VALIDACIÓN 3: Valores extremos
  // ==============================
  static uint8_t extremeCount = 0;
  if (raw <= PedalValidation::ADC_EXTREME_LOW || raw >= PedalValidation::ADC_EXTREME_HIGH) {
    if (++extremeCount >= PedalValidation::MAX_EXTREME_VALUE_READS) {
      s.valid = false;
      s.percent = 0.0f;
      s.raw = 0;
      return;
    }
  } else {
    extremeCount = 0;
  }

  // ==============================
  // EMA filter (FIXED)
  // ==============================
  if (!emaInitialized) {
    rawFiltered = (float)raw;
    emaInitialized = true;
  } else {
    rawFiltered = EMA_ALPHA * raw + (1.0f - EMA_ALPHA) * rawFiltered;
  }

  s.raw = (int)rawFiltered;

  // ==============================
  // Mapear a porcentaje
  // ==============================
  int clamped = constrain(s.raw, adcMin, adcMax);
  float mapped = (float)(clamped - adcMin) / (float)(adcMax - adcMin) * 100.0f;

  if (mapped < deadbandPct) mapped = 0.0f;

  float curved = applyCurve(mapped / 100.0f);
  s.percent = constrain(curved * 100.0f, 0.0f, 100.0f);

  // ==============================
  // Detección de picos EMI
  // ==============================
  if (fabs(s.percent - lastPercent) > PedalValidation::MAX_PERCENT_CHANGE) {
    s.percent = (s.percent > lastPercent)
                ? lastPercent + PedalValidation::MAX_PERCENT_CHANGE
                : lastPercent - PedalValidation::MAX_PERCENT_CHANGE;
  }

  lastPercent = s.percent;
  s.valid = true;
}

void Pedal::setCalibration(int minAdc, int maxAdc, uint8_t curve) {
  adcMin = minAdc;
  adcMax = maxAdc;
  curveMode = curve;
}

void Pedal::getCalibration(int &minAdc, int &maxAdc, uint8_t &curve) {
  minAdc = adcMin;
  maxAdc = adcMax;
  curve = curveMode;
}

void Pedal::setDeadband(float percent) {
  deadbandPct = constrain(percent, 0.0f, 20.0f);
}

float Pedal::getDeadband() { return deadbandPct; }

const Pedal::State &Pedal::get() { return s; }

bool Pedal::initOK() { return initialized; }