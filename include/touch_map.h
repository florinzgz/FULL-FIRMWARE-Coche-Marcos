#pragma once

// ============================================================================
// touch_map.h - Zonas táctiles del HUD (480x320, rotación 3)
// ============================================================================
// 🔒 v2.8.3: Centralización de constantes de calibración táctil
// El XPT2046 y el TFT ST7796S comparten el bus SPI con CS separados:
// - TFT_CS = GPIO 16
// - TOUCH_CS = GPIO 21
// Esto permite operación simultánea sin interferencia.
// ============================================================================

#include "icons.h"   // usamos las constantes de layout de Icons

// ============================================================================
// Constantes de calibración táctil XPT2046
// ============================================================================
// El XPT2046 tiene un ADC de 12 bits (rango teórico 0-4095), pero en la
// práctica el rango útil del panel táctil resistivo es aproximadamente 200-3900.
// Estos valores calibrados excluyen las zonas de borde donde las lecturas
// son menos precisas. Ajustar si se cambia el panel táctil.
namespace TouchConstants {
    // Rango calibrado del ADC táctil (zona útil del panel)
    // El rango teórico 0-4095 se reduce a 200-3900 para mayor precisión
    constexpr int RAW_MIN = 200;    // Valor mínimo calibrado (excluye borde)
    constexpr int RAW_MAX = 3900;   // Valor máximo calibrado (excluye borde)
    
    // Dimensiones de pantalla objetivo (después de rotación 3)
    constexpr int SCREEN_WIDTH = 480;
    constexpr int SCREEN_HEIGHT = 320;
    
}

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