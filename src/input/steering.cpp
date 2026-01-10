#include "steering.h"
#include "logger.h"
#include "pins.h"
#include "settings.h"
#include "steering_model.h"
#include "storage.h"
#include "system.h" // para logError()

extern Storage::Config cfg;

static volatile long ticks = 0;
static long zeroOffset = 0;
static long ticksPerTurn = 1024; // ajustar a tu encoder
static volatile bool zSeen = false;

// 🔒 v2.16.0: OVERFLOW PROTECTION - Encoder safety limits
// E6B2-CWZ6C 1200 PPR encoder can overflow int32 after ~1.79 million rotations
// Limit to ±100,000 ticks (~83 full rotations) to prevent overflow and
// erroneous steering commands
static const int32_t TICKS_MAX_ABS = 100000; // Safety limit: prevents overflow

// Flag de inicialización
static bool initialized = false;

// Variables para timeout de centrado
static unsigned long centeringStartMs = 0;
static bool warnedNotCentered = false;

static Steering::State s;

// =============================
// 🔒 Steering signal conditioning
// =============================

// Filtro EMA para suavizar ruido EMI
static float steeringFilteredDeg = 0.0f;
static float steeringLastDeg = 0.0f;
static bool steeringFilterInit = false;

// Parámetros de protección
static constexpr float STEERING_EMA_ALPHA = 0.18f;      // Suavizado (0 = lento, 1 = sin filtro)
static constexpr float STEERING_MAX_JUMP = 15.0f;       // Rechazo EMI (grados por frame)
static constexpr float STEERING_MAX_RATE = 8.0f;        // Máx grados permitidos por update
static constexpr float STEERING_HYSTERESIS = 0.30f;     // Zona muerta para eliminar jitter

// 🔒 Función segura para leer ticks desde código no-ISR
static long getTicksSafe() {
  noInterrupts();
  long result = ticks;
  interrupts();
  return result;
}

void IRAM_ATTR isrEncA() {
  int a = digitalRead(PIN_ENCODER_A);
  int b = digitalRead(PIN_ENCODER_B);

  // 🔒 v2.16.0: OVERFLOW PROTECTION - Saturate ticks at safety limits
  // Prevents integer overflow that could cause sudden steering angle jumps
  // If encoder accumulated ±100,000 ticks without centering, saturate at limit
  // This prevents overflow from +2^31 to -2^31 which would cause dangerous
  // steering command
  int32_t delta;
  if (a == HIGH) {
    delta = (b == HIGH) ? +1 : -1;
  } else {
    delta = (b == HIGH) ? -1 : +1;
  }

  int32_t newTicks = ticks + delta;

  // Saturate at limits instead of overflowing
  if (newTicks > TICKS_MAX_ABS) {
    ticks = TICKS_MAX_ABS;
    // Note: Cannot call Logger from ISR, will be detected in update() loop
  } else if (newTicks < -TICKS_MAX_ABS) {
    ticks = -TICKS_MAX_ABS;
    // Note: Cannot call Logger from ISR, will be detected in update() loop
  } else {
    ticks = newTicks;
  }
}

void IRAM_ATTR isrEncZ() { zSeen = true; }

static float ticksToDegrees(long t) {
  return (float)(t - zeroOffset) * 360.0f / (float)ticksPerTurn;
}

void Steering::init() {
  // 🔒 CORRECCIÓN 1.2: Validación de pines antes de configurar
  if (PIN_ENCODER_A < 0 || PIN_ENCODER_B < 0 || PIN_ENCODER_Z < 0) {
    Logger::errorf("Steering: pines encoder inválidos (A=%d, B=%d, Z=%d)",
                   PIN_ENCODER_A, PIN_ENCODER_B, PIN_ENCODER_Z);
    System::logError(200); // código: pines encoder inválidos
    initialized = false;
    return;
  }

  // Verificar que los pines estén asignados correctamente
  if (!pin_is_assigned(PIN_ENCODER_A) || !pin_is_assigned(PIN_ENCODER_B) ||
      !pin_is_assigned(PIN_ENCODER_Z)) {
    Logger::errorf("Steering: pines encoder no asignados en pins.h");
    System::logError(201); // código: pines no asignados
    initialized = false;
    return;
  }

  pinMode(PIN_ENCODER_A, INPUT);
  pinMode(PIN_ENCODER_B, INPUT);
  pinMode(PIN_ENCODER_Z, INPUT);

  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_A), isrEncA, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIN_ENCODER_Z), isrEncZ, RISING);

  // 🔒 CORRECCIÓN 1.2: Inicialización explícita por campo
  s.ticks = 0;
  s.angleDeg = 0.0f;
  s.angleFL = 0.0f;
  s.angleFR = 0.0f;
  s.centered = false;
  s.valid = false;

  SteeringModel::setGeometry(0.95f, 0.70f, 54.0f);

  Logger::info("Steering init OK");
  initialized = true;
}

void Steering::update() {
  if (!cfg.steeringEnabled) {
    // Guard: si está desactivado → estado neutro
    s.ticks = 0;
    s.angleDeg = 0.0f;
    s.angleFL = 0.0f;
    s.angleFR = 0.0f;
    s.centered = false;
    s.valid = false;
    centeringStartMs = 0;
    warnedNotCentered = false;
    return;
  }

  // 🔒 CORRECCIÓN 1.1: Lectura atómica de ticks
  long t = getTicksSafe();
  s.ticks = t;

  // 🔒 v2.16.0: OVERFLOW DETECTION - Warn if encoder reached safety limits
  // This indicates encoder has not been centered for very long time (~83 full
  // rotations) Suggests Z signal not working or very unusual steering behavior
  if (t >= TICKS_MAX_ABS || t <= -TICKS_MAX_ABS) {
    static uint32_t lastOverflowWarning = 0;
    uint32_t now = millis();
    // Throttle warning to once every 10 seconds
    if (now - lastOverflowWarning > 10000) {
      Logger::warnf(
          "Steering: encoder at safety limit %ld ticks - check Z signal", t);
      System::logError(214); // código: encoder overflow protection activated
      lastOverflowWarning = now;
    }
  }

  s.angleDeg = ticksToDegrees(t);

  // =============================
  // 🔒 EMI + JITTER FILTERING LAYER
  // =============================
  float rawAngle = s.angleDeg;

  // Inicializar filtro en primer uso
  if (!steeringFilterInit) {
    steeringFilteredDeg = rawAngle;
    steeringLastDeg = rawAngle;
    steeringFilterInit = true;
  }

  // --- EMI spike rejection (saltos eléctricos) ---
  float delta = rawAngle - steeringLastDeg;
  if (fabs(delta) > STEERING_MAX_JUMP) {
    // Ignorar salto imposible → ruido EMI
    rawAngle = steeringLastDeg;
  } else {
    steeringLastDeg = rawAngle;
  }

  // --- EMA smoothing ---
  steeringFilteredDeg =
      STEERING_EMA_ALPHA * rawAngle +
      (1.0f - STEERING_EMA_ALPHA) * steeringFilteredDeg;

  // --- Rate limiting (velocidad máxima de giro) ---
  float rateDelta = steeringFilteredDeg - s.angleDeg;
  if (rateDelta > STEERING_MAX_RATE) {
    steeringFilteredDeg = s.angleDeg + STEERING_MAX_RATE;
  } else if (rateDelta < -STEERING_MAX_RATE) {
    steeringFilteredDeg = s.angleDeg - STEERING_MAX_RATE;
  }

  // --- Hysteresis (eliminar micro-jitter) ---
  float hystDelta = steeringFilteredDeg - s.angleDeg;
  if (fabs(hystDelta) < STEERING_HYSTERESIS) {
    steeringFilteredDeg = s.angleDeg; // mantener valor anterior
  }

  // 🔒 Aplicar resultado filtrado
  s.angleDeg = steeringFilteredDeg;

  // Clamp de ángulo global
  if (s.angleDeg > 54.0f) s.angleDeg = 54.0f;
  if (s.angleDeg < -54.0f) s.angleDeg = -54.0f;

  // 🔒 CORRECCIÓN 1.6: Timeout en centrado automático
  if (!s.centered && !zSeen) {
    if (centeringStartMs == 0) {
      centeringStartMs = millis();
    } else if (millis() - centeringStartMs > 10000) { // 10s timeout
      Logger::errorf("Steering: timeout centrado (10s) - usando fallback");
      System::logError(213); // código: timeout señal Z
      zeroOffset = t;        // fallback: posición actual como centro
      s.centered = true;
      centeringStartMs = 0;
      Logger::info("Steering centered by timeout fallback");
    }

    // 🔒 CORRECCIÓN 1.4: Log no repetitivo
    if (!warnedNotCentered) {
      Logger::warn("Steering not centered yet, waiting for Z signal");
      System::logError(210); // código: steering sin centrado
      warnedNotCentered = true;
    }
    s.valid = false;
  } else {
    s.valid = true;
    warnedNotCentered = false;
  }

  if (zSeen && !s.centered) {
    noInterrupts();
    zeroOffset = t;
    s.centered = true;
    zSeen = false;
    interrupts();
    centeringStartMs = 0; // reset timeout
    Logger::info("Steering centered at Z signal");
  }

  auto ack = SteeringModel::compute(s.angleDeg);

  if (s.angleDeg >= 0) {
    s.angleFR = constrain(ack.innerDeg, -60.0f, 60.0f);
    s.angleFL = constrain(ack.outerDeg, -60.0f, 60.0f);
  } else {
    s.angleFL = constrain(ack.innerDeg, -60.0f, 60.0f);
    s.angleFR = constrain(ack.outerDeg, -60.0f, 60.0f);
  }
}

void Steering::center() {
  if (s.centered) return;

  // 🔒 CORRECCIÓN: Lectura atómica de variables compartidas
  noInterrupts();
  bool zDetected = zSeen;
  long currentTicks = ticks;
  interrupts();

  if (zDetected) {
    noInterrupts();
    zeroOffset = currentTicks;
    s.centered = true;
    zSeen = false;
    interrupts();
    centeringStartMs = 0;
    Logger::info("Steering center via Z signal");
  } else {
    // Fallback manual: centrar en posición actual
    noInterrupts();
    zeroOffset = currentTicks;
    interrupts();
    s.centered = true;
    Logger::warn(
        "Steering center fallback - Z not detected, using current position");
    System::logError(211); // código: fallo de centrado por Z
  }
}

void Steering::setTicksPerTurn(long tpt) {
  // 🔒 CORRECCIÓN 1.3: Validación de rango razonable
  // Encoders típicos: 100-10000 PPR (pulsos por revolución)
  const long MIN_TPT = 100;
  const long MAX_TPT = 10000;
  const long DEFAULT_TPT = 1024;

  if (tpt < MIN_TPT || tpt > MAX_TPT) {
    Logger::errorf("Steering ticksPerTurn fuera de rango (%ld), válido: "
                   "%ld-%ld, usando default %ld",
                   tpt, MIN_TPT, MAX_TPT, DEFAULT_TPT);
    System::logError(212); // código: ticks per turn inválido
    ticksPerTurn = DEFAULT_TPT;
    return;
  }

  ticksPerTurn = tpt;
  Logger::infof("Steering ticksPerTurn set: %ld", ticksPerTurn);
}

long Steering::getTicksPerTurn() { return ticksPerTurn; }

void Steering::setZeroOffset(long offset) {
  zeroOffset = offset;
  s.centered = true;
  Logger::infof("Steering zeroOffset set: %ld", zeroOffset);
}

long Steering::getZeroOffset() { return zeroOffset; }

const Steering::State &Steering::get() { return s; }

bool Steering::initOK() { return initialized; }