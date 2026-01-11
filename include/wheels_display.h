#pragma once
#include <TFT_eSPI.h>
// Opcional: incluir Arduino.h si se usan utilidades de Arduino en el futuro
// #include <Arduino.h>

namespace WheelsDisplay {
// Inicializa el módulo con el puntero a la pantalla
void init(TFT_eSPI *display);

// Dibuja una rueda en (cx, cy) con:
// - angleDeg: ángulo de dirección en grados
// - tempC: temperatura en °C
// - effortPct: esfuerzo en porcentaje (0–100)
// Phase 6.3: Added sprite parameter for compositor mode (nullptr = use TFT)
void drawWheel(int cx, int cy, float angleDeg, float tempC, float effortPct,
               TFT_eSprite *sprite = nullptr);
} // namespace WheelsDisplay