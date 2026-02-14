#include "mcp_shared.h"
#include "logger.h"
#include "pins.h"
#include "system.h"
#include <new> // For std::nothrow

/**
 * @deprecated This namespace is legacy code and not used in current firmware.
 * Use MCP23017Manager singleton instead for all MCP23017 operations.
 * Kept for backward compatibility only.
 */
namespace MCPShared {
#ifndef STANDALONE_DISPLAY
// 🔒 v2.18.4: BOOTLOOP FIX - Pointer-based lazy initialization
// Global constructor Adafruit_MCP23X17() was running before main(),
// which can cause early-boot crashes on ESP32-S3 OPI.
// Now using pointer allocated in init() after Arduino core is ready.
static Adafruit_MCP23X17 *mcpPtr = nullptr;
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

  // 🔒 v2.18.4: Allocate MCP23017 object on first init (lazy initialization)
  if (mcpPtr == nullptr) {
    mcpPtr = new (std::nothrow) Adafruit_MCP23X17();
    if (mcpPtr == nullptr) {
      Logger::error("MCPShared: MCP23017 allocation failed");
      System::logError(833);
      return false;
    }
  }

  // Intentar inicialización con retry
  if (!mcpPtr->begin_I2C(I2C_ADDR_MCP23017)) {
    Logger::error("MCPShared: MCP23017 init FAIL - retrying...");
    delay(50);
    if (!mcpPtr->begin_I2C(I2C_ADDR_MCP23017)) {
      Logger::error("MCPShared: MCP23017 init FAIL definitivo");
      System::logError(833); // Nuevo código: MCP shared init failure
      return false;
    }
  }

  // Configurar GPIOA0-A7 para tracción (IN1/IN2 motores)
  for (int pin = FIRST_DIR_PIN; pin <= LAST_DIR_PIN; pin++) {
    mcpPtr->pinMode(pin, OUTPUT);
    mcpPtr->digitalWrite(pin, LOW);
  }

  // Configurar GPIOB0-B4 para shifter (se completa en Shifter::init)
  // Los pull-ups se configuran en Shifter::init() por claridad

  initialized = true;
  Logger::info("MCPShared: MCP23017 init OK");
  return true;
#endif
}
} // namespace MCPShared
