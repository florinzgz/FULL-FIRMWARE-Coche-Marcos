// traction.cpp
#include "traction.h"
#include "logger.h"
#include "hardware/pca9685.h"
#include "hardware/mcp23017.h"
#include "obstacle_safety.h"
#include "adaptive_cruise.h"

// ... (rest of your existing code up to line ~454)

void Traction::updateWheels(float base) {
    // Emergency stop check - collision imminent
    if (ObstacleSafety::isCollisionImminent()) {
        // Set all wheel demands to 0
        wheelFL_demand = 0;
        wheelFR_demand = 0;
        wheelRL_demand = 0;
        wheelRR_demand = 0;
        
        // Apply hardware stop to all PCA9685 channels
        PCA9685::setAllChannels(0);
        
        // Apply hardware stop to MCP pins
        MCP23017::writeAllPins(0x00);
        
        Logger::error("Traction: EMERGENCY STOP - Collision imminent!");
        return;
    }
    
    // Calculate gear multiplier
    float gearMultiplier = 1.0f;
    switch (currentGear) {
        case GEAR_LOW:
            gearMultiplier = 0.5f;
            break;
        case GEAR_MEDIUM:
            gearMultiplier = 0.75f;
            break;
        case GEAR_HIGH:
            gearMultiplier = 1.0f;
            break;
        default:
            gearMultiplier = 1.0f;
            break;
    }
    
    // Add obstacle factor
    float obstacleFactor = ObstacleSafety::getSpeedReductionFactor();
    
    // Add ACC factor check
    float accFactor = 1.0f;
    auto accStatus = AdaptiveCruise::getStatus();
    if (accStatus.state == AdaptiveCruise::ACC_ACTIVE) {
        accFactor = AdaptiveCruise::getSpeedAdjustment();
        Logger::infof("Traction: ACC active - adjustment %.2f", accFactor);
    }
    
    // Apply all factors: gear, obstacle, and ACC
    const float adjustedBase = base * gearMultiplier * obstacleFactor * accFactor;
    
    // Log when safety factors are active
    if (obstacleFactor < 1.0f || accFactor < 1.0f) {
        Logger::debugf("Traction: gear=%.2f, obstacle=%.2f, ACC=%.2f, final=%.1f%%", 
                      gearMultiplier, obstacleFactor, accFactor, adjustedBase);
    }
    
    // ... (rest of your existing code)
}
