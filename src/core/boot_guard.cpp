#include "boot_guard.h"
#include <Arduino.h>
#include "obstacle_config.h"
#include "pins.h"
#include "logger.h"

void BootGuard::applyXshutStrappingGuard() {
    constexpr uint8_t XSHUT_COUNT = ObstacleConfig::NUM_SENSORS;
    constexpr uint32_t XSHUT_SETTLE_DELAY_MS = 10;

    bool applied = false;
    for (uint8_t i = 0; i < XSHUT_COUNT; i++) {
        uint8_t pin = ObstacleConfig::XSHUT_PINS[i];
        // Drive HIGH immediately if it is a strapping pin
        if (pin_is_strapping(pin)) {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, HIGH);
            applied = true;
            Logger::infof("BootGuard: XSHUT strapping pin %u forced HIGH early", pin);
        }
    }
    if (applied) {
        delay(XSHUT_SETTLE_DELAY_MS); // allow line to settle before other peripherals
    }
}
