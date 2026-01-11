#pragma once
#include <TFT_eSPI.h>

namespace HUD {
void init();
void showLogo();
void showReady();
void showError();
// Phase 6.4: Added sprite parameter for compositor mode (nullptr = use TFT)
void update(TFT_eSprite *sprite = nullptr);
void drawPedalBar(
    float pedalPercent,
    TFT_eSprite *sprite = nullptr); // Barra de pedal en parte inferior

// Control de giro sobre eje (axis rotation)
void toggleAxisRotation();
bool isAxisRotationEnabled();
} // namespace HUD