#include "steering_motor.h"
#include "pins.h"
#include "current.h"
#include "steering.h"
#include "logger.h"
#include "system.h"      // 🔒 v2.4.0: Para logError()
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <cmath>         // 🔒 v2.4.0: Para std::isfinite()

// PCA9685 para motor dirección (I²C 0x42 según pins.h I2C_ADDR_PCA9685_STEERING)
static Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(I2C_ADDR_PCA9685_STEERING);
static SteeringMotor::State s;
static bool initialized = false;
static bool pcaOK = false;  // 🔒 v2.4.0: Track PCA9685 initialization status

static const uint16_t kFreqHz = 1000;  // PWM estable para BTS7960
static const uint8_t  kChannelFwd = PCA_STEER_CH_PWM_FWD; // canal PCA para dirección forward
static const uint8_t  kChannelRev = PCA_STEER_CH_PWM_REV; // canal PCA para dirección reverse
static const float kDeadbandDeg = 0.5f;  // Zona muerta para evitar oscilación del motor
static const float kMaxCurrentA = 30.0f; // 🔒 v2.4.0: Límite de corriente para protección motor

static uint16_t pctToTicks(float pct) {
    pct = constrain(pct, 0.0f, 100.0f);
    // PCA9685 usa 12-bit (0..4095). Mapear 0..100% → 0..4095 duty
    return (uint16_t)(pct * 40.95f);
}

void SteeringMotor::init() {
    // NOTA: Wire.begin() ya se llama en main.cpp vía I2CRecovery::init()
    // No llamar Wire.begin() aquí para evitar resetear configuración I2C
    
    // 🔒 v2.4.0: Validar inicialización PCA9685 con retry
    pcaOK = pca.begin();
    if (!pcaOK) {
        Logger::error("SteeringMotor: PCA9685 init FAIL - retrying...");
        delay(50);  // Breve pausa antes de retry
        pcaOK = pca.begin();
        
        if (!pcaOK) {
            Logger::error("SteeringMotor: PCA9685 init FAIL definitivo");
            System::logError(250);  // Código: PCA9685 dirección no responde
            initialized = false;
            return;
        }
    }
    
    pca.setPWMFreq(kFreqHz);
    
    // 🔒 v2.4.0: Inicializar canales en estado apagado por seguridad
    pca.setPWM(kChannelFwd, 0, 0);
    pca.setPWM(kChannelRev, 0, 0);

    s = {0, 0, 0};
    initialized = true;
    Logger::info("SteeringMotor init OK");
}

void SteeringMotor::setDemandAngle(float deg) {
    s.demandDeg = deg;
}

void SteeringMotor::update() {
    // 🔒 CORRECCIÓN CRÍTICA: Verificar inicialización antes de actualizar
    if (!initialized || !pcaOK) {
        Logger::warn("SteeringMotor update llamado sin init");
        // NOTA: No intentamos parada de emergencia aquí porque pca.begin() 
        // no ha sido llamado y el objeto PCA9685 no está configurado.
        // El control de potencia debe hacerse vía relés (Relays::disablePower())
        return;
    }
    
    // 🔒 v2.4.0: Protección por sobrecorriente
    float currentA = Sensors::getCurrent(5);  // Canal 5 = motor dirección
    if (currentA > kMaxCurrentA && std::isfinite(currentA)) {
        Logger::errorf("SteeringMotor: OVERCURRENT %.1fA (límite %.0fA)", currentA, kMaxCurrentA);
        System::logError(251);  // Código: overcurrent motor dirección
        // Detener motor inmediatamente
        pca.setPWM(kChannelFwd, 0, 0);
        pca.setPWM(kChannelRev, 0, 0);
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

// 🔒 v2.4.0: Estado de inicialización
bool SteeringMotor::initOK() {
    return initialized && pcaOK;
}
