#include "touch_calibration.h"
#include "logger.h"
#include "storage.h"
#include "pins.h"
#include <Arduino.h>

// External reference to global config
extern Storage::Config cfg;

namespace TouchCalibration {
    
    // Module state
    static TFT_eSPI* tft = nullptr;
    static CalibrationState state = CalibrationState::Idle;
    static CalibrationResult result;
    static uint32_t stateStartTime = 0;
    
    // Touch release tracking
    static bool waitingForRelease = false;
    static uint32_t releaseWaitStart = 0;
    
    // Calibration points (screen coordinates)
    static const int CALIB_MARGIN = 30;  // Margin from screen edge
    static const int CALIB_RADIUS = 10;  // Radius of calibration target
    
    // Touch data for calibration points
    static uint16_t point1_rawX = 0, point1_rawY = 0;  // Top-left corner
    static uint16_t point2_rawX = 0, point2_rawY = 0;  // Bottom-right corner
    
    // Touch sampling
    static const int SAMPLE_COUNT = 10;        // Number of samples to average
    static const uint32_t SAMPLE_INTERVAL = 50; // ms between samples
    static int samplesCollected = 0;
    static uint32_t sumX = 0, sumY = 0;
    static uint32_t lastSampleTime = 0;
    
    // Timeout constants
    static const uint32_t INSTRUCTION_TIMEOUT = 30000;  // 30 seconds
    static const uint32_t POINT_TIMEOUT = 30000;         // 30 seconds per point
    
    // Touch controller constants
    static const uint16_t MAX_TOUCH_VALUE = 4095;        // 12-bit ADC maximum
    static const uint32_t TOUCH_RELEASE_WAIT = 500;      // ms to wait for touch release
    
    // Forward declarations
    static void drawCalibrationPoint(int x, int y, uint16_t color);
    static void drawInstructions();
    static bool collectTouchSample(uint16_t& avgX, uint16_t& avgY);
    static void calculateCalibration();
    static void drawVerification();
    
    void init(TFT_eSPI* display) {
        tft = display;
        state = CalibrationState::Idle;
        Logger::info("TouchCalibration: Module initialized");
    }
    
    bool start() {
        if (tft == nullptr) {
            Logger::error("TouchCalibration: TFT not initialized");
            return false;
        }
        
        if (state != CalibrationState::Idle) {
            Logger::warn("TouchCalibration: Already calibrating");
            return false;
        }
        
        Logger::info("TouchCalibration: Starting calibration routine");
        state = CalibrationState::Instructions;
        stateStartTime = millis();
        result.success = false;
        
        // Reset sampling
        samplesCollected = 0;
        sumX = sumY = 0;
        
        // Draw instructions
        drawInstructions();
        
        return true;
    }
    
    bool update() {
        if (state == CalibrationState::Idle || 
            state == CalibrationState::Complete || 
            state == CalibrationState::Failed) {
            return false;  // Not calibrating
        }
        
        uint32_t now = millis();
        
        // Non-blocking touch release wait
        if (waitingForRelease) {
            uint16_t tx, ty;
            if (!tft->getTouch(&tx, &ty)) {
                // Touch released
                waitingForRelease = false;
            } else if (now - releaseWaitStart > TOUCH_RELEASE_WAIT) {
                // Timeout waiting for release
                Logger::warn("TouchCalibration: Timeout waiting for touch release");
                waitingForRelease = false;
            }
            return true;  // Still waiting
        }
        
        switch (state) {
            case CalibrationState::Instructions: {
                // Wait for any touch to proceed
                uint16_t tx, ty;
                if (tft->getTouch(&tx, &ty)) {
                    // Touch detected - initiate non-blocking wait for release
                    waitingForRelease = true;
                    releaseWaitStart = now;
                    
                    // Start calibration
                    state = CalibrationState::Point1;
                    stateStartTime = now;
                    samplesCollected = 0;
                    sumX = sumY = 0;
                    
                    // Draw first calibration point (top-left)
                    tft->fillScreen(TFT_BLACK);
                    tft->setTextColor(TFT_WHITE, TFT_BLACK);
                    tft->setTextDatum(TC_DATUM);
                    tft->drawString("Touch the RED target", 240, 10, 2);
                    tft->drawString("Point 1 of 2", 240, 30, 2);
                    drawCalibrationPoint(CALIB_MARGIN, CALIB_MARGIN, TFT_RED);
                }
                
                // Check timeout
                if (now - stateStartTime > INSTRUCTION_TIMEOUT) {
                    Logger::warn("TouchCalibration: Instruction timeout");
                    state = CalibrationState::Failed;
                    snprintf(result.message, sizeof(result.message), "Timeout waiting for touch");
                }
                break;
            }
            
            case CalibrationState::Point1: {
                // Collect samples for first point
                if (collectTouchSample(point1_rawX, point1_rawY)) {
                    Logger::infof("TouchCalibration: Point 1 collected (raw: %d, %d)", 
                                 point1_rawX, point1_rawY);
                    
                    // Initiate non-blocking wait for touch release
                    waitingForRelease = true;
                    releaseWaitStart = now;
                    
                    // Move to second point
                    state = CalibrationState::Point2;
                    stateStartTime = now;
                    samplesCollected = 0;
                    sumX = sumY = 0;
                    
                    // Draw second calibration point (bottom-right)
                    tft->fillScreen(TFT_BLACK);
                    tft->setTextColor(TFT_WHITE, TFT_BLACK);
                    tft->setTextDatum(TC_DATUM);
                    tft->drawString("Touch the RED target", 240, 10, 2);
                    tft->drawString("Point 2 of 2", 240, 30, 2);
                    drawCalibrationPoint(480 - CALIB_MARGIN, 320 - CALIB_MARGIN, TFT_RED);
                }
                
                // Check timeout
                if (now - stateStartTime > POINT_TIMEOUT) {
                    Logger::warn("TouchCalibration: Point 1 timeout");
                    state = CalibrationState::Failed;
                    snprintf(result.message, sizeof(result.message), "Timeout on point 1");
                }
                break;
            }
            
            case CalibrationState::Point2: {
                // Collect samples for second point
                if (collectTouchSample(point2_rawX, point2_rawY)) {
                    Logger::infof("TouchCalibration: Point 2 collected (raw: %d, %d)", 
                                 point2_rawX, point2_rawY);
                    
                    // Calculate calibration
                    calculateCalibration();
                    
                    // Show success
                    state = CalibrationState::Verification;
                    stateStartTime = now;
                    drawVerification();
                }
                
                // Check timeout
                if (now - stateStartTime > POINT_TIMEOUT) {
                    Logger::warn("TouchCalibration: Point 2 timeout");
                    state = CalibrationState::Failed;
                    snprintf(result.message, sizeof(result.message), "Timeout on point 2");
                }
                break;
            }
            
            case CalibrationState::Verification: {
                // Wait for touch to confirm or timeout to auto-complete
                uint16_t tx, ty;
                if (tft->getTouch(&tx, &ty) || (now - stateStartTime > 3000)) {
                    state = CalibrationState::Complete;
                    Logger::info("TouchCalibration: Calibration complete!");
                }
                break;
            }
            
            default:
                break;
        }
        
        return true;  // Still calibrating
    }
    
    CalibrationState getState() {
        return state;
    }
    
    bool isActive() {
        return (state != CalibrationState::Idle && 
                state != CalibrationState::Complete && 
                state != CalibrationState::Failed);
    }
    
    CalibrationResult getResult() {
        return result;
    }
    
    void cancel() {
        if (isActive()) {
            Logger::info("TouchCalibration: Cancelled by user");
            state = CalibrationState::Failed;
            snprintf(result.message, sizeof(result.message), "Cancelled");
        } else {
            state = CalibrationState::Idle;
        }
    }
    
    bool applyCalibration(const uint16_t calibData[5]) {
        if (tft == nullptr) {
            Logger::error("TouchCalibration: TFT not initialized");
            return false;
        }
        
        // Apply to TFT_eSPI (need non-const pointer)
        uint16_t tempCalibData[5];
        for (int i = 0; i < 5; i++) {
            tempCalibData[i] = calibData[i];
        }
        tft->setTouch(tempCalibData);
        
        // Save to storage
        for (int i = 0; i < 5; i++) {
            cfg.touchCalibration[i] = calibData[i];
        }
        cfg.touchCalibrated = true;
        
        if (Storage::save(cfg)) {
            Logger::info("TouchCalibration: Calibration saved to storage");
            return true;
        } else {
            Logger::error("TouchCalibration: Failed to save calibration");
            return false;
        }
    }
    
    // ========================================================================
    // Internal helper functions
    // ========================================================================
    
    static void drawCalibrationPoint(int x, int y, uint16_t color) {
        // Draw crosshair target
        tft->fillCircle(x, y, CALIB_RADIUS, color);
        tft->drawCircle(x, y, CALIB_RADIUS + 5, TFT_WHITE);
        tft->drawLine(x - 20, y, x - 10, y, TFT_WHITE);
        tft->drawLine(x + 10, y, x + 20, y, TFT_WHITE);
        tft->drawLine(x, y - 20, x, y - 10, TFT_WHITE);
        tft->drawLine(x, y + 10, x, y + 20, TFT_WHITE);
    }
    
    static void drawInstructions() {
        tft->fillScreen(TFT_BLACK);
        tft->setTextColor(TFT_YELLOW, TFT_BLACK);
        tft->setTextDatum(TC_DATUM);
        
        tft->drawString("TOUCH CALIBRATION", 240, 60, 4);
        
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->setTextDatum(TL_DATUM);
        
        int y = 120;
        tft->drawString("You will be asked to touch", 60, y, 2); y += 25;
        tft->drawString("2 points on the screen.", 60, y, 2); y += 25;
        y += 25;  // Blank line for spacing
        tft->drawString("Touch accurately for best", 60, y, 2); y += 25;
        tft->drawString("results.", 60, y, 2); y += 30;
        
        tft->setTextColor(TFT_GREEN, TFT_BLACK);
        tft->setTextDatum(TC_DATUM);
        tft->drawString("Touch anywhere to start", 240, 260, 2);
    }
    
    static bool collectTouchSample(uint16_t& avgX, uint16_t& avgY) {
        uint32_t now = millis();
        uint16_t tx, ty;
        
        // Get raw touch values (before calibration)
        if (tft->getTouch(&tx, &ty)) {
            if (now - lastSampleTime >= SAMPLE_INTERVAL) {
                sumX += tx;
                sumY += ty;
                samplesCollected++;
                lastSampleTime = now;
                
                // Draw progress indicator
                int progress = (samplesCollected * 100) / SAMPLE_COUNT;
                tft->fillRect(100, 290, 280, 20, TFT_BLACK);
                tft->drawRect(100, 290, 280, 20, TFT_WHITE);
                tft->fillRect(102, 292, (progress * 276) / 100, 16, TFT_GREEN);
                
                if (samplesCollected >= SAMPLE_COUNT) {
                    // Calculate average
                    avgX = sumX / samplesCollected;
                    avgY = sumY / samplesCollected;
                    
                    // Reset for next point
                    samplesCollected = 0;
                    sumX = sumY = 0;
                    
                    return true;  // Sample collection complete
                }
            }
        }
        
        return false;  // Still collecting
    }
    
    static void calculateCalibration() {
        // Calculate calibration based on two corner points
        // Point 1: Top-left (CALIB_MARGIN, CALIB_MARGIN)
        // Point 2: Bottom-right (480 - CALIB_MARGIN, 320 - CALIB_MARGIN)
        
        uint16_t minX = point1_rawX;
        uint16_t maxX = point2_rawX;
        uint16_t minY = point1_rawY;
        uint16_t maxY = point2_rawY;
        
        // Ensure proper ordering
        if (minX > maxX) {
            uint16_t temp = minX;
            minX = maxX;
            maxX = temp;
        }
        if (minY > maxY) {
            uint16_t temp = minY;
            minY = maxY;
            maxY = temp;
        }
        
        // Apply margin compensation
        // The screen coordinates we touched were CALIB_MARGIN from edge
        // So we need to extrapolate to the actual edges (0, 0) and (480, 320)
        float scaleX = (float)(480) / (float)(480 - 2 * CALIB_MARGIN);
        float scaleY = (float)(320) / (float)(320 - 2 * CALIB_MARGIN);
        
        uint16_t rangeX = maxX - minX;
        uint16_t rangeY = maxY - minY;
        
        uint16_t extrapolateX = (uint16_t)(rangeX * (scaleX - 1.0f) / 2.0f);
        uint16_t extrapolateY = (uint16_t)(rangeY * (scaleY - 1.0f) / 2.0f);
        
        minX = (minX > extrapolateX) ? (minX - extrapolateX) : 0;
        maxX = ((uint32_t)maxX + (uint32_t)extrapolateX < MAX_TOUCH_VALUE) ? (maxX + extrapolateX) : MAX_TOUCH_VALUE;
        minY = ((uint32_t)minY > (uint32_t)extrapolateY) ? (minY - extrapolateY) : 0;
        maxY = ((uint32_t)maxY + (uint32_t)extrapolateY < MAX_TOUCH_VALUE) ? (maxY + extrapolateY) : MAX_TOUCH_VALUE;
        
        // Store calibration result
        result.calibData[0] = minX;
        result.calibData[1] = maxX;
        result.calibData[2] = minY;
        result.calibData[3] = maxY;
        result.calibData[4] = tft->getRotation();  // Store current display rotation
        result.success = true;
        
        Logger::infof("TouchCalibration: Calculated calibration [%d, %d, %d, %d, %d]",
                     minX, maxX, minY, maxY, result.calibData[4]);
        
        snprintf(result.message, sizeof(result.message), "Calibration successful!");
    }
    
    static void drawVerification() {
        tft->fillScreen(TFT_BLACK);
        tft->setTextColor(TFT_GREEN, TFT_BLACK);
        tft->setTextDatum(TC_DATUM);
        
        tft->drawString("CALIBRATION COMPLETE!", 240, 100, 4);
        
        tft->setTextColor(TFT_WHITE, TFT_BLACK);
        tft->setTextDatum(TL_DATUM);
        
        int y = 160;
        tft->drawString("Calibration values:", 60, y, 2); y += 25;
        
        char buf[64];
        snprintf(buf, sizeof(buf), "Min X: %d", result.calibData[0]);
        tft->drawString(buf, 60, y, 2); y += 20;
        snprintf(buf, sizeof(buf), "Max X: %d", result.calibData[1]);
        tft->drawString(buf, 60, y, 2); y += 20;
        snprintf(buf, sizeof(buf), "Min Y: %d", result.calibData[2]);
        tft->drawString(buf, 60, y, 2); y += 20;
        snprintf(buf, sizeof(buf), "Max Y: %d", result.calibData[3]);
        tft->drawString(buf, 60, y, 2); y += 25;
        
        tft->setTextColor(TFT_CYAN, TFT_BLACK);
        tft->setTextDatum(TC_DATUM);
        tft->drawString("Saving calibration...", 240, 280, 2);
        
        // Apply and save calibration
        applyCalibration(result.calibData);
    }
    
}  // namespace TouchCalibration
