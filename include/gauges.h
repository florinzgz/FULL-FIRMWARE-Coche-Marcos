#pragma once
#include <TFT_eSPI.h>

namespace Gauges {
void init(TFT_eSPI *display);
void drawSpeed(int cx, int cy, float kmh, int maxKmh, float pedalPct);
void drawRPM(int cx, int cy, float rpm, int maxRpm);
} // namespace Gauges