// main.cpp - Entry point for the vehicle firmware
// Handles initialization, mode management, and main control loop

#include <Arduino.h>
#include "core/SystemConfig.h"
#include "core/Watchdog.h"
#include "core/Logger.h"
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
// Auto-recovery permite que el sistema intente recuperarse de errores críticos
// en lugar de entrar en un halt permanente sin posibilidad de recuperación.
namespace CriticalErrorConfig {
    constexpr uint8_t MAX_RETRIES = 3;              // Máximo de reintentos antes de watchdog reset
    constexpr uint32_t RETRY_DELAY_MS = 5000;       // 5 segundos entre reintentos
    constexpr uint32_t WDT_FINAL_TIMEOUT_S = 30;    // Timeout final del watchdog
}

// Forward declarations
void initializeSystem();
void handleCriticalError(const char* errorMsg);

void setup() {
    // Initialize serial early for debugging
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {
        ; // Wait up to 2 seconds for serial to connect
    }
    
    Serial.println("[BOOT] Starting vehicle firmware...");
    Serial.print("[BOOT] Firmware version: ");
    Serial.println(FIRMWARE_VERSION);

    // Critical boot sequence - DO NOT CHANGE ORDER
    // 1. Initialize core system state
    System::init();
    
    // 2. Initialize storage (EEPROM/config)
    Storage::init();
    
    // 3. Initialize watchdog timer
    Watchdog::init();
    Watchdog::feed();
    
    // 4. Initialize logger
    Logger::init();
    Logger::info("Boot sequence started");
    Watchdog::feed();

    // 5. Perform full system initialization
    initializeSystem();

    Serial.println("[BOOT] System initialization complete");
    Logger::info("Vehicle firmware ready");
}

void loop() {
    // Feed watchdog at the start of each loop
    Watchdog::feed();

    // Update all managers in sequence
    PowerManager::update();
    SensorManager::update();
    SafetyManager::update();
    
    // Get current mode and execute mode-specific logic
    VehicleMode currentMode = ModeManager::getCurrentMode();
    ModeManager::update();
    
    // Control updates based on current mode
    ControlManager::update();
    
    // Update telemetry and HUD
    TelemetryManager::update();
    HUDManager::update();

    // Small delay to prevent CPU hogging
    delay(SYSTEM_TICK_MS);
}

void initializeSystem() {
    Watchdog::feed();
    
    // Initialize Power Manager
    Serial.println("[INIT] Power Manager initialization...");
    if (!PowerManager::init()) {
        handleCriticalError("Power Manager initialization failed");
    }
    Logger::info("Power Manager initialized");
    
    Watchdog::feed();
    
    // Initialize Sensor Manager
    Serial.println("[INIT] Sensor Manager initialization...");
    if (!SensorManager::init()) {
        handleCriticalError("Sensor Manager initialization failed");
    }
    Logger::info("Sensor Manager initialized");
    
    Watchdog::feed();
    
    // Initialize Safety Manager
    Serial.println("[INIT] Safety Manager initialization...");
    if (!SafetyManager::init()) {
        handleCriticalError("Safety Manager initialization failed");
    }
    Logger::info("Safety Manager initialized");
    
    Watchdog::feed();
    
    // Initialize HUD Manager
    Serial.println("[INIT] HUD Manager initialization...");
    if (!HUDManager::init()) {
        handleCriticalError("HUD Manager initialization failed");
    }
    Logger::info("HUD Manager initialized");
    
    Watchdog::feed();
    
    // Initialize Control Manager
    Serial.println("[INIT] Control Manager initialization...");
    if (!ControlManager::init()) {
        handleCriticalError("Control Manager initialization failed");
    }
    Logger::info("Control Manager initialized");
    
    Watchdog::feed();
    
    // Initialize Telemetry Manager
    Serial.println("[INIT] Telemetry Manager initialization...");
    if (!TelemetryManager::init()) {
        handleCriticalError("Telemetry Manager initialization failed");
    }
    Logger::info("Telemetry Manager initialized");
    
    Watchdog::feed();
    
    // Initialize Mode Manager (must be last as it depends on other managers)
    Serial.println("[INIT] Mode Manager initialization...");
    if (!ModeManager::init()) {
        handleCriticalError("Mode Manager initialization failed");
    }
    Logger::info("Mode Manager initialized");
    
    Watchdog::feed();
}

void handleCriticalError(const char* errorMsg) {
    Watchdog::feed();
    
    // Contador estático de reintentos (persiste entre llamadas)
    static uint8_t retryCount = 0;
    
    // Log del error
    Serial.print("[CRITICAL ERROR] ");
    Serial.println(errorMsg);
    Logger::error(errorMsg);
    
    retryCount++;
    
    // Si excedemos reintentos, permitir watchdog reset
    if (retryCount >= CriticalErrorConfig::MAX_RETRIES) {
        Logger::errorf("Critical error: Max retries reached (%d/%d)", retryCount, CriticalErrorConfig::MAX_RETRIES);
        Logger::error("Allowing watchdog timeout for system reset");
        
        // Mostrar en display si está disponible
        #ifndef STANDALONE_DISPLAY
        HUDManager::showError();
        #endif
        
        Serial.println("[CRITICAL ERROR] Max retries - stopping watchdog feeds");
        Serial.flush();
        
        // Detener watchdog feeds - sistema se reseteará en 30 segundos
        while (true) {
            delay(1000);
            Serial.println("[CRITICAL ERROR] Waiting for watchdog reset...");
        }
    }
    
    // Reintento: reiniciar después de delay
    Logger::infof("Critical error retry %d/%d - restarting in %lums", 
                  retryCount, CriticalErrorConfig::MAX_RETRIES, CriticalErrorConfig::RETRY_DELAY_MS);
    
    // Mostrar error en display
    #ifndef STANDALONE_DISPLAY
    HUDManager::showError();
    #endif
    
    Serial.printf("[CRITICAL ERROR] Retry %d/%d in %lums\n", 
                  retryCount, CriticalErrorConfig::MAX_RETRIES, CriticalErrorConfig::RETRY_DELAY_MS);
    Serial.flush();
    
    // Esperar antes de reiniciar
    uint32_t delayStart = millis();
    while (millis() - delayStart < CriticalErrorConfig::RETRY_DELAY_MS) {
        Watchdog::feed();  // Mantener watchdog vivo durante delay
        delay(100);
    }
    
    // Reiniciar sistema
    Logger::info("Attempting system restart...");
    Serial.println("[CRITICAL ERROR] Restarting...");
    Serial.flush();
    
    ESP.restart();
}