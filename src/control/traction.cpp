#include "traction.h"
#include <Arduino.h>
#include "hardware/pwm_manager.h"
#include "hardware/mcp23017_manager.h"

// Constructor
Traction::Traction() : 
    initialized(false),
    currentSpeed(0),
    targetSpeed(0),
    currentDirection(STOP),
    targetDirection(STOP),
    acceleration(DEFAULT_ACCELERATION),
    deceleration(DEFAULT_DECELERATION),
    maxSpeed(DEFAULT_MAX_SPEED),
    minSpeed(DEFAULT_MIN_SPEED),
    lastUpdateTime(0),
    emergencyStopActive(false),
    motorBrakeActive(false),
    axisRotationEnabled(false),
    currentAxisAngle(0),
    targetAxisAngle(0),
    axisRotationSpeed(DEFAULT_AXIS_ROTATION_SPEED)
{
}

// Destructor
Traction::~Traction() {
    if (initialized) {
        emergencyStop();
    }
}

// Initialize traction system
bool Traction::begin() {
    Serial.println("[TRACTION] Initializing traction system...");
    
    // Initialize PWM manager
    if (!PWMManager::getInstance().begin()) {
        Serial.println("[TRACTION] ERROR: Failed to initialize PWM manager");
        return false;
    }
    
    // Initialize MCP23017 manager
    if (!MCP23017Manager::getInstance().begin()) {
        Serial.println("[TRACTION] ERROR: Failed to initialize MCP23017 manager");
        return false;
    }
    
    // Configure motor control pins on MCP23017
    if (!configureMCP23017Pins()) {
        Serial.println("[TRACTION] ERROR: Failed to configure MCP23017 pins");
        return false;
    }
    
    // Configure PWM channels for motors
    if (!configurePWMChannels()) {
        Serial.println("[TRACTION] ERROR: Failed to configure PWM channels");
        return false;
    }
    
    // Initialize axis rotation system
    if (!initAxisRotation()) {
        Serial.println("[TRACTION] WARNING: Axis rotation initialization failed");
        // Not critical, continue
    }
    
    // Set initial state
    currentSpeed = 0;
    targetSpeed = 0;
    currentDirection = STOP;
    targetDirection = STOP;
    emergencyStopActive = false;
    motorBrakeActive = false;
    lastUpdateTime = millis();
    
    // Apply initial stop state
    applyMotorControl(STOP, 0);
    
    initialized = true;
    Serial.println("[TRACTION] Traction system initialized successfully");
    return true;
}

// Configure MCP23017 pins for motor control
bool Traction::configureMCP23017Pins() {
    MCP23017Manager& mcp = MCP23017Manager::getInstance();
    
    // Configure direction control pins as outputs
    if (!mcp.pinMode(MOTOR_LEFT_DIR1, OUTPUT)) return false;
    if (!mcp.pinMode(MOTOR_LEFT_DIR2, OUTPUT)) return false;
    if (!mcp.pinMode(MOTOR_RIGHT_DIR1, OUTPUT)) return false;
    if (!mcp.pinMode(MOTOR_RIGHT_DIR2, OUTPUT)) return false;
    
    // Configure brake pins as outputs
    if (!mcp.pinMode(MOTOR_LEFT_BRAKE, OUTPUT)) return false;
    if (!mcp.pinMode(MOTOR_RIGHT_BRAKE, OUTPUT)) return false;
    
    // Configure axis rotation pins as outputs
    if (!mcp.pinMode(AXIS_ROTATION_DIR1, OUTPUT)) return false;
    if (!mcp.pinMode(AXIS_ROTATION_DIR2, OUTPUT)) return false;
    if (!mcp.pinMode(AXIS_ROTATION_BRAKE, OUTPUT)) return false;
    
    // Set all pins to initial LOW state
    mcp.digitalWrite(MOTOR_LEFT_DIR1, LOW);
    mcp.digitalWrite(MOTOR_LEFT_DIR2, LOW);
    mcp.digitalWrite(MOTOR_RIGHT_DIR1, LOW);
    mcp.digitalWrite(MOTOR_RIGHT_DIR2, LOW);
    mcp.digitalWrite(MOTOR_LEFT_BRAKE, LOW);
    mcp.digitalWrite(MOTOR_RIGHT_BRAKE, LOW);
    mcp.digitalWrite(AXIS_ROTATION_DIR1, LOW);
    mcp.digitalWrite(AXIS_ROTATION_DIR2, LOW);
    mcp.digitalWrite(AXIS_ROTATION_BRAKE, LOW);
    
    Serial.println("[TRACTION] MCP23017 pins configured");
    return true;
}

// Configure PWM channels
bool Traction::configurePWMChannels() {
    PWMManager& pwm = PWMManager::getInstance();
    
    // Configure motor speed PWM channels
    if (!pwm.configureChannel(MOTOR_LEFT_PWM, PWM_FREQUENCY, 0)) return false;
    if (!pwm.configureChannel(MOTOR_RIGHT_PWM, PWM_FREQUENCY, 0)) return false;
    
    // Configure axis rotation PWM channel
    if (!pwm.configureChannel(AXIS_ROTATION_PWM, PWM_FREQUENCY, 0)) return false;
    
    Serial.println("[TRACTION] PWM channels configured");
    return true;
}

// Initialize axis rotation system
bool Traction::initAxisRotation() {
    axisRotationEnabled = false;
    currentAxisAngle = 0;
    targetAxisAngle = 0;
    
    // Set initial axis rotation state
    MCP23017Manager& mcp = MCP23017Manager::getInstance();
    mcp.digitalWrite(AXIS_ROTATION_DIR1, LOW);
    mcp.digitalWrite(AXIS_ROTATION_DIR2, LOW);
    mcp.digitalWrite(AXIS_ROTATION_BRAKE, HIGH); // Brake engaged
    
    PWMManager::getInstance().setPWM(AXIS_ROTATION_PWM, 0);
    
    Serial.println("[TRACTION] Axis rotation system initialized");
    return true;
}

// Update traction system (call regularly in loop)
void Traction::update() {
    if (!initialized) return;
    
    unsigned long currentTime = millis();
    unsigned long deltaTime = currentTime - lastUpdateTime;
    
    if (deltaTime < UPDATE_INTERVAL) {
        return; // Not time to update yet
    }
    
    lastUpdateTime = currentTime;
    
    // Handle emergency stop
    if (emergencyStopActive) {
        return; // Don't update anything during emergency stop
    }
    
    // Update speed gradually towards target
    updateSpeed(deltaTime);
    
    // Update direction if needed
    updateDirection();
    
    // Update axis rotation if enabled
    if (axisRotationEnabled) {
        updateAxisRotation(deltaTime);
    }
    
    // Apply current motor control state
    applyMotorControl(currentDirection, currentSpeed);
}

// Update speed with acceleration/deceleration
void Traction::updateSpeed(unsigned long deltaTime) {
    if (currentSpeed == targetSpeed) {
        return; // Already at target speed
    }
    
    float speedChange;
    if (targetSpeed > currentSpeed) {
        // Accelerating
        speedChange = (acceleration * deltaTime) / 1000.0;
        currentSpeed += speedChange;
        if (currentSpeed > targetSpeed) {
            currentSpeed = targetSpeed;
        }
    } else {
        // Decelerating
        speedChange = (deceleration * deltaTime) / 1000.0;
        currentSpeed -= speedChange;
        if (currentSpeed < targetSpeed) {
            currentSpeed = targetSpeed;
        }
    }
    
    // Clamp to valid range
    if (currentSpeed > maxSpeed) currentSpeed = maxSpeed;
    if (currentSpeed < 0) currentSpeed = 0;
}

// Update direction
void Traction::updateDirection() {
    if (currentDirection != targetDirection) {
        // Must stop before changing direction
        if (currentSpeed == 0) {
            currentDirection = targetDirection;
        } else {
            // Force deceleration to stop
            targetSpeed = 0;
        }
    }
}

// Apply motor control
void Traction::applyMotorControl(Direction dir, uint16_t speed) {
    MCP23017Manager& mcp = MCP23017Manager::getInstance();
    PWMManager& pwm = PWMManager::getInstance();
    
    // Release brakes if moving
    if (speed > 0 && !motorBrakeActive) {
        mcp.digitalWrite(MOTOR_LEFT_BRAKE, LOW);
        mcp.digitalWrite(MOTOR_RIGHT_BRAKE, LOW);
    } else {
        mcp.digitalWrite(MOTOR_LEFT_BRAKE, HIGH);
        mcp.digitalWrite(MOTOR_RIGHT_BRAKE, HIGH);
    }
    
    // Set direction pins
    switch (dir) {
        case FORWARD:
            mcp.digitalWrite(MOTOR_LEFT_DIR1, HIGH);
            mcp.digitalWrite(MOTOR_LEFT_DIR2, LOW);
            mcp.digitalWrite(MOTOR_RIGHT_DIR1, HIGH);
            mcp.digitalWrite(MOTOR_RIGHT_DIR2, LOW);
            break;
            
        case BACKWARD:
            mcp.digitalWrite(MOTOR_LEFT_DIR1, LOW);
            mcp.digitalWrite(MOTOR_LEFT_DIR2, HIGH);
            mcp.digitalWrite(MOTOR_RIGHT_DIR1, LOW);
            mcp.digitalWrite(MOTOR_RIGHT_DIR2, HIGH);
            break;
            
        case LEFT:
            mcp.digitalWrite(MOTOR_LEFT_DIR1, LOW);
            mcp.digitalWrite(MOTOR_LEFT_DIR2, HIGH);
            mcp.digitalWrite(MOTOR_RIGHT_DIR1, HIGH);
            mcp.digitalWrite(MOTOR_RIGHT_DIR2, LOW);
            break;
            
        case RIGHT:
            mcp.digitalWrite(MOTOR_LEFT_DIR1, HIGH);
            mcp.digitalWrite(MOTOR_LEFT_DIR2, LOW);
            mcp.digitalWrite(MOTOR_RIGHT_DIR1, LOW);
            mcp.digitalWrite(MOTOR_RIGHT_DIR2, HIGH);
            break;
            
        case STOP:
        default:
            mcp.digitalWrite(MOTOR_LEFT_DIR1, LOW);
            mcp.digitalWrite(MOTOR_LEFT_DIR2, LOW);
            mcp.digitalWrite(MOTOR_RIGHT_DIR1, LOW);
            mcp.digitalWrite(MOTOR_RIGHT_DIR2, LOW);
            break;
    }
    
    // Set PWM speed
    uint16_t pwmValue = (speed > 0) ? map(speed, 0, 100, minSpeed, 4095) : 0;
    pwm.setPWM(MOTOR_LEFT_PWM, pwmValue);
    pwm.setPWM(MOTOR_RIGHT_PWM, pwmValue);
}

// Set target speed (0-100)
void Traction::setSpeed(uint16_t speed) {
    if (!initialized || emergencyStopActive) return;
    
    targetSpeed = constrain(speed, 0, maxSpeed);
    Serial.print("[TRACTION] Target speed set to: ");
    Serial.println(targetSpeed);
}

// Set direction
void Traction::setDirection(Direction dir) {
    if (!initialized || emergencyStopActive) return;
    
    targetDirection = dir;
    Serial.print("[TRACTION] Target direction set to: ");
    Serial.println(getDirectionString(dir));
}

// Move in a specific direction with speed
void Traction::move(Direction dir, uint16_t speed) {
    if (!initialized || emergencyStopActive) return;
    
    setDirection(dir);
    setSpeed(speed);
}

// Stop motors
void Traction::stop() {
    if (!initialized) return;
    
    targetSpeed = 0;
    targetDirection = STOP;
    Serial.println("[TRACTION] Stop requested");
}

// Emergency stop (immediate)
void Traction::emergencyStop() {
    if (!initialized) return;
    
    Serial.println("[TRACTION] EMERGENCY STOP!");
    
    emergencyStopActive = true;
    currentSpeed = 0;
    targetSpeed = 0;
    currentDirection = STOP;
    targetDirection = STOP;
    
    // Immediately stop all motors
    applyMotorControl(STOP, 0);
    
    // Engage brakes
    activateBrake(true);
    
    // Stop axis rotation
    if (axisRotationEnabled) {
        stopAxisRotation();
    }
}

// Reset emergency stop
void Traction::resetEmergencyStop() {
    if (!initialized) return;
    
    Serial.println("[TRACTION] Resetting emergency stop");
    emergencyStopActive = false;
    activateBrake(false);
}

// Activate/deactivate brake
void Traction::activateBrake(bool active) {
    if (!initialized) return;
    
    motorBrakeActive = active;
    MCP23017Manager& mcp = MCP23017Manager::getInstance();
    
    if (active) {
        mcp.digitalWrite(MOTOR_LEFT_BRAKE, HIGH);
        mcp.digitalWrite(MOTOR_RIGHT_BRAKE, HIGH);
        Serial.println("[TRACTION] Brakes engaged");
    } else {
        mcp.digitalWrite(MOTOR_LEFT_BRAKE, LOW);
        mcp.digitalWrite(MOTOR_RIGHT_BRAKE, LOW);
        Serial.println("[TRACTION] Brakes released");
    }
}

// Set acceleration rate
void Traction::setAcceleration(uint16_t accel) {
    acceleration = constrain(accel, MIN_ACCELERATION, MAX_ACCELERATION);
    Serial.print("[TRACTION] Acceleration set to: ");
    Serial.println(acceleration);
}

// Set deceleration rate
void Traction::setDeceleration(uint16_t decel) {
    deceleration = constrain(decel, MIN_ACCELERATION, MAX_ACCELERATION);
    Serial.print("[TRACTION] Deceleration set to: ");
    Serial.println(deceleration);
}

// Set maximum speed
void Traction::setMaxSpeed(uint16_t speed) {
    maxSpeed = constrain(speed, minSpeed, 100);
    if (targetSpeed > maxSpeed) {
        targetSpeed = maxSpeed;
    }
    Serial.print("[TRACTION] Maximum speed set to: ");
    Serial.println(maxSpeed);
}

// Set minimum speed
void Traction::setMinSpeed(uint16_t speed) {
    minSpeed = constrain(speed, 0, maxSpeed);
    Serial.print("[TRACTION] Minimum speed set to: ");
    Serial.println(minSpeed);
}

// Axis rotation functions
void Traction::enableAxisRotation(bool enable) {
    axisRotationEnabled = enable;
    Serial.print("[TRACTION] Axis rotation ");
    Serial.println(enable ? "enabled" : "disabled");
    
    if (!enable) {
        stopAxisRotation();
    }
}

void Traction::setAxisAngle(int16_t angle) {
    if (!initialized || !axisRotationEnabled) return;
    
    targetAxisAngle = constrain(angle, -MAX_AXIS_ANGLE, MAX_AXIS_ANGLE);
    Serial.print("[TRACTION] Target axis angle set to: ");
    Serial.println(targetAxisAngle);
}

void Traction::rotateAxisLeft(uint16_t speed) {
    if (!initialized || !axisRotationEnabled || emergencyStopActive) return;
    
    MCP23017Manager& mcp = MCP23017Manager::getInstance();
    PWMManager& pwm = PWMManager::getInstance();
    
    // Set direction
    mcp.digitalWrite(AXIS_ROTATION_DIR1, HIGH);
    mcp.digitalWrite(AXIS_ROTATION_DIR2, LOW);
    mcp.digitalWrite(AXIS_ROTATION_BRAKE, LOW);
    
    // Set speed
    uint16_t pwmValue = map(speed, 0, 100, 0, 4095);
    pwm.setPWM(AXIS_ROTATION_PWM, pwmValue);
    
    Serial.print("[TRACTION] Rotating axis left at speed: ");
    Serial.println(speed);
}

void Traction::rotateAxisRight(uint16_t speed) {
    if (!initialized || !axisRotationEnabled || emergencyStopActive) return;
    
    MCP23017Manager& mcp = MCP23017Manager::getInstance();
    PWMManager& pwm = PWMManager::getInstance();
    
    // Set direction
    mcp.digitalWrite(AXIS_ROTATION_DIR1, LOW);
    mcp.digitalWrite(AXIS_ROTATION_DIR2, HIGH);
    mcp.digitalWrite(AXIS_ROTATION_BRAKE, LOW);
    
    // Set speed
    uint16_t pwmValue = map(speed, 0, 100, 0, 4095);
    pwm.setPWM(AXIS_ROTATION_PWM, pwmValue);
    
    Serial.print("[TRACTION] Rotating axis right at speed: ");
    Serial.println(speed);
}

void Traction::stopAxisRotation() {
    if (!initialized) return;
    
    MCP23017Manager& mcp = MCP23017Manager::getInstance();
    PWMManager& pwm = PWMManager::getInstance();
    
    // Stop motor
    mcp.digitalWrite(AXIS_ROTATION_DIR1, LOW);
    mcp.digitalWrite(AXIS_ROTATION_DIR2, LOW);
    mcp.digitalWrite(AXIS_ROTATION_BRAKE, HIGH);
    pwm.setPWM(AXIS_ROTATION_PWM, 0);
    
    currentAxisAngle = targetAxisAngle;
    
    Serial.println("[TRACTION] Axis rotation stopped");
}

void Traction::updateAxisRotation(unsigned long deltaTime) {
    if (currentAxisAngle == targetAxisAngle) {
        stopAxisRotation();
        return;
    }
    
    int16_t angleDiff = targetAxisAngle - currentAxisAngle;
    int16_t angleStep = (axisRotationSpeed * deltaTime) / 1000;
    
    if (abs(angleDiff) <= angleStep) {
        currentAxisAngle = targetAxisAngle;
        stopAxisRotation();
    } else if (angleDiff > 0) {
        rotateAxisRight(axisRotationSpeed);
        currentAxisAngle += angleStep;
    } else {
        rotateAxisLeft(axisRotationSpeed);
        currentAxisAngle -= angleStep;
    }
}

void Traction::setAxisRotationSpeed(uint16_t speed) {
    axisRotationSpeed = constrain(speed, 10, 100);
    Serial.print("[TRACTION] Axis rotation speed set to: ");
    Serial.println(axisRotationSpeed);
}

// Getters
uint16_t Traction::getCurrentSpeed() const {
    return currentSpeed;
}

uint16_t Traction::getTargetSpeed() const {
    return targetSpeed;
}

Traction::Direction Traction::getCurrentDirection() const {
    return currentDirection;
}

Traction::Direction Traction::getTargetDirection() const {
    return targetDirection;
}

uint16_t Traction::getAcceleration() const {
    return acceleration;
}

uint16_t Traction::getDeceleration() const {
    return deceleration;
}

uint16_t Traction::getMaxSpeed() const {
    return maxSpeed;
}

uint16_t Traction::getMinSpeed() const {
    return minSpeed;
}

bool Traction::isEmergencyStopped() const {
    return emergencyStopActive;
}

bool Traction::isBrakeActive() const {
    return motorBrakeActive;
}

bool Traction::isAxisRotationEnabled() const {
    return axisRotationEnabled;
}

int16_t Traction::getCurrentAxisAngle() const {
    return currentAxisAngle;
}

int16_t Traction::getTargetAxisAngle() const {
    return targetAxisAngle;
}

// Get direction as string
const char* Traction::getDirectionString(Direction dir) const {
    switch (dir) {
        case FORWARD: return "FORWARD";
        case BACKWARD: return "BACKWARD";
        case LEFT: return "LEFT";
        case RIGHT: return "RIGHT";
        case STOP: return "STOP";
        default: return "UNKNOWN";
    }
}

// Print status
void Traction::printStatus() const {
    Serial.println("\n========== TRACTION STATUS ==========");
    Serial.print("Initialized: ");
    Serial.println(initialized ? "YES" : "NO");
    Serial.print("Current Speed: ");
    Serial.print(currentSpeed);
    Serial.print(" / Target: ");
    Serial.println(targetSpeed);
    Serial.print("Current Direction: ");
    Serial.print(getDirectionString(currentDirection));
    Serial.print(" / Target: ");
    Serial.println(getDirectionString(targetDirection));
    Serial.print("Acceleration: ");
    Serial.println(acceleration);
    Serial.print("Deceleration: ");
    Serial.println(deceleration);
    Serial.print("Max Speed: ");
    Serial.print(maxSpeed);
    Serial.print(" / Min Speed: ");
    Serial.println(minSpeed);
    Serial.print("Emergency Stop: ");
    Serial.println(emergencyStopActive ? "ACTIVE" : "inactive");
    Serial.print("Brake: ");
    Serial.println(motorBrakeActive ? "ENGAGED" : "released");
    Serial.print("Axis Rotation: ");
    Serial.println(axisRotationEnabled ? "ENABLED" : "disabled");
    if (axisRotationEnabled) {
        Serial.print("Current Angle: ");
        Serial.print(currentAxisAngle);
        Serial.print(" / Target: ");
        Serial.println(targetAxisAngle);
    }
    Serial.println("=====================================\n");
}

bool Traction::initOK() {
    return initialized;
}