#include "traction.h"
#include "shifter.h"
#include "logger.h"
#include "pedal.h"
#include "steering.h"
#include "config.h"
#include <algorithm>
#include <cmath>

namespace Control {

Traction::Traction()
    : m_state{}
    , m_lastUpdateTime(0)
{
}

Traction& Traction::get() {
    static Traction instance;
    return instance;
}

void Traction::init() {
    Logger::info("Traction: Initializing");
    m_state = State{};
    m_lastUpdateTime = millis();
}

void Traction::update() {
    static float rampedDemand = 0.0f;
    constexpr float RAMP_RATE_PCT_PER_SEC = 50.0f;  // 50% per second
    constexpr float UPDATE_RATE_HZ = 20.0f;  // Assuming 20Hz update rate
    constexpr float MAX_DELTA_PER_UPDATE = RAMP_RATE_PCT_PER_SEC / UPDATE_RATE_HZ;  // 2.5% per update

    auto& s = m_state;
    
    // Get pedal input
    float pedalPct = Pedal::get().getPositionPct();
    s.demandPct = pedalPct;
    
    // Apply smooth power ramping
    float targetDemand = s.demandPct;
    if (targetDemand > rampedDemand) {
        rampedDemand = std::min(rampedDemand + MAX_DELTA_PER_UPDATE, targetDemand);
    } else {
        rampedDemand = std::max(rampedDemand - MAX_DELTA_PER_UPDATE, targetDemand);
    }
    
    // Get steering input
    float steeringAngle = Steering::get().getAngleDeg();
    s.steeringAngle = steeringAngle;
    
    // Check for axis rotation mode
    bool axisRotationMode = false; // Placeholder for actual axis rotation detection
    
    if (axisRotationMode) {
        // Axis rotation mode - all wheels reverse with equal power
        for (int i = 0; i < 4; ++i) {
            s.w[i].reverse = true;
            s.w[i].powerPct = s.demandPct;
        }
    } else {
        // Normal driving mode
        // Reverse gear detection from Shifter
        bool reverseGear = (Shifter::get().gear == Shifter::R);
        
        for (int i = 0; i < 4; ++i) {
            s.w[i].reverse = reverseGear;
        }
        
        // Apply gear-based power limiting
        float gearMultiplier = 1.0f;
        auto shifterGear = Shifter::get().gear;
        if (shifterGear == Shifter::D1) {
            gearMultiplier = 0.70f;  // 70% power in D1 for terrain
            Logger::debugf("Traction: D1 mode - limiting to 70%% power");
        } else if (shifterGear == Shifter::D2) {
            gearMultiplier = 1.0f;   // 100% power in D2 for speed
        } else if (shifterGear == Shifter::R) {
            gearMultiplier = 0.80f;  // 80% power in reverse for safety
            Logger::debugf("Traction: Reverse mode - limiting to 80%% power");
        }
        
        // Use rampedDemand instead of s.demandPct for calculations
        const float base = rampedDemand * gearMultiplier;
        
        // Simple differential steering
        // Inside wheels get less power, outside wheels get more
        float steerFactor = steeringAngle / 45.0f; // Normalize to +/- 1.0 at 45 degrees
        steerFactor = std::max(-1.0f, std::min(1.0f, steerFactor));
        
        if (steerFactor > 0) {
            // Turning right
            s.w[0].powerPct = base * (1.0f - steerFactor * 0.5f); // Front left (inside)
            s.w[1].powerPct = base * (1.0f + steerFactor * 0.3f); // Front right (outside)
            s.w[2].powerPct = base * (1.0f - steerFactor * 0.5f); // Rear left (inside)
            s.w[3].powerPct = base * (1.0f + steerFactor * 0.3f); // Rear right (outside)
        } else {
            // Turning left
            float absSteerFactor = -steerFactor;
            s.w[0].powerPct = base * (1.0f + absSteerFactor * 0.3f); // Front left (outside)
            s.w[1].powerPct = base * (1.0f - absSteerFactor * 0.5f); // Front right (inside)
            s.w[2].powerPct = base * (1.0f + absSteerFactor * 0.3f); // Rear left (outside)
            s.w[3].powerPct = base * (1.0f - absSteerFactor * 0.5f); // Rear right (inside)
        }
    }
    
    m_lastUpdateTime = millis();
}

const Traction::State& Traction::getState() const {
    return m_state;
}

} // namespace Control
