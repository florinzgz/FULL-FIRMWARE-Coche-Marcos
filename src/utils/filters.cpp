#include "filters.h"
#include <stdlib.h>
#include <string.h>

using namespace Filters;

// MovingAverage
MovingAverage::MovingAverage(size_t window)
    : buf(nullptr), win(window), idx(0), count(0), sum(0.0f) {
  if (win == 0) win = 1;
  buf = (float *)malloc(sizeof(float) * win);
  // 🔒 CRITICAL FIX: Check malloc success
  if (buf == nullptr) {
    // Allocation failed - set window to 0 to prevent crashes
    // Log error to serial for debugging (Logger may not be initialized yet)
    Serial.printf("[ERROR] MovingAverage malloc failed for %zu bytes\n", sizeof(float) * win);
    win = 0;
    count = 0;
    return;
  }
  memset(buf, 0, sizeof(float) * win);
}

MovingAverage::~MovingAverage() {
  if (buf) free(buf);
}

void MovingAverage::reset() {
  idx = 0;
  count = 0;
  sum = 0.0f;
  // 🔒 CRITICAL FIX: Check buffer is valid before use
  if (buf == nullptr) return;
  memset(buf, 0, sizeof(float) * win);
}

void MovingAverage::push(float v) {
  // 🔒 CRITICAL FIX: Check buffer is valid before use
  if (buf == nullptr || win == 0) return;

  if (count < win) {
    buf[count++] = v;
    sum += v;
  } else {
    sum -= buf[idx];
    buf[idx] = v;
    sum += v;
    idx = (idx + 1) % win;
  }
}

float MovingAverage::value() const {
  if (count == 0) return 0.0f;
  return sum / (float)count;
}

bool MovingAverage::full() const { return count == win; }

// LowPass
LowPass::LowPass(float alpha) : a(alpha), y(0.0f), initialized(false) {}

void LowPass::reset(float initial) {
  y = initial;
  initialized = true;
}

float LowPass::apply(float x) {
  if (!initialized) {
    y = x;
    initialized = true;
  }
  y = a * x + (1.0f - a) * y;
  return y;
}

float LowPass::value() const { return y; }

// HighPass
HighPass::HighPass(float alpha)
    : a(alpha), y(0.0f), prevX(0.0f), prevY(0.0f), initialized(false) {}

void HighPass::reset(float initial) {
  y = initial;
  prevX = initial;
  prevY = initial;
  initialized = true;
}

float HighPass::apply(float x) {
  if (!initialized) {
    prevX = x;
    prevY = 0.0f;
    initialized = true;
  }
  y = a * (y + x - prevX);
  prevX = x;
  prevY = y;
  return y;
}

float HighPass::value() const { return y; }

// RateLimiter
RateLimiter::RateLimiter(float risePerSec, float fallPerSec)
    : rise(risePerSec), fall(fallPerSec), y(0.0f), initialized(false) {}

void RateLimiter::reset(float initial) {
  y = initial;
  initialized = true;
}

float RateLimiter::apply(float target, float dtSec) {
  if (!initialized) {
    y = target;
    initialized = true;
    return y;
  }
  float delta = target - y;
  float maxRise = rise * dtSec;
  float maxFall = fall * dtSec;
  if (delta > maxRise) delta = maxRise;
  if (delta < -maxFall) delta = -maxFall;
  y += delta;
  return y;
}

float RateLimiter::value() const { return y; }
