#pragma once
#include <Adafruit_MCP23X17.h>

/**
 * @deprecated This namespace is legacy code and not used in current firmware.
 * Shared MCP23017 handle (Adafruit_MCP23X17 @ 0x20) for Traction/Shifter.
 * Use MCP23017Manager singleton instead for all MCP23017 operations.
 * Kept for backward compatibility only; disabled in STANDALONE_DISPLAY.
 */
namespace MCPShared {
#ifndef STANDALONE_DISPLAY
// 🔒 v2.18.4: MCP23017 object is now allocated lazily in init()
// No global extern needed — pointer is file-local in mcp_shared.cpp
#endif
extern bool initialized;

/**
 * @brief Inicializa MCP23017 compartido entre Traction y Shifter
 * @return true si inicialización exitosa
 * @note Solo inicializa una vez, llamadas subsecuentes retornan true sin
 * re-init
 * @deprecated Use MCP23017Manager::getInstance().init() instead
 */
bool init();
} // namespace MCPShared
