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
constexpr uint8_t MAX_STATIC_READS = 50; // Lecturas estáticas antes de error
constexpr uint8_t MAX_EXTREME_VALUE_READS =
    20;                                  // Lecturas en extremos antes de error
constexpr uint16_t ADC_EXTREME_LOW = 10; // Umbral inferior extremo
constexpr uint16_t ADC_EXTREME_HIGH = 4085; // Umbral superior extremo
constexpr float MAX_PERCENT_CHANGE = 20.0f; // Cambio máximo por update (%)
constexpr uint32_t WARN_THROTTLE_MS = 5000; // Throttle de warnings (5s)
constexpr uint32_t STATIC_WARN_THROTTLE_MS =
    10000; // Throttle error estático (10s)
} // namespace PedalValidation

static Pedal::State s;
static int adcMin = 200;
static int adcMax = 3800;
static uint8_t curveMode = 0;    // 0 lineal, 1 suave, 2 agresiva
static float deadbandPct = 3.0f; // % muerto inicial
static float lastPercent = 0.0f;

// 🔒 EMA
static constexpr float EMA_ALPHA = 0.15f;
static float rawFiltered = 0.0f;
static bool emaInitialized = false;   // ← FIX CRÍTICO

static bool initialized = false;

static float applyCurve(float x) {
  // x en [0,1]
  switch (curveMode) {
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
    Logger::infof("Pedal: Usando calibración por defecto %d-%d", adcMin,
                  adcMax);
  }

  rawFiltered = 0.0f;
  emaInitialized = false;

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
  const int MARGIN =
      (adcMax - adcMin) * PedalValidation::OUT_OF_RANGE_MARGIN_PERCENT / 100;

  if (raw < (adcMin - MARGIN) || raw > (adcMax + MARGIN)) {
    static uint32_t lastOutOfRangeWarn = 0;
    if (millis() - lastOutOfRangeWarn > PedalValidation::WARN_THROTTLE_MS) {
      Logger::warnf(
          "Pedal: Lectura fuera de rango calibrado: %d (esperado %d-%d ±%d)",
          raw, adcMin, adcMax, MARGIN);
      lastOutOfRangeWarn = millis();
    }

    s.valid = false;
    s.percent = lastPercent;
    return;
  }

  // ========================================
  // VALIDACIÓN 2: ADC congelado
  // ========================================
  static int lastRaw = -1;
  static uint8_t staticReadCount = 0;

  if (raw == lastRaw) {
    staticReadCount++;
    if (staticReadCount >= PedalValidation::MAX_STATIC_READS) {
      static uint32_t lastStaticWarn = 0;
      if (millis() - lastStaticWarn >
          PedalValidation::STATIC_WARN_THROTTLE_MS) {
        Logger::errorf(
            "Pedal: Lectura estática detectada (%d) - posible desconexión",
            raw);
        lastStaticWarn = millis();
      }

      s.valid = false;
      s.percent = 0.0f;
      s.raw = 0;
      return;
    }
  } else {
    staticReadCount = 0;
  }
  lastRaw = raw;

  // ========================================
  // VALIDACIÓN 3: Valores extremos sostenidos
  // ========================================
  static uint8_t extremeValueCount = 0;

  if (raw <= PedalValidation::ADC_EXTREME_LOW ||
      raw >= PedalValidation::ADC_EXTREME_HIGH) {
    extremeValueCount++;
    if (extremeValueCount >= PedalValidation::MAX_EXTREME_VALUE_READS) {
      s.valid = false;
      s.percent = 0.0f;
      s.raw = 0;
      return;
    }
  } else {
    extremeValueCount = 0;
  }

  // ========================================
  // EMA FILTER (FIXED)
  // ========================================
  if (!emaInitialized) {
    rawFiltered = (float)raw;
    emaInitialized = true;
  } else {
    rawFiltered = EMA_ALPHA * (float)raw + (1.0f - EMA_ALPHA) * rawFiltered;
  }

  s.raw = (int)rawFiltered;

  // ========================================
  // Mapeo a porcentaje
  // ========================================
  int clamped = constrain(s.raw, adcMin, adcMax);
  float mapped = (float)(clamped - adcMin) / (float)(adcMax - adcMin) * 100.0f;

  if (mapped < deadbandPct) mapped = 0.0f;

  float normalized = mapped / 100.0f;
  float curved = applyCurve(normalized);
  s.percent = curved * 100.0f;
  s.percent = constrain(s.percent, 0.0f, 100.0f);

  // ========================================
  // EMI spike clamp
  // ========================================
  if (fabs(s.percent - lastPercent) > PedalValidation::MAX_PERCENT_CHANGE) {
    if (s.percent > lastPercent)
      s.percent = lastPercent + PedalValidation::MAX_PERCENT_CHANGE;
    else
      s.percent = lastPercent - PedalValidation::MAX_PERCENT_CHANGE;
  }

  lastPercent = s.percent;
  s.valid = true;
}

void Pedal::setCalibration(int minAdc, int maxAdc, uint8_t curve) {
  adcMin = minAdc;
  adcMax = maxAdc;
  curveMode = curve;
  Logger::infof("Pedal calibration updated: min=%d max=%d curve=%u", minAdc,
                maxAdc, curve);
}

void Pedal::getCalibration(int &minAdc, int &maxAdc, uint8_t &curve) {
  minAdc = adcMin;
  maxAdc = adcMax;
  curve = curveMode;
}

void Pedal::setDeadband(float percent) {
  deadbandPct = constrain(percent, 0.0f, 20.0f);
  Logger::infof("Pedal deadband set: %.1f%%", deadbandPct);
}

float Pedal::getDeadband() { return deadbandPct; }

const Pedal::State &Pedal::get() { return s; }

bool Pedal::initOK() { return initialized; }