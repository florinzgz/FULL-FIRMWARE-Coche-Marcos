// ============================================================================
// main.cpp - ESP32-S3 Car Control System - Arduino Framework Entry Point
// ============================================================================
// Version: 2.11.4+
// Hardware: ESP32-S3-DevKitC-1 (44 pins)
// Framework: Arduino + PlatformIO
// ============================================================================
// This file implements the setup() and loop() functions required by the
// Arduino framework. It initializes all core systems and runs the main
// control loop at approximately 30 FPS.
//
// Features:
// - Fault-tolerant initialization with operation modes (FULL, DEGRADED, SAFE)
// - STANDALONE_DISPLAY mode for display diagnostics
// - Non-blocking timing for smooth 30 FPS operation
// - Watchdog feeding to prevent boot loops
// - HUD warnings based on system health
// ============================================================================

#include <Arduino.h>
#include <Wire.h>

// Core system headers
#include "system.h"
#include "storage.h"
#include "logger.h"
#include "operation_modes.h"
#include "watchdog.h"
#include "pins.h"

#ifndef STANDALONE_DISPLAY
// Sensor headers (excluded in standalone mode)
#include "sensors.h"
#include "car_sensors.h"
#include "current.h"
#include "temperature.h"
#include "wheels.h"

// Control system headers
#include "traction.h"
#include "steering.h"
#include "steering_motor.h"
#include "pedal.h"
#include "shifter.h"
#include "relays.h"
#include "tcs_system.h"
#include "abs_system.h"
#include "regen_ai.h"
#include "adaptive_cruise.h"
#include "obstacle_detection.h"
#include "obstacle_safety.h"

// Input headers
#include "buttons.h"

// Safety headers
// Note: boot_guard.h only provides applyXshutStrappingGuard() which is called
// automatically during VL53L5CX sensor initialization. No explicit init needed here.
// #include "boot_guard.h"

// Lighting headers
#include "led_controller.h"

// Audio headers
#include "dfplayer.h"
#include "alerts.h"
#endif // !STANDALONE_DISPLAY

// HUD headers (always needed)
#include "hud_manager.h"
#include "touch_calibration.h"  // For touch calibration function

// Global configuration (defined in storage.cpp)
extern Storage::Config cfg;

// ============================================================================
// Touch Calibration - Called by 5-second button press on 4X4 button
// ============================================================================
#ifndef STANDALONE_DISPLAY
void activateTouchCalibration() {
    Logger::info("Touch calibration requested via button press");
    // Start touch calibration through HUD menu system
    // The HUD manager will handle the calibration UI
    HUDManager::showMenu(MenuType::CALIBRATION);
}
#endif

// Main loop timing constants
static constexpr uint32_t TARGET_FPS = 30;
static constexpr uint32_t FRAME_TIME_MS = 1000 / TARGET_FPS;  // ~33ms per frame
static constexpr uint32_t WATCHDOG_FEED_INTERVAL_MS = 1000;   // Feed watchdog every 1s
static constexpr uint32_t SECONDARY_SENSOR_UPDATE_MS = 500;   // Update temps, etc every 500ms
static constexpr uint32_t MIN_HEAP_THRESHOLD_BYTES = 10000;   // Minimum heap before warning

// Standalone mode simulation constants
static constexpr float SIMULATION_SPEED_BASE = 25.0f;
static constexpr float SIMULATION_SPEED_AMPLITUDE = 10.0f;
static constexpr float SIMULATION_SPEED_PERIOD = 1000.0f;
static constexpr float SIMULATION_RPM_BASE = 1500.0f;
static constexpr float SIMULATION_RPM_AMPLITUDE = 500.0f;
static constexpr float SIMULATION_RPM_PERIOD = 800.0f;

// Frame timing state
static uint32_t lastFrameMs = 0;
static uint32_t lastWatchdogFeedMs = 0;
static uint32_t lastSecondarySensorUpdateMs = 0;

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Generate simulated car data for standalone display mode
 * @param now Current time in milliseconds
 * @return CarData structure with simulated values
 */
static CarData generateSimulatedData(uint32_t now) {
    CarData data = {};
    data.speed = SIMULATION_SPEED_BASE + SIMULATION_SPEED_AMPLITUDE * sin(now / SIMULATION_SPEED_PERIOD);
    data.rpm = SIMULATION_RPM_BASE + SIMULATION_RPM_AMPLITUDE * sin(now / SIMULATION_RPM_PERIOD);
    data.voltage = 24.5f;
    data.batteryPercent = 75.0f;
    data.current = 15.0f;
    data.temperature = 35.0f;
    data.gear = GearPosition::DRIVE1;
    data.status.lights = (now / 1000) % 2 == 0;  // Blink every second
    data.status.fourWheelDrive = true;
    return data;
}

// ============================================================================
// STANDALONE DISPLAY MODE - Display diagnostics only
// ============================================================================
#ifdef STANDALONE_DISPLAY

void setup() {
    // Initialize serial
    Serial.begin(115200);
    yield();  // Allow serial to stabilize without blocking
    Serial.println();
    Serial.println("========================================");
    Serial.println("ESP32-S3 Car Control - STANDALONE MODE");
    Serial.println("========================================");
    
    // Initialize watchdog
    Watchdog::init();
    Watchdog::feed();
    
    // Initialize I2C (minimal)
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(I2C_FREQUENCY);
    
    // Initialize storage (for config)
    Storage::init();
    Watchdog::feed();
    
    // Initialize logger
    Logger::init();
    Logger::info("Standalone display mode starting...");
    
    // Set operation mode to standalone
    SystemMode::setMode(OperationMode::MODE_STANDALONE);
    
    // Initialize HUD for display
    HUDManager::init();
    HUDManager::showLogo();
    uint32_t logoStart = millis();
    while (millis() - logoStart < 1000) {
        Watchdog::feed();
        yield();
    }
    HUDManager::showReady();
    
    Serial.println("[STANDALONE] Setup complete - showing simulated data");
    Watchdog::feed();
}

void loop() {
    // Feed watchdog periodically
    uint32_t now = millis();
    if (now - lastWatchdogFeedMs >= WATCHDOG_FEED_INTERVAL_MS) {
        lastWatchdogFeedMs = now;
        Watchdog::feed();
    }
    
    // Frame rate limiting
    uint32_t elapsed = now - lastFrameMs;
    if (elapsed < FRAME_TIME_MS) {
        yield();
    }
    lastFrameMs = now;
    
    // Show simulated data on HUD
    CarData simulatedData = generateSimulatedData(now);
    HUDManager::updateCarData(simulatedData);
    HUDManager::update();
    
    // Optional: Add timeout to exit standalone mode
    #ifdef STANDALONE_TIMEOUT
    static bool timeoutWarned = false;
    if (now > STANDALONE_TIMEOUT && !timeoutWarned) {
        Serial.println("[STANDALONE] Timeout reached - system will continue running display");
        timeoutWarned = true;
    }
    #endif
}

// ============================================================================
// FULL MODE - Complete car control system
// ============================================================================
#else // !STANDALONE_DISPLAY

void setup() {
    // 1. Initialize Serial communication
    Serial.begin(115200);
    yield();  // Allow serial to stabilize without blocking
    Serial.println();
    Serial.println("========================================");
    Serial.println("ESP32-S3 Car Control System");
    Serial.println("Firmware: v2.11.4+");
    Serial.println("========================================");
    
    // 2. Initialize I2C bus (required by many modules)
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(I2C_FREQUENCY);
    Serial.printf("I2C initialized: SDA=%d, SCL=%d, Freq=%d Hz\n", I2C_SDA, I2C_SCL, I2C_FREQUENCY);
    
    // 3. Initialize Storage (loads configuration from EEPROM)
    Storage::init();
    Serial.println("Storage initialized");
    
    // 4. Initialize Logger
    Logger::init();
    Logger::info("==== System Boot ====");
    Logger::infof("Free heap: %u bytes", ESP.getFreeHeap());
    
    // 5. Initialize Watchdog (must be early to prevent boot loops)
    Watchdog::init();
    Watchdog::feed();
    Logger::info("Watchdog initialized (10s timeout)");
    
    // 6. Apply Boot Guard (prevents boot loops from strapping pin conflicts)
    // Note: BootGuard doesn't have init(), only applyXshutStrappingGuard()
    // This is called automatically by VL53L5CX sensor initialization if needed
    Watchdog::feed();
    
    // 7. Initialize System (loads settings, initializes operation modes)
    System::init();
    Watchdog::feed();
    Logger::info("System core initialized");
    
    // 8. Initialize HUD Manager (display + touch)
    HUDManager::init();
    Watchdog::feed();
        Logger::error("HUD initialization failed; entering safe halt state");
        // Critical failure: stop further initialization to avoid undefined behavior.
        uint32_t lastFeed = millis();
        while (true) {
            if (millis() - lastFeed >= 100) {
                lastFeed = millis();
                Watchdog::feed();
            }
            yield();
        }
    }
    Logger::info("HUD initialized");
    
    // Show logo during initialization
    HUDManager::showLogo();
    delay(1000);  // Show logo briefly
    Watchdog::feed();
    
    // 9. Initialize Sensors
    Logger::info("Initializing sensors...");
    Sensors::init();
    CarSensors::init();
    Watchdog::feed();
    
    // 10. Initialize Input Devices
    Logger::info("Initializing input devices...");
    Pedal::init();
    Shifter::init();
    Steering::init();
    Buttons::init();
    Watchdog::feed();
    
    // 11. Initialize Control Systems
    Logger::info("Initializing control systems...");
    Relays::init();
    Traction::init();
    SteeringMotor::init();
    Watchdog::feed();
    
    // 12. Initialize Safety Systems
    Logger::info("Initializing safety systems...");
    ABSSystem::init();
    TCSSystem::init();
    RegenAI::init();
    AdaptiveCruise::init();
    ObstacleDetection::init();
    ObstacleSafety::init();
    Watchdog::feed();
    
    // 13. Initialize Lighting
    Logger::info("Initializing LED system...");
    LEDController::init();
    Watchdog::feed();
    
    // 14. Initialize Audio (if enabled)
    if (cfg.audioEnabled) {
        Logger::info("Initializing audio system...");
        Audio::DFPlayer::init();      // Initialize DFPlayer first (hardware driver)
        Audio::AudioQueue::init();    // Initialize audio queue (requires DFPlayer)
        Alerts::init();                // Initialize alerts last (uses AudioQueue)
        Watchdog::feed();
    } else {
        Logger::info("Audio disabled by configuration");
    }
    
    // 15. Perform Self-Test
    Logger::info("Running system self-test...");
    Watchdog::feed();
    System::Health health = System::selfTest();
    Watchdog::feed();
    
    // 16. Handle operation mode based on self-test results
    OperationMode mode = SystemMode::getMode();
    Logger::infof("Operation mode: %s", SystemMode::getModeName());
    
    if (mode == OperationMode::MODE_FULL) {
        Logger::info("✓ All systems operational - FULL MODE");
        HUDManager::showReady();
        
        // Enable power relays
        Relays::enablePower();
        Logger::info("Power relays enabled");
        
    } else if (mode == OperationMode::MODE_DEGRADED) {
        Logger::warn("⚠ Some optional sensors failed - DEGRADED MODE");
        Logger::warn("System will continue with reduced functionality");
        HUDManager::showReady();
        
        // Show warning on HUD
        CarData warningData = CarSensors::readAll();
        warningData.status.warnings = true;
        HUDManager::updateCarData(warningData);
        
        // Enable power relays (still safe to operate)
        Relays::enablePower();
        Logger::info("Power relays enabled (degraded mode)");
        
    } else if (mode == OperationMode::MODE_SAFE) {
        Logger::error("✗ Critical components failed - SAFE MODE");
        Logger::error("Motors disabled - monitoring only");
        HUDManager::showError("SAFE MODE: Check critical sensors");
        
        // Do NOT enable power relays
        Logger::warn("Power relays remain disabled");
        
    } else if (mode == OperationMode::MODE_STANDALONE) {
        Logger::info("Standalone display mode active");
        HUDManager::showReady();
    }
    
    // 17. Log any errors detected during initialization
    if (System::hasError()) {
        int errorCount = System::getErrorCount();
        Logger::warnf("System has %d error(s) logged", errorCount);
        const Storage::ErrorLog* errors = System::getErrors();
        for (int i = 0; i < errorCount && i < 10; i++) {
            Logger::warnf("  Error %d: Code %u", i+1, errors[i].code);
        }
    }
    
    // 18. Final watchdog feed and ready message
    Watchdog::feed();
    Logger::info("==== Setup Complete ====");
    Logger::infof("Free heap after init: %u bytes", ESP.getFreeHeap());
    
    Serial.println("========================================");
    Serial.println("System ready - entering main loop");
    Serial.println("========================================");
}

void loop() {
    uint32_t now = millis();
    
    // Feed watchdog periodically (every 1 second)
    if (now - lastWatchdogFeedMs >= WATCHDOG_FEED_INTERVAL_MS) {
        lastWatchdogFeedMs = now;
        Watchdog::feed();
        
        // Optional: Monitor stack health (warn if low)
        uint32_t freeHeap = ESP.getFreeHeap();
        if (freeHeap < MIN_HEAP_THRESHOLD_BYTES) {
            Logger::warnf("Low heap: %u bytes", freeHeap);
        }
    }
    
    // Frame rate limiting - target 30 FPS (~33ms per frame)
    uint32_t elapsed = now - lastFrameMs;
    if (elapsed < FRAME_TIME_MS) {
        // Not time for next frame yet - yield and return
        yield();
    }
    lastFrameMs = now;
    
    // Get current operation mode
    OperationMode mode = SystemMode::getMode();
    
    // Update sensors (critical sensors every frame)
    if (mode != OperationMode::MODE_STANDALONE) {
        // Update critical input devices every frame
        Pedal::update();
        Shifter::update();
        Steering::update();
        Buttons::update();
        
        // Update primary sensors every frame
        Sensors::update();
        
        // Update secondary sensors less frequently (temperatures, etc.)
        if (now - lastSecondarySensorUpdateMs >= SECONDARY_SENSOR_UPDATE_MS) {
            lastSecondarySensorUpdateMs = now;
            CarSensors::readSecondary();
        }
        
        // Read all sensor data for HUD
        CarData carData = CarSensors::readAll();
        
        // Update control systems (if not in safe mode)
        if (mode == OperationMode::MODE_FULL || mode == OperationMode::MODE_DEGRADED) {
            // Update traction control
            Traction::update();
            
            // Update steering motor
            SteeringMotor::update();
            
            // Update safety systems
            ABSSystem::update();
            TCSSystem::update();
            RegenAI::update();
            AdaptiveCruise::update();
            ObstacleDetection::update();
            ObstacleSafety::update();
            
            // Update relays
            Relays::update();
        }
        
        // Update lighting (always active)
        LEDController::update();
        
        // Update audio (if enabled)
        // DFPlayer handles hardware, AudioQueue dispatches queued tracks
        if (cfg.audioEnabled) {
            Audio::DFPlayer::update();
            Audio::AudioQueue::update();
        }
        
        // Update HUD with latest sensor data
        HUDManager::updateCarData(carData);
    } else {
        // Standalone mode - show simulated/test data
        CarData simulatedData = generateSimulatedData(now);
        HUDManager::updateCarData(simulatedData);
    }
    
    // Update HUD display (renders dashboard, menus, etc.)
    HUDManager::update();
    
    // Handle menu interactions (touch/button events are processed in HUDManager::update)
    // Additional menu logic can go here if needed
    
    // Update system state machine
    System::update();
}

#endif // STANDALONE_DISPLAY
