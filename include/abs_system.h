#pragma once
#include <Arduino.h>

namespace ABSSystem {

// ABS configuration parameters
struct Config {
  bool enabled;            // ABS enable/disable
  float slipThreshold;     // Wheel slip threshold (%) to trigger ABS
  float minSpeedKmh;       // Minimum speed to activate ABS (km/h)
  uint16_t cycleMs;        // ABS pulse cycle time (ms)
  float pressureReduction; // Brake pressure reduction factor (0-1)
};

// ABS state per wheel
struct WheelABSState {
  bool active;           // ABS currently active on this wheel
  float slipRatio;       // Current slip ratio (%)
  uint32_t lastCycleMs;  // Last ABS cycle timestamp
  uint8_t pulsesApplied; // Number of ABS pulses applied
};

// Global ABS state
struct State {
  bool systemActive;        // At least one wheel has ABS active
  WheelABSState wheels[4];  // State for each wheel (FL, FR, RL, RR)
  uint32_t activationCount; // Total ABS activations
  float avgSlipRatio;       // Average slip ratio across all wheels
};

// Initialize ABS system
void init();

// Update ABS system (call from main loop)
void update();

// Enable/disable ABS
void setEnabled(bool enabled);

// Get current ABS state
const State &getState();

// Check if ABS is active on any wheel
bool isActive();

// Get configuration
Config &getConfig();

// Apply ABS modulation to brake demand for a specific wheel
// Returns modulated brake demand (0-100%)
float modulateBrake(int wheel, float brakeDemand);

// 🔒 v2.4.0: Estado de inicialización
bool initOK();
} // namespace ABSSystem
