#pragma once
#include <Arduino.h>

namespace MathUtils {

// Mapear valor lineal con saturación
float mapf(float x, float inMin, float inMax, float outMin, float outMax);

// Constrain flotante
float clamp(float x, float minV, float maxV);

// Convertir km/h ↔ RPM (requiere circunferencia y relación)
float kmhToRpm(float kmh, float wheelCircumMm, float gearRatio);
float rpmToKmh(float rpm, float wheelCircumMm, float gearRatio);

// Ackermann básico: factor interior/exterior 0..1 según ángulo y límite de
// dirección
void ackermannFactors(float angleDeg, float maxSteerDeg, float &inner,
                      float &outer);

// Medias móviles y suavizado convenientes
float ema(float prev, float x, float alpha);
} // namespace MathUtils