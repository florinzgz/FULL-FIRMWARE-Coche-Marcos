// managers/ModeManager.h
// Mode management stub
#pragma once

enum class VehicleMode {
    IDLE,
    DRIVE,
    REVERSE,
    PARK
};

namespace ModeManager {
    inline bool init() { return true; }
    inline void update() {}
    inline VehicleMode getCurrentMode() { return VehicleMode::IDLE; }
}
