// managers/ControlManager.h
// Control management - integrates traction, steering, and relays
#pragma once

#include "../../include/traction.h"
#include "../../include/steering_motor.h"
#include "../../include/relays.h"
#include "../../include/mcp23017_manager.h"
#include "../../include/shifter.h"
#include "../../include/logger.h"

namespace ControlManager {
    inline bool init() {
        // Initialize shared MCP23017 manager first (used by multiple modules)
        MCP23017Manager& mcpMgr = MCP23017Manager::getInstance();
        bool mcpOK = mcpMgr.init();
        
        if (!mcpOK) {
            Logger::error("ControlManager: MCP23017 shared init FAILED");
        }
        
        // Initialize control subsystems (they will use the MCP manager)
        Traction::init();
        SteeringMotor::init();
        Relays::init();
        
        // Initialize shifter (also uses MCP manager)
        Shifter::init();
        
        // Verify all subsystems initialized correctly
        bool tractionOK = Traction::initOK();
        bool steeringOK = SteeringMotor::initOK();
        bool relaysOK = Relays::initOK();
        bool shifterOK = Shifter::initOK();
        
        return mcpOK && tractionOK && steeringOK && relaysOK && shifterOK;
    }
    
    inline void update() {
        Traction::update();
        SteeringMotor::update();
        Relays::update();
        // Note: Shifter::update() is called separately by input manager
    }
}
