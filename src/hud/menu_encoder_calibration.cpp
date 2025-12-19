/**
 * @file menu_encoder_calibration.cpp
 * @brief Encoder calibration menu for steering wheel calibration
 */

#include "menu_encoder_calibration.h"
#include "steering.h"
#include "config_storage.h"
#include "logger.h"
#include "alerts.h"

// Static member definitions
TFT_eSPI* MenuEncoderCalibration::tft = nullptr;
bool MenuEncoderCalibration::needsRedraw = true;
MenuEncoderCalibration::CalibrationStep MenuEncoderCalibration::currentStep = STEP_CENTER;
int32_t MenuEncoderCalibration::tempCenter = 600;
int32_t MenuEncoderCalibration::tempLeftLimit = 0;
int32_t MenuEncoderCalibration::tempRightLimit = 1200;
int32_t MenuEncoderCalibration::liveEncoderValue = 0;
uint32_t MenuEncoderCalibration::lastUpdateTime = 0;
bool MenuEncoderCalibration::saveConfirmed = false;
uint32_t MenuEncoderCalibration::saveConfirmTime = 0;

// Button bounds definitions
MenuEncoderCalibration::ButtonBounds MenuEncoderCalibration::btnSetCenter = {20, 180, 100, 35};
MenuEncoderCalibration::ButtonBounds MenuEncoderCalibration::btnSetLeft = {130, 180, 100, 35};
MenuEncoderCalibration::ButtonBounds MenuEncoderCalibration::btnSetRight = {240, 180, 100, 35};
MenuEncoderCalibration::ButtonBounds MenuEncoderCalibration::btnSave = {350, 180, 110, 35};
MenuEncoderCalibration::ButtonBounds MenuEncoderCalibration::btnReset = {20, 230, 150, 35};
MenuEncoderCalibration::ButtonBounds MenuEncoderCalibration::btnBack = {310, 230, 150, 35};

// Colors
static const uint16_t COLOR_BG = TFT_BLACK;
static const uint16_t COLOR_ACTIVE = TFT_GREEN;
static const uint16_t COLOR_INACTIVE = TFT_DARKGREY;
static const uint16_t COLOR_COMPLETE = TFT_BLUE;
static const uint16_t COLOR_WARNING = TFT_YELLOW;
static const uint16_t COLOR_ERROR = TFT_RED;
static const uint16_t COLOR_TEXT = TFT_WHITE;

void MenuEncoderCalibration::init(TFT_eSPI* display) {
    tft = display;
    loadCurrentCalibration();
    currentStep = STEP_CENTER;
    needsRedraw = true;
    saveConfirmed = false;
    Logger::info("MenuEncoderCalibration: initialized");
}

void MenuEncoderCalibration::update() {
    if (tft == nullptr) return;
    
    uint32_t now = millis();
    
    // Update live encoder value at 20Hz
    if (now - lastUpdateTime > 50) {
        lastUpdateTime = now;
        
        // Get live encoder reading (non-blocking)
        auto steer = Steering::get();
        int32_t newValue = steer.ticks;
        
        // Only update display if value changed significantly (reduces redraws)
        // This prevents excessive screen updates during rapid encoder changes
        if (abs(newValue - liveEncoderValue) > 0) {  // Update on any change
            liveEncoderValue = newValue;
            
            // Non-blocking partial updates instead of full redraws
            // Only redraw the changing parts to maintain responsiveness
            drawLiveValue();
            drawVisualIndicator();
            
            // Prevent needsRedraw flag from being set during rapid updates
            // This ensures UI remains responsive at 20Hz
        }
    }
    
    // Clear save confirmation after 2 seconds
    if (saveConfirmed && (now - saveConfirmTime > 2000)) {
        saveConfirmed = false;
        needsRedraw = true;
    }
}

void MenuEncoderCalibration::draw() {
    if (tft == nullptr || !needsRedraw) return;
    
    needsRedraw = false;
    
    // Clear screen
    tft->fillScreen(COLOR_BG);
    
    drawHeader();
    drawLiveValue();
    drawCalibrationSteps();
    drawVisualIndicator();
    drawButtons();
    drawInstructions();
}

void MenuEncoderCalibration::handleTouch(uint16_t x, uint16_t y) {
    if (tft == nullptr) return;
    
    // Check Set Center button
    if (x >= btnSetCenter.x && x <= btnSetCenter.x + btnSetCenter.w &&
        y >= btnSetCenter.y && y <= btnSetCenter.y + btnSetCenter.h) {
        handleSetCenter();
        return;
    }
    
    // Check Set Left button
    if (x >= btnSetLeft.x && x <= btnSetLeft.x + btnSetLeft.w &&
        y >= btnSetLeft.y && y <= btnSetLeft.y + btnSetLeft.h) {
        handleSetLeft();
        return;
    }
    
    // Check Set Right button
    if (x >= btnSetRight.x && x <= btnSetRight.x + btnSetRight.w &&
        y >= btnSetRight.y && y <= btnSetRight.y + btnSetRight.h) {
        handleSetRight();
        return;
    }
    
    // Check Save button
    if (x >= btnSave.x && x <= btnSave.x + btnSave.w &&
        y >= btnSave.y && y <= btnSave.y + btnSave.h) {
        handleSave();
        return;
    }
    
    // Check Reset button
    if (x >= btnReset.x && x <= btnReset.x + btnReset.w &&
        y >= btnReset.y && y <= btnReset.y + btnReset.h) {
        handleReset();
        return;
    }
    
    // Check Back button - handled externally
    if (x >= btnBack.x && x <= btnBack.x + btnBack.w &&
        y >= btnBack.y && y <= btnBack.y + btnBack.h) {
        handleBack();
        return;
    }
}

void MenuEncoderCalibration::drawHeader() {
    tft->setTextDatum(TC_DATUM);
    tft->setTextColor(TFT_CYAN, COLOR_BG);
    tft->drawString("ENCODER CALIBRATION", 240, 10, 4);
    
    tft->drawLine(20, 40, 460, 40, TFT_DARKGREY);
}

void MenuEncoderCalibration::drawLiveValue() {
    // Clear previous value area
    tft->fillRect(160, 50, 160, 40, COLOR_BG);
    
    // Draw label
    tft->setTextDatum(TR_DATUM);
    tft->setTextColor(COLOR_TEXT, COLOR_BG);
    tft->drawString("Live:", 155, 60, 2);
    
    // Draw live value in large font
    tft->setTextDatum(TL_DATUM);
    char valueStr[16];
    snprintf(valueStr, sizeof(valueStr), "%ld", liveEncoderValue);
    
    // Color based on current step target
    uint16_t valueColor = COLOR_TEXT;
    if (currentStep == STEP_CENTER) {
        valueColor = (abs(liveEncoderValue - tempCenter) < 10) ? COLOR_ACTIVE : COLOR_TEXT;
    } else if (currentStep == STEP_LEFT) {
        valueColor = (liveEncoderValue < tempCenter - 100) ? COLOR_ACTIVE : COLOR_WARNING;
    } else if (currentStep == STEP_RIGHT) {
        valueColor = (liveEncoderValue > tempCenter + 100) ? COLOR_ACTIVE : COLOR_WARNING;
    }
    
    tft->setTextColor(valueColor, COLOR_BG);
    tft->drawString(valueStr, 165, 55, 4);
}

void MenuEncoderCalibration::drawCalibrationSteps() {
    int y = 100;
    int stepSpacing = 25;
    
    // Draw step indicators
    drawStepIndicator(STEP_CENTER, "Center", tempCenter, currentStep == STEP_CENTER);
    
    tft->setTextDatum(TL_DATUM);
    y += stepSpacing;
    drawStepIndicator(STEP_LEFT, "Left Limit", tempLeftLimit, currentStep == STEP_LEFT);
    
    y += stepSpacing;
    drawStepIndicator(STEP_RIGHT, "Right Limit", tempRightLimit, currentStep == STEP_RIGHT);
}

void MenuEncoderCalibration::drawStepIndicator(uint8_t step, const char* label, int32_t value, bool active) {
    int y = 100 + step * 25;
    int iconX = 30;
    int labelX = 55;
    int valueX = 200;
    
    // Step status icon
    bool complete = (step < currentStep) || (currentStep == STEP_COMPLETE);
    uint16_t iconColor = getStepColor(active, complete);
    
    tft->fillCircle(iconX, y + 6, 8, iconColor);
    if (complete) {
        // Draw checkmark
        tft->drawLine(iconX - 4, y + 6, iconX - 1, y + 9, COLOR_BG);
        tft->drawLine(iconX - 1, y + 9, iconX + 5, y + 3, COLOR_BG);
    } else if (active) {
        // Draw dot
        tft->fillCircle(iconX, y + 6, 3, COLOR_BG);
    }
    
    // Step label
    tft->setTextDatum(TL_DATUM);
    tft->setTextColor(active ? COLOR_ACTIVE : COLOR_TEXT, COLOR_BG);
    tft->drawString(label, labelX, y, 2);
    
    // Step value
    char valueStr[16];
    snprintf(valueStr, sizeof(valueStr), "%ld", value);
    tft->setTextDatum(TR_DATUM);
    tft->setTextColor(complete ? COLOR_COMPLETE : (active ? COLOR_ACTIVE : COLOR_INACTIVE), COLOR_BG);
    tft->drawString(valueStr, valueX, y, 2);
}

void MenuEncoderCalibration::drawButtons() {
    // Set Center button
    uint16_t centerColor = (currentStep == STEP_CENTER) ? COLOR_ACTIVE : COLOR_INACTIVE;
    tft->fillRoundRect(btnSetCenter.x, btnSetCenter.y, btnSetCenter.w, btnSetCenter.h, 5, centerColor);
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(COLOR_BG, centerColor);
    tft->drawString("Set Center", btnSetCenter.x + btnSetCenter.w/2, btnSetCenter.y + btnSetCenter.h/2, 2);
    
    // Set Left button
    uint16_t leftColor = (currentStep == STEP_LEFT) ? COLOR_WARNING : COLOR_INACTIVE;
    tft->fillRoundRect(btnSetLeft.x, btnSetLeft.y, btnSetLeft.w, btnSetLeft.h, 5, leftColor);
    tft->setTextColor(COLOR_BG, leftColor);
    tft->drawString("Set Left", btnSetLeft.x + btnSetLeft.w/2, btnSetLeft.y + btnSetLeft.h/2, 2);
    
    // Set Right button
    uint16_t rightColor = (currentStep == STEP_RIGHT) ? COLOR_WARNING : COLOR_INACTIVE;
    tft->fillRoundRect(btnSetRight.x, btnSetRight.y, btnSetRight.w, btnSetRight.h, 5, rightColor);
    tft->setTextColor(COLOR_BG, rightColor);
    tft->drawString("Set Right", btnSetRight.x + btnSetRight.w/2, btnSetRight.y + btnSetRight.h/2, 2);
    
    // Save button
    uint16_t saveColor = (currentStep == STEP_COMPLETE) ? COLOR_ACTIVE : 
                         (saveConfirmed ? TFT_BLUE : COLOR_INACTIVE);
    tft->fillRoundRect(btnSave.x, btnSave.y, btnSave.w, btnSave.h, 5, saveColor);
    tft->setTextColor(COLOR_TEXT, saveColor);
    tft->drawString(saveConfirmed ? "SAVED!" : "Save", btnSave.x + btnSave.w/2, btnSave.y + btnSave.h/2, 2);
    
    // Reset button
    tft->fillRoundRect(btnReset.x, btnReset.y, btnReset.w, btnReset.h, 5, COLOR_ERROR);
    tft->setTextColor(COLOR_TEXT, COLOR_ERROR);
    tft->drawString("Reset Defaults", btnReset.x + btnReset.w/2, btnReset.y + btnReset.h/2, 2);
    
    // Back button
    tft->fillRoundRect(btnBack.x, btnBack.y, btnBack.w, btnBack.h, 5, TFT_DARKGREY);
    tft->setTextColor(COLOR_TEXT, TFT_DARKGREY);
    tft->drawString("Back", btnBack.x + btnBack.w/2, btnBack.y + btnBack.h/2, 2);
}

void MenuEncoderCalibration::drawInstructions() {
    tft->setTextDatum(BC_DATUM);
    tft->setTextColor(COLOR_WARNING, COLOR_BG);
    tft->drawString(getStepInstruction(), 240, 295, 2);
}

void MenuEncoderCalibration::drawVisualIndicator() {
    // Visual bar showing encoder position
    int barX = 220;
    int barY = 100;
    int barW = 240;
    int barH = 60;
    
    // Clear area
    tft->fillRect(barX, barY, barW, barH, COLOR_BG);
    
    // Draw frame
    tft->drawRect(barX, barY, barW, barH, TFT_DARKGREY);
    
    // Calculate position indicator
    int32_t range = tempRightLimit - tempLeftLimit;
    
    // Guard against division by zero - use safe default range
    if (range <= 0) {
        range = 1200;  // Default range
    }
    
    int32_t pos = liveEncoderValue - tempLeftLimit;
    int32_t centerOffset = tempCenter - tempLeftLimit;
    
    // Map to bar width (range is guaranteed to be > 0 here)
    int markerX = barX + 5 + (int)((pos * (barW - 10)) / range);
    int centerX = barX + 5 + (int)((centerOffset * (barW - 10)) / range);
    
    // Clamp marker position
    markerX = constrain(markerX, barX + 5, barX + barW - 10);
    centerX = constrain(centerX, barX + 5, barX + barW - 10);
    
    // Draw center line
    tft->drawLine(centerX, barY + 5, centerX, barY + barH - 5, TFT_CYAN);
    
    // Draw left/right zones
    tft->fillRect(barX + 2, barY + 25, centerX - barX - 4, 10, COLOR_WARNING);
    tft->fillRect(centerX + 2, barY + 25, barX + barW - centerX - 4, 10, COLOR_WARNING);
    
    // Draw position marker
    tft->fillTriangle(markerX - 8, barY + 50, markerX + 8, barY + 50, markerX, barY + 38, COLOR_ACTIVE);
}

void MenuEncoderCalibration::handleSetCenter() {
    tempCenter = liveEncoderValue;
    currentStep = STEP_LEFT;
    needsRedraw = true;
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
    Logger::infof("Encoder center set: %ld", tempCenter);
    
    // Validate center is within reasonable bounds
    if (tempCenter < -2000 || tempCenter > 4000) {
        Logger::warnf("Encoder center value unusual: %ld (expected range -2000 to 4000)", tempCenter);
    }
}

void MenuEncoderCalibration::handleSetLeft() {
    tempLeftLimit = liveEncoderValue;
    
    // Validate left limit is less than center
    if (tempLeftLimit >= tempCenter) {
        Logger::errorf("Invalid left limit: %ld >= center %ld. Please turn wheel LEFT from center.", 
                      tempLeftLimit, tempCenter);
        Alerts::play({Audio::AUDIO_ERROR, Audio::Priority::PRIO_HIGH});
        return; // Don't advance step
    }
    
    currentStep = STEP_RIGHT;
    needsRedraw = true;
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
    Logger::infof("Encoder left limit set: %ld", tempLeftLimit);
}

void MenuEncoderCalibration::handleSetRight() {
    tempRightLimit = liveEncoderValue;
    
    // Validate right limit is greater than center
    if (tempRightLimit <= tempCenter) {
        Logger::errorf("Invalid right limit: %ld <= center %ld. Please turn wheel RIGHT from center.", 
                      tempRightLimit, tempCenter);
        Alerts::play({Audio::AUDIO_ERROR, Audio::Priority::PRIO_HIGH});
        return; // Don't advance step
    }
    
    // Validate the range is reasonable (at least 100 ticks on each side)
    int32_t leftRange = tempCenter - tempLeftLimit;
    int32_t rightRange = tempRightLimit - tempCenter;
    
    if (leftRange < 100 || rightRange < 100) {
        Logger::warnf("Calibration range seems small: left=%ld, right=%ld (expected >100 each)", 
                     leftRange, rightRange);
    }
    
    currentStep = STEP_COMPLETE;
    needsRedraw = true;
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
    Logger::infof("Encoder right limit set: %ld", tempRightLimit);
}

void MenuEncoderCalibration::handleSave() {
    saveCalibration();
    saveConfirmed = true;
    saveConfirmTime = millis();
    needsRedraw = true;
    Alerts::play({Audio::AUDIO_ENCODER_OK, Audio::Priority::PRIO_HIGH});
}

void MenuEncoderCalibration::handleReset() {
    resetCalibration();
    needsRedraw = true;
    Alerts::play({Audio::AUDIO_MODULO_OK, Audio::Priority::PRIO_NORMAL});
}

void MenuEncoderCalibration::handleBack() {
    // This should be handled by parent menu
    // Reset state for next entry
    currentStep = STEP_CENTER;
    needsRedraw = true;
}

uint16_t MenuEncoderCalibration::getStepColor(bool active, bool complete) {
    if (complete) return COLOR_COMPLETE;
    if (active) return COLOR_ACTIVE;
    return COLOR_INACTIVE;
}

const char* MenuEncoderCalibration::getStepInstruction() {
    switch (currentStep) {
        case STEP_CENTER:
            return "Center the steering wheel, then press 'Set Center'";
        case STEP_LEFT:
            return "Turn wheel full LEFT, then press 'Set Left'";
        case STEP_RIGHT:
            return "Turn wheel full RIGHT, then press 'Set Right'";
        case STEP_COMPLETE:
            return "Calibration complete! Press 'Save' to store.";
        default:
            return "";
    }
}

void MenuEncoderCalibration::loadCurrentCalibration() {
    auto& config = ConfigStorage::getCurrentConfig();
    tempCenter = config.encoder_center;
    tempLeftLimit = config.encoder_left_limit;
    tempRightLimit = config.encoder_right_limit;
    liveEncoderValue = tempCenter;
    Logger::info("Loaded encoder calibration from storage");
}

void MenuEncoderCalibration::saveCalibration() {
    // Final validation before saving
    if (tempLeftLimit >= tempCenter || tempRightLimit <= tempCenter) {
        Logger::errorf("Invalid calibration: left=%ld, center=%ld, right=%ld", 
                      tempLeftLimit, tempCenter, tempRightLimit);
        Alerts::play({Audio::AUDIO_ERROR, Audio::Priority::PRIO_HIGH});
        return;
    }
    
    // Check for reasonable range
    int32_t totalRange = tempRightLimit - tempLeftLimit;
    if (totalRange < 200) {
        Logger::warnf("Calibration range too small: %ld (expected >200)", totalRange);
    }
    
    auto& config = ConfigStorage::getCurrentConfig();
    config.encoder_center = tempCenter;
    config.encoder_left_limit = tempLeftLimit;
    config.encoder_right_limit = tempRightLimit;
    
    // Save to EEPROM with error handling
    bool saveSuccess = ConfigStorage::save(config);
    
    if (!saveSuccess) {
        Logger::error("Failed to save encoder calibration to EEPROM");
        Alerts::play({Audio::AUDIO_ERROR, Audio::Priority::PRIO_HIGH});
        return;
    }
    
    // Verify the save by reading back
    auto& verifyConfig = ConfigStorage::getCurrentConfig();
    if (verifyConfig.encoder_center != tempCenter ||
        verifyConfig.encoder_left_limit != tempLeftLimit ||
        verifyConfig.encoder_right_limit != tempRightLimit) {
        Logger::error("EEPROM verification failed - saved values don't match");
        Alerts::play({Audio::AUDIO_ERROR, Audio::Priority::PRIO_HIGH});
        return;
    }
    
    // Apply to steering module
    Steering::setZeroOffset(tempCenter);
    
    Logger::infof("Encoder calibration saved and verified: C=%ld, L=%ld, R=%ld", 
                  tempCenter, tempLeftLimit, tempRightLimit);
}

void MenuEncoderCalibration::resetCalibration() {
    tempCenter = ConfigStorage::DEFAULT_CONFIG.encoder_center;
    tempLeftLimit = ConfigStorage::DEFAULT_CONFIG.encoder_left_limit;
    tempRightLimit = ConfigStorage::DEFAULT_CONFIG.encoder_right_limit;
    currentStep = STEP_CENTER;
    Logger::info("Encoder calibration reset to defaults");
}
