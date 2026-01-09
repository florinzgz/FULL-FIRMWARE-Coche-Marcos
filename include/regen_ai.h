#pragma once
#include <Arduino.h>

namespace RegenAI {

// AI-based regenerative braking configuration
struct Config {
  bool enabled;         // AI regen enable/disable
  float maxRegenPower;  // Maximum regenerative power (%)
  float batterySOCMin;  // Minimum battery SOC to enable regen (%)
  float batterySOCMax;  // Maximum battery SOC (stops regen) (%)
  float tempMax;        // Maximum battery temp for regen (°C)
  float aggressiveness; // Regen aggressiveness factor (0-1)
};

// Input features for AI model
struct Features {
  float speed;          // Current speed (km/h)
  float acceleration;   // Current acceleration (m/s²)
  float batterySOC;     // Battery state of charge (%)
  float batteryTemp;    // Battery temperature (°C)
  float batteryCurrent; // Current battery current (A)
  float roadSlope;      // Estimated road slope (degrees)
  float pedalPosition;  // Brake pedal position (%)
};

// AI model prediction output
struct Prediction {
  float regenPower; // Predicted optimal regen power (%)
  float efficiency; // Predicted energy recovery efficiency (%)
  float confidence; // Model confidence (0-1)
};

// System state
struct State {
  bool active;              // Regen currently active
  Features currentFeatures; // Current input features
  Prediction prediction;    // Current AI prediction
  float actualRegenPower;   // Actual regen power applied (%)
  float energyRecovered;    // Total energy recovered (Wh)
  uint32_t cycleCount;      // Number of regen cycles
};

// Initialize AI regenerative braking system
void init();

// Update AI regen system (call from main loop)
void update();

// Enable/disable AI regen
void setEnabled(bool enabled);

// Get current state
const State &getState();

// Check if regen is active
bool isActive();

// Get configuration
Config &getConfig();

// Manually trigger regen evaluation
void evaluateRegen();

// Get optimal regen power based on current conditions
// Returns regen power (0-100%)
float getOptimalRegenPower();

// Simple AI model: Uses lookup table optimized from historical data
// In future, can be replaced with TensorFlow Lite Micro model
float predictRegenPower(const Features &features);

// Update energy recovery statistics
void updateEnergyStats(float powerW, float durationMs);

// 🔒 v2.5.0: Estado de inicialización
bool initOK();
} // namespace RegenAI
