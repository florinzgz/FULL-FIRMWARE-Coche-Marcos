#include "boot_guard.h"
#include <Arduino.h>
#include "obstacle_config.h"
#include "pins.h"
#include "logger.h"

void BootGuard::applyXshutStrappingGuard() {
    static bool alreadyApplied = false;
    if (alreadyApplied) {
        return;
    }

    constexpr size_t XSHUT_PIN_COUNT = sizeof(::ObstacleConfig::XSHUT_PINS) / sizeof(::ObstacleConfig::XSHUT_PINS[0]);
    static_assert(::ObstacleConfig::NUM_SENSORS == XSHUT_PIN_COUNT,
                  "ObstacleConfig::XSHUT_PINS size must match NUM_SENSORS");
    constexpr uint32_t XSHUT_SETTLE_DELAY_MS = 10;

    bool applied = false;
    for (uint8_t i = 0; i < ::ObstacleConfig::NUM_SENSORS; i++) {
        uint8_t pin = ::ObstacleConfig::XSHUT_PINS[i];
        // Drive HIGH immediately if it is a strapping pin
        if (pin_is_strapping(pin)) {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, HIGH);
            applied = true;
        }
    }
    if (applied) {
        delay(XSHUT_SETTLE_DELAY_MS); // allow line to settle before other peripherals
    }
    alreadyApplied = true;
}
