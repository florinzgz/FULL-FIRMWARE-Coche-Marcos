#include "steering_model.h"
#include <cmath>

static float L = 0.95f;        // distancia entre ejes (m)
static float T = 0.70f;        // vía (m)
static float MAX_INNER = 54.0f; // ángulo máximo interior (°)

void SteeringModel::setGeometry(float wheelbase_m, float track_m, float maxInnerDeg) {
    L = wheelbase_m;
    T = track_m;
    MAX_INNER = maxInnerDeg;
}

static inline float clamp(float x, float lo, float hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

SteeringModel::Ackermann SteeringModel::compute(float wheelAngleDeg) {
    SteeringModel::Ackermann out{0.0f, 0.0f};
    float sign = (wheelAngleDeg >= 0.0f) ? 1.0f : -1.0f;
    float innerDeg = std::fabs(wheelAngleDeg);
    innerDeg = clamp(innerDeg, 0.0f, MAX_INNER);

    if(innerDeg < 1e-3f) {
        out.innerDeg = 0.0f;
        out.outerDeg = 0.0f;
        return out;
    }

    float innerRad = innerDeg * (float)M_PI / 180.0f;
    float R = (L / std::tan(innerRad)) + (T * 0.5f);
    float outerRad = std::atan(L / (R + (T * 0.5f)));
    float outerDeg = outerRad * 180.0f / (float)M_PI;

    out.innerDeg = sign * innerDeg;
    out.outerDeg = sign * outerDeg;
    return out;
}