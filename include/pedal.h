#pragma once
#include <Arduino.h>

namespace Pedal {
struct State {
  int raw;       // Lectura ADC cruda
  float percent; // 0-100 % tras curva y deadband
  bool valid;    // Plausibilidad OK
};

void init();
void update();

// Nuevo: estado de inicialización
bool initOK();

// Calibración
void setCalibration(int minAdc, int maxAdc, uint8_t curve);
void getCalibration(int &minAdc, int &maxAdc, uint8_t &curve);

// Deadband (en % de escala)
void setDeadband(float percent);
float getDeadband();

const State &get();
} // namespace Pedal