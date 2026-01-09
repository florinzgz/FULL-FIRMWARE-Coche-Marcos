#include "touch_map.h"
#include "icons.h" // para las coordenadas de cada icono
#include "logger.h"

// Touch calibration notes:
// - Current mapping uses hardcoded icon coordinates
// - If touch doesn't align with visible icons, calibration may be needed
// - XPT2046 touch controller may have different offsets on different ST7796S
// units ✅ v2.9.0: Dynamic calibration routine implemented in
// touch_calibration.cpp Access via Hidden Menu > Option 3 (Calibrar touch)

TouchAction getTouchedZone(int x, int y) {
  if (x >= Icons::BATTERY_X1 && x <= Icons::BATTERY_X2 &&
      y >= Icons::BATTERY_Y1 && y <= Icons::BATTERY_Y2)
    return TouchAction::Battery;
  // v2.14.0: Lights and Multimedia removed
  if (x >= Icons::MODE4X4_X1 && x <= Icons::MODE4X4_X2 &&
      y >= Icons::MODE4X4_Y1 && y <= Icons::MODE4X4_Y2)
    return TouchAction::Mode4x4;
  if (x >= Icons::WARNING_X1 && x <= Icons::WARNING_X2 &&
      y >= Icons::WARNING_Y1 && y <= Icons::WARNING_Y2)
    return TouchAction::Warning;

  return TouchAction::None;
}

// Ejemplo de uso
void checkTouch(int x, int y) {
  TouchAction action = getTouchedZone(x, y);
  switch (action) {
  case TouchAction::Battery:
    Logger::info("Tocado: Battery");
    break;
  // v2.14.0: Lights and Multimedia removed
  case TouchAction::Mode4x4:
    Logger::info("Tocado: 4x4");
    break;
  case TouchAction::Warning:
    Logger::info("Tocado: Warning");
    break;
  default:
    // Nada
    break;
  }
}