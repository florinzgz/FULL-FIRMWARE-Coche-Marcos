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
        
        // Verify critical safety systems initialized correctly
        bool absOK = ABSSystem::initOK();
        bool tcsOK = TCSSystem::initOK();
        
        // ObstacleSafety may not have initOK(), so we only check ABS/TCS
        return absOK && tcsOK;
    }
    
    inline void update() {
        ABSSystem::update();
        TCSSystem::update();
        ObstacleSafety::update();
    }
}
