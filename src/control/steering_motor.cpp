// Steering Motor Control Implementation
#include "control/steering_motor.h"
#include <Arduino.h>

// Constructor
SteeringMotor::SteeringMotor(uint8_t pwmChannel, uint8_t in1Pin, uint8_t in2Pin, 
                             uint8_t encoderPinA, uint8_t encoderPinB,
                             float gearRatio, uint16_t ppr)
    : pwmChannel(pwmChannel), in1Pin(in1Pin), in2Pin(in2Pin),
      encoderPinA(encoderPinA), encoderPinB(encoderPinB),
      gearRatio(gearRatio), ppr(ppr),
      currentAngle(0), targetAngle(0), centerAngle(0),
      minAngle(-45), maxAngle(45),
      kp(2.0), ki(0.1), kd(0.5),
      integral(0), lastError(0), lastUpdateTime(0),
      initialized(false), pcaOK(false), mcpManager(nullptr) {}

// Initialize the steering motor
bool SteeringMotor::begin(Adafruit_PWMServoDriver* pca, MCP23017Manager* mcp) {
    if (!pca || !mcp) {
        Serial.println("Error: PCA9685 or MCP23017Manager pointer is null");
        return false;
    }
    
    this->pca = pca;
    this->mcpManager = mcp;
    
    // Initialize PCA9685 PWM
    pcaOK = true; // Assume PCA is initialized externally
    
    // Configure MCP23017 pins for motor control
    if (!mcpManager->pinMode(in1Pin, OUTPUT) || !mcpManager->pinMode(in2Pin, OUTPUT)) {
        Serial.println("Error: Failed to configure MCP23017 pins for steering motor");
        return false;
    }
    
    // Initialize encoder
    pinMode(encoderPinA, INPUT_PULLUP);
    pinMode(encoderPinB, INPUT_PULLUP);
    
    // Stop motor initially
    stop();
    
    initialized = true;
    lastUpdateTime = millis();
    
    Serial.println("Steering motor initialized successfully");
    return true;
}

// Set the target steering angle
void SteeringMotor::setAngle(float angle) {
    targetAngle = constrain(angle, minAngle, maxAngle);
}

// Get current steering angle
float SteeringMotor::getAngle() {
    return currentAngle;
}

// Set angle limits
void SteeringMotor::setLimits(float minAngle, float maxAngle) {
    this->minAngle = minAngle;
    this->maxAngle = maxAngle;
}

// Calibrate center position
void SteeringMotor::calibrateCenter() {
    centerAngle = currentAngle;
    Serial.print("Steering center calibrated at: ");
    Serial.println(centerAngle);
}

// Set PID parameters
void SteeringMotor::setPID(float kp, float ki, float kd) {
    this->kp = kp;
    this->ki = ki;
    this->kd = kd;
}

// Update steering control (PID loop)
void SteeringMotor::update() {
    if (!initialized || !mcpManager || !mcpManager->isOK()) {
        return;
    }
    
    unsigned long currentTime = millis();
    float dt = (currentTime - lastUpdateTime) / 1000.0; // Convert to seconds
    
    if (dt < 0.01) return; // Update at most every 10ms
    
    // Calculate error
    float error = targetAngle - currentAngle;
    
    // PID calculations
    integral += error * dt;
    integral = constrain(integral, -100, 100); // Anti-windup
    
    float derivative = (error - lastError) / dt;
    
    float output = kp * error + ki * integral + kd * derivative;
    
    // Apply output to motor
    if (abs(error) < 0.5) {
        stop();
    } else if (output > 0) {
        setSpeed(constrain(abs(output), 0, 100));
        setDirection(CLOCKWISE);
    } else {
        setSpeed(constrain(abs(output), 0, 100));
        setDirection(COUNTERCLOCKWISE);
    }
    
    // Update for next iteration
    lastError = error;
    lastUpdateTime = currentTime;
}

// Stop the steering motor
void SteeringMotor::stop() {
    if (!initialized || !mcpManager) return;
    
    // Set PWM to 0
    if (pca && pcaOK) {
        pca->setPWM(pwmChannel, 0, 0);
    }
    
    // Set both direction pins LOW
    mcpManager->digitalWrite(in1Pin, LOW);
    mcpManager->digitalWrite(in2Pin, LOW);
}

// Set motor direction
void SteeringMotor::setDirection(MotorDirection dir) {
    if (!initialized || !mcpManager) return;
    
    switch (dir) {
        case CLOCKWISE:
            mcpManager->digitalWrite(in1Pin, HIGH);
            mcpManager->digitalWrite(in2Pin, LOW);
            break;
        case COUNTERCLOCKWISE:
            mcpManager->digitalWrite(in1Pin, LOW);
            mcpManager->digitalWrite(in2Pin, HIGH);
            break;
        case BRAKE:
            mcpManager->digitalWrite(in1Pin, HIGH);
            mcpManager->digitalWrite(in2Pin, HIGH);
            break;
    }
}

// Set motor speed (0-100%)
void SteeringMotor::setSpeed(float speed) {
    if (!initialized || !pca || !pcaOK) return;
    
    speed = constrain(speed, 0, 100);
    uint16_t pwmValue = map(speed, 0, 100, 0, 4095);
    pca->setPWM(pwmChannel, 0, pwmValue);
}

// Check if initialization was successful
bool SteeringMotor::initOK() {
    return initialized && pcaOK && mcpManager && mcpManager->isOK();
}
