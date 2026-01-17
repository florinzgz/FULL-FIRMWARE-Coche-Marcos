#pragma once

#include <Adafruit_MCP23X17.h>

/**
 * @brief Singleton manager for shared MCP23017 device (I2C 0x20)
 *
 * Prevents I2C conflicts by ensuring only ONE instance manages the device.
 * Multiple modules (shifter, traction, steering) share access via this manager.
 */
class MCP23017Manager {
public:
  static MCP23017Manager &getInstance();

  // Initialize MCP23017 - must be called once during system init
  bool init();

  // Get access to MCP23017 object (use carefully - prefer helper methods)
  Adafruit_MCP23X17 *getMCP();

  // Check if initialized successfully
  bool isOK() const { return mcpOK; }

  // Configure pin mode (wraps mcp.pinMode)
  void pinMode(uint8_t pin, uint8_t mode);

  // Digital write (wraps mcp.digitalWrite)
  void digitalWrite(uint8_t pin, uint8_t value);

  // Digital read (wraps mcp.digitalRead)
  uint8_t digitalRead(uint8_t pin);

private:
  MCP23017Manager() = default;
  ~MCP23017Manager() = default;

  // Prevent copying
  MCP23017Manager(const MCP23017Manager &) = delete;
  MCP23017Manager &operator=(const MCP23017Manager &) = delete;

#ifndef STANDALONE_DISPLAY
  // Only instantiate MCP23017 in full vehicle mode
  // In standalone mode, this peripheral is not needed and its
  // global constructor could cause bootloop issues
  Adafruit_MCP23X17 mcp;
#endif
  bool mcpOK = false;
  bool initialized = false;
};
