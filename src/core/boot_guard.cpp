#include "boot_guard.h"
#include <Arduino.h>
#include "obstacle_config.h"
#include "pins.h"
#include "logger.h"
#include <esp_system.h>
#include <rom/rtc.h>

// ============================================================================
// 🔒 v2.17.1: Boot Counter for Bootloop Detection
// ============================================================================
// Uses RTC_NOINIT_ATTR to persist data across soft resets
// RTC memory survives warm resets but not power cycles
// ============================================================================

#define BOOTLOOP_DETECTION_THRESHOLD 3
#define BOOTLOOP_DETECTION_WINDOW_MS 60000  // 60 seconds

struct BootCounterData {
    uint32_t magic;           // Magic number to validate data
    uint8_t bootCount;        // Number of boots
    uint32_t firstBootMs;     // Timestamp of first boot in sequence
    uint32_t lastBootMs;      // Timestamp of last boot
    bool safeModeRequested;   // Flag to enter safe mode
};

// RTC memory - survives warm reset, cleared on power cycle
static RTC_NOINIT_ATTR BootCounterData bootCounterData;
static constexpr uint32_t BOOT_COUNTER_MAGIC = 0xB007C047;  // "BOOT COTR"

void BootGuard::applyXshutStrappingGuard() {
    // v2.15.0: TOFSense-M S migration - No XSHUT pins needed (UART sensor)
    // VL53L5X I2C sensors required XSHUT pins for power sequencing
    // TOFSense-M S uses UART and doesn't have XSHUT pins
    // This function is now a no-op but retained for backward compatibility
    static bool alreadyApplied = false;
#if defined(ESP32) || defined(ESP_PLATFORM)
    static portMUX_TYPE bootGuardMux = portMUX_INITIALIZER_UNLOCKED;
    portENTER_CRITICAL(&bootGuardMux);
#endif
    if (alreadyApplied) {
#if defined(ESP32) || defined(ESP_PLATFORM)
        portEXIT_CRITICAL(&bootGuardMux);
#endif
        return;
    }
    alreadyApplied = true;
#if defined(ESP32) || defined(ESP_PLATFORM)
    portEXIT_CRITICAL(&bootGuardMux);
#endif

    // No action needed - TOFSense-M S doesn't use XSHUT pins
    Logger::info("BootGuard: XSHUT guard skipped (TOFSense-M S has no XSHUT pins)");
}

// ============================================================================
// Boot Counter Implementation
// ============================================================================

void BootGuard::initBootCounter() {
    // Check if RTC memory has valid data
    if (bootCounterData.magic != BOOT_COUNTER_MAGIC) {
        // First boot or power cycle - initialize
        bootCounterData.magic = BOOT_COUNTER_MAGIC;
        bootCounterData.bootCount = 0;
        bootCounterData.firstBootMs = 0;
        bootCounterData.lastBootMs = 0;
        bootCounterData.safeModeRequested = false;
        Logger::info("BootGuard: Boot counter initialized (power cycle or first boot)");
    } else {
        Logger::infof("BootGuard: Boot counter found - %d previous boots", bootCounterData.bootCount);
    }
}

void BootGuard::incrementBootCounter() {
    uint32_t now = millis();
    
    // If this is the first boot in the sequence, record timestamp
    if (bootCounterData.bootCount == 0) {
        bootCounterData.firstBootMs = now;
        bootCounterData.lastBootMs = now;
        bootCounterData.bootCount = 1;
        Logger::info("BootGuard: Starting new boot sequence");
        return;
    }
    
    // Check if we're still within the detection window
    uint32_t timeSinceFirst = now - bootCounterData.firstBootMs;
    
    if (timeSinceFirst > BOOTLOOP_DETECTION_WINDOW_MS) {
        // Outside window - reset counter
        bootCounterData.firstBootMs = now;
        bootCounterData.lastBootMs = now;
        bootCounterData.bootCount = 1;
        bootCounterData.safeModeRequested = false;
        Logger::info("BootGuard: Boot detection window expired - resetting counter");
        return;
    }
    
    // Within window - increment counter
    bootCounterData.bootCount++;
    bootCounterData.lastBootMs = now;
    
    Logger::infof("BootGuard: Boot #%d within %lu ms", bootCounterData.bootCount, timeSinceFirst);
    
    // Check for bootloop condition
    if (bootCounterData.bootCount >= BOOTLOOP_DETECTION_THRESHOLD) {
        bootCounterData.safeModeRequested = true;
        Logger::errorf("BootGuard: BOOTLOOP DETECTED - %d boots in %lu ms", 
                      bootCounterData.bootCount, timeSinceFirst);
        Logger::error("BootGuard: Safe mode will be activated");
    }
}

void BootGuard::clearBootCounter() {
    // Called from loop() to indicate successful boot
    if (bootCounterData.bootCount > 0) {
        Logger::infof("BootGuard: Boot successful - clearing counter (was %d)", bootCounterData.bootCount);
    }
    
    bootCounterData.bootCount = 0;
    bootCounterData.firstBootMs = 0;
    bootCounterData.lastBootMs = 0;
    bootCounterData.safeModeRequested = false;
}

uint8_t BootGuard::getBootCount() {
    return bootCounterData.bootCount;
}

bool BootGuard::isBootloopDetected() {
    return bootCounterData.bootCount >= BOOTLOOP_DETECTION_THRESHOLD;
}

bool BootGuard::shouldEnterSafeMode() {
    return bootCounterData.safeModeRequested;
}
