#include "boot_guard.h"
#include <Arduino.h>
#include "obstacle_config.h"
#include "pins.h"
#include "logger.h"

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
