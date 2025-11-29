#include "wheels_display.h"
#include "settings.h"
#include "logger.h"
#include <TFT_eSPI.h>
#include <Arduino.h>   // para constrain() y DEG_TO_RAD
#include <math.h>      // para fabs()

// Puntero global a la pantalla
static TFT_eSPI *tft = nullptr;
static bool initialized = false;

// 🔒 v2.8.4: Cache por rueda para evitar redibujos innecesarios
// Cada posición tiene su propio estado para no bloquear otras ruedas
struct WheelCache {
    float lastAngle = -999.0f;
    float lastTemp = -999.0f;
    float lastEffort = -999.0f;
};

// Cache para 4 ruedas (FL=0, FR=1, RL=2, RR=3) basado en posición cx,cy
static WheelCache wheelCaches[4];

// Determinar índice de rueda basado en posición (usando coordenadas conocidas)
static int getWheelIndex(int cx, int cy) {
    // Coordenadas de HUD.cpp: FL(195,115), FR(285,115), RL(195,235), RR(285,235)
    if (cy < 175) {  // Ruedas delanteras (y < centro)
        return (cx < 240) ? 0 : 1;  // FL=0, FR=1
    } else {  // Ruedas traseras (y >= centro)
        return (cx < 240) ? 2 : 3;  // RL=2, RR=3
    }
}

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
    // Resetear cache de todas las ruedas
    for (int i = 0; i < 4; i++) {
        wheelCaches[i].lastAngle = -999.0f;
        wheelCaches[i].lastTemp = -999.0f;
        wheelCaches[i].lastEffort = -999.0f;
    }
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

    // 🔒 v2.8.4: Cache por rueda - solo redibujar si hay cambios significativos
    int wheelIdx = getWheelIndex(cx, cy);
    WheelCache& cache = wheelCaches[wheelIdx];
    
    bool angleChanged = fabs(angleDeg - cache.lastAngle) > 0.5f;
    bool tempChanged = fabs(tempC - cache.lastTemp) > 0.5f;
    bool effortChanged = fabs(effortPct - cache.lastEffort) > 0.5f;
    
    // Si nada cambió, no redibujar
    if (!angleChanged && !tempChanged && !effortChanged) {
        return;
    }
    
    // Actualizar cache
    cache.lastAngle = angleDeg;
    cache.lastTemp = tempC;
    cache.lastEffort = effortPct;

    // Rueda base: rectángulo rotado
    int w = 40, h = 12;
    float rad = angleDeg * DEG_TO_RAD;
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