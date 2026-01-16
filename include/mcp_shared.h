#pragma once
#include <Adafruit_MCP23X17.h>

namespace MCPShared {
#ifndef STANDALONE_DISPLAY
// Objeto compartido MCP23017 @ 0x20
// Not instantiated in STANDALONE_DISPLAY mode to prevent bootloop
extern Adafruit_MCP23X17 mcp;
#endif
extern bool initialized;

/**
 * @brief Inicializa MCP23017 compartido entre Traction y Shifter
 * @return true si inicialización exitosa
 * @note Solo inicializa una vez, llamadas subsecuentes retornan true sin
 * re-init
 */
bool init();
} // namespace MCPShared
