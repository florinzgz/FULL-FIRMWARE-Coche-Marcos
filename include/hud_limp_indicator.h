#pragma once
#include <TFT_eSPI.h>
#include "limp_mode.h"

/**
 * @file hud_limp_indicator.h
 * @brief Non-intrusive HUD overlay for limp mode status
 * 
 * PHASE 4.2 — LIMP MODE HUD OVERLAY
 * 
 * Exposes the Limp-Mode state machine (Phase 4.1) to the driver via a small
 * visual indicator in the top-right corner of the HUD.
 * 
 * The car must never silently enter power-limited or safety mode without
 * visual feedback to the driver.
 * 
 * DESIGN PRINCIPLES:
 * - Non-intrusive: Does not overlap gauges or existing UI
 * - Efficient: Only redraws when state changes (not every frame)
 * - Read-only: Does not modify limp mode logic
 * - Safe: Survives sprite migration (Phase 7)
 */

namespace HudLimpIndicator {

/**
 * @brief Initialize the limp mode indicator
 * 
 * Must be called during HUD initialization after TFT is ready.
 * 
 * @param tftDisplay Pointer to TFT_eSPI display instance
 */
void init(TFT_eSPI* tftDisplay);

/**
 * @brief Update the limp mode indicator
 * 
 * Reads current limp mode state and redraws indicator ONLY if state changed.
 * Efficient: No rendering if state unchanged.
 * 
 * Call this every frame in HUD::update() after LimpMode::update().
 */
void draw();

/**
 * @brief Force redraw of indicator
 * 
 * Useful when screen has been cleared or after menu overlay.
 * Normally not needed - draw() handles state changes automatically.
 */
void forceRedraw();

/**
 * @brief Clear the indicator area
 * 
 * Erases the indicator box. Useful before entering menu screens.
 */
void clear();

} // namespace HudLimpIndicator
