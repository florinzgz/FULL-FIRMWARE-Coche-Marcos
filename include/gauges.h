#pragma once
#include "hud_layer.h" // Phase 10: RenderContext support
#include <TFT_eSPI.h>

namespace Gauges {
void init(TFT_eSPI *display);
// Phase 6: Added sprite parameter for compositor mode (nullptr = use TFT)
void drawSpeed(int cx, int cy, float kmh, int maxKmh, float pedalPct,
               TFT_eSprite *sprite = nullptr);
void drawRPM(int cx, int cy, float rpm, int maxRpm,
             TFT_eSprite *sprite = nullptr);

// Phase 10: RenderContext versions for dirty tracking
void drawSpeed(int cx, int cy, float kmh, int maxKmh, float pedalPct,
               HudLayer::RenderContext &ctx);
void drawRPM(int cx, int cy, float rpm, int maxRpm,
             HudLayer::RenderContext &ctx);
} // namespace Gauges