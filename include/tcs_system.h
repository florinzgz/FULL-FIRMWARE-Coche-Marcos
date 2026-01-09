#pragma once
#include <Arduino.h>

namespace TCSSystem {

// TCS configuration parameters
struct Config {
  bool enabled;              // TCS enable/disable
  float slipThreshold;       // Wheel slip threshold (%) to trigger TCS
  float minSpeedKmh;         // Minimum speed to activate TCS (km/h)
  float aggressiveReduction; // Aggressive power reduction on activation (%)
  float smoothReduction;     // Smooth gradual reduction per cycle (%)
  float recoveryRatePerSec;  // Power recovery rate (%/s)
  bool enabled4WD;           // Enable 4WD differential control
};

// TCS state per wheel
struct WheelTCSState {
  bool active;               // TCS currently active on this wheel
  float slipRatio;           // Current slip ratio (%)
  float powerReduction;      // Current power reduction (0-100%)
  uint32_t lastActiveMs;     // Last time TCS was active
  uint32_t lastActivationMs; // Last time TCS was activated
  uint32_t activationCount;  // Number of times TCS activated on this wheel
};

// Global TCS state
struct State {
  bool systemActive;         // At least one wheel has TCS active
  WheelTCSState wheels[4];   // State for each wheel (FL, FR, RL, RR)
  uint32_t totalActivations; // Total TCS activations across all wheels
  float avgSlipRatio;        // Average slip ratio across all wheels
  float vehicleSpeed;        // Current vehicle speed (km/h)
  float lateralG;            // Lateral G force in corners
  float efficiencyGain;      // Efficiency gain from TCS (%)
};

// Initialize TCS system
void init();

// Update TCS system (call from main loop)
void update();

// Enable/disable TCS
void setEnabled(bool enabled);

// Get current TCS state
const State &getState();

// Check if TCS is active on any wheel
bool isActive();

// Get configuration
Config &getConfig();

// Apply TCS modulation to power demand for a specific wheel
// Returns modulated power demand (0-100%)
float modulatePower(int wheel, float powerDemand);

// Set drive mode (0=Eco, 1=Normal, 2=Sport)
void setDriveMode(uint8_t mode);

// 🔒 v2.4.0: Estado de inicialización
bool initOK();
} // namespace TCSSystem
