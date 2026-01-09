#ifndef MENU_ENCODER_CALIBRATION_H
#define MENU_ENCODER_CALIBRATION_H

#include "config_storage.h"
#include "steering.h"
#include <Arduino.h>
#include <TFT_eSPI.h>

/**
 * Encoder Calibration Menu
 *
 * Allows user to calibrate steering wheel encoder center position and
 * left/right limits Critical for precise steering control and Ackermann
 * geometry calculations
 *
 * Features:
 * - Live encoder value display
 * - 3-step calibration process (Center → Left Max → Right Max)
 * - Visual feedback with current/target positions
 * - Save to EEPROM with ConfigStorage
 * - Reset to defaults option
 *
 * Calibration Process:
 * 1. Center wheel → Press "Set Center"
 * 2. Turn full left → Press "Set Left Limit"
 * 3. Turn full right → Press "Set Right Limit"
 * 4. Press "Save" to store in EEPROM
 */

class MenuEncoderCalibration {
public:
  static void init(TFT_eSPI *display);
  static void update();
  static void draw();
  static void handleTouch(uint16_t x, uint16_t y);

private:
  static TFT_eSPI *tft;
  static bool needsRedraw;

  // Calibration state
  enum CalibrationStep {
    STEP_CENTER = 0,
    STEP_LEFT = 1,
    STEP_RIGHT = 2,
    STEP_COMPLETE = 3
  };

  static CalibrationStep currentStep;
  static int32_t tempCenter;
  static int32_t tempLeftLimit;
  static int32_t tempRightLimit;
  static int32_t liveEncoderValue;

  // UI state
  static uint32_t lastUpdateTime;
  static bool saveConfirmed;
  static uint32_t saveConfirmTime;

  // Drawing functions
  static void drawHeader();
  static void drawLiveValue();
  static void drawCalibrationSteps();
  static void drawStepIndicator(uint8_t step, const char *label, int32_t value,
                                bool active);
  static void drawButtons();
  static void drawInstructions();
  static void drawVisualIndicator();

  // Button handlers
  static void handleSetCenter();
  static void handleSetLeft();
  static void handleSetRight();
  static void handleSave();
  static void handleReset();
  static void handleBack();

  // Helper functions
  static uint16_t getStepColor(bool active, bool complete);
  static const char *getStepInstruction();
  static void loadCurrentCalibration();
  static void saveCalibration();
  static void resetCalibration();

  // Button bounds
  struct ButtonBounds {
    uint16_t x, y, w, h;
  };

  static ButtonBounds btnSetCenter;
  static ButtonBounds btnSetLeft;
  static ButtonBounds btnSetRight;
  static ButtonBounds btnSave;
  static ButtonBounds btnReset;
  static ButtonBounds btnBack;
};

#endif // MENU_ENCODER_CALIBRATION_H
