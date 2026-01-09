#pragma once

// ============================================================================
// touch_calibration.h - Touch screen calibration routine for XPT2046
// ============================================================================
// 🔒 v2.9.0: Dynamic touch calibration implementation
// Allows user to calibrate the touch screen by touching specific points
// and saves the calibration data to Storage::Config for persistence
// ============================================================================

#include <TFT_eSPI.h>

namespace TouchCalibration {

// Calibration routine state
enum class CalibrationState {
  Idle,         // Not calibrating
  Instructions, // Showing instructions
  Point1,       // Waiting for first corner (top-left)
  Point2,       // Waiting for second corner (bottom-right)
  Verification, // Verify calibration with center point
  Complete,     // Calibration successful
  Failed        // Calibration failed
};

// Calibration result structure
struct CalibrationResult {
  bool success;
  uint16_t calibData[5]; // [min_x, max_x, min_y, max_y, rotation]
  char message[64];
};

// Initialize calibration module
void init(TFT_eSPI *display);

// Start calibration routine (returns true if calibration started)
bool start();

// Update calibration routine (call in main loop when calibrating)
// Returns true if calibration is still in progress
bool update();

// Get current calibration state
CalibrationState getState();

// Check if calibration is active
bool isActive();

// Get calibration result (only valid after completion)
CalibrationResult getResult();

// Cancel calibration
void cancel();

// Apply calibration to TFT_eSPI and save to storage
bool applyCalibration(const uint16_t calibData[5]);

} // namespace TouchCalibration
