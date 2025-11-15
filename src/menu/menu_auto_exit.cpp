#include "menu_auto_exit.h"
#include "config_storage.h"
// #include "display.h"  // Display module not yet implemented

// Static members
unsigned long MenuAutoExit::lastActivityTime = 0;
uint8_t MenuAutoExit::timeoutSeconds = 30;
bool MenuAutoExit::enabled = true;
uint16_t MenuAutoExit::protectionPIN = 8989;
uint8_t MenuAutoExit::failedAttempts = 0;
unsigned long MenuAutoExit::lockoutUntil = 0;

void MenuAutoExit::init() {
    // Load settings from EEPROM
    auto config = ConfigStorage::getCurrentConfig();
    timeoutSeconds = 30;  // Default 30 seconds
    protectionPIN = 8989; // Default PIN
    enabled = true;
    
    lastActivityTime = millis();
    failedAttempts = 0;
    lockoutUntil = 0;
}

void MenuAutoExit::update() {
    // Non-blocking update, just tracks time
    // Actual exit handled by shouldExit()
}

void MenuAutoExit::resetTimer() {
    lastActivityTime = millis();
}

bool MenuAutoExit::shouldExit() {
    if (!enabled) return false;
    
    unsigned long elapsed = millis() - lastActivityTime;
    return (elapsed > (timeoutSeconds * 1000UL));
}

bool MenuAutoExit::requiresPIN(const char* operation) {
    // Critical operations that require PIN
    if (strcmp(operation, "power_config") == 0) return true;
    if (strcmp(operation, "sensor_disable") == 0) return true;
    if (strcmp(operation, "encoder_calibration") == 0) return true;
    if (strcmp(operation, "factory_reset") == 0) return true;
    
    return false;
}

bool MenuAutoExit::verifyPIN(uint16_t pin) {
    // Check lockout
    if (millis() < lockoutUntil) {
        return false;
    }
    
    if (pin == protectionPIN) {
        failedAttempts = 0;
        return true;
    }
    
    failedAttempts++;
    if (failedAttempts >= MAX_ATTEMPTS) {
        lockoutUntil = millis() + LOCKOUT_DURATION;
        failedAttempts = 0;
    }
    
    return false;
}

void MenuAutoExit::enable() {
    enabled = true;
    resetTimer();
}

void MenuAutoExit::disable() {
    enabled = false;
}

bool MenuAutoExit::isEnabled() {
    return enabled;
}

void MenuAutoExit::setTimeoutSeconds(uint8_t seconds) {
    timeoutSeconds = seconds;
    resetTimer();
}

uint8_t MenuAutoExit::getTimeoutSeconds() {
    return timeoutSeconds;
}

uint8_t MenuAutoExit::getRemainingSeconds() {
    if (!enabled) return 0;
    
    unsigned long elapsed = millis() - lastActivityTime;
    unsigned long remaining = (timeoutSeconds * 1000UL) - elapsed;
    
    if (remaining > (timeoutSeconds * 1000UL)) return 0;
    
    return (uint8_t)(remaining / 1000);
}

void MenuAutoExit::setPIN(uint16_t pin) {
    protectionPIN = pin;
}

uint16_t MenuAutoExit::getPIN() {
    return protectionPIN;
}

void MenuAutoExit::draw() {
    if (!enabled) return;
    
    uint8_t remaining = getRemainingSeconds();
    
    // Display module not yet implemented - draw() disabled temporarily
    /*
    // Draw countdown in top-right corner
    Display::fillRect(Display::width() - 60, 5, 55, 20, TFT_BLACK);
    Display::setTextColor(remaining < 10 ? TFT_RED : TFT_YELLOW);
    Display::setTextSize(2);
    Display::setCursor(Display::width() - 55, 10);
    Display::printf("%02ds", remaining);
    
    // Draw progress bar
    uint16_t barWidth = 50;
    uint16_t fillWidth = (barWidth * remaining) / timeoutSeconds;
    Display::drawRect(Display::width() - 55, 30, barWidth, 5, TFT_WHITE);
    Display::fillRect(Display::width() - 55, 30, fillWidth, 5, 
                     remaining < 10 ? TFT_RED : TFT_GREEN);
    */
}
