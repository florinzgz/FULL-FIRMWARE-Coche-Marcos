#pragma once
#include <Adafruit_MCP23X17.h>

namespace MCPShared {
// Objeto compartido MCP23017 @ 0x20
extern Adafruit_MCP23X17 mcp;
extern bool initialized;

/**
 * @brief Inicializa MCP23017 compartido entre Traction y Shifter
 * @return true si inicialización exitosa
 * @note Solo inicializa una vez, llamadas subsecuentes retornan true sin
 * re-init
 */
bool init();
} // namespace MCPShared
