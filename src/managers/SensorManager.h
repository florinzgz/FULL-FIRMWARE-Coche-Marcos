// managers/SensorManager.h
// Sensor management - integrates all sensor subsystems
#pragma once

#include "../../include/sensors.h"
#include "../../include/wheels.h"
#include "../../include/temperature.h"
#include "../../include/current.h"
#include "../../include/pedal.h"
#include "../../include/steering.h"
#include "../../include/shifter.h"
#include "../../include/buttons.h"

namespace SensorManager {
    inline bool init() {
#ifdef DISABLE_SENSORS
        // 🔒 v2.11.6: BOOTLOOP FIX - Skip sensor initialization in DISABLE_SENSORS mode
        Serial.println("[SensorManager] DISABLE_SENSORS mode - skipping all sensor init");
        return true;  // Report success to allow boot to continue
#else
        // Initialize input devices (always needed for vehicle operation)
        Pedal::init();
        Steering::init();
        Shifter::init();
        Buttons::init();
        
        // Initialize sensor subsystems
        Sensors::init();
        return Sensors::initOK();
#endif
    }
    
    inline void update() {
#ifndef DISABLE_SENSORS
        Sensors::update();
        
        // Update input devices
        Pedal::update();
        Steering::update();
        Shifter::update();
        Buttons::update();
#endif
    }
}
