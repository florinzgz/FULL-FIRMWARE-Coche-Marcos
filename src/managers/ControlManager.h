// managers/ControlManager.h
// Control management - integrates traction, steering, and relays
#pragma once

#include "../../include/traction.h"
#include "../../include/steering_motor.h"
#include "../../include/relays.h"

namespace ControlManager {
    inline bool init() {
        Traction::init();
        SteeringMotor::init();
        Relays::init();
        return Relays::initOK() && SteeringMotor::initOK();
    }
    
    inline void update() {
        Traction::update();
        SteeringMotor::update();
        Relays::update();
    }
}
