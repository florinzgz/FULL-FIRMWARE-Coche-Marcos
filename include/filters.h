#pragma once
#include <Arduino.h>

namespace Filters {

    // Media móvil simple de ventana fija
    class MovingAverage {
    public:
        MovingAverage(size_t window);
        ~MovingAverage();

        void reset();
        void push(float v);
        float value() const;   // promedio actual
        bool full() const;     // ventana llena

    private:
        float *buf;
        size_t win;
        size_t idx;
        size_t count;
        float sum;
    };

    // Filtro paso bajo (exponencial) y paso alto derivados
    class LowPass {
    public:
        LowPass(float alpha = 0.2f); // 0..1 (más bajo = más suave)
        void reset(float initial = 0.0f);
        float apply(float x);
        float value() const;

    private:
        float a;
        float y;
        bool initialized;
    };

    class HighPass {
    public:
        HighPass(float alpha = 0.2f);
        void reset(float initial = 0.0f);
        float apply(float x);
        float value() const;

    private:
        float a;
        float y;
        float prevX;
        float prevY;
        bool initialized;
    };

    // Limitador con ataque/liberación (%/s o unidades/s)
    class RateLimiter {
    public:
        RateLimiter(float risePerSec, float fallPerSec);
        void reset(float initial = 0.0f);
        float apply(float target, float dtSec);
        float value() const;

    private:
        float rise;
        float fall;
        float y;
        bool initialized;
    };
}