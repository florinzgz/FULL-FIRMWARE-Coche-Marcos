#include "steering_motor.h"
#include "current.h"
#include "logger.h"
#include "mcp23017_manager.h"
#include "pins.h"
#include "pwm_channels.h" // 🔒 v2.8.5: PWM channel validation
#include "steering.h"
#include "system.h" // 🔒 v2.4.0: Para logError()
#include <Adafruit_PWMServoDriver.h>
#include <Wire.h>
#include <cmath> // 🔒 v2.4.0: Para std::isfinite()

// PCA9685 para motor dirección (I²C 0x42 según pins.h
// I2C_ADDR_PCA9685_STEERING) 🔒 v2.11.6: BOOTLOOP FIX - Use default constructor
// only Explicit constructor call Adafruit_PWMServoDriver(addr) runs I2C init in
// global constructor (before main) which can crash on ESP32-S3 OPI The address
// will be set during begin() call in init()
static Adafruit_PWMServoDriver pca;
// MCP23017 manager for shared control IN1/IN2 (I²C 0x20)
static MCP23017Manager *mcpManager = nullptr;
static SteeringMotor::State s;
static bool initialized = false;
static bool pcaOK = false; // 🔒 v2.4.0: Track PCA9685 initialization status

static const uint16_t kFreqHz = 1000; // PWM estable para BTS7960
static const uint8_t kChannelFwd =
    PCA_STEER_CH_PWM_FWD; // canal PCA para dirección forward
static const uint8_t kChannelRev =
    PCA_STEER_CH_PWM_REV; // canal PCA para dirección reverse
static const float kDeadbandDeg =
    0.5f; // Zona muerta para evitar oscilación del motor
static const float kMaxCurrentA =
    30.0f; // 🔒 v2.4.0: Límite de corriente para protección motor
static const uint32_t kRetryIntervalMs =
    50; // Non-blocking retry interval for I2C init

static uint16_t pctToTicks(float pct) {
  pct = constrain(pct, 0.0f, 100.0f);
  // PCA9685 usa 12-bit (0..4095). Mapear 0..100% → 0..4095 duty
  return (uint16_t)(pct * 40.95f);
}

void SteeringMotor::init() {
  // NOTA: Wire.begin() ya se llama en main.cpp vía I2CRecovery::init()
  // No llamar Wire.begin() aquí para evitar resetear configuración I2C

  // 🔒 v2.8.5: Validate PWM channels match expected steering configuration
  if (!pwm_channels_match_steering_config(kChannelFwd, kChannelRev)) {
    Logger::errorf("SteeringMotor: PWM channel config mismatch FWD=%d REV=%d",
                   kChannelFwd, kChannelRev);
    System::logError(252); // Código: PWM channel inválido
    initialized = false;
    pcaOK = false;
    return;
  }

  // Non-blocking retry state for PCA9685
  static uint32_t pcaRetryTime = 0;
  static bool pcaRetrying = false;

  // 🔒 v2.4.0: Validar inicialización PCA9685 con retry no bloqueante
  if (!pcaOK && !pcaRetrying) {
    pca.begin(I2C_ADDR_PCA9685_STEERING); // 🔒 v2.11.6: Explicit address since
                                          // no constructor param
    Wire.beginTransmission(I2C_ADDR_PCA9685_STEERING);
    pcaOK = (Wire.endTransmission() == 0);
    if (!pcaOK) {
      Logger::error(
          "SteeringMotor: PCA9685 init FAIL - will retry asynchronously");
      pcaRetrying = true;
      pcaRetryTime = millis();
    }
  }

  if (pcaRetrying && (millis() - pcaRetryTime >= kRetryIntervalMs)) {
    pca.begin(I2C_ADDR_PCA9685_STEERING); // 🔒 v2.11.6: Explicit address since
                                          // no constructor param
    Wire.beginTransmission(I2C_ADDR_PCA9685_STEERING);
    pcaOK = (Wire.endTransmission() == 0);
    pcaRetrying = false;

    if (!pcaOK) {
      Logger::error("SteeringMotor: PCA9685 init FAIL definitivo");
      System::logError(250); // Código: PCA9685 dirección no responde
      initialized = false;
      return;
    }
  }

  if (pcaOK) {
    pca.setPWMFreq(kFreqHz);

    // 🔒 v2.16.0: SAFETY - Validate PWM channels before use to prevent crashes
    // Invalid channel numbers can cause I2C bus errors or undefined behavior
    if (pwm_channel_valid(kChannelFwd) && pwm_channel_valid(kChannelRev)) {
      // Inicializar canales en estado apagado por seguridad
      pca.setPWM(kChannelFwd, 0, 0);
      pca.setPWM(kChannelRev, 0, 0);
    } else {
      Logger::errorf("SteeringMotor: Invalid PWM channels FWD=%d REV=%d",
                     kChannelFwd, kChannelRev);
      System::logError(253); // Código: PWM channel inválido en init
      initialized = false;
      pcaOK = false;
      return;
    }
  }

  // Get shared MCP23017 manager instance (initialized by ControlManager)
  mcpManager = &MCP23017Manager::getInstance();

  if (mcpManager && mcpManager->isOK()) {
    mcpManager->pinMode(MCP_PIN_STEER_IN1, OUTPUT);
    mcpManager->pinMode(MCP_PIN_STEER_IN2, OUTPUT);
    mcpManager->digitalWrite(MCP_PIN_STEER_IN1, LOW);
    mcpManager->digitalWrite(MCP_PIN_STEER_IN2, LOW);
    Logger::info("SteeringMotor: MCP23017 IN1/IN2 configured via manager");
  } else {
    Logger::error("SteeringMotor: MCP23017 manager not available");
    System::logError(254);
  }

  s = {0, 0, 0};
  initialized = (pcaOK && mcpManager && mcpManager->isOK());
  Logger::infof("SteeringMotor init: %s", initialized ? "OK" : "FAIL");
}

void SteeringMotor::setDemandAngle(float deg) { s.demandDeg = deg; }

void SteeringMotor::update() {
  // 🔒 CORRECCIÓN CRÍTICA: Verificar inicialización antes de actualizar
  if (!initialized || !pcaOK || !mcpManager || !mcpManager->isOK()) {
    Logger::warn("SteeringMotor update llamado sin init");
    // NOTA: No intentamos parada de emergencia aquí porque pca.begin()
    // no ha sido llamado y el objeto PCA9685 no está configurado.
    // El control de potencia debe hacerse vía relés (Relays::disablePower())
    return;
  }

  // 🔒 v2.4.0: Protección por sobrecorriente
  float currentA = Sensors::getCurrent(5); // Canal 5 = motor dirección
  if (currentA > kMaxCurrentA && std::isfinite(currentA)) {
    Logger::errorf("SteeringMotor: OVERCURRENT %.1fA (límite %.0fA)", currentA,
                   kMaxCurrentA);
    System::logError(251); // Código: overcurrent motor dirección
    // Detener motor inmediatamente
    pca.setPWM(kChannelFwd, 0, 0);
    pca.setPWM(kChannelRev, 0, 0);
    mcpManager->digitalWrite(MCP_PIN_STEER_IN1, LOW);
    mcpManager->digitalWrite(MCP_PIN_STEER_IN2, LOW);
    s.pwmOut = 0;
    s.currentA = currentA;
    return;
  }

  // Control sencillo: seguir el ángulo de mando (puede venir de alg. superior)
  float target = s.demandDeg;
  float actual = Steering::get().angleDeg;
  float error = target - actual;
  float absError = fabs(error);

  // PID muy básico (proporcional)
  float kp = 1.2f;
  float cmdPct = constrain(absError * kp, 0.0f, 100.0f);

  // Control bidireccional usando canales FWD/REV según signo del error
  // 🔒 CORRECCIÓN: Zona muerta para evitar oscilación del motor con errores
  // pequeños 🔒 v2.16.0: PWM channel validation to prevent crashes from invalid
  // channels
  uint16_t ticks = pctToTicks(cmdPct);
  if (absError < kDeadbandDeg) {
    // Error dentro de zona muerta: parar motor para evitar oscilación
    if (pwm_channel_valid(kChannelFwd)) pca.setPWM(kChannelFwd, 0, 0);
    if (pwm_channel_valid(kChannelRev)) pca.setPWM(kChannelRev, 0, 0);
    mcpManager->digitalWrite(MCP_PIN_STEER_IN1, LOW);
    mcpManager->digitalWrite(MCP_PIN_STEER_IN2, LOW);
  } else if (error > 0) {
    // Girar hacia la derecha: activar canal FWD, desactivar REV
    if (pwm_channel_valid(kChannelFwd) && pwm_channel_valid(kChannelRev)) {
      pca.setPWM(kChannelFwd, 0, ticks);
      pca.setPWM(kChannelRev, 0, 0);
    }
    mcpManager->digitalWrite(MCP_PIN_STEER_IN1, HIGH);
    mcpManager->digitalWrite(MCP_PIN_STEER_IN2, LOW);
  } else {
    // Girar hacia la izquierda: activar canal REV, desactivar FWD
    if (pwm_channel_valid(kChannelFwd) && pwm_channel_valid(kChannelRev)) {
      pca.setPWM(kChannelFwd, 0, 0);
      pca.setPWM(kChannelRev, 0, ticks);
    }
    mcpManager->digitalWrite(MCP_PIN_STEER_IN1, LOW);
    mcpManager->digitalWrite(MCP_PIN_STEER_IN2, HIGH);
  }
  s.pwmOut = cmdPct;

  // Corriente de dirección (canal INA226 = 5)
  s.currentA = Sensors::getCurrent(5);
}

// 🔒 v2.4.0: Estado de inicialización
bool SteeringMotor::initOK() {
  return initialized && pcaOK && mcpManager && mcpManager->isOK();
}

// 🔒 v2.4.0: Obtener estado actual del motor de dirección
const SteeringMotor::State &SteeringMotor::get() { return s; }
