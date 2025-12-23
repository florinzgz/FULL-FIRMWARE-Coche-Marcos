#include "traction.h"
#include "logger.h"
#include "config.h"
#include "driver/motor_driver.h"
#include "wire_manager.h"
#include "obstacle_safety.h"
#include "adaptive_cruise.h"

static const char* TAG = "Traction";

namespace Traction {

struct WheelState {
  float demandPct;
  float outPWM;
};

struct State {
  float demandPct;
  WheelState w[4];
};

static State s;

// External I²C manager
static WireManager* wireManager = nullptr;

// External motor driver manager
static MotorDriverManager* motorDriverManager = nullptr;

// MCP23017 manager for motor control
static MCP23017Manager* mcpManager = nullptr;

// PCA9685 instances
static Adafruit_PCA9685 pcaFront;
static Adafruit_PCA9685 pcaRear;
static bool pcaFrontOK = false;
static bool pcaRearOK = false;

// MCP23017 pin definitions (motor control pins)
#define MCP_PIN_FL_IN1  0
#define MCP_PIN_FL_IN2  1
#define MCP_PIN_FR_IN1  2
#define MCP_PIN_FR_IN2  3
#define MCP_PIN_RL_IN1  4
#define MCP_PIN_RL_IN2  5
#define MCP_PIN_RR_IN1  6
#define MCP_PIN_RR_IN2  7

void setWireManager(WireManager* wm) {
  wireManager = wm;
  Logger::info("Traction: WireManager assigned");
}

void setMotorDriverManager(MotorDriverManager* mdm) {
  motorDriverManager = mdm;
  Logger::info("Traction: MotorDriverManager assigned");
}

void setMCP23017Manager(MCP23017Manager* mcp) {
  mcpManager = mcp;
  Logger::info("Traction: MCP23017Manager assigned for motor control");
}

void init() {
  Logger::info("Traction: Initializing...");
  
  s.demandPct = 0.0f;
  for (int i = 0; i < 4; i++) {
    s.w[i].demandPct = 0.0f;
    s.w[i].outPWM = 0.0f;
  }

  // Initialize PCA9685 devices for PWM control
  if (wireManager && wireManager->isI2CAvailable()) {
    TwoWire* wire = wireManager->getWire();
    
    // Front PCA9685 (0x40)
    pcaFront.begin(0x40, wire);
    pcaFront.setPWMFreq(1600);
    pcaFrontOK = true;
    Logger::info("Traction: Front PCA9685 (0x40) initialized");
    
    // Rear PCA9685 (0x41)
    pcaRear.begin(0x41, wire);
    pcaRear.setPWMFreq(1600);
    pcaRearOK = true;
    Logger::info("Traction: Rear PCA9685 (0x41) initialized");
  } else {
    Logger::error("Traction: I2C not available - PCA9685 init skipped");
  }

  // Configure MCP23017 pins for motor control
  if (mcpManager && mcpManager->isOK()) {
    for (int pin = MCP_PIN_FL_IN1; pin <= MCP_PIN_RR_IN2; pin++) {
      mcpManager->pinMode(pin, OUTPUT);
      mcpManager->digitalWrite(pin, LOW);
    }
    Logger::info("Traction: MCP23017 motor control pins configured");
  } else {
    Logger::error("Traction: MCP23017Manager not available");
  }

  Logger::info("Traction: Initialized");
}

void setDemand(float pct) {
  s.demandPct = constrain(pct, -100.0f, 100.0f);
}

float getDemand() {
  return s.demandPct;
}

static void setMotorDirection(int motorIndex, bool forward) {
  if (!mcpManager || !mcpManager->isOK()) return;

  int in1Pin = motorIndex * 2;
  int in2Pin = motorIndex * 2 + 1;

  if (forward) {
    mcpManager->digitalWrite(in1Pin, HIGH);
    mcpManager->digitalWrite(in2Pin, LOW);
  } else {
    mcpManager->digitalWrite(in1Pin, LOW);
    mcpManager->digitalWrite(in2Pin, HIGH);
  }
}

static void setMotorPWM(int motorIndex, uint16_t pwm) {
  // Motor 0,1 -> Front PCA (ch 0,1)
  // Motor 2,3 -> Rear PCA (ch 0,1)
  
  if (motorIndex == 0) {
    if (pcaFrontOK) pcaFront.setPWM(0, 0, pwm);
  } else if (motorIndex == 1) {
    if (pcaFrontOK) pcaFront.setPWM(1, 0, pwm);
  } else if (motorIndex == 2) {
    if (pcaRearOK) pcaRear.setPWM(0, 0, pwm);
  } else if (motorIndex == 3) {
    if (pcaRearOK) pcaRear.setPWM(1, 0, pwm);
  }
}

void update() {
  const float base = s.demandPct;

  // 🔒 v2.12.0 CRITICAL SAFETY: Emergency stop for collision <20cm
  if (ObstacleSafety::isCollisionImminent()) {
    for (int i = 0; i < 4; ++i) {
      s.w[i].demandPct = 0.0f;
      s.w[i].outPWM = 0.0f;
    }
    
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
    if (mcpManager && mcpManager->isOK()) {
      for (int pin = MCP_PIN_FL_IN1; pin <= MCP_PIN_RR_IN2; pin++) {
        mcpManager->digitalWrite(pin, LOW);
      }
    }
    
    Logger::error("Traction: EMERGENCY STOP <20cm - Collision imminent!");
    return;
  }

  // Get current gear multiplier
  float gearMultiplier = 1.0f;
  if (motorDriverManager) {
    int gear = motorDriverManager->getCurrentGear();
    if (gear == 1) gearMultiplier = 0.33f;
    else if (gear == 2) gearMultiplier = 0.66f;
    else gearMultiplier = 1.0f;
  }

  // 🔒 v2.12.0 PROGRESSIVE SAFETY: Apply intelligent obstacle reduction
  float obstacleFactor = ObstacleSafety::getSpeedReductionFactor();
  
  // 🔒 v2.12.0 ACC INTEGRATION: Apply adaptive cruise control adjustment
  float accFactor = 1.0f;
  auto accStatus = AdaptiveCruise::getStatus();
  if (accStatus.state == AdaptiveCruise::ACC_ACTIVE) {
    accFactor = AdaptiveCruise::getSpeedAdjustment();
    Logger::infof("Traction: ACC active - adjustment %.2f", accFactor);
  }
  
  // Apply QUADRUPLE factor: base × gear × obstacle × ACC
  const float adjustedBase = base * gearMultiplier * obstacleFactor * accFactor;
  
  // Log interventions (only when active)
  if (obstacleFactor < 1.0f || accFactor < 1.0f) {
    Logger::debugf("Traction: gear=%.2f, obstacle=%.2f, ACC=%.2f, final=%.1f%%", 
                   gearMultiplier, obstacleFactor, accFactor, adjustedBase);
  }

  // Apply to all wheels
  for (int i = 0; i < 4; i++) {
    s.w[i].demandPct = adjustedBase;
    
    // Convert to PWM (0-4095 for PCA9685)
    float absPct = fabs(s.w[i].demandPct);
    s.w[i].outPWM = (absPct / 100.0f) * 4095.0f;
    
    // Set direction
    bool forward = (s.w[i].demandPct >= 0);
    setMotorDirection(i, forward);
    
    // Set PWM
    uint16_t pwm = (uint16_t)s.w[i].outPWM;
    setMotorPWM(i, pwm);
  }
}

void stop() {
  s.demandPct = 0.0f;
  
  for (int i = 0; i < 4; i++) {
    s.w[i].demandPct = 0.0f;
    s.w[i].outPWM = 0.0f;
    setMotorPWM(i, 0);
    
    if (mcpManager && mcpManager->isOK()) {
      int in1Pin = i * 2;
      int in2Pin = i * 2 + 1;
      mcpManager->digitalWrite(in1Pin, LOW);
      mcpManager->digitalWrite(in2Pin, LOW);
    }
  }
  
  Logger::info("Traction: Stopped");
}

} // namespace Traction