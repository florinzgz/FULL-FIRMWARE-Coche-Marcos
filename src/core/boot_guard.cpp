#include "boot_guard.h"
#include <Arduino.h>
#include "obstacle_config.h"
#include "pins.h"
#include "logger.h"

void BootGuard::applyXshutStrappingGuard() {
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

    constexpr uint32_t XSHUT_SETTLE_DELAY_MS = 10;

    uint8_t guardedCount = 0;
    for (uint8_t i = 0; i < ObstacleConfig::NUM_SENSORS; i++) {
        uint8_t pin = ObstacleConfig::XSHUT_PINS[i];
        // Drive HIGH immediately if it is a strapping pin
        if (pin_is_strapping(pin)) {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, HIGH);
            guardedCount++;
        }
    }
    if (guardedCount > 0) {
        delay(XSHUT_SETTLE_DELAY_MS); // allow line to settle before other peripherals
        Logger::infof("BootGuard: XSHUT strapping pins guarded early (%u)", guardedCount);
    }
}
