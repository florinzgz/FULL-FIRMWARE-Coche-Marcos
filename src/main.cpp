// ============================================================================
// main.cpp - ESP32-S3 Car Control System
// ============================================================================
// Main entry point for the ESP32-S3 based car control system
// Handles initialization and main loop for all vehicle subsystems
// ============================================================================

#include <Arduino.h>
#include "system.h"
#include "storage.h"
#include "logger.h"
#include "watchdog.h"
#include "i2c_recovery.h"
#include "relays.h"
#include "hud_manager.h"
#include "dfplayer.h"
#include "queue.h"
#include "alerts.h"
#include "sensors.h"
#include "pedal.h"
#include "steering.h"
#include "buttons.h"
#include "shifter.h"
#include "traction.h"
#include "steering_motor.h"
#include "abs_system.h"
#include "tcs_system.h"
#include "regen_ai.h"
#include "obstacle_detection.h"
#include "obstacle_safety.h"
#include "telemetry.h"
#include "car_sensors.h"
#include "pins.h"
#include "debug.h"

// Global configuration instance
extern Storage::Config cfg;

// ============================================================================
// SETUP - System Initialization
// ============================================================================
void setup() {
    // Phase 1: Critical Startup - Serial and Display
    Serial.begin(115200);
    
    // Wait for serial with timeout
    uint32_t serialStart = millis();
    while (!Serial && (millis() - serialStart < 1000)) {
        yield();
    }
    
    Serial.println("\n========================================");
    Serial.println("ESP32-S3 Car Control System");
    Serial.println("========================================");
    
    // Enable TFT backlight
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);
    
    // Hardware reset TFT
    pinMode(PIN_TFT_RST, OUTPUT);
    digitalWrite(PIN_TFT_RST, LOW);
    delay(10);
    digitalWrite(PIN_TFT_RST, HIGH);
    delay(50);
    
    Debug::setLevel(2);
    
    // Phase 2: Basic Initialization
    Serial.println("[INIT] System initialization...");
    System::init();
    
    Serial.println("[INIT] Storage initialization...");
    Storage::init();
    Storage::load(cfg);
    
    // Validate display brightness
    if (cfg.displayBrightness < 0 || cfg.displayBrightness > 255) {
        cfg.displayBrightness = 128;
    }
    
    Serial.println("[INIT] Logger initialization...");
    Logger::init();
    
    // Phase 4: Critical Systems
    Serial.println("[INIT] Watchdog initialization...");
    Watchdog::init();
    Watchdog::feed();
    
    Serial.println("[INIT] I2C Recovery initialization...");
    I2CRecovery::init();
    Watchdog::feed();
    
    Serial.println("[INIT] Relays initialization...");
    Relays::init();
    Watchdog::feed();
    
    Serial.println("[INIT] Car Sensors initialization...");
    CarSensors::init();
    Watchdog::feed();
    
    // Phase 5: Display and Audio
    Serial.println("[INIT] HUD Manager initialization...");
    HUDManager::init();
    Watchdog::feed();
    
    // Initialize Audio (if enabled)
    if (cfg.audioEnabled) {
        Logger::info("Initializing audio system...");
        
        // Initialize in correct order: DFPlayer → Queue → Alerts
        Audio::DFPlayer::init();      // Initialize hardware first
        Audio::AudioQueue::init();    // Initialize queue system
        Alerts::init();                // Initialize alert sounds
        
        Watchdog::feed();
        Logger::info("Audio system initialized");
    } else {
        Logger::info("Audio disabled by configuration");
    }
    
    // Phase 6: Individual Sensors
    Serial.println("[INIT] Sensors initialization...");
    Sensors::initCurrent();
    Watchdog::feed();
    
    Sensors::initTemperature();
    Watchdog::feed();
    
    Sensors::initWheels();
    Watchdog::feed();
    
    // Phase 7: Input Systems
    Serial.println("[INIT] Input systems initialization...");
    Pedal::init();
    Steering::init();
    Buttons::init();
    Shifter::init();
    Watchdog::feed();
    
    // Phase 8: Control Systems
    Serial.println("[INIT] Control systems initialization...");
    Traction::init();
    SteeringMotor::init();
    Watchdog::feed();
    
    // Phase 9: Advanced Safety Systems
    Serial.println("[INIT] Safety systems initialization...");
    ABSSystem::init();
    TCSSystem::init();
    RegenAI::init();
    Watchdog::feed();
    
    // Phase 10: Obstacle Detection
    Serial.println("[INIT] Obstacle detection initialization...");
    ObstacleDetection::init();
    ObstacleSafety::init();
    Watchdog::feed();
    
    // Phase 11: Telemetry
    Serial.println("[INIT] Telemetry initialization...");
    Telemetry::init();
    Watchdog::feed();
    
    // Phase 12: Self-Test and Ready
    Serial.println("[INIT] Starting self-test...");
    HUDManager::showLogo();
    
    if (cfg.audioEnabled) {
        Alerts::play(Audio::AUDIO_INICIO);
    }
    
    System::Health health = System::selfTest();
    
    if (health.ok) {
        Serial.println("[INIT] Self-test PASSED");
        HUDManager::showReady();
        Relays::enablePower();
        HUDManager::showMenu(0);  // Show dashboard
    } else {
        Serial.println("[INIT] Self-test FAILED");
        HUDManager::showError();
    }
    
    Watchdog::feed();
    
    Serial.println("========================================");
    Serial.println("[INIT] Initialization complete!");
    Serial.println("========================================");
}

// ============================================================================
// LOOP - Main System Update
// ============================================================================
void loop() {
    // Feed watchdog first
    Watchdog::feed();
    
    // Update system state machine
    System::update();
    
    // Update sensors
    CarSensors::update();
    
    // Update input systems
    Pedal::update();
    Steering::update();
    Buttons::update();
    Shifter::update();
    
    // Update control systems
    Traction::update();
    SteeringMotor::update();
    
    // Update safety systems
    ABSSystem::update();
    TCSSystem::update();
    RegenAI::update();
    
    // Update obstacle detection
    ObstacleDetection::update();
    ObstacleSafety::update();
    
    // Update display
    HUDManager::update();
    
    // Update audio (if enabled)
    if (cfg.audioEnabled) {
        Audio::DFPlayer::update();      // Update hardware player
        Audio::AudioQueue::update();    // Process audio queue
    }
    
    // Update telemetry
    Telemetry::update();
    
    // Update relays
    Relays::update();
    
    // Small yield to prevent watchdog issues
    yield();
}

