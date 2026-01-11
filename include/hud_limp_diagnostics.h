#pragma once
#include "limp_mode.h"
#include <TFT_eSPI.h>

/**
 * @file hud_limp_diagnostics.h
 * @brief Diagnostic HUD overlay for limp mode details
 *
 * PHASE 4.3 — LIMP MODE DIAGNOSTICS HUD
 *
 * Shows the driver WHY the car is limited when LimpMode is not NORMAL.
 * Phase 4.1 decides the limits.
 * Phase 4.2 indicates there is a problem.
 * Phase 4.3 explains the reason and what limits are active.
 *
 * The HUD must be a mirror of the safety engine, not parallel logic.
 *
 * DESIGN PRINCIPLES:
 * - Data source: ONLY LimpMode::getDiagnostics()
 * - No sensor reading, no config reading, no inference
 * - Efficient: Only redraws when diagnostics change (not every frame)
 * - Read-only: Does not modify limp mode logic
 * - Shows detailed breakdown of system health and active limits
 */

namespace HudLimpDiagnostics {

/**
 * @brief Initialize the limp mode diagnostics display
 *
 * Must be called during HUD initialization after TFT is ready.
 *
 * @param tftDisplay Pointer to TFT_eSPI display instance
 */
void init(TFT_eSPI *tftDisplay);

/**
 * @brief Update the diagnostics display
 *
 * Reads current limp mode diagnostics and redraws ONLY if changed.
 * Only displays when state != NORMAL.
 * Efficient: No rendering if diagnostics unchanged.
 *
 * Call this every frame in HUD::update() after LimpMode::update().
 */
void draw();

/**
 * @brief Force redraw of diagnostics
 *
 * Useful when screen has been cleared or after menu overlay.
 * Normally not needed - draw() handles changes automatically.
 */
void forceRedraw();

/**
 * @brief Clear the diagnostics area
 *
 * Erases the diagnostics display. Useful before entering menu screens.
 */
void clear();

} // namespace HudLimpDiagnostics
