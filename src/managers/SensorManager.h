// managers/SensorManager.h
// Sensor management - integrates all sensor subsystems
#pragma once

#include "../../include/buttons.h"
#include "../../include/current.h"
#include "../../include/pedal.h"
#include "../../include/sensors.h"
#include "../../include/shifter.h"
#include "../../include/steering.h"
#include "../../include/temperature.h"
#include "../../include/wheels.h"

namespace SensorManager {
// Check if sensors are disabled at compile time
inline bool sensorsDisabled() {
#ifdef DISABLE_SENSORS
  return true;
#else
  return false;
#endif
}

inline bool init() {
#ifdef DISABLE_SENSORS
  // 🔒 v2.11.6: BOOTLOOP FIX - Skip sensor initialization in DISABLE_SENSORS
  // mode
  Serial.println("[SensorManager] DISABLE_SENSORS mode - sensors disabled");
  return true; // Return success to allow boot to continue (sensors are
               // intentionally disabled)
#else
  // Initialize input devices (always needed for vehicle operation)
  Pedal::init();
  Steering::init();
  Shifter::init();
  Buttons::init();

  // Initialize sensor subsystems
  Sensors::init();
  return Sensors::initOK();
#endif
}

inline void update() {
#ifndef DISABLE_SENSORS
  Sensors::update();

  // Update input devices
  Pedal::update();
  Steering::update();
  Shifter::update();
  Buttons::update();
#endif
}
} // namespace SensorManager
