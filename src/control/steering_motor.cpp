#include "steering_motor.h"
#include "pins.h"
#include "current.h"
#include "steering.h"
#include "logger.h"
#include "system.h"
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <cmath>  // 🔒 Para fabsf()

// 🔒 PCA9685 para dirección usa dirección 0x42 según pins.h
static Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(I2C_ADDR_PCA9685_STEERING);
static SteeringMotor::State s;

// 🔒 Flag de inicialización
static bool initialized = false;

static const uint16_t kFreqHz = 1000;  // PWM estable para BTS7960
// 🔒 Usar canales definidos en pins.h para control bidireccional
static const uint8_t kChannelFwd = PCA_STEER_CH_PWM_FWD;  // Canal 0: Forward
static const uint8_t kChannelRev = PCA_STEER_CH_PWM_REV;  // Canal 1: Reverse

// 🔒 Parámetros de control PID
static constexpr float KP = 1.2f;               // Ganancia proporcional
static constexpr float KI = 0.0f;               // Ganancia integral (futuro)
static constexpr float DEADBAND_DEG = 1.0f;     // Banda muerta para evitar oscilación
static constexpr float MAX_CURRENT_A = 15.0f;   // Corriente máxima del motor (protección)
static constexpr float MAX_PWM_PCT = 100.0f;    // PWM máximo

static uint16_t pctToTicks(float pct) {
    pct = constrain(pct, 0.0f, MAX_PWM_PCT);
    // PCA9685 usa 12-bit (0..4095). Mapear 0..100% → 0..4095 duty
    return (uint16_t)(pct * 40.95f);
}

void SteeringMotor::init() {
    // 🔒 Verificar que I2C está inicializado
    // Si Wire no fue iniciado, intentar inicializarlo con pines por defecto
    // Esto es un guard para casos donde se llama init() antes que otros módulos
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    
    if (!pca.begin()) {
        Logger::errorf("SteeringMotor: Fallo inicializar PCA9685 en 0x%02X", I2C_ADDR_PCA9685_STEERING);
        System::logError(700);
        initialized = false;
        return;
    }
    
    pca.setPWMFreq(kFreqHz);
    
    // 🔒 Asegurar motor parado al inicializar
    pca.setPWM(kChannelFwd, 0, 0);
    pca.setPWM(kChannelRev, 0, 0);

    s = {0.0f, 0.0f, 0.0f};
    initialized = true;
    Logger::infof("SteeringMotor init OK (PCA9685 @ 0x%02X, freq %dHz)", 
                  I2C_ADDR_PCA9685_STEERING, kFreqHz);
}

void SteeringMotor::setDemandAngle(float deg) {
    // 🔒 Clamp de seguridad
    s.demandDeg = constrain(deg, -60.0f, 60.0f);
}

void SteeringMotor::update() {
    if (!initialized) {
        Logger::warn("SteeringMotor update llamado sin init");
        return;
    }

    // Leer corriente actual del motor de dirección (canal INA226 = 5)
    s.currentA = Sensors::getCurrent(5);
    
    // 🔒 Protección por sobrecorriente (usar fabsf para mejor rendimiento)
    if (fabsf(s.currentA) > MAX_CURRENT_A) {
        Logger::errorf("SteeringMotor: Sobrecorriente %.1fA (límite %.1fA)", s.currentA, MAX_CURRENT_A);
        System::logError(701);
        // Parar motor inmediatamente
        pca.setPWM(kChannelFwd, 0, 0);
        pca.setPWM(kChannelRev, 0, 0);
        s.pwmOut = 0.0f;
        return;
    }

    // Obtener ángulo actual desde encoder de dirección
    float target = s.demandDeg;
    float actual = Steering::get().angleDeg;
    float error = target - actual;
    
    // 🔒 Banda muerta para evitar oscilación cerca del objetivo (usar fabsf)
    if (fabsf(error) < DEADBAND_DEG) {
        // Dentro de banda muerta - parar motor
        pca.setPWM(kChannelFwd, 0, 0);
        pca.setPWM(kChannelRev, 0, 0);
        s.pwmOut = 0.0f;
        return;
    }

    // Control proporcional (PID básico, usar fabsf)
    float cmdPct = constrain(fabsf(error) * KP, 0.0f, MAX_PWM_PCT);
    uint16_t ticks = pctToTicks(cmdPct);

    // 🔒 Control de dirección bidireccional vía canales FWD/REV
    if (error > 0) {
        // Girar en dirección positiva (CW)
        pca.setPWM(kChannelFwd, 0, ticks);
        pca.setPWM(kChannelRev, 0, 0);
    } else {
        // Girar en dirección negativa (CCW)
        pca.setPWM(kChannelFwd, 0, 0);
        pca.setPWM(kChannelRev, 0, ticks);
    }
    
    s.pwmOut = cmdPct;
}

// 🔒 Nuevo: Parada de emergencia
void SteeringMotor::emergencyStop() {
    if (!initialized) return;
    
    pca.setPWM(kChannelFwd, 0, 0);
    pca.setPWM(kChannelRev, 0, 0);
    s.pwmOut = 0.0f;
    Logger::warn("SteeringMotor: EMERGENCY STOP");
}

// 🔒 Nuevo: Obtener estado de inicialización
bool SteeringMotor::initOK() {
    return initialized;
}

// 🔒 Nuevo: Obtener estado completo
const SteeringMotor::State& SteeringMotor::get() {
    return s;
}