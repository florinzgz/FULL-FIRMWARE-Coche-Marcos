#include "traction.h"
#include "../config.h"
#include "../hardware/motor.h"
#include "../hardware/encoders.h"
#include "../hardware/mcp23017_manager.h"
#include <Arduino.h>

// Variables globales
volatile float leftSpeed = 0;
volatile float rightSpeed = 0;
volatile float targetLeftSpeed = 0;
volatile float targetRightSpeed = 0;

// PID Variables
float kp = 2.0, ki = 0.5, kd = 0.1;
float leftError = 0, rightError = 0;
float leftIntegral = 0, rightIntegral = 0;
float leftLastError = 0, rightLastError = 0;

// Límites
const float MAX_INTEGRAL = 100.0;
const float MAX_SPEED = 255.0;

// Timing
unsigned long lastPIDUpdate = 0;
const unsigned long PID_INTERVAL = 20; // 20ms = 50Hz

// Estado de tracción
bool tractionEnabled = false;

// Funciones auxiliares
float constrainFloat(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void initTraction() {
    // Inicialización de motores
    initMotors();
    
    // Inicialización de encoders
    initEncoders();
    
    // Reset de variables
    leftSpeed = 0;
    rightSpeed = 0;
    targetLeftSpeed = 0;
    targetRightSpeed = 0;
    leftIntegral = 0;
    rightIntegral = 0;
    leftError = 0;
    rightError = 0;
    leftLastError = 0;
    rightLastError = 0;
    
    tractionEnabled = false;
    
    Serial.println("Traction system initialized");
}

void enableTraction(bool enable) {
    tractionEnabled = enable;
    if (!enable) {
        stopMotors();
        leftIntegral = 0;
        rightIntegral = 0;
        leftError = 0;
        rightError = 0;
    }
    Serial.print("Traction ");
    Serial.println(enable ? "enabled" : "disabled");
}

bool isTractionEnabled() {
    return tractionEnabled;
}

void updateSpeeds() {
    // Leer velocidades actuales de los encoders
    leftSpeed = getLeftSpeed();
    rightSpeed = getRightSpeed();
}

void updatePID() {
    if (!tractionEnabled) return;
    
    unsigned long currentTime = millis();
    if (currentTime - lastPIDUpdate < PID_INTERVAL) {
        return;
    }
    
    float dt = (currentTime - lastPIDUpdate) / 1000.0; // Convertir a segundos
    lastPIDUpdate = currentTime;
    
    // Actualizar velocidades desde encoders
    updateSpeeds();
    
    // Calcular errores
    leftError = targetLeftSpeed - leftSpeed;
    rightError = targetRightSpeed - rightSpeed;
    
    // Actualizar integrales
    leftIntegral += leftError * dt;
    rightIntegral += rightError * dt;
    
    // Anti-windup: limitar integrales
    leftIntegral = constrainFloat(leftIntegral, -MAX_INTEGRAL, MAX_INTEGRAL);
    rightIntegral = constrainFloat(rightIntegral, -MAX_INTEGRAL, MAX_INTEGRAL);
    
    // Calcular derivadas
    float leftDerivative = (leftError - leftLastError) / dt;
    float rightDerivative = (rightError - rightLastError) / dt;
    
    // Calcular salidas PID
    float leftOutput = kp * leftError + ki * leftIntegral + kd * leftDerivative;
    float rightOutput = kp * rightError + ki * rightIntegral + kd * rightDerivative;
    
    // Limitar salidas
    leftOutput = constrainFloat(leftOutput, -MAX_SPEED, MAX_SPEED);
    rightOutput = constrainFloat(rightOutput, -MAX_SPEED, MAX_SPEED);
    
    // Aplicar a motores
    setMotorSpeed(MOTOR_LEFT, (int)leftOutput);
    setMotorSpeed(MOTOR_RIGHT, (int)rightOutput);
    
    // Guardar errores para siguiente iteración
    leftLastError = leftError;
    rightLastError = rightError;
}

void setTargetSpeed(float left, float right) {
    targetLeftSpeed = constrainFloat(left, -MAX_SPEED, MAX_SPEED);
    targetRightSpeed = constrainFloat(right, -MAX_SPEED, MAX_SPEED);
}

void setTargetLinearSpeed(float speed) {
    setTargetSpeed(speed, speed);
}

void setTargetAngularSpeed(float speed) {
    // Velocidad angular: un motor hacia adelante, otro hacia atrás
    setTargetSpeed(speed, -speed);
}

float getLeftSpeed() {
    return leftSpeed;
}

float getRightSpeed() {
    return rightSpeed;
}

float getTargetLeftSpeed() {
    return targetLeftSpeed;
}

float getTargetRightSpeed() {
    return targetRightSpeed;
}

void setPIDParameters(float p, float i, float d) {
    kp = p;
    ki = i;
    kd = d;
    Serial.print("PID parameters updated: P=");
    Serial.print(kp);
    Serial.print(", I=");
    Serial.print(ki);
    Serial.print(", D=");
    Serial.println(kd);
}

void getPIDParameters(float* p, float* i, float* d) {
    *p = kp;
    *i = ki;
    *d = kd;
}

void stopMotors() {
    setMotorSpeed(MOTOR_LEFT, 0);
    setMotorSpeed(MOTOR_RIGHT, 0);
    targetLeftSpeed = 0;
    targetRightSpeed = 0;
    leftIntegral = 0;
    rightIntegral = 0;
}

void emergencyStop() {
    enableTraction(false);
    stopMotors();
    Serial.println("EMERGENCY STOP!");
}

// Movimientos básicos con velocidad configurable
void moveForward(float speed) {
    if (!tractionEnabled) {
        Serial.println("Traction not enabled!");
        return;
    }
    setTargetSpeed(speed, speed);
    Serial.print("Moving forward at speed: ");
    Serial.println(speed);
}

void moveBackward(float speed) {
    if (!tractionEnabled) {
        Serial.println("Traction not enabled!");
        return;
    }
    setTargetSpeed(-speed, -speed);
    Serial.print("Moving backward at speed: ");
    Serial.println(speed);
}

void turnLeft(float speed) {
    if (!tractionEnabled) {
        Serial.println("Traction not enabled!");
        return;
    }
    setTargetSpeed(-speed, speed);
    Serial.print("Turning left at speed: ");
    Serial.println(speed);
}

void turnRight(float speed) {
    if (!tractionEnabled) {
        Serial.println("Traction not enabled!");
        return;
    }
    setTargetSpeed(speed, -speed);
    Serial.print("Turning right at speed: ");
    Serial.println(speed);
}

void stop() {
    setTargetSpeed(0, 0);
    Serial.println("Stopping");
}

// Función de diagnóstico
void printTractionStatus() {
    Serial.println("\n=== TRACTION STATUS ===");
    Serial.print("Enabled: ");
    Serial.println(tractionEnabled ? "YES" : "NO");
    Serial.print("Left - Target: ");
    Serial.print(targetLeftSpeed);
    Serial.print(", Current: ");
    Serial.print(leftSpeed);
    Serial.print(", Error: ");
    Serial.println(leftError);
    Serial.print("Right - Target: ");
    Serial.print(targetRightSpeed);
    Serial.print(", Current: ");
    Serial.print(rightSpeed);
    Serial.print(", Error: ");
    Serial.println(rightError);
    Serial.print("PID - P: ");
    Serial.print(kp);
    Serial.print(", I: ");
    Serial.print(ki);
    Serial.print(", D: ");
    Serial.println(kd);
    Serial.println("=====================\n");
}

// Nueva función para control de dirección del eje de rotación
void setAxisRotationDirection(AxisRotationDirection direction) {
    extern MCP23017Manager* mcpManager;
    
    if (mcpManager && mcpManager->isOK()) {
        switch(direction) {
            case AXIS_ROTATION_CW:
                mcpManager->digitalWrite(MCP_AXIS_DIR_PIN, LOW);
                Serial.println("Axis rotation: Clockwise");
                break;
            case AXIS_ROTATION_CCW:
                mcpManager->digitalWrite(MCP_AXIS_DIR_PIN, HIGH);
                Serial.println("Axis rotation: Counter-clockwise");
                break;
            case AXIS_ROTATION_STOP:
                // Mantener la última dirección pero detener el motor
                Serial.println("Axis rotation: Stop");
                break;
        }
    } else {
        Serial.println("Error: MCP23017 not available for axis rotation control");
    }
}

AxisRotationDirection getAxisRotationDirection() {
    extern MCP23017Manager* mcpManager;
    
    if (mcpManager && mcpManager->isOK()) {
        bool dirState = mcpManager->digitalRead(MCP_AXIS_DIR_PIN);
        return dirState ? AXIS_ROTATION_CCW : AXIS_ROTATION_CW;
    }
    return AXIS_ROTATION_STOP;
}