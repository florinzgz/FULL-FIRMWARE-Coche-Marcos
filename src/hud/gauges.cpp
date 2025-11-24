#include "gauges.h"
#include "settings.h"
#include <Arduino.h>   // para constrain(), snprintf, etc.

static TFT_eSPI *tft;

// Guardamos el último valor para poder borrar solo la aguja anterior
static float lastSpeed = -1;
static float lastRpm   = -1;

// -----------------------
// Helpers
// -----------------------
static void drawArc(int cx, int cy, int r, uint16_t color) {
    tft->drawCircle(cx, cy, r, color);
}

static void drawNeedle(int cx, int cy, float value, float maxValue, int r, uint16_t color) {
    // 🔒 CORRECCIÓN ALTA: Proteger contra división por cero
    if (maxValue <= 0.0f) {
        maxValue = 1.0f;  // Fallback seguro
    }
    float angle = -135.0f + (value / maxValue) * 270.0f; // -135° a +135°
    float rad = angle * 0.0174533f;
    int x = cx + (int)(cosf(rad) * r);
    int y = cy + (int)(sinf(rad) * r);
    tft->drawLine(cx, cy, x, y, color);
}

// -----------------------
// Fondo estático
// -----------------------
static void drawGaugeBackground(int cx, int cy) {
    tft->fillCircle(cx, cy, 70, TFT_BLACK);
    drawArc(cx, cy, 70, TFT_BLUE);
    drawArc(cx, cy, 60, TFT_GREEN);
    drawArc(cx, cy, 50, TFT_RED);
}

// -----------------------
// API
// -----------------------
void Gauges::init(TFT_eSPI *display) {
    tft = display;
    lastSpeed = -1;
    lastRpm   = -1;
}

void Gauges::drawSpeed(int cx, int cy, float kmh, int maxKmh, float pedalPct) {
    // 🔒 CORRECCIÓN ALTA: Clamp speed con límite superior seguro
    kmh = constrain(kmh, 0.0f, min((float)maxKmh, 999.0f));  // Prevenir overflow visual
    
    // 🔒 Validar maxKmh para prevenir división por cero
    if (maxKmh <= 0) maxKmh = 200;  // Fallback seguro

    // Redibujar fondo solo si es la primera vez
    if (lastSpeed < 0) {
        drawGaugeBackground(cx, cy);
    } else {
        // Borrar aguja anterior
        drawNeedle(cx, cy, lastSpeed, (float)maxKmh, 60, TFT_BLACK);
    }

    // Dibujar aguja nueva
    drawNeedle(cx, cy, kmh, (float)maxKmh, 60, TFT_WHITE);

    // Texto central
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->fillRect(cx - 40, cy - 10, 80, 20, TFT_BLACK); // limpiar zona texto
    char buf[16];
    snprintf(buf, sizeof(buf), "%d km/h", (int)kmh);
    tft->drawString(buf, cx, cy, 2);

    // Pedal %
    tft->fillRect(cx - 20, cy + 15, 40, 20, TFT_BLACK);
    snprintf(buf, sizeof(buf), "%d%%", (int)pedalPct);
    tft->drawString(buf, cx, cy + 22, 2);

    lastSpeed = kmh;
}

void Gauges::drawRPM(int cx, int cy, float rpm, int maxRpm) {
    // 🔒 CORRECCIÓN ALTA: Validar maxRpm para prevenir división por cero
    if (maxRpm <= 0) maxRpm = 10000;  // Fallback seguro
    
    rpm = constrain(rpm, 0.0f, (float)maxRpm);

    if (lastRpm < 0) {
        drawGaugeBackground(cx, cy);
    } else {
        // Borrar aguja anterior
        drawNeedle(cx, cy, lastRpm, (float)maxRpm, 60, TFT_BLACK);
    }

    // Dibujar aguja nueva
    drawNeedle(cx, cy, rpm, (float)maxRpm, 60, TFT_WHITE);

    // Texto central
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_WHITE, TFT_BLACK);
    tft->fillRect(cx - 40, cy - 10, 80, 20, TFT_BLACK);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d RPM", (int)rpm);
    tft->drawString(buf, cx, cy, 2);

    lastRpm = rpm;
}