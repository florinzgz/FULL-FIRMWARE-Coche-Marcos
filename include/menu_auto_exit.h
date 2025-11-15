#ifndef MENU_AUTO_EXIT_H
#define MENU_AUTO_EXIT_H

#include <Arduino.h>

class MenuAutoExit {
public:
    static void init();
    static void update();
    static void resetTimer();
    static bool shouldExit();
    static bool requiresPIN(const char* operation);
    static bool verifyPIN(uint16_t pin);
    
    static void enable();
    static void disable();
    static bool isEnabled();
    
    static void setTimeoutSeconds(uint8_t seconds);
    static uint8_t getTimeoutSeconds();
    static uint8_t getRemainingSeconds();
    
    static void setPIN(uint16_t pin);
    static uint16_t getPIN();
    
    static void draw();  // Draw countdown indicator
    
private:
    static unsigned long lastActivityTime;
    static uint8_t timeoutSeconds;
    static bool enabled;
    static uint16_t protectionPIN;
    static uint8_t failedAttempts;
    static unsigned long lockoutUntil;
    
    static const uint8_t MAX_ATTEMPTS = 3;
    static const unsigned long LOCKOUT_DURATION = 60000; // 1 minute
};

#endif
