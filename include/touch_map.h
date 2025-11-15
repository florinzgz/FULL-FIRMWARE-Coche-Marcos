#pragma once

// ============================================================================
// touch_map.h - Zonas táctiles del HUD (480x320, rotación 1)
// ============================================================================

#include "icons.h"   // usamos las constantes de layout de Icons

// Acciones posibles al tocar la pantalla
enum class TouchAction {
    None,
    Battery,
    Lights,
    Multimedia,
    Mode4x4,
    Warning
};

// Estructura de zona táctil rectangular
struct TouchZone {
    int x1, y1, x2, y2;
    TouchAction action;
    constexpr bool contains(int x, int y) const {
        return (x >= x1 && x <= x2 && y >= y1 && y <= y2);
    }
};

// -----------------------
// Definición de zonas
// -----------------------
constexpr TouchZone TOUCH_ZONES[] = {
    {Icons::BATTERY_X1,   Icons::BATTERY_Y1,   Icons::BATTERY_X2,   Icons::BATTERY_Y2,   TouchAction::Battery},
    {Icons::LIGHTS_X1,    Icons::LIGHTS_Y1,    Icons::LIGHTS_X2,    Icons::LIGHTS_Y2,    TouchAction::Lights},
    {Icons::MEDIA_X1,     Icons::MEDIA_Y1,     Icons::MEDIA_X2,     Icons::MEDIA_Y2,     TouchAction::Multimedia},
    {Icons::MODE4X4_X1,   Icons::MODE4X4_Y1,   Icons::MODE4X4_X2,   Icons::MODE4X4_Y2,   TouchAction::Mode4x4},
    {Icons::WARNING_X1,   Icons::WARNING_Y1,   Icons::WARNING_X2,   Icons::WARNING_Y2,   TouchAction::Warning}
};

// -----------------------
// API
// -----------------------
TouchAction getTouchedZone(int x, int y);

// Función auxiliar opcional para debug
void checkTouch(int x, int y);