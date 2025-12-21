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
        
        // Verify all subsystems initialized correctly
        bool tractionOK = Traction::initOK();
        bool steeringOK = SteeringMotor::initOK();
        bool relaysOK = Relays::initOK();
        
        return tractionOK && steeringOK && relaysOK;
    }
    
    inline void update() {
        Traction::update();
        SteeringMotor::update();
        Relays::update();
    }
}
