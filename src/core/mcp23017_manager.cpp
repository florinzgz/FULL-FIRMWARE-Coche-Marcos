#include "mcp23017_manager.h"
#include "logger.h"
#include "pins.h"
#include "system.h"

// Get singleton instance
MCP23017Manager& MCP23017Manager::getInstance() {
    static MCP23017Manager instance;
    return instance;
}

// Initialize MCP23017 with non-blocking retry logic
bool MCP23017Manager::init() {
    if (initialized) {
        Logger::warn("MCP23017Manager: Already initialized");
        return mcpOK;
    }
    
    Logger::info("MCP23017Manager: Initializing shared MCP23017 (0x20)...");
    
    // First attempt
    mcpOK = mcp.begin_I2C(I2C_ADDR_MCP23017);
    
    if (!mcpOK) {
        Logger::error("MCP23017Manager: First init attempt FAILED - will retry asynchronously");
        // Don't use delay() - just mark as failed for now
        // The retry will happen through subsequent calls or system retry mechanism
        System::logError(833);  // MCP23017 shared init fail
    } else {
        Logger::info("MCP23017Manager: Init successful on first attempt");
    }
    
    initialized = true;
    return mcpOK;
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
