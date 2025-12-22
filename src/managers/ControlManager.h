// managers/ControlManager.h
// Control management - integrates traction, steering, and relays
#pragma once

#include "../../include/traction.h"
#include "../../include/steering_motor.h"
#include "../../include/relays.h"
#include "../../include/mcp_shared.h"
#include "../../include/logger.h"

namespace ControlManager {
    inline bool init() {
        // Initialize shared MCP23017 FIRST before subsystems
        if (!MCPShared::init()) {
            Logger::error("ControlManager: MCP23017 shared init FAIL");
            return false;
        }
        
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
