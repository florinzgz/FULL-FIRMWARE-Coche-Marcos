// managers/ModeManager.h
// Mode management - integrates operation modes
#pragma once

#include "../../include/operation_modes.h"

// Map OperationMode to VehicleMode for compatibility
enum class VehicleMode {
    IDLE,
    DRIVE,
    PARK
};

namespace ModeManager {
    inline bool init() {
        // Operation modes don't have init, just use the current mode
        return true;
    }
    
    inline void update() {
        // Operation modes are managed by the system, no periodic update needed
    }
    
    inline VehicleMode getCurrentMode() {
        // Map OperationMode to VehicleMode
        OperationMode opMode = SystemMode::getMode();
        switch (opMode) {
            case OperationMode::MODE_FULL:
            case OperationMode::MODE_DEGRADED:
                return VehicleMode::DRIVE;
            case OperationMode::MODE_SAFE:
                return VehicleMode::PARK;
            case OperationMode::MODE_STANDALONE:
                return VehicleMode::IDLE;
            default:
                return VehicleMode::IDLE;
        }
    }
}
