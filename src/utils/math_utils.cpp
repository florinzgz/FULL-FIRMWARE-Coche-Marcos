#include "math_utils.h"
#include <math.h>

using namespace MathUtils;

float MathUtils::mapf(float x, float inMin, float inMax, float outMin, float outMax) {
    if(inMax == inMin) return outMin;
    float t = (x - inMin) / (inMax - inMin);
    if(t < 0) t = 0;
    if(t > 1) t = 1;
    return outMin + t * (outMax - outMin);
}

float MathUtils::clamp(float x, float minV, float maxV) {
    if(x < minV) return minV;
    if(x > maxV) return maxV;
    return x;
}

float MathUtils::kmhToRpm(float kmh, float wheelCircumMm, float gearRatio) {
    // km/h -> m/s
    float ms = kmh / 3.6f;
    float revPerSecWheel = ms / (wheelCircumMm / 1000.0f);
    float revPerSecMotor = revPerSecWheel * gearRatio;
    return revPerSecMotor * 60.0f;
}

float MathUtils::rpmToKmh(float rpm, float wheelCircumMm, float gearRatio) {
    float revPerSecMotor = rpm / 60.0f;
    float revPerSecWheel = revPerSecMotor / gearRatio;
    float ms = revPerSecWheel * (wheelCircumMm / 1000.0f);
    return ms * 3.6f;
}

void MathUtils::ackermannFactors(float angleDeg, float maxSteerDeg, float &inner, float &outer) {
    float f = fabs(angleDeg) / maxSteerDeg;
    if(f > 1.0f) f = 1.0f;
    // reduce al interior hasta 30% según el ángulo
    inner = 1.0f - 0.3f * f;
    outer = 1.0f;
}

float MathUtils::ema(float prev, float x, float alpha) {
    return alpha * x + (1.0f - alpha) * prev;
}