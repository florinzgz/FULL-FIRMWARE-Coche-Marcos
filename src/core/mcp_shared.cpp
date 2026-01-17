#include "mcp_shared.h"
#include "logger.h"
#include "pins.h"
#include "system.h"

namespace MCPShared {
#ifndef STANDALONE_DISPLAY
// Only instantiate MCP23017 in full vehicle mode
// In standalone mode, this peripheral is not needed and its
// global constructor could cause bootloop issues
Adafruit_MCP23X17 mcp;
#endif

bool initialized = false;

// Pin range constants for motor direction control
constexpr int FIRST_DIR_PIN = MCP_PIN_FL_IN1;
constexpr int LAST_DIR_PIN = MCP_PIN_RR_IN2;

bool init() {
#ifdef STANDALONE_DISPLAY
  // In standalone mode, MCP23017 is not used
  Logger::info("MCPShared: Skipped in STANDALONE_DISPLAY mode");
  initialized = true;
  return true;
#else
  if (initialized) {
    Logger::info("MCPShared: Already initialized");
    return true;
  }

  // Intentar inicialización con retry
  if (!mcp.begin_I2C(I2C_ADDR_MCP23017)) {
    Logger::error("MCPShared: MCP23017 init FAIL - retrying...");
    delay(50);
    if (!mcp.begin_I2C(I2C_ADDR_MCP23017)) {
      Logger::error("MCPShared: MCP23017 init FAIL definitivo");
      System::logError(833); // Nuevo código: MCP shared init failure
      return false;
    }
  }

  // Configurar GPIOA0-A7 para tracción (IN1/IN2 motores)
  for (int pin = FIRST_DIR_PIN; pin <= LAST_DIR_PIN; pin++) {
    mcp.pinMode(pin, OUTPUT);
    mcp.digitalWrite(pin, LOW);
  }

  // Configurar GPIOB0-B4 para shifter (se completa en Shifter::init)
  // Los pull-ups se configuran en Shifter::init() por claridad

  initialized = true;
  Logger::info("MCPShared: MCP23017 init OK");
  return true;
#endif
}
} // namespace MCPShared
