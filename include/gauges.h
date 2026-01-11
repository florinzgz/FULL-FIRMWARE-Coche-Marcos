#pragma once
#include <TFT_eSPI.h>

namespace Gauges {
void init(TFT_eSPI *display);
// Phase 6: Added sprite parameter for compositor mode (nullptr = use TFT)
void drawSpeed(int cx, int cy, float kmh, int maxKmh, float pedalPct,
               TFT_eSprite *sprite = nullptr);
void drawRPM(int cx, int cy, float rpm, int maxRpm,
             TFT_eSprite *sprite = nullptr);
} // namespace Gauges