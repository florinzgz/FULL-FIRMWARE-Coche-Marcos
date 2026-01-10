#pragma once
#include <Arduino.h>

/**
 * @file limp_mode.h
 * @brief Fail-safe limp mode engine for vehicle safety
 * 
 * PHASE 4.1 — FAIL-SAFE LIMP MODE ENGINE
 * 
 * Provides centralized safety controller that prevents the car from becoming
 * dead or uncontrollable when sensors fail. The car always remains drivable
 * at reduced power ("drive-home mode") instead of freezing or shutting down.
 * 
 * This module does NOT change existing control logic directly.
 * It only limits outputs based on degradation level.
 */

namespace LimpMode {

/**
 * @brief System degradation states
 * 
 * Represents how badly the system is degraded and what safety limits apply.
 */
enum class LimpState {
  NORMAL,    // All systems operational, no limits
  DEGRADED,  // Minor sensor issues, apply soft limits
  LIMP,      // Significant failures, severe limits (drive-home mode)
  CRITICAL   // Critical failures, minimal operation only
};

/**
 * @brief Diagnostic information about current system state
 */
struct Diagnostics {
  // Current state
  LimpState state;
  
  // Input validity flags (from sensors)
  bool pedalValid;
  bool steeringValid;
  
  // System health indicators
  uint8_t systemErrorCount;
  bool batteryUndervoltage;
  bool temperatureWarning;
  bool steeringCentered;
  
  // Applied limits (percentage of normal)
  float powerLimit;       // 0.0-1.0 (multiplier for pedal output)
  float steeringLimit;    // 0.0-1.0 (multiplier for steering assist)
  float maxSpeedLimit;    // 0.0-1.0 (max allowed speed as fraction of normal)
  
  // State transition tracking
  uint32_t timeInStateMs;   // How long in current state
  uint32_t lastTransitionMs; // When last state change occurred
};

/**
 * @brief Initialize limp mode system
 * 
 * Must be called during system initialization, after Pedal, Steering,
 * and Sensors modules are initialized.
 */
void init();

/**
 * @brief Update limp mode state machine
 * 
 * Reads sensor validity, error counts, battery voltage, temperature warnings.
 * Determines current degradation level and applies appropriate limits.
 * 
 * Call this every frame in main loop, before control outputs are calculated.
 */
void update();

/**
 * @brief Get current limp mode state
 * 
 * @return LimpState Current degradation level
 */
LimpState getState();

/**
 * @brief Get full diagnostic information
 * 
 * @return Diagnostics Complete system state and limits
 */
Diagnostics getDiagnostics();

/**
 * @brief Apply power limit to pedal percentage
 * 
 * Takes desired pedal output and applies current safety limit.
 * 
 * @param pedalPercent Desired pedal output (0-100%)
 * @return float Limited pedal output (0-100%)
 * 
 * Example:
 *   float limited = LimpMode::limitPower(Pedal::get().percent);
 */
float limitPower(float pedalPercent);

/**
 * @brief Apply steering assist limit
 * 
 * Takes desired steering assist level and applies current safety limit.
 * 
 * @param assistLevel Desired assist level (0.0-1.0)
 * @return float Limited assist level (0.0-1.0)
 * 
 * Example:
 *   float limited = LimpMode::limitSteering(desiredAssist);
 */
float limitSteering(float assistLevel);

/**
 * @brief Apply speed limit
 * 
 * Takes desired speed and applies current safety limit.
 * 
 * @param speedKmh Desired speed (km/h)
 * @return float Limited speed (km/h)
 * 
 * Example:
 *   float limited = LimpMode::limitSpeed(requestedSpeed);
 */
float limitSpeed(float speedKmh);

/**
 * @brief Check if system is in safe operating state
 * 
 * @return true if NORMAL or DEGRADED (safe to drive)
 * @return false if LIMP or CRITICAL (warnings should be shown)
 */
bool isSafeToOperate();

/**
 * @brief Check if system allows full power
 * 
 * @return true if in NORMAL state (no limits)
 * @return false if any degradation active
 */
bool isFullPowerAllowed();

/**
 * @brief Force limp mode for testing
 * 
 * Manually set state for diagnostic purposes.
 * State will revert on next update() based on actual conditions.
 * 
 * @param state Desired test state
 */
void forceState(LimpState state);

/**
 * @brief Reset limp mode system
 * 
 * Clears state history and re-evaluates conditions.
 * Useful after clearing system errors.
 */
void reset();

} // namespace LimpMode
