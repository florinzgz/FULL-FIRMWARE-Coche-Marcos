// src/control/traction.cpp - Complete implementation with safety fix

#include "traction.h"
#include "../sensors/shifter.h"
#include "../utils/logger.h"

// ... [previous code sections] ...

  // 🔒 FIX: Apply gear-based power limiting with safety protection
  float gearMultiplier = 1.0f;
  auto currentGear = Shifter::get().gear;
  
  if (currentGear == Shifter::P || currentGear == Shifter::N) {
    gearMultiplier = 0.0f;  // 0% in Park/Neutral - SAFETY: block all power
    Logger::debug("Traction: Park/Neutral - power blocked");
  } else if (currentGear == Shifter::D1) {
    gearMultiplier = 0.60f;  // 60% in D1 for difficult terrain
  } else if (currentGear == Shifter::D2) {
    gearMultiplier = 1.0f;   // 100% in D2 for speed
  } else if (currentGear == Shifter::R) {
    gearMultiplier = 0.30f;  // 30% in reverse for safety
  }

// ... [rest of the file] ...
