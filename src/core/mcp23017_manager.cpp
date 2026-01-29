#include "mcp23017_manager.h"
#include "logger.h"
#include "pins.h"
#include "system.h"
#include <Arduino.h>

// Retry timing for I2C device initialization
static constexpr uint32_t I2C_RETRY_INTERVAL_MS =
    50; // Non-blocking retry interval
static constexpr uint8_t MAX_RETRY_ATTEMPTS =
    3; // Maximum retry attempts before permanent failure

// 🔒 CRITICAL v2.18.3: Spinlock for thread-safe mutex creation
static portMUX_TYPE mcp_spinlock = portMUX_INITIALIZER_UNLOCKED;

// Get singleton instance
MCP23017Manager &MCP23017Manager::getInstance() {
  static MCP23017Manager instance;
  return instance;
}

// Initialize MCP23017 with REAL non-blocking retry logic
bool MCP23017Manager::init() {
#ifdef STANDALONE_DISPLAY
  // In standalone mode, MCP23017 is not used
  Logger::info("MCP23017Manager: Skipped in STANDALONE_DISPLAY mode");
  initialized = true;
  mcpOK = false;
  return true;
#else
  // 🔒 CRITICAL v2.18.3: Create I2C mutex on first initialization (thread-safe)
  // Use critical section to prevent race condition if both cores call init
  // simultaneously
  if (i2cMutex == nullptr) {
    portENTER_CRITICAL(&mcp_spinlock);
    if (i2cMutex == nullptr) { // Double-check after acquiring lock
      i2cMutex = xSemaphoreCreateMutex();
      if (i2cMutex == nullptr) {
        portEXIT_CRITICAL(&mcp_spinlock);
        Logger::error("MCP23017Manager: Failed to create I2C mutex!");
        return false;
      }
      Logger::info(
          "MCP23017Manager: I2C mutex created for dual-core protection");
    }
    portEXIT_CRITICAL(&mcp_spinlock);
  }

  // Static variables persist across calls for retry state
  static uint32_t retryTime = 0;
  static bool retrying = false;
  static uint8_t retryCount = 0;

  // If already successfully initialized, return current state
  if (initialized) { return mcpOK; }

  // First attempt or not currently retrying
  if (!retrying) {
    // 🔒 CRITICAL v2.18.3: Protect I2C bus access during initialization
    if (i2cMutex != nullptr &&
        xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      mcpOK = mcp.begin_I2C(I2C_ADDR_MCP23017);
      xSemaphoreGive(i2cMutex);
    } else {
      Logger::error("MCP23017Manager: Init - I2C mutex timeout");
      mcpOK = false;
    }

    if (!mcpOK) {
      Logger::error("MCP23017Manager: Init FAIL - will retry asynchronously");
      retrying = true;
      retryTime = millis();
      retryCount = 1; // First retry
    } else {
      Logger::info("MCP23017Manager: Init successful");
      initialized = true;
      retryCount = 0;
      return true;
    }
  }

  // Handle scheduled retry
  if (retrying && (millis() - retryTime >= I2C_RETRY_INTERVAL_MS)) {
    // 🔒 CRITICAL v2.18.3: Protect I2C bus access during retry
    if (i2cMutex != nullptr &&
        xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      mcpOK = mcp.begin_I2C(I2C_ADDR_MCP23017);
      xSemaphoreGive(i2cMutex);
    } else {
      Logger::error("MCP23017Manager: Retry - I2C mutex timeout");
      mcpOK = false;
    }

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
        Logger::error(
            "MCP23017Manager: Retry FAIL definitivo after 3 attempts");
        System::logError(833); // MCP23017 shared init fail
        initialized = true; // Mark as initialized to prevent further retry spam
        retrying = false;
        retryCount = 0;
        return false;
      } else {
        // Schedule another retry
        Logger::warnf(
            "MCP23017Manager: Retry %d/%d failed, scheduling next retry",
            retryCount, MAX_RETRY_ATTEMPTS);
        retryTime = millis();
        // Keep retrying = true
      }
    }
  }

  // Not yet initialized successfully
  return false;
#endif
}

// Get direct access to MCP object (use with caution)
Adafruit_MCP23X17 *MCP23017Manager::getMCP() {
#ifdef STANDALONE_DISPLAY
  Logger::warn("MCP23017Manager: getMCP() called in STANDALONE_DISPLAY mode");
  return nullptr;
#else
  if (!mcpOK) {
    Logger::warn("MCP23017Manager: getMCP() called but device not initialized");
    return nullptr;
  }
  return &mcp;
#endif
}

// Wrapper for pinMode
void MCP23017Manager::pinMode(uint8_t pin, uint8_t mode) {
#ifdef STANDALONE_DISPLAY
  // No-op in standalone mode
  return;
#else
  if (!mcpOK) {
    Logger::errorf(
        "MCP23017Manager: pinMode() called but device not ready (pin=%d)", pin);
    return;
  }

  // 🔒 CRITICAL v2.18.3: Protect I2C access with mutex
  if (i2cMutex != nullptr &&
      xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    mcp.pinMode(pin, mode);
    xSemaphoreGive(i2cMutex);
  } else {
    Logger::errorf("MCP23017Manager: pinMode() I2C mutex timeout (pin=%d)",
                   pin);
  }
#endif
}

// Wrapper for digitalWrite
void MCP23017Manager::digitalWrite(uint8_t pin, uint8_t value) {
#ifdef STANDALONE_DISPLAY
  // No-op in standalone mode
  return;
#else
  if (!mcpOK) {
    Logger::errorf(
        "MCP23017Manager: digitalWrite() called but device not ready (pin=%d)",
        pin);
    return;
  }

  // 🔒 CRITICAL v2.18.3: Protect I2C access with mutex
  if (i2cMutex != nullptr &&
      xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    mcp.digitalWrite(pin, value);
    xSemaphoreGive(i2cMutex);
  } else {
    Logger::errorf("MCP23017Manager: digitalWrite() I2C mutex timeout (pin=%d)",
                   pin);
  }
#endif
}

// Wrapper for digitalRead
uint8_t MCP23017Manager::digitalRead(uint8_t pin) {
#ifdef STANDALONE_DISPLAY
  // Return 0 in standalone mode
  return 0;
#else
  if (!mcpOK) {
    Logger::errorf(
        "MCP23017Manager: digitalRead() called but device not ready (pin=%d)",
        pin);
    return 0;
  }

  // 🔒 CRITICAL v2.18.3: Protect I2C access with mutex
  // NOTE: On timeout, returns 0 (LOW) as fail-safe default
  // This is intentional for safety-critical operations like shifter inputs
  // where a timeout should default to the safe/inactive state
  uint8_t result = 0;
  if (i2cMutex != nullptr &&
      xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
    result = mcp.digitalRead(pin);
    xSemaphoreGive(i2cMutex);
  } else {
    Logger::errorf("MCP23017Manager: digitalRead() I2C mutex timeout (pin=%d), "
                   "returning fail-safe LOW",
                   pin);
  }
  return result;
#endif
}
