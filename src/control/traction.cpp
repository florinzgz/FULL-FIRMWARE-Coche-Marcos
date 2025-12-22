#include "traction.h"
#include <Arduino.h>

// Pin definitions
#define MOTOR_PWM_PIN 9
#define MOTOR_DIR_PIN 8
#define BRAKE_PIN 7

// Power limiting constants
#define POWER_LIMIT_D1 0.70  // 70% power in D1
#define POWER_LIMIT_D2 1.00  // 100% power in D2
#define POWER_LIMIT_REVERSE 0.80  // 80% power in reverse

// Ramping constants
#define POWER_RAMP_STEP 0.02  // 2% change per update
#define MIN_SPEED_FOR_PARK 0.5  // km/h - minimum speed to allow Park

// Global variables
static GearPosition currentGear = GEAR_PARK;
static float currentPower = 0.0;
static float targetPower = 0.0;
static float currentSpeed = 0.0;
static unsigned long lastRampUpdate = 0;
static const unsigned long RAMP_UPDATE_INTERVAL = 20; // ms

void Traction_Init() {
    pinMode(MOTOR_PWM_PIN, OUTPUT);
    pinMode(MOTOR_DIR_PIN, OUTPUT);
    pinMode(BRAKE_PIN, OUTPUT);
    
    // Initialize in safe state
    digitalWrite(MOTOR_DIR_PIN, LOW);
    analogWrite(MOTOR_PWM_PIN, 0);
    digitalWrite(BRAKE_PIN, HIGH); // Brake engaged
    
    currentPower = 0.0;
    targetPower = 0.0;
    currentGear = GEAR_PARK;
}

bool Traction_SetGear(GearPosition gear) {
    // Safety check: Don't allow gear change from Park if vehicle is moving
    if (currentGear == GEAR_PARK && currentSpeed > MIN_SPEED_FOR_PARK) {
        return false; // Reject gear change
    }
    
    // Safety check: Must go through Neutral when changing between Drive and Reverse
    if ((currentGear == GEAR_DRIVE_1 || currentGear == GEAR_DRIVE_2) && gear == GEAR_REVERSE) {
        return false; // Must shift to Neutral first
    }
    if (currentGear == GEAR_REVERSE && (gear == GEAR_DRIVE_1 || gear == GEAR_DRIVE_2)) {
        return false; // Must shift to Neutral first
    }
    
    // Engage brake during gear change
    digitalWrite(BRAKE_PIN, HIGH);
    
    // Apply gear change
    currentGear = gear;
    
    // Reset power when changing gears
    targetPower = 0.0;
    
    // Set appropriate direction
    if (gear == GEAR_REVERSE) {
        digitalWrite(MOTOR_DIR_PIN, HIGH); // Reverse direction
    } else {
        digitalWrite(MOTOR_DIR_PIN, LOW); // Forward direction
    }
    
    // Release brake if not in Park
    if (gear != GEAR_PARK) {
        digitalWrite(BRAKE_PIN, LOW);
    }
    
    return true;
}

GearPosition Traction_GetGear() {
    return currentGear;
}

void Traction_SetThrottle(float throttle) {
    // Clamp throttle to valid range
    if (throttle < 0.0) throttle = 0.0;
    if (throttle > 1.0) throttle = 1.0;
    
    // Apply gear-based power limiting
    float powerLimit = 1.0;
    
    switch (currentGear) {
        case GEAR_PARK:
        case GEAR_NEUTRAL:
            powerLimit = 0.0;
            break;
            
        case GEAR_DRIVE_1:
            powerLimit = POWER_LIMIT_D1;
            break;
            
        case GEAR_DRIVE_2:
            powerLimit = POWER_LIMIT_D2;
            break;
            
        case GEAR_REVERSE:
            powerLimit = POWER_LIMIT_REVERSE;
            break;
    }
    
    // Set target power based on throttle and gear limit
    targetPower = throttle * powerLimit;
}

void Traction_Update(float speed) {
    currentSpeed = speed;
    unsigned long currentTime = millis();
    
    // Check if it's time to update ramp
    if (currentTime - lastRampUpdate < RAMP_UPDATE_INTERVAL) {
        return;
    }
    lastRampUpdate = currentTime;
    
    // Smooth power ramping
    if (currentPower < targetPower) {
        // Accelerating
        currentPower += POWER_RAMP_STEP;
        if (currentPower > targetPower) {
            currentPower = targetPower;
        }
    } else if (currentPower > targetPower) {
        // Decelerating
        currentPower -= POWER_RAMP_STEP;
        if (currentPower < targetPower) {
            currentPower = targetPower;
        }
    }
    
    // Apply power to motor
    int pwmValue = (int)(currentPower * 255.0);
    
    // Safety: Ensure Park and Neutral have no power
    if (currentGear == GEAR_PARK || currentGear == GEAR_NEUTRAL) {
        pwmValue = 0;
        currentPower = 0.0;
        targetPower = 0.0;
        digitalWrite(BRAKE_PIN, HIGH);
    } else {
        digitalWrite(BRAKE_PIN, LOW);
    }
    
    analogWrite(MOTOR_PWM_PIN, pwmValue);
}

float Traction_GetCurrentPower() {
    return currentPower;
}

void Traction_EmergencyStop() {
    targetPower = 0.0;
    currentPower = 0.0;
    analogWrite(MOTOR_PWM_PIN, 0);
    digitalWrite(BRAKE_PIN, HIGH);
    currentGear = GEAR_PARK;
}
