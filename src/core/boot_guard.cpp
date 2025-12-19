#include "boot_guard.h"
#include <Arduino.h>
#include "obstacle_config.h"
#include "logger.h"

namespace {
    bool isStrappingPin(uint8_t gpio) {
        switch (gpio) {
            case 0:   // BOOT
            case 3:   // JTAG
            case 45:  // VDD_SPI
            case 46:  // BOOT/ROM log
                return true;
            default:
                return false;
        }
    }
}

void BootGuard::applyXshutStrappingGuard() {
    bool applied = false;
    for (uint8_t i = 0; i < ObstacleConfig::NUM_SENSORS; i++) {
        uint8_t pin = ObstacleConfig::XSHUT_PINS[i];
        // Drive HIGH immediately if it is a strapping pin
        if (isStrappingPin(pin)) {
            pinMode(pin, OUTPUT);
            digitalWrite(pin, HIGH);
            applied = true;
            Logger::infof("BootGuard: XSHUT strapping pin %u forced HIGH early", pin);
        }
    }
    if (applied) {
        delay(10); // allow line to settle before other peripherals
    }
}
