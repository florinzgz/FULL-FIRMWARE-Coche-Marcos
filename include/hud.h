#pragma once
#include "hud_layer.h" // Phase 10: RenderContext support
#include <TFT_eSPI.h>

namespace HUD {
void init();
void showLogo();
void showReady();
void showError();
// Phase 10: Accept RenderContext for granular dirty tracking
// Still support legacy sprite parameter for backwards compatibility
void update(TFT_eSprite *sprite = nullptr);
void update(HudLayer::RenderContext &ctx);

void drawPedalBar(
    float pedalPercent,
    TFT_eSprite *sprite = nullptr); // Barra de pedal en parte inferior
void drawPedalBar(
    float pedalPercent,
    HudLayer::RenderContext &ctx); // Phase 10: RenderContext version

// Control de giro sobre eje (axis rotation)
void toggleAxisRotation();
bool isAxisRotationEnabled();
} // namespace HUD