#include "wheels_display.h"
#include "settings.h"
#include "logger.h"
#include <TFT_eSPI.h>
#include <Arduino.h>   // para constrain() y DEG_TO_RAD
#include <math.h>      // para fabs()

// Puntero global a la pantalla
static TFT_eSPI *tft = nullptr;
static bool initialized = false;

// Colores 3D para ruedas
static const uint16_t COLOR_WHEEL_OUTER = 0x3186;     // Gris oscuro borde
static const uint16_t COLOR_WHEEL_INNER = 0x4A49;     // Gris medio llanta
static const uint16_t COLOR_WHEEL_TREAD = 0x2104;     // Negro neumático
static const uint16_t COLOR_WHEEL_HIGHLIGHT = 0x6B4D;  // Highlight 3D
static const uint16_t COLOR_WHEEL_SHADOW = 0x1082;    // Sombra 3D
static const uint16_t COLOR_HUB_CENTER = 0x9CF3;      // Centro plateado
static const uint16_t COLOR_HUB_BOLT = 0xC618;        // Tornillos brillantes
static const uint16_t COLOR_INFO_BG = 0x2104;         // Fondo de info (temperatura/esfuerzo)
static const uint16_t COLOR_BAR_BG = 0x1082;          // Fondo de barras de progreso

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

// Helpers: colores según temperatura
static uint16_t colorByTemp(float t) {
    if(t < TEMP_WARN_MOTOR) return TFT_GREEN;
    if(t < TEMP_MAX_MOTOR)  return TFT_YELLOW;
    return TFT_RED;
}

// Helpers: colores según esfuerzo
static uint16_t colorByEffort(float e) {
    if(e < 60.0f) return TFT_GREEN;
    if(e < 85.0f) return TFT_YELLOW;
    return TFT_RED;
}

// Dibujar rueda 3D con efecto de profundidad
static void drawWheel3D(int cx, int cy, float angleDeg) {
    int w = 38, h = 16;  // Dimensiones base de la rueda
    float rad = angleDeg * DEG_TO_RAD;
    
    // Calcular puntos de los vértices del rectángulo rotado
    int dx = (int)(cosf(rad) * w/2);
    int dy = (int)(sinf(rad) * w/2);
    int ex = (int)(-sinf(rad) * h/2);
    int ey = (int)( cosf(rad) * h/2);

    // 4 esquinas del rectángulo
    int x0 = cx - dx - ex, y0 = cy - dy - ey;
    int x1 = cx + dx - ex, y1 = cy + dy - ey;
    int x2 = cx + dx + ex, y2 = cy + dy + ey;
    int x3 = cx - dx + ex, y3 = cy - dy + ey;

    // Fondo negro para limpiar
    tft->fillRect(cx - w/2 - 4, cy - h/2 - 4, w + 8, h + 8, TFT_BLACK);

    // Sombra de la rueda (desplazada 2px abajo-derecha)
    tft->fillTriangle(x0 + 2, y0 + 2, x1 + 2, y1 + 2, x2 + 2, y2 + 2, COLOR_WHEEL_SHADOW);
    tft->fillTriangle(x0 + 2, y0 + 2, x2 + 2, y2 + 2, x3 + 2, y3 + 2, COLOR_WHEEL_SHADOW);
    
    // Neumático exterior (banda de rodadura)
    tft->fillTriangle(x0, y0, x1, y1, x2, y2, COLOR_WHEEL_TREAD);
    tft->fillTriangle(x0, y0, x2, y2, x3, y3, COLOR_WHEEL_TREAD);
    
    // Borde exterior brillante
    tft->drawLine(x0, y0, x1, y1, COLOR_WHEEL_OUTER);
    tft->drawLine(x1, y1, x2, y2, COLOR_WHEEL_OUTER);
    tft->drawLine(x2, y2, x3, y3, COLOR_WHEEL_OUTER);
    tft->drawLine(x3, y3, x0, y0, COLOR_WHEEL_OUTER);
    
    // Llanta interior (más clara)
    int innerScale = 60;  // 60% del tamaño
    int ix0 = cx - dx * innerScale / 100 - ex * innerScale / 100;
    int iy0 = cy - dy * innerScale / 100 - ey * innerScale / 100;
    int ix1 = cx + dx * innerScale / 100 - ex * innerScale / 100;
    int iy1 = cy + dy * innerScale / 100 - ey * innerScale / 100;
    int ix2 = cx + dx * innerScale / 100 + ex * innerScale / 100;
    int iy2 = cy + dy * innerScale / 100 + ey * innerScale / 100;
    int ix3 = cx - dx * innerScale / 100 + ex * innerScale / 100;
    int iy3 = cy - dy * innerScale / 100 + ey * innerScale / 100;
    
    tft->fillTriangle(ix0, iy0, ix1, iy1, ix2, iy2, COLOR_WHEEL_INNER);
    tft->fillTriangle(ix0, iy0, ix2, iy2, ix3, iy3, COLOR_WHEEL_INNER);
    
    // Centro de la rueda (hub) con efecto 3D
    tft->fillCircle(cx, cy, 5, COLOR_HUB_CENTER);
    tft->drawCircle(cx, cy, 5, COLOR_WHEEL_OUTER);
    
    // Punto de luz central (highlight)
    tft->fillCircle(cx - 1, cy - 1, 1, COLOR_HUB_BOLT);
    
    // Flecha de dirección mejorada (solo para ruedas que giran)
    if (fabs(angleDeg) > 0.1f) {
        int arrowLen = 18;
        int ax = cx + (int)(cosf(rad) * arrowLen);
        int ay = cy + (int)(sinf(rad) * arrowLen);
        
        // Línea principal
        tft->drawLine(cx, cy, ax, ay, TFT_CYAN);
        
        // Punta de flecha
        float arrowAngle1 = rad + 2.5f;
        float arrowAngle2 = rad - 2.5f;
        int arrowSize = 5;
        int ax1 = ax - (int)(cosf(arrowAngle1) * arrowSize);
        int ay1 = ay - (int)(sinf(arrowAngle1) * arrowSize);
        int ax2 = ax - (int)(cosf(arrowAngle2) * arrowSize);
        int ay2 = ay - (int)(sinf(arrowAngle2) * arrowSize);
        
        tft->drawLine(ax, ay, ax1, ay1, TFT_CYAN);
        tft->drawLine(ax, ay, ax2, ay2, TFT_CYAN);
    }
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

    // Dibujar rueda 3D mejorada
    drawWheel3D(cx, cy, angleDeg);

    // Temperatura encima con fondo semitransparente
    tft->fillRoundRect(cx - 26, cy - 36, 52, 16, 3, COLOR_INFO_BG);
    tft->drawRoundRect(cx - 26, cy - 36, 52, 16, 3, TFT_DARKGREY);
    tft->setTextDatum(MC_DATUM);
    char buf[16];
    
    // Si temperatura es -999, mostrar "--" (sensor deshabilitado)
    if (tempC < -998.0f) {
        tft->setTextColor(TFT_DARKGREY, COLOR_INFO_BG);
        snprintf(buf, sizeof(buf), "-- C");
    } else {
        tft->setTextColor(colorByTemp(tempC), COLOR_INFO_BG);
        snprintf(buf, sizeof(buf), "%d C", (int)tempC);
    }
    tft->drawString(buf, cx, cy - 28, 2);

    // Esfuerzo debajo con fondo semitransparente
    tft->fillRoundRect(cx - 26, cy + 18, 52, 16, 3, COLOR_INFO_BG);
    tft->drawRoundRect(cx - 26, cy + 18, 52, 16, 3, TFT_DARKGREY);
    
    // Si esfuerzo es -1, mostrar "--" (sensor deshabilitado)
    if (effortPct < 0.0f) {
        tft->setTextColor(TFT_DARKGREY, COLOR_INFO_BG);
        snprintf(buf, sizeof(buf), "-- %%");
    } else {
        tft->setTextColor(colorByEffort(effortPct), COLOR_INFO_BG);
        snprintf(buf, sizeof(buf), "%d%%", (int)effortPct);
    }
    tft->drawString(buf, cx, cy + 26, 2);

    // Barra de esfuerzo 3D (solo si hay valor válido)
    int barW = 48, barH = 8;
    int barX = cx - barW/2;
    int barY = cy + 36;
    
    // Fondo de barra con efecto 3D
    tft->fillRoundRect(barX, barY, barW, barH, 2, COLOR_BAR_BG);
    tft->drawRoundRect(barX, barY, barW, barH, 2, TFT_DARKGREY);
    
    if (effortPct >= 0.0f) {
        int filled = (int)((effortPct / 100.0f) * (barW - 4));
        if (filled > 0) {
            uint16_t barColor = colorByEffort(effortPct);
            tft->fillRoundRect(barX + 2, barY + 2, filled, barH - 4, 1, barColor);
            // Highlight superior
            tft->drawFastHLine(barX + 2, barY + 2, filled, 0xFFFF);
        }
    }
}