#include "steering_motor.h"
#include "pins.h"
#include "current.h"
#include "steering.h"
#include "logger.h"
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// PCA9685 para motor dirección (I²C 0x42 según pins.h I2C_ADDR_PCA9685_STEERING)
static Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(I2C_ADDR_PCA9685_STEERING);
static SteeringMotor::State s;
static bool initialized = false;

static const uint16_t kFreqHz = 1000;  // PWM estable para BTS7960
static const uint8_t  kChannelFwd = PCA_STEER_CH_PWM_FWD; // canal PCA para dirección forward
static const uint8_t  kChannelRev = PCA_STEER_CH_PWM_REV; // canal PCA para dirección reverse
static const float kDeadbandDeg = 0.5f;  // Zona muerta para evitar oscilación del motor

static uint16_t pctToTicks(float pct) {
    pct = constrain(pct, 0.0f, 100.0f);
    // PCA9685 usa 12-bit (0..4095). Mapear 0..100% → 0..4095 duty
    return (uint16_t)(pct * 40.95f);
}

void SteeringMotor::init() {
    // NOTA: Wire.begin() ya se llama en main.cpp vía I2CRecovery::init()
    // No llamar Wire.begin() aquí para evitar resetear configuración I2C
    pca.begin();
    pca.setPWMFreq(kFreqHz);

    s = {0, 0, 0};
    initialized = true;
    Logger::info("SteeringMotor init");
}

void SteeringMotor::setDemandAngle(float deg) {
    s.demandDeg = deg;
}

void SteeringMotor::update() {
    // 🔒 CORRECCIÓN CRÍTICA: Verificar inicialización antes de actualizar
    if (!initialized) {
        Logger::warn("SteeringMotor update llamado sin init");
        // NOTA: No intentamos parada de emergencia aquí porque pca.begin() 
        // no ha sido llamado y el objeto PCA9685 no está configurado.
        // El control de potencia debe hacerse vía relés (Relays::disablePower())
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
    // 🔒 CORRECCIÓN: Zona muerta para evitar oscilación del motor con errores pequeños
    uint16_t ticks = pctToTicks(cmdPct);
    if (absError < kDeadbandDeg) {
        // Error dentro de zona muerta: parar motor para evitar oscilación
        pca.setPWM(kChannelFwd, 0, 0);
        pca.setPWM(kChannelRev, 0, 0);
    } else if (error > 0) {
        // Girar hacia la derecha: activar canal FWD, desactivar REV
        pca.setPWM(kChannelFwd, 0, ticks);
        pca.setPWM(kChannelRev, 0, 0);
    } else {
        // Girar hacia la izquierda: activar canal REV, desactivar FWD
        pca.setPWM(kChannelFwd, 0, 0);
        pca.setPWM(kChannelRev, 0, ticks);
    }
    s.pwmOut = cmdPct;

    // Corriente de dirección (canal INA226 = 5)
    s.currentA = Sensors::getCurrent(5);
}