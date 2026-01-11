#pragma once
#include "hud_layer.h"
#include <TFT_eSPI.h>

/**
 * @file hud_graphics_telemetry.h
 * @brief Graphics performance telemetry overlay for HUD diagnostics
 *
 * PHASE 9 — RUNTIME GRAPHICS TELEMETRY & PERFORMANCE HUD
 *
 * Displays real-time performance metrics from the HUD compositor:
 * - FPS (frames per second)
 * - Frame time (ms)
 * - Dirty rectangle count and pixel area
 * - Memory bandwidth usage
 * - PSRAM usage by sprites
 * - Shadow mode status and statistics
 *
 * DESIGN PRINCIPLES:
 * - Zero performance penalty when hidden
 * - No heap allocations (fixed structures only)
 * - No String class usage
 * - No floating point arithmetic where possible
 * - Color-coded FPS indicators (GREEN/YELLOW/RED)
 * - Integrates as DIAGNOSTICS layer in compositor
 *
 * DISPLAY LOCATION:
 * X = 10, Y = 10, W = 220, H = 120
 *
 * VISIBILITY:
 * Displayed when hidden menu is open OR diagnostics mode is active.
 */

namespace HudGraphicsTelemetry {

/**
 * @brief Initialize the graphics telemetry overlay
 * @param tftDisplay Pointer to TFT_eSPI display instance
 */
void init(TFT_eSPI *tftDisplay);

/**
 * @brief Set the visibility of the telemetry overlay
 * @param visible true to show overlay, false to hide
 *
 * When hidden, the overlay does not render and has zero cost.
 */
void setVisible(bool visible);

/**
 * @brief Check if telemetry overlay is visible
 * @return true if visible, false if hidden
 */
bool isVisible();

/**
 * @brief Get the layer renderer for compositor registration
 *
 * COMPOSITOR INTEGRATION
 * This function returns a LayerRenderer that can be registered
 * with HudCompositor for the DIAGNOSTICS layer.
 *
 * @return Pointer to the layer renderer instance
 */
HudLayer::LayerRenderer *getRenderer();

} // namespace HudGraphicsTelemetry
