#pragma once

#include <FastLED.h>

// LED Strip Configuration
#define LED_FRONT_PIN 48          // GPIO pin for front LEDs
#define LED_REAR_PIN 47           // GPIO pin for rear LEDs
#define LED_FRONT_COUNT 28        // Number of front LEDs
#define LED_REAR_COUNT 16         // Number of rear LEDs (3 left + 10 center + 3 right)

// LED indices for rear strip
#define LED_REAR_LEFT_START 0     // Indices 0-2: Left turn signal
#define LED_REAR_LEFT_END 2
#define LED_REAR_CENTER_START 3   // Indices 3-12: Center brake/position
#define LED_REAR_CENTER_END 12
#define LED_REAR_RIGHT_START 13   // Indices 13-15: Right turn signal
#define LED_REAR_RIGHT_END 15

namespace LEDController {
    
    // LED Modes
    enum FrontMode {
        FRONT_OFF,          // All off
        FRONT_KITT_IDLE,    // KITT scanner effect (red, slow)
        FRONT_ACCEL_LOW,    // Acceleration 1-25% (red→orange)
        FRONT_ACCEL_MED,    // Acceleration 25-50% (orange→yellow)
        FRONT_ACCEL_HIGH,   // Acceleration 50-75% (yellow→green)
        FRONT_ACCEL_MAX,    // Acceleration 75-100% (green→blue→rainbow)
        FRONT_REVERSE,      // Reverse mode (white scanner)
        FRONT_ABS_ALERT,    // ABS/TCS active (red/white flash)
        FRONT_TCS_ALERT     // TCS active (amber flash)
    };
    
    enum RearMode {
        REAR_OFF,           // All off
        REAR_POSITION,      // Position lights (red 20%)
        REAR_BRAKE,         // Brake lights (red 100%)
        REAR_BRAKE_EMERGENCY, // Emergency brake (red flashing)
        REAR_REVERSE,       // Reverse (center white + side amber)
        REAR_REGEN_ACTIVE   // Regenerative braking (blue pulse)
    };
    
    enum TurnSignal {
        TURN_OFF,           // No turn signal
        TURN_LEFT,          // Left turn signal
        TURN_RIGHT,         // Right turn signal
        TURN_HAZARD         // Hazard lights (both sides)
    };
    
    // Configuration
    struct Config {
        uint8_t brightness;      // Global brightness (0-255)
        uint16_t updateRateMs;   // Update rate in milliseconds
        bool smoothTransitions;  // Enable smooth color transitions
    };
    
    // Initialize LED system
    void init();
    
    // Update LED animations (call in main loop)
    void update();
    
    // Set front LED mode based on throttle/pedal
    void setFrontMode(FrontMode mode);
    
    // Set front LED mode from throttle percentage
    void setFrontFromThrottle(float throttlePercent);
    
    // Set rear LED mode
    void setRearMode(RearMode mode);
    
    // Set turn signals
    void setTurnSignal(TurnSignal signal);
    
    // Set brightness (0-255)
    void setBrightness(uint8_t brightness);
    
    // Enable/disable LED system
    void setEnabled(bool enabled);
    
    // Get configuration reference
    Config& getConfig();
    
    // Emergency flash all LEDs (for critical alerts)
    void emergencyFlash(uint8_t count);
}
