// managers/SafetyManager.h
// Safety management - integrates ABS, TCS, and obstacle safety
#pragma once

#include "../../include/abs_system.h"
#include "../../include/tcs_system.h"
#include "../../include/obstacle_safety.h"

namespace SafetyManager {
    inline bool init() {
        ABSSystem::init();
        TCSSystem::init();
        ObstacleSafety::init();
        return true;
    }
    
    inline void update() {
        ABSSystem::update();
        TCSSystem::update();
        ObstacleSafety::update();
    }
}
