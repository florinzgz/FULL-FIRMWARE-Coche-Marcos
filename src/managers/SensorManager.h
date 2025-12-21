// managers/SensorManager.h
// Sensor management - integrates all sensor subsystems
#pragma once

#include "../../include/sensors.h"
#include "../../include/wheels.h"
#include "../../include/temperature.h"
#include "../../include/current.h"

namespace SensorManager {
    inline bool init() {
        Sensors::init();
        return Sensors::initOK();
    }
    
    inline void update() {
        Sensors::update();
    }
}
