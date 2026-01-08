// main.cpp - Entry point for the vehicle firmware
// Handles initialization, mode management, and main control loop

#include <Arduino.h>
#include "SystemConfig.h"
#include "watchdog.h"
#include "logger.h"
#include "managers/PowerManager.h"
#include "managers/SensorManager.h"
#include "managers/ControlManager.h"
#include "managers/TelemetryManager.h"
#include "managers/HUDManager.h"
#include "managers/ModeManager.h"
#include "managers/SafetyManager.h"

// Include core system components for proper boot sequence
#include "system.h"
#include "storage.h"

// ============================================================================
// Critical error recovery configuration - v2.11.5
// ============================================================================
namespace CriticalErrorConfig {
    constexpr uint8_t MAX_RETRIES = 3;
    constexpr uint32_t RETRY_DELAY_MS = 5000;
}

// Forward declarations
void initializeSystem();
void handleCriticalError(const char* errorMsg);

void setup() {
    // 🔒 v2.11.6: BOOTLOOP FIX - Early UART diagnostic output
    // Initialize Serial first for all modes
    Serial.begin(115200);
    
    #ifdef STANDALONE_DISPLAY
    // Early diagnostic output for standalone mode
    delay(100);  // Give UART time to stabilize
    Serial.println("\n\n=== ESP32-S3 EARLY BOOT ===");
    Serial.println("[STANDALONE] Mode active");
    Serial.flush();
    #endif
    
    while (!Serial && millis() < 2000) { ; }

    Serial.println("[BOOT] Starting vehicle firmware...");
    Serial.print("[BOOT] Firmware version: ");
    Serial.println(FIRMWARE_VERSION);

    // Critical boot sequence
    System::init();
    Storage::init();
    Watchdog::init();
    Watchdog::feed();

    Logger::init();
    Logger::info("Boot sequence started");
    Watchdog::feed();

    initializeSystem();

    Serial.println("[BOOT] System initialization complete");
    Logger::info("Vehicle firmware ready");
}

void loop() {
    Watchdog::feed();

#ifdef STANDALONE_DISPLAY
    // ===========================
    // STANDALONE DISPLAY MODE
    // ===========================
    HUDManager::update();
    delay(33);   // ~30 FPS

#else
    // ===========================
    // FULL VEHICLE MODE
    // ===========================
    PowerManager::update();
    SensorManager::update();
    SafetyManager::update();

    VehicleMode currentMode = ModeManager::getCurrentMode();
    ModeManager::update();

    ControlManager::update();

    TelemetryManager::update();
    HUDManager::update();

    delay(SYSTEM_TICK_MS);
#endif
}

void initializeSystem() {
    Watchdog::feed();

#ifdef STANDALONE_DISPLAY
    // ===========================
    // STANDALONE DISPLAY INIT
    // ===========================
    Serial.println("🧪 STANDALONE DISPLAY MODE");
    Serial.flush();
    delay(100);  // Ensure UART message is sent
    
    Serial.println("[INIT] HUD Manager initialization...");
    Serial.flush();

    if (!HUDManager::init()) {
        Serial.println("[ERROR] HUD Manager initialization failed");
        Serial.flush();
        handleCriticalError("HUD Manager initialization failed");
    }

    Logger::info("HUD Manager initialized (standalone)");
    Watchdog::feed();

    Serial.println("🧪 STANDALONE: Skipping other managers");
    Serial.flush();
    return;   // ¡MUY IMPORTANTE!

#else
    // ===========================
    // FULL VEHICLE INIT
    // ===========================

    Serial.println("[INIT] Power Manager initialization...");
    if (!PowerManager::init()) {
        handleCriticalError("Power Manager initialization failed");
    }
    Logger::info("Power Manager initialized");
    Watchdog::feed();

    Serial.println("[INIT] Sensor Manager initialization...");
    if (!SensorManager::init()) {
        handleCriticalError("Sensor Manager initialization failed");
    }
    Logger::info("Sensor Manager initialized");
    Watchdog::feed();

    Serial.println("[INIT] Safety Manager initialization...");
    if (!SafetyManager::init()) {
        handleCriticalError("Safety Manager initialization failed");
    }
    Logger::info("Safety Manager initialized");
    Watchdog::feed();

    Serial.println("[INIT] HUD Manager initialization...");
    if (!HUDManager::init()) {
        handleCriticalError("HUD Manager initialization failed");
    }
    Logger::info("HUD Manager initialized");
    Watchdog::feed();

    Serial.println("[INIT] Control Manager initialization...");
    if (!ControlManager::init()) {
        handleCriticalError("Control Manager initialization failed");
    }
    Logger::info("Control Manager initialized");
    Watchdog::feed();

    Serial.println("[INIT] Telemetry Manager initialization...");
    if (!TelemetryManager::init()) {
        handleCriticalError("Telemetry Manager initialization failed");
    }
    Logger::info("Telemetry Manager initialized");
    Watchdog::feed();

    Serial.println("[INIT] Mode Manager initialization...");
    if (!ModeManager::init()) {
        handleCriticalError("Mode Manager initialization failed");
    }
    Logger::info("Mode Manager initialized");
    Watchdog::feed();

#endif
}

void handleCriticalError(const char* errorMsg) {
    Watchdog::feed();

    static uint8_t retryCount = 0;
    static const char* lastErrorSource = nullptr;

    Serial.print("[CRITICAL ERROR] ");
    Serial.println(errorMsg);
    Logger::error(errorMsg);

    if (lastErrorSource != errorMsg) {
        retryCount = 0;
        lastErrorSource = errorMsg;
    }

    retryCount++;

    if (retryCount >= CriticalErrorConfig::MAX_RETRIES) {
        Logger::errorf("Critical error: Max retries reached (%d/%d)",
                       retryCount, CriticalErrorConfig::MAX_RETRIES);
        Logger::error("Allowing watchdog timeout for system reset");

#ifndef STANDALONE_DISPLAY
        HUDManager::showError();
#endif

        Serial.println("[CRITICAL ERROR] Max retries - stopping watchdog feeds");
        Serial.flush();

        uint32_t lastPrintTime = millis();
        while (true) {
            yield();
            if (millis() - lastPrintTime >= 1000) {
                Serial.println("[CRITICAL ERROR] Waiting for watchdog reset...");
                lastPrintTime = millis();
            }
        }
    }

#ifndef STANDALONE_DISPLAY
    HUDManager::showError();
#endif

    Serial.printf("[CRITICAL ERROR] Retry %d/%d in %lums\n",
                  retryCount,
                  CriticalErrorConfig::MAX_RETRIES,
                  CriticalErrorConfig::RETRY_DELAY_MS);

    Serial.flush();

    uint32_t delayStart = millis();
    while (millis() - delayStart < CriticalErrorConfig::RETRY_DELAY_MS) {
        Watchdog::feed();
        yield();
    }

    Logger::info("Attempting system restart...");
    Serial.println("[CRITICAL ERROR] Restarting...");
    Serial.flush();

    ESP.restart();

    while (true) {
        delay(1000);
    }
}
