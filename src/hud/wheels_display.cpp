#include "wheels_display.h"
#include "settings.h"
#include "logger.h"
#include <TFT_eSPI.h>
#include <Arduino.h>   // para constrain()
#include <math.h>      // para fabs()

// Puntero global a la pantalla
static TFT_eSPI *tft = nullptr;
static bool initialized = false;

// 🔒 v2.8.4: Eliminado cache global lastAngle que impedía dibujar otras ruedas
// si compartían el mismo ángulo (p.ej., todas las ruedas con 0°)

// Helpers: colores
static uint16_t colorByTemp(float t) {
    if(t < TEMP_WARN_MOTOR) return TFT_GREEN;
    if(t < TEMP_MAX_MOTOR)  return TFT_YELLOW;
    return TFT_RED;
}
static uint16_t colorByEffort(float e) {
    if(e < 60.0f) return TFT_GREEN;
    if(e < 85.0f) return TFT_YELLOW;
    return TFT_RED;
}

void WheelsDisplay::init(TFT_eSPI *display) { 
    tft = display; 
    initialized = true;
    Logger::info("WheelsDisplay init OK");
}

void WheelsDisplay::drawWheel(int cx, int cy, float angleDeg, float tempC, float effortPct) {
    if(!initialized) {
        Logger::warn("WheelsDisplay drawWheel() llamado sin init");
        return;
    }

    // Clamp valores
    effortPct = constrain(effortPct, 0.0f, 100.0f);
    tempC = constrain(tempC, -40.0f, 150.0f);

    // 🔒 v2.8.4: Siempre dibujar la rueda (eliminado cache global que bloqueaba otras ruedas)
    // Rueda base: rectángulo rotado
    int w = 40, h = 12;
    float rad = angleDeg * 0.0174533f;
    int dx = (int)(cosf(rad) * w/2);
    int dy = (int)(sinf(rad) * w/2);
    int ex = (int)(-sinf(rad) * h/2);
    int ey = (int)( cosf(rad) * h/2);

    int x0 = cx - dx - ex, y0 = cy - dy - ey;
    int x1 = cx + dx - ex, y1 = cy + dy - ey;
    int x2 = cx + dx + ex, y2 = cy + dy + ey;
    int x3 = cx - dx + ex, y3 = cy - dy + ey;

    // Fondo negro para limpiar antes de redibujar
    tft->fillRect(cx - w/2 - 2, cy - h/2 - 2, w+4, h+4, TFT_BLACK);

    tft->fillTriangle(x0,y0, x1,y1, x2,y2, TFT_DARKGREY);
    tft->fillTriangle(x0,y0, x2,y2, x3,y3, TFT_DARKGREY);

    // Flecha de dirección
    int x2a = cx + (int)(cosf(rad) * 20);
    int y2a = cy + (int)(sinf(rad) * 20);
    tft->drawLine(cx, cy, x2a, y2a, TFT_WHITE);

    // Temperatura encima
    tft->fillRect(cx - 30, cy - 30, 60, 15, TFT_BLACK);
    tft->setTextDatum(MC_DATUM);
    char buf[16];
    
    // Si temperatura es -999, mostrar "--" (sensor deshabilitado)
    if (tempC < -998.0f) {
        tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
        snprintf(buf, sizeof(buf), "-- °C");
    } else {
        tft->setTextColor(colorByTemp(tempC), TFT_BLACK);
        snprintf(buf, sizeof(buf), "%d °C", (int)tempC);
    }
    tft->drawString(buf, cx, cy - 20, 2);

    // Esfuerzo debajo
    tft->fillRect(cx - 30, cy + 10, 60, 15, TFT_BLACK);
    
    // Si esfuerzo es -1, mostrar "--" (sensor deshabilitado)
    if (effortPct < 0.0f) {
        tft->setTextColor(TFT_DARKGREY, TFT_BLACK);
        snprintf(buf, sizeof(buf), "-- %%");
    } else {
        tft->setTextColor(colorByEffort(effortPct), TFT_BLACK);
        snprintf(buf, sizeof(buf), "%d %%", (int)effortPct);
    }
    tft->drawString(buf, cx, cy + 20, 2);

    // Barra de esfuerzo (solo si hay valor válido)
    int barW = 50, barH = 6;
    tft->fillRect(cx - barW/2, cy + 28, barW, barH, TFT_BLACK); // limpiar
    tft->drawRect(cx - barW/2, cy + 28, barW, barH, TFT_WHITE);
    
    if (effortPct >= 0.0f) {
        int filled = (int)((effortPct / 100.0f) * barW);
        tft->fillRect(cx - barW/2, cy + 28, filled, barH, colorByEffort(effortPct));
    }
}