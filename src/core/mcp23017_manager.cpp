#include "mcp23017_manager.h"
#include "logger.h"
#include "pins.h"
#include "system.h"
#include <Arduino.h>

// Retry timing for I2C device initialization
static constexpr uint32_t I2C_RETRY_INTERVAL_MS = 50;  // Non-blocking retry interval
static constexpr uint8_t MAX_RETRY_ATTEMPTS = 3;       // Maximum retry attempts before permanent failure

// Get singleton instance
MCP23017Manager& MCP23017Manager::getInstance() {
    static MCP23017Manager instance;
    return instance;
}

// Initialize MCP23017 with REAL non-blocking retry logic
bool MCP23017Manager::init() {
    // Static variables persist across calls for retry state
    static uint32_t retryTime = 0;
    static bool retrying = false;
    static uint8_t retryCount = 0;
    
    // If already successfully initialized, return current state
    if (initialized) {
        return mcpOK;
    }
    
    // First attempt or not currently retrying
    if (!retrying) {
        mcpOK = mcp.begin_I2C(I2C_ADDR_MCP23017);
        
        if (!mcpOK) {
            Logger::error("MCP23017Manager: Init FAIL - will retry asynchronously");
            retrying = true;
            retryTime = millis();
            retryCount = 1;  // First retry
        } else {
            Logger::info("MCP23017Manager: Init successful");
            initialized = true;
            retryCount = 0;
            return true;
        }
    }
    
    // Handle scheduled retry
    if (retrying && (millis() - retryTime >= I2C_RETRY_INTERVAL_MS)) {
        mcpOK = mcp.begin_I2C(I2C_ADDR_MCP23017);
        
        if (mcpOK) {
            Logger::info("MCP23017Manager: Init successful on retry");
            initialized = true;
            retrying = false;
            retryCount = 0;
            return true;
        } else {
            retryCount++;
            
            if (retryCount >= MAX_RETRY_ATTEMPTS) {
                // Permanent failure after max retries
                Logger::error("MCP23017Manager: Retry FAIL definitivo after 3 attempts");
                System::logError(833);  // MCP23017 shared init fail
                initialized = true;  // Mark as initialized to prevent further retry spam
                retrying = false;
                retryCount = 0;
                return false;
            } else {
                // Schedule another retry
                Logger::warnf("MCP23017Manager: Retry %d/%d failed, scheduling next retry", 
                             retryCount, MAX_RETRY_ATTEMPTS);
                retryTime = millis();
                // Keep retrying = true
            }
        }
    }
    
    // Not yet initialized successfully
    return false;
}

// Get direct access to MCP object (use with caution)
Adafruit_MCP23X17* MCP23017Manager::getMCP() {
    if (!mcpOK) {
        Logger::warn("MCP23017Manager: getMCP() called but device not initialized");
        return nullptr;
    }
    return &mcp;
}

// Wrapper for pinMode
void MCP23017Manager::pinMode(uint8_t pin, uint8_t mode) {
    if (!mcpOK) {
        Logger::errorf("MCP23017Manager: pinMode() called but device not ready (pin=%d)", pin);
        return;
    }
    mcp.pinMode(pin, mode);
}

// Wrapper for digitalWrite
void MCP23017Manager::digitalWrite(uint8_t pin, uint8_t value) {
    if (!mcpOK) {
        Logger::errorf("MCP23017Manager: digitalWrite() called but device not ready (pin=%d)", pin);
        return;
    }
    mcp.digitalWrite(pin, value);
}

// Wrapper for digitalRead
uint8_t MCP23017Manager::digitalRead(uint8_t pin) {
    if (!mcpOK) {
        Logger::errorf("MCP23017Manager: digitalRead() called but device not ready (pin=%d)", pin);
        return 0;
    }
    return mcp.digitalRead(pin);
}
