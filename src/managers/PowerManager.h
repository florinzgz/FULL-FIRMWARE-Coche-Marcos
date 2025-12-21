// managers/PowerManager.h
// Power management - integrates power system
#pragma once

#include "../../include/power_mgmt.h"

namespace PowerManager {
    inline bool init() {
        PowerMgmt::init();
        return true;
    }
    
    inline void update() {
        PowerMgmt::update();
    }
}
