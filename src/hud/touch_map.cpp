#include "touch_map.h"
#include "icons.h"   // para las coordenadas de cada icono
#include "logger.h"

TouchAction getTouchedZone(int x, int y) {
    if (x >= Icons::BATTERY_X1 && x <= Icons::BATTERY_X2 && y >= Icons::BATTERY_Y1 && y <= Icons::BATTERY_Y2)
        return TouchAction::Battery;
    if (x >= Icons::LIGHTS_X1 && x <= Icons::LIGHTS_X2 && y >= Icons::LIGHTS_Y1 && y <= Icons::LIGHTS_Y2)
        return TouchAction::Lights;
    if (x >= Icons::MEDIA_X1 && x <= Icons::MEDIA_X2 && y >= Icons::MEDIA_Y1 && y <= Icons::MEDIA_Y2)
        return TouchAction::Multimedia;
    if (x >= Icons::MODE4X4_X1 && x <= Icons::MODE4X4_X2 && y >= Icons::MODE4X4_Y1 && y <= Icons::MODE4X4_Y2)
        return TouchAction::Mode4x4;
    if (x >= Icons::WARNING_X1 && x <= Icons::WARNING_X2 && y >= Icons::WARNING_Y1 && y <= Icons::WARNING_Y2)
        return TouchAction::Warning;

    return TouchAction::None;
}

// Ejemplo de uso
void checkTouch(int x, int y) {
    TouchAction action = getTouchedZone(x, y);
    switch(action) {
        case TouchAction::Battery:
            Logger::info("Tocado: Battery");
            break;
        case TouchAction::Lights:
            Logger::info("Tocado: Lights");
            break;
        case TouchAction::Multimedia:
            Logger::info("Tocado: Multimedia");
            break;
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