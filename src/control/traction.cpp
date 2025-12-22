#include "traction.h"
#include "current.h"
#include "logger.h"
#include "pedal.h"
#include "pins.h"
#include "sensors.h"
#include "settings.h"
#include "steering.h"
#include "storage.h"
#include "system.h"
#include "wheels.h"
#include "mcp23017_manager.h"

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <algorithm> // std::min, std::max
#include <cmath>     // std::isfinite, std::fabs
#include <cstdint>
#include <cstring>

extern Storage::Config cfg;

// PCA9685 objects for traction motor control
static Adafruit_PWMServoDriver pcaFront = Adafruit_PWMServoDriver(I2C_ADDR_PCA9685_FRONT);
static Adafruit_PWMServoDriver pcaRear = Adafruit_PWMServoDriver(I2C_ADDR_PCA9685_REAR);
static bool pcaFrontOK = false;
static bool pcaRearOK = false;

// MCP23017 manager for shared motor direction control (IN1/IN2)
static MCP23017Manager* mcpManager = nullptr;

static Traction::State s;
static bool initialized = false;

namespace {
// Implementación independiente de std::clamp para máxima compatibilidad
inline float clampf(float v, float lo, float hi) {
  if (v < lo)
    return lo;
  if (v > hi)
    return hi;
  return v;
}

// Constante de conversión PWM a ticks PCA9685
// PCA9685 usa 12 bits (0-4095 = 4096 valores totales)
// PWM interno es 8 bits (0-255 = 256 valores totales)
// Multiplicador: 16.0 proporciona mapeo conservador (255*16=4080, deja margen de 15 ticks)
// Para mapeo exacto usar 4095/255≈16.06, pero 16.0 es más seguro y evita saturación
constexpr float PWM_TO_TICKS_MULTIPLIER = 16.0f;

// Helper: Convertir PWM (0-255) a ticks PCA9685 (0-4095)
inline uint16_t pwmToTicks(float pwm) {
  uint16_t ticks = static_cast<uint16_t>(pwm * PWM_TO_TICKS_MULTIPLIER);
  // Arduino constrain() function: clamps value to range [0, 4095]
  return constrain(ticks, static_cast<uint16_t>(0), static_cast<uint16_t>(4095));
}

// Helper: Aplicar PWM y dirección a hardware según rueda
inline void applyHardwareControl(int wheelIndex, uint16_t pwmTicks, bool reverse) {
  // ✅ Validación añadida: verificar índice válido
  if (wheelIndex < 0 || wheelIndex >= 4) {
    Logger::errorf("Traction: Invalid wheel index %d", wheelIndex);
    return;
  }
  
  // Apply according to wheel position
  if (wheelIndex == Traction::FL) {
    if (pcaFrontOK) {
      pcaFront.setPWM(PCA_FRONT_CH_FL_FWD, 0, reverse ? 0 : pwmTicks);
      pcaFront.setPWM(PCA_FRONT_CH_FL_REV, 0, reverse ? pwmTicks : 0);
    }
    if (mcpManager && mcpManager->isOK()) {
      mcpManager->digitalWrite(MCP_PIN_FL_IN1, reverse ? LOW : HIGH);
      mcpManager->digitalWrite(MCP_PIN_FL_IN2, reverse ? HIGH : LOW);
    }
  } else if (wheelIndex == Traction::FR) {
    if (pcaFrontOK) {
      pcaFront.setPWM(PCA_FRONT_CH_FR_FWD, 0, reverse ? 0 : pwmTicks);
      pcaFront.setPWM(PCA_FRONT_CH_FR_REV, 0, reverse ? pwmTicks : 0);
    }
    if (mcpManager && mcpManager->isOK()) {
      mcpManager->digitalWrite(MCP_PIN_FR_IN1, reverse ? LOW : HIGH);
      mcpManager->digitalWrite(MCP_PIN_FR_IN2, reverse ? HIGH : LOW);
    }
  } else if (wheelIndex == Traction::RL) {
    if (pcaRearOK) {
      pcaRear.setPWM(PCA_REAR_CH_RL_FWD, 0, reverse ? 0 : pwmTicks);
      pcaRear.setPWM(PCA_REAR_CH_RL_REV, 0, reverse ? pwmTicks : 0);
    }
    if (mcpManager && mcpManager->isOK()) {
      mcpManager->digitalWrite(MCP_PIN_RL_IN1, reverse ? LOW : HIGH);
      mcpManager->digitalWrite(MCP_PIN_RL_IN2, reverse ? HIGH : LOW);
    }
  } else if (wheelIndex == Traction::RR) {
    if (pcaRearOK) {
      pcaRear.setPWM(PCA_REAR_CH_RR_FWD, 0, reverse ? 0 : pwmTicks);
      pcaRear.setPWM(PCA_REAR_CH_RR_REV, 0, reverse ? pwmTicks : 0);
    }
    if (mcpManager && mcpManager->isOK()) {
      mcpManager->digitalWrite(MCP_PIN_RR_IN1, reverse ? LOW : HIGH);
      mcpManager->digitalWrite(MCP_PIN_RR_IN2, reverse ? HIGH : LOW);
    }
  }
}

// 🔒 CORRECCIÓN 2.1: Obtener corriente máxima desde configuración
// En lugar de constante hardcodeada, usar valores configurables
inline float getMaxCurrentA(int channel) {
  // Canal 4 = batería (típico 100A), resto = motores (típico 50A)
  // 🔒 v2.10.2: Ahora usa valores configurables desde cfg
  if (channel == 4) {
    // Batería: usar valor configurado
    return cfg.maxBatteryCurrentA;
  } else {
    // Motores: usar valor configurado
    return cfg.maxMotorCurrentA;
  }
}

// Constantes de seguridad para PWM
constexpr float PWM_MAX_SAFE = 255.0f;  // Máximo PWM permitido (8-bit)
constexpr float PWM_MIN = 0.0f;          // Mínimo PWM

// Límites de seguridad para sensores
constexpr float TEMP_MIN_VALID = -40.0f;   // Temperatura mínima válida (°C)
constexpr float TEMP_MAX_VALID = 150.0f;   // Temperatura máxima válida (°C)
constexpr float TEMP_CRITICAL = 120.0f;    // Temperatura crítica (°C)
constexpr float CURRENT_MAX_REASONABLE = 200.0f;  // Corriente máxima razonable (A)

// Retry timing for I2C device initialization
constexpr uint32_t I2C_RETRY_INTERVAL_MS = 50;  // Non-blocking retry interval

// Mapea 0..100% -> 0..255 PWM con validación de límites
inline float demandPctToPwm(float pct) {
  float pwm = clampf(pct, 0.0f, 100.0f) * 255.0f / 100.0f;
  // Aplicar techo de seguridad de hardware
  return clampf(pwm, PWM_MIN, PWM_MAX_SAFE);
}

// Validar lectura de corriente
inline bool isCurrentValid(float currentA) {
  return std::isfinite(currentA) && 
         currentA >= -CURRENT_MAX_REASONABLE && 
         currentA <= CURRENT_MAX_REASONABLE;
}

// Validar lectura de temperatura
inline bool isTempValid(float tempC) {
  return std::isfinite(tempC) && 
         tempC >= TEMP_MIN_VALID && 
         tempC <= TEMP_MAX_VALID;
}
} // namespace

void Traction::init() {
  // Initialize state structure
  s = {};
  for (int i = 0; i < 4; ++i) {
    s.w[i] = {};
    s.w[i].demandPct = 0.0f;
    s.w[i].outPWM = 0.0f;
    s.w[i].effortPct = 0.0f;
    s.w[i].currentA = 0.0f;
    s.w[i].speedKmh = 0.0f;
    s.w[i].tempC = 0.0f;
    s.w[i].reverse = false;
  }
  s.enabled4x4 = false;
  s.demandPct = 0.0f;
  s.axisRotation = false;

  // NOTA: Wire.begin() ya se llama en main.cpp vía I2CRecovery::init()
  // No llamar Wire.begin() aquí para evitar resetear configuración I2C

  // Non-blocking retry state for PCA9685 front
  static uint32_t pcaFrontRetryTime = 0;
  static bool pcaFrontRetrying = false;

  // Initialize PCA9685 front axle (0x40) with non-blocking retry
  if (!pcaFrontOK && !pcaFrontRetrying) {
    pcaFrontOK = pcaFront.begin();
    if (!pcaFrontOK) {
      Logger::error("Traction: PCA9685 Front (0x40) init FAIL - will retry asynchronously");
      pcaFrontRetrying = true;
      pcaFrontRetryTime = millis();
    }
  }
  
  if (pcaFrontRetrying && (millis() - pcaFrontRetryTime >= I2C_RETRY_INTERVAL_MS)) {
    pcaFrontOK = pcaFront.begin();
    pcaFrontRetrying = false;
    
    if (!pcaFrontOK) {
      Logger::error("Traction: PCA9685 Front (0x40) init FAIL definitivo");
      System::logError(830);  // Error code: PCA9685 Front init failure
    }
  }
  
  if (pcaFrontOK) {
    pcaFront.setPWMFreq(1000);  // 1kHz for BTS7960
    // Initialize all channels to 0 for safety
    for (int ch = 0; ch < 4; ch++) {
      pcaFront.setPWM(ch, 0, 0);
    }
    Logger::info("Traction: PCA9685 Front (0x40) init OK");
  }

  // Non-blocking retry state for PCA9685 rear
  static uint32_t pcaRearRetryTime = 0;
  static bool pcaRearRetrying = false;

  // Initialize PCA9685 rear axle (0x41) with non-blocking retry
  if (!pcaRearOK && !pcaRearRetrying) {
    pcaRearOK = pcaRear.begin();
    if (!pcaRearOK) {
      Logger::error("Traction: PCA9685 Rear (0x41) init FAIL - will retry asynchronously");
      pcaRearRetrying = true;
      pcaRearRetryTime = millis();
    }
  }
  
  if (pcaRearRetrying && (millis() - pcaRearRetryTime >= I2C_RETRY_INTERVAL_MS)) {
    pcaRearOK = pcaRear.begin();
    pcaRearRetrying = false;
    
    if (!pcaRearOK) {
      Logger::error("Traction: PCA9685 Rear (0x41) init FAIL definitivo");
      System::logError(831);  // Error code: PCA9685 Rear init failure
    }
  }
  
  if (pcaRearOK) {
    pcaRear.setPWMFreq(1000);  // 1kHz for BTS7960
    // Initialize all channels to 0 for safety
    for (int ch = 0; ch < 4; ch++) {
      pcaRear.setPWM(ch, 0, 0);
    }
    Logger::info("Traction: PCA9685 Rear (0x41) init OK");
  }

  // Get shared MCP23017 manager instance (initialized by ControlManager)
  mcpManager = &MCP23017Manager::getInstance();
  
  if (mcpManager->isOK()) {
    // Configure GPIOA0-A7 as OUTPUT for IN1/IN2 control
    for (int pin = MCP_PIN_FL_IN1; pin <= MCP_PIN_RR_IN2; pin++) {
      mcpManager->pinMode(pin, OUTPUT);
      mcpManager->digitalWrite(pin, LOW);  // Initialize to LOW for safety
    }
    Logger::info("Traction: MCP23017 (0x20) GPIOA configured via manager");
  } else {
    Logger::error("Traction: MCP23017 manager not available");
  }

  // System is initialized if all hardware components are OK
  // SAFETY: Strict initialization required - all I2C devices must be functional
  // Partial operation is not allowed to prevent unpredictable vehicle behavior
  // (e.g., unbalanced traction if one axle fails could cause loss of control)
  // Future enhancement: Could implement graceful degradation with FWD-only mode
  // if rear PCA9685 fails, but would require extensive testing for safety
  initialized = (pcaFrontOK && pcaRearOK && mcpManager && mcpManager->isOK());
  Logger::infof("Traction init: %s", initialized ? "OK" : "FAIL");
}

void Traction::setMode4x4(bool on) {
  s.enabled4x4 = on;
  Logger::infof("Traction mode set: %s", on ? "4x4" : "4x2");
  // Si hay acciones hardware (p. ej. activar relés), se deberían llamar aquí.
}

// Establecer modo de giro sobre eje (tank turn)
// SEGURIDAD: La velocidad de giro es controlada por el pedal
// El parámetro speedPct se mantiene por compatibilidad pero no se usa
void Traction::setAxisRotation(bool enabled, float speedPct) {
  (void)speedPct; // No se usa, velocidad controlada por pedal
  
  bool wasEnabled = s.axisRotation;
  s.axisRotation = enabled;

  if (enabled && !wasEnabled) {
    Logger::info("Traction: AXIS ROTATION ON (velocidad controlada por pedal)");
    // Inicializar direcciones cuando se activa el modo
    for (int i = 0; i < 4; ++i) {
      s.w[i].reverse = false;
    }
  } else if (!enabled && wasEnabled) {
    Logger::info("Traction: AXIS ROTATION OFF - resetting to normal mode");
    // Reset controlado: asegurar que todas las ruedas vuelvan a modo normal
    for (int i = 0; i < 4; ++i) {
      s.w[i].reverse = false;
      s.w[i].demandPct = 0.0f;  // Detener todas las ruedas suavemente
      s.w[i].outPWM = 0.0f;
    }
    // Apagar motores en hardware
    if (pcaFrontOK) {
      for (int ch = 0; ch < 4; ch++) {
        pcaFront.setPWM(ch, 0, 0);
      }
    }
    if (pcaRearOK) {
      for (int ch = 0; ch < 4; ch++) {
        pcaRear.setPWM(ch, 0, 0);
      }
    }
    if (mcpOK) {
      for (int pin = MCP_PIN_FL_IN1; pin <= MCP_PIN_RR_IN2; pin++) {
        mcp.digitalWrite(pin, LOW);
      }
    }
    // Resetear demanda global para transición suave
    s.demandPct = 0.0f;
    Logger::info("Traction: All wheels reset to forward, demand cleared");
  }
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
      s.w[i].reverse = false;
    }
    s.enabled4x4 = false;
    s.axisRotation = false;
    return;
  }

  // ============================================================
  // MODO GIRO SOBRE EJE (AXIS ROTATION / TANK TURN)
  // ============================================================
  // Ruedas izquierdas (FL, RL) giran hacia adelante
  // Ruedas derechas (FR, RR) giran hacia atrás (o viceversa)
  // Esto crea una rotación sobre el eje vertical del vehículo
  //
  // SEGURIDAD: La velocidad de giro es controlada por el pedal
  // Si se suelta el pedal, el giro se detiene inmediatamente
  if (s.axisRotation) {
    // Usar demanda del pedal como velocidad de rotación (seguridad)
    // Si el pedal se suelta, demandPct = 0 y el giro para
    float rotSpeed = s.demandPct;

    // Lado izquierdo: adelante
    s.w[FL].demandPct = rotSpeed;
    s.w[FL].reverse = false;
    s.w[RL].demandPct = rotSpeed;
    s.w[RL].reverse = false;

    // Lado derecho: atrás (giro en sentido antihorario visto desde arriba)
    s.w[FR].demandPct = rotSpeed;
    s.w[FR].reverse = true;
    s.w[RR].demandPct = rotSpeed;
    s.w[RR].reverse = true;

    Logger::debugf("Traction AXIS ROTATION: pedal=%.1f%%, L=FWD, R=REV",
                   rotSpeed);

    // Calcular PWM y leer sensores con validación mejorada
    for (int i = 0; i < 4; ++i) {
      s.w[i].outPWM = demandPctToPwm(s.w[i].demandPct);
      
      // Apply PWM and direction to hardware
      uint16_t pwmTicks = pwmToTicks(s.w[i].outPWM);
      applyHardwareControl(i, pwmTicks, s.w[i].reverse);

      // Leer corriente con validación
      if (cfg.currentSensorsEnabled) {
        float currentA = Sensors::getCurrent(i);
        if (!isCurrentValid(currentA)) {
          System::logError(810 + i);  // códigos 810-813 para motores FL-RR
          Logger::warnf("Axis rotation: invalid current wheel %d: %.2fA", i, currentA);
          currentA = 0.0f;
        }
        s.w[i].currentA = currentA;
        float maxA = getMaxCurrentA(i);
        s.w[i].effortPct = clampf((currentA / maxA) * 100.0f, 0.0f, 100.0f);
      }

      // Leer temperatura con validación
      if (cfg.tempSensorsEnabled) {
        float tempC = Sensors::getTemperature(i);
        if (!isTempValid(tempC)) {
          System::logError(820 + i);  // códigos 820-823 para motores FL-RR
          Logger::warnf("Axis rotation: invalid temp wheel %d: %.1f°C", i, tempC);
          tempC = 0.0f;
        } else if (tempC > TEMP_CRITICAL) {
          Logger::warnf("Axis rotation: critical temp wheel %d: %.1f°C", i, tempC);
        }
        s.w[i].tempC = tempC;
      }
    }

    return; // Salir del update, no procesar modo normal
  }

  // Reset reverse flags in normal mode (solo cuando no está en axis rotation)
  for (int i = 0; i < 4; ++i) {
    s.w[i].reverse = false;
  }

  // 🔒 CORRECCIÓN 2.3: Reparto básico 50/50 entre ejes en 4x4
  // En modo 4x2, toda la potencia va al eje delantero
  const float base = s.demandPct;
  float front = 0.0f;
  float rear = 0.0f;

  if (s.enabled4x4) {
    // Modo 4x4: reparto 50% delantero, 50% trasero
    front = base * 0.5f;
    rear = base * 0.5f;
    Logger::debugf("Traction 4x4: base=%.1f%%, front=%.1f%%, rear=%.1f%%", base,
                   front, rear);
  } else {
    // Modo 4x2: toda la potencia a ejes delanteros, traseros en 0
    front = base;
    rear = 0.0f;
    Logger::debugf("Traction 4x2: base=%.1f%%, front=%.1f%%, rear=0%%", base,
                   front);
  }

  // Ackermann: ajustar según ángulo de dirección
  auto steer = Steering::get();
  float factorFL = 1.0f;
  float factorFR = 1.0f;

  if (cfg.steeringEnabled && steer.valid) {
    float angle = std::fabs(steer.angleDeg);

    // 🔒 CORRECCIÓN 2.4: Escalado Ackermann mejorado para curvas más suaves
    // Curva progresiva: a 30° -> 85%, a 45° -> 77.5%, a 60° -> 70%
    // Evita reducción brusca en curvas cerradas
    // Fórmula optimizada: scale = 1.0 - (angle / 60.0)^1.2 * 0.3
    float angleNormalized = clampf(angle / 60.0f, 0.0f, 1.0f);
    float x = angleNormalized;
    float x_pow_1_2 = static_cast<float>(std::pow(x, 1.2f));  // x^1.2 exacto
    float scale = 1.0f - x_pow_1_2 * 0.3f;
    scale = clampf(scale, 0.70f, 1.0f);  // Mínimo 70% en curvas máximas

    if (steer.angleDeg > 0.0f) {
      // Giro a la derecha: reducir rueda derecha
      factorFR = scale;
    } else if (steer.angleDeg < 0.0f) {
      // Giro a la izquierda: reducir rueda izquierda
      factorFL = scale;
    }

    Logger::debugf("Ackermann: angle=%.1f°, factorFL=%.3f, factorFR=%.3f",
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
      // 🔒 CORRECCIÓN 2.5: API de sensores usa índices 0-based (0=FL, 1=FR,
      // 2=RL, 3=RR) Documentado claramente en sensors.h
      float currentA = Sensors::getCurrent(i);

      // 🔒 MEJORA: Validación robusta con verificación de rango
      if (!isCurrentValid(currentA)) {
        System::logError(810 + i); // códigos 810-813 para motores FL-RR
        Logger::errorf("Traction: corriente inválida rueda %d: %.2fA (límite ±%.0fA)", 
                      i, currentA, CURRENT_MAX_REASONABLE);
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
      
      // 🔒 MEJORA: Validación robusta con verificación de rango
      if (!isTempValid(t)) {
        System::logError(820 + i); // códigos 820-823 para motores FL-RR
        Logger::errorf("Traction: temperatura inválida rueda %d: %.1f°C (rango %.0f-%.0f°C)", 
                      i, t, TEMP_MIN_VALID, TEMP_MAX_VALID);
        t = 0.0f;
      } else {
        // Advertir si temperatura es crítica
        if (t > TEMP_CRITICAL) {
          Logger::warnf("Traction: temperatura crítica rueda %d: %.1f°C (>%.0f°C)", 
                       i, t, TEMP_CRITICAL);
        }
      }
      
      s.w[i].tempC = clampf(t, TEMP_MIN_VALID, TEMP_MAX_VALID);
    } else {
      s.w[i].tempC = 0.0f;
    }

    // -- Velocidad: si tienes Sensors::getSpeed o similar, añádelo aquí
    // s.w[i].speedKmh = Sensors::getSpeedKmh(i);

    // -- PWM de salida (valor a aplicar al driver BTS7960 u otro)
    // 🔒 MEJORA: Aplicar validación de techo de PWM (realizada dentro de demandPctToPwm)
    s.w[i].outPWM = demandPctToPwm(s.w[i].demandPct);
    
    // Apply PWM and direction to hardware
    uint16_t pwmTicks = pwmToTicks(s.w[i].outPWM);
    applyHardwareControl(i, pwmTicks, s.w[i].reverse);
  }

  // 🔒 CORRECCIÓN 2.6: Validación mejorada de reparto anómalo
  float sumDemand = s.w[FL].demandPct + s.w[FR].demandPct + s.w[RL].demandPct +
                    s.w[RR].demandPct;

  // Calcular límite esperado según modo
  float maxExpectedSum =
      s.enabled4x4 ? (base * 2.0f) : base; // 4x4: base*2, 4x2: base
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
    Logger::warnf(
        "Traction: reparto asimétrico extremo (max=%.1f%%, min=%.1f%%)",
        maxWheel, minWheel);
  }
}

const Traction::State &Traction::get() { return s; }

bool Traction::initOK() { return initialized; }
