// managers/HUDManager.h
// HUD management - integrates display system
#pragma once

#include "../../include/hud.h"

// Create a namespace HUDManager that wraps the HUD namespace functions
// This avoids conflicts with the existing HUDManager class
namespace HUDManager {
    inline bool init() {
        HUD::init();
        return HUD::initOK();
    }
    
    inline void update() {
        HUD::update();
    }
}
