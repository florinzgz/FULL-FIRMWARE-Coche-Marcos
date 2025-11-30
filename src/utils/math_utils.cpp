#include "math_utils.h"
#include <cmath>  // 🔒 v2.4.1: For std::isfinite()

using namespace MathUtils;

float MathUtils::mapf(float x, float inMin, float inMax, float outMin, float outMax) {
    // 🔒 v2.4.1: NaN/Inf validation
    if (!std::isfinite(x) || !std::isfinite(inMin) || !std::isfinite(inMax) || 
        !std::isfinite(outMin) || !std::isfinite(outMax)) {
        return outMin;  // Safe fallback
    }
    if(inMax == inMin) return outMin;
    float t = (x - inMin) / (inMax - inMin);
    if(t < 0) t = 0;
    if(t > 1) t = 1;
    return outMin + t * (outMax - outMin);
}

float MathUtils::clamp(float x, float minV, float maxV) {
    // 🔒 v2.4.1: NaN/Inf validation - check all parameters
    if (!std::isfinite(minV) || !std::isfinite(maxV)) {
        return 0.0f;  // Safe fallback when bounds are invalid
    }
    if (!std::isfinite(x)) return minV;  // Safe fallback for input
    if(x < minV) return minV;
    if(x > maxV) return maxV;
    return x;
}

float MathUtils::kmhToRpm(float kmh, float wheelCircumMm, float gearRatio) {
    // 🔒 v2.4.1: NaN/Inf and division-by-zero validation
    if (!std::isfinite(kmh) || !std::isfinite(wheelCircumMm) || 
        !std::isfinite(gearRatio) || wheelCircumMm <= 0.0f || gearRatio <= 0.0f) {
        return 0.0f;  // Safe fallback
    }
    // km/h -> m/s
    float ms = kmh / 3.6f;
    float revPerSecWheel = ms / (wheelCircumMm / 1000.0f);
    float revPerSecMotor = revPerSecWheel * gearRatio;
    return revPerSecMotor * 60.0f;
}

float MathUtils::rpmToKmh(float rpm, float wheelCircumMm, float gearRatio) {
    // 🔒 v2.4.1: NaN/Inf and division-by-zero validation
    if (!std::isfinite(rpm) || !std::isfinite(wheelCircumMm) || 
        !std::isfinite(gearRatio) || gearRatio <= 0.0f) {
        return 0.0f;  // Safe fallback
    }
    float revPerSecMotor = rpm / 60.0f;
    float revPerSecWheel = revPerSecMotor / gearRatio;
    float ms = revPerSecWheel * (wheelCircumMm / 1000.0f);
    return ms * 3.6f;
}

void MathUtils::ackermannFactors(float angleDeg, float maxSteerDeg, float &inner, float &outer) {
    // 🔒 v2.4.1: NaN/Inf and division-by-zero validation
    if (!std::isfinite(angleDeg) || !std::isfinite(maxSteerDeg) || maxSteerDeg <= 0.0f) {
        inner = 1.0f;
        outer = 1.0f;
        return;
    }
    float f = fabs(angleDeg) / maxSteerDeg;
    if(f > 1.0f) f = 1.0f;
    // Reduce inner wheel speed up to 30% based on steering angle
    inner = 1.0f - 0.3f * f;
    outer = 1.0f;
}

float MathUtils::ema(float prev, float x, float alpha) {
    // 🔒 v2.4.1: NaN/Inf validation
    if (!std::isfinite(prev) || !std::isfinite(x) || !std::isfinite(alpha)) {
        return std::isfinite(x) ? x : 0.0f;  // Use current value or 0
    }
    // Clamp alpha to valid range
    if (alpha < 0.0f) alpha = 0.0f;
    if (alpha > 1.0f) alpha = 1.0f;
    return alpha * x + (1.0f - alpha) * prev;
}