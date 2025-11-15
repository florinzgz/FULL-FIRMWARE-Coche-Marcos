#include "steering_motor.h"
#include "pins.h"
#include "current.h"
#include "steering.h"
#include "logger.h"
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

static Adafruit_PWMServoDriver pca = Adafruit_PWMServoDriver(0x41); // I2C PCA9685 en 0x41 (evitar conflicto con INA226 0x40)
static SteeringMotor::State s;

static const uint16_t kFreqHz = 1000;  // PWM estable para BTS7960
static const uint8_t  kChannelPWM = 0; // canal PCA para dirección

static uint16_t pctToTicks(float pct) {
    pct = constrain(pct, 0.0f, 100.0f);
    // PCA9685 usa 12-bit (0..4095). Mapear 0..100% → 0..4095 duty
    return (uint16_t)(pct * 40.95f);
}

void SteeringMotor::init() {
    Wire.begin(); // usa PIN 21/22 ya conectados
    pca.begin();
    pca.setPWMFreq(kFreqHz);

    s = {0, 0, 0};
    Logger::info("SteeringMotor init");
}

void SteeringMotor::setDemandAngle(float deg) {
    s.demandDeg = deg;
}

void SteeringMotor::update() {
    // Control sencillo: seguir el ángulo de mando (puede venir de alg. superior)
    float target = s.demandDeg;
    float actual = Steering::get().angleDeg;
    float error = target - actual;

    // PID muy básico (proporcional)
    float kp = 1.2f;
    float cmdPct = constrain(fabs(error) * kp, 0.0f, 100.0f);

    // Sentido: según signo del error (aquí solo PWM, el puente BTS define sentido)
    // Si necesitas IN1/IN2, usa canales extra del PCA o pines GPIO.
    uint16_t ticks = pctToTicks(cmdPct);
    pca.setPWM(kChannelPWM, 0, ticks);
    s.pwmOut = cmdPct;

    // Corriente de dirección (canal INA226 = 5)
    s.currentA = Sensors::getCurrent(5);
}