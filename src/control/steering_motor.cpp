// steering_motor.cpp - Control del motor de dirección
#include "steering_motor.h"
#include "hw_config.h"
#include <Arduino.h>

// Variables globales del módulo
static bool initialized = false;
static Adafruit_PWMServoDriver* pca = nullptr;
static MCP23017Manager* mcpManager = nullptr;
static bool pcaOK = false;

// Configuración de pines MCP23017
static const uint8_t DIR_ENABLE_PIN = MCP_PIN_DIR_ENABLE;
static const uint8_t DIR_IN1_PIN = MCP_PIN_DIR_IN1;
static const uint8_t DIR_IN2_PIN = MCP_PIN_DIR_IN2;

// Límites de PWM para el PCA9685
static const uint16_t PWM_MIN = 0;
static const uint16_t PWM_MAX = 4095;

// Variables de estado
static int16_t currentSpeed = 0;
static SteeringDirection currentDirection = STEERING_STOP;

// Prototipos de funciones privadas
static void setDirection(SteeringDirection dir);
static void setPWM(uint16_t value);

bool SteeringMotor_Init(Adafruit_PWMServoDriver* pcaPtr, MCP23017Manager* mcpMgr) {
  if (initialized) {
    Serial.println("[STEER] Ya inicializado");
    return true;
  }

  Serial.println("[STEER] Inicializando motor de dirección...");

  // Guardar referencias
  pca = pcaPtr;
  mcpManager = mcpMgr;

  // Verificar PCA9685
  if (pca == nullptr) {
    Serial.println("[STEER] ERROR: PCA9685 no disponible");
    pcaOK = false;
  } else {
    pcaOK = true;
    Serial.println("[STEER] PCA9685 OK");
  }

  // Verificar MCP23017
  if (mcpManager == nullptr || !mcpManager->isOK()) {
    Serial.println("[STEER] ERROR: MCP23017 no disponible");
    initialized = false;
    return false;
  }

  Serial.println("[STEER] MCP23017 OK");

  // Configurar pines del MCP23017 como salidas
  if (!mcpManager->pinMode(DIR_ENABLE_PIN, OUTPUT)) {
    Serial.println("[STEER] ERROR: No se pudo configurar DIR_ENABLE_PIN");
    return false;
  }
  if (!mcpManager->pinMode(DIR_IN1_PIN, OUTPUT)) {
    Serial.println("[STEER] ERROR: No se pudo configurar DIR_IN1_PIN");
    return false;
  }
  if (!mcpManager->pinMode(DIR_IN2_PIN, OUTPUT)) {
    Serial.println("[STEER] ERROR: No se pudo configurar DIR_IN2_PIN");
    return false;
  }

  // Estado inicial: motor detenido
  mcpManager->digitalWrite(DIR_ENABLE_PIN, HIGH); // Habilitar driver
  setDirection(STEERING_STOP);
  setPWM(0);

  currentSpeed = 0;
  currentDirection = STEERING_STOP;

  initialized = true;
  Serial.println("[STEER] Motor de dirección inicializado correctamente");
  return true;
}

void SteeringMotor_SetSpeed(int16_t speed) {
  if (!initialized) {
    Serial.println("[STEER] ERROR: No inicializado");
    return;
  }

  // Limitar velocidad entre -255 y 255
  if (speed > 255) speed = 255;
  if (speed < -255) speed = -255;

  // Determinar dirección
  SteeringDirection newDirection;
  uint16_t absSpeed;

  if (speed > 0) {
    newDirection = STEERING_RIGHT;
    absSpeed = speed;
  } else if (speed < 0) {
    newDirection = STEERING_LEFT;
    absSpeed = -speed;
  } else {
    newDirection = STEERING_STOP;
    absSpeed = 0;
  }

  // Actualizar dirección si ha cambiado
  if (newDirection != currentDirection) {
    setDirection(newDirection);
    currentDirection = newDirection;
  }

  // Convertir velocidad (0-255) a PWM (0-4095)
  uint16_t pwmValue = map(absSpeed, 0, 255, PWM_MIN, PWM_MAX);
  setPWM(pwmValue);

  currentSpeed = speed;

  Serial.printf("[STEER] Velocidad: %d (PWM: %d, Dir: %s)\n", 
                speed, pwmValue, 
                newDirection == STEERING_RIGHT ? "DERECHA" : 
                newDirection == STEERING_LEFT ? "IZQUIERDA" : "STOP");
}

void SteeringMotor_Stop() {
  if (!initialized) {
    return;
  }

  setDirection(STEERING_STOP);
  setPWM(0);
  currentSpeed = 0;
  currentDirection = STEERING_STOP;

  Serial.println("[STEER] Motor detenido");
}

int16_t SteeringMotor_GetSpeed() {
  return currentSpeed;
}

SteeringDirection SteeringMotor_GetDirection() {
  return currentDirection;
}

bool SteeringMotor_IsInitialized() {
  return initialized && pcaOK && mcpManager && mcpManager->isOK();
}

// Funciones privadas

static void setDirection(SteeringDirection dir) {
  if (!mcpManager || !mcpManager->isOK()) {
    return;
  }

  switch (dir) {
    case STEERING_RIGHT:
      mcpManager->digitalWrite(DIR_IN1_PIN, HIGH);
      mcpManager->digitalWrite(DIR_IN2_PIN, LOW);
      break;
    case STEERING_LEFT:
      mcpManager->digitalWrite(DIR_IN1_PIN, LOW);
      mcpManager->digitalWrite(DIR_IN2_PIN, HIGH);
      break;
    case STEERING_STOP:
    default:
      mcpManager->digitalWrite(DIR_IN1_PIN, LOW);
      mcpManager->digitalWrite(DIR_IN2_PIN, LOW);
      break;
  }
}

static void setPWM(uint16_t value) {
  if (!pcaOK || pca == nullptr) {
    return;
  }

  // Limitar valor PWM
  if (value > PWM_MAX) value = PWM_MAX;

  // Establecer PWM en el canal del motor de dirección
  pca->setPWM(PCA_CHANNEL_DIR_PWM, 0, value);
}
