#include "gauges.h"
#include "settings.h"
#include <Arduino.h>   // para constrain(), snprintf, etc.

static TFT_eSPI *tft;

// Guardamos el último valor para poder borrar solo la aguja anterior
static float lastSpeed = -1;
static float lastRpm   = -1;

// Constantes matemáticas
static const float DEG_TO_RAD_CONST = 0.0174533f;  // π/180 para conversión grados a radianes

// Constantes de configuración por defecto
static const int DEFAULT_MAX_KMH = 35;    // Velocidad máxima por defecto
static const int DEFAULT_MAX_RPM = 400;   // RPM máxima por defecto

// Colores 3D para efectos de profundidad
static const uint16_t COLOR_GAUGE_OUTER = 0x4A69;    // Gris oscuro metálico
static const uint16_t COLOR_GAUGE_INNER = 0x2124;    // Negro profundo
static const uint16_t COLOR_GAUGE_RING = 0x7BCF;     // Plata brillante
static const uint16_t COLOR_GAUGE_HIGHLIGHT = 0xBDF7; // Blanco/plata claro
static const uint16_t COLOR_NEEDLE_BASE = 0xF800;    // Rojo principal
static const uint16_t COLOR_NEEDLE_TIP = 0xFFFF;     // Blanco punta
static const uint16_t COLOR_NEEDLE_SHADOW = 0x8000;  // Rojo oscuro sombra
static const uint16_t COLOR_SCALE_NORMAL = 0x07E0;   // Verde
static const uint16_t COLOR_SCALE_WARN = 0xFD20;     // Naranja
static const uint16_t COLOR_SCALE_DANGER = 0xF800;   // Rojo

// -----------------------
// Helpers mejorados
// -----------------------

// Dibujar arco grueso con efecto 3D
static void drawThickArc(int cx, int cy, int r, int thickness, uint16_t color, float startAngle, float endAngle) {
    // Use TFT_eSPI's optimized drawArc() method for better performance
    for (int i = 0; i < thickness; i++) {
        // drawArc expects angles in degrees, and draws a ring segment with given thickness
        tft->drawArc(cx, cy, r - i, r - i, (int)startAngle, (int)endAngle, color, color, true);
    }
}

// Dibujar marcas de escala con números
static void drawScaleMarks(int cx, int cy, int r, int maxValue, int step, bool showNumbers) {
    int numMarks = maxValue / step;
    for (int i = 0; i <= numMarks; i++) {
        float value = i * step;
        float angle = -135.0f + (value / (float)maxValue) * 270.0f;
        float rad = angle * DEG_TO_RAD_CONST;
        
        // Determinar color según zona
        uint16_t color;
        float ratio = value / (float)maxValue;
        if (ratio < 0.6f) {
            color = COLOR_SCALE_NORMAL;
        } else if (ratio < 0.85f) {
            color = COLOR_SCALE_WARN;
        } else {
            color = COLOR_SCALE_DANGER;
        }
        
        // Marca exterior (línea gruesa)
        int x1 = cx + (int)(cosf(rad) * (r - 5));
        int y1 = cy + (int)(sinf(rad) * (r - 5));
        int x2 = cx + (int)(cosf(rad) * (r - 15));
        int y2 = cy + (int)(sinf(rad) * (r - 15));
        tft->drawLine(x1, y1, x2, y2, color);
        tft->drawLine(x1 + 1, y1, x2 + 1, y2, color);
        
        // Número de escala
        if (showNumbers && (i % 2 == 0 || numMarks <= 6)) {
            int xNum = cx + (int)(cosf(rad) * (r - 25));
            int yNum = cy + (int)(sinf(rad) * (r - 25));
            tft->setTextDatum(MC_DATUM);
            tft->setTextColor(TFT_WHITE, TFT_BLACK);
            char buf[8];
            snprintf(buf, sizeof(buf), "%d", (int)value);
            tft->drawString(buf, xNum, yNum, 1);
        }
    }
    
    // Marcas menores (entre las principales)
    for (int i = 0; i <= numMarks * 2; i++) {
        if (i % 2 == 1) {  // Solo marcas intermedias
            float value = i * step / 2.0f;
            float angle = -135.0f + (value / (float)maxValue) * 270.0f;
            float rad = angle * DEG_TO_RAD_CONST;
            
            int x1 = cx + (int)(cosf(rad) * (r - 5));
            int y1 = cy + (int)(sinf(rad) * (r - 5));
            int x2 = cx + (int)(cosf(rad) * (r - 10));
            int y2 = cy + (int)(sinf(rad) * (r - 10));
            tft->drawLine(x1, y1, x2, y2, TFT_DARKGREY);
        }
    }
}

// Aguja 3D con efecto de profundidad
static void drawNeedle3D(int cx, int cy, float value, float maxValue, int r, bool erase) {
    // 🔒 CORRECCIÓN ALTA: Proteger contra división por cero
    if (maxValue <= 0.0f) {
        maxValue = 1.0f;  // Fallback seguro
    }
    
    float angle = -135.0f + (value / maxValue) * 270.0f;
    float rad = angle * DEG_TO_RAD_CONST;
    
    // Puntos de la aguja (forma triangular)
    int tipX = cx + (int)(cosf(rad) * r);
    int tipY = cy + (int)(sinf(rad) * r);
    
    // Perpendicular para ancho de base
    float perpRad = rad + 1.5708f; // +90 grados
    int baseOffset = 4;
    int baseX1 = cx + (int)(cosf(perpRad) * baseOffset);
    int baseY1 = cy + (int)(sinf(perpRad) * baseOffset);
    int baseX2 = cx - (int)(cosf(perpRad) * baseOffset);
    int baseY2 = cy - (int)(sinf(perpRad) * baseOffset);
    
    if (erase) {
        // Borrar con negro
        tft->fillTriangle(tipX, tipY, baseX1, baseY1, baseX2, baseY2, TFT_BLACK);
        tft->fillCircle(cx, cy, 8, COLOR_GAUGE_INNER);
    } else {
        // Sombra de la aguja (desplazada 2px)
        tft->fillTriangle(tipX + 1, tipY + 1, baseX1 + 1, baseY1 + 1, baseX2 + 1, baseY2 + 1, COLOR_NEEDLE_SHADOW);
        
        // Aguja principal
        tft->fillTriangle(tipX, tipY, baseX1, baseY1, baseX2, baseY2, COLOR_NEEDLE_BASE);
        
        // Línea central blanca (efecto brillo)
        int midX = cx + (int)(cosf(rad) * (r * 0.6f));
        int midY = cy + (int)(sinf(rad) * (r * 0.6f));
        tft->drawLine(cx, cy, midX, midY, COLOR_NEEDLE_TIP);
        
        // Centro de la aguja (círculo 3D)
        tft->fillCircle(cx, cy, 10, TFT_DARKGREY);
        tft->fillCircle(cx, cy, 8, COLOR_GAUGE_RING);
        tft->fillCircle(cx - 2, cy - 2, 3, COLOR_GAUGE_HIGHLIGHT);
    }
}

// -----------------------
// Fondo estático mejorado con efecto 3D
// -----------------------
static void drawGaugeBackground(int cx, int cy, int maxValue, int step, const char* unit) {
    int outerRadius = 68;
    int innerRadius = 55;
    
    // Fondo negro profundo
    tft->fillCircle(cx, cy, outerRadius + 5, TFT_BLACK);
    
    // Anillo exterior metálico (efecto 3D con gradiente)
    for (int r = outerRadius + 4; r >= outerRadius; r--) {
        uint16_t shade = (r == outerRadius + 4) ? COLOR_GAUGE_HIGHLIGHT : 
                        (r == outerRadius) ? COLOR_GAUGE_OUTER : COLOR_GAUGE_RING;
        tft->drawCircle(cx, cy, r, shade);
    }
    
    // Interior negro
    tft->fillCircle(cx, cy, innerRadius, COLOR_GAUGE_INNER);
    
    // Arco de escala coloreado (verde-amarillo-rojo)
    // Verde (0-60%)
    drawThickArc(cx, cy, outerRadius - 2, 3, COLOR_SCALE_NORMAL, -135.0f, -135.0f + 270.0f * 0.6f);
    // Amarillo (60-85%)
    drawThickArc(cx, cy, outerRadius - 2, 3, COLOR_SCALE_WARN, -135.0f + 270.0f * 0.6f, -135.0f + 270.0f * 0.85f);
    // Rojo (85-100%)
    drawThickArc(cx, cy, outerRadius - 2, 3, COLOR_SCALE_DANGER, -135.0f + 270.0f * 0.85f, 135.0f);
    
    // Marcas de escala con números
    drawScaleMarks(cx, cy, outerRadius, maxValue, step, true);
    
    // Etiqueta de unidad
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(TFT_CYAN, TFT_BLACK);
    tft->drawString(unit, cx, cy + 35, 1);
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
    if (maxKmh <= 0) maxKmh = DEFAULT_MAX_KMH;
    
    // Calcular step de escala según maxKmh
    int step = (maxKmh <= 50) ? 5 : 10;

    // Redibujar fondo solo si es la primera vez
    if (lastSpeed < 0) {
        drawGaugeBackground(cx, cy, maxKmh, step, "km/h");
    } else {
        // Borrar aguja anterior
        drawNeedle3D(cx, cy, lastSpeed, (float)maxKmh, 50, true);
    }

    // Dibujar aguja nueva con efecto 3D
    drawNeedle3D(cx, cy, kmh, (float)maxKmh, 50, false);

    // Texto central grande con valor
    tft->setTextDatum(MC_DATUM);
    tft->fillRect(cx - 25, cy - 5, 50, 22, COLOR_GAUGE_INNER);
    
    // Color según velocidad
    uint16_t textColor;
    float ratio = kmh / (float)maxKmh;
    if (ratio < 0.6f) {
        textColor = TFT_GREEN;
    } else if (ratio < 0.85f) {
        textColor = TFT_YELLOW;
    } else {
        textColor = TFT_RED;
    }
    
    tft->setTextColor(textColor, COLOR_GAUGE_INNER);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)kmh);
    tft->drawString(buf, cx, cy + 5, 4);

    lastSpeed = kmh;
}

void Gauges::drawRPM(int cx, int cy, float rpm, int maxRpm) {
    // 🔒 CORRECCIÓN ALTA: Validar maxRpm para prevenir división por cero
    if (maxRpm <= 0) maxRpm = DEFAULT_MAX_RPM;
    
    rpm = constrain(rpm, 0.0f, (float)maxRpm);
    
    // Calcular step de escala según maxRpm
    int step = (maxRpm <= 500) ? 50 : 100;

    if (lastRpm < 0) {
        drawGaugeBackground(cx, cy, maxRpm, step, "RPM");
    } else {
        // Borrar aguja anterior
        drawNeedle3D(cx, cy, lastRpm, (float)maxRpm, 50, true);
    }

    // Dibujar aguja nueva con efecto 3D
    drawNeedle3D(cx, cy, rpm, (float)maxRpm, 50, false);

    // Texto central grande con valor
    tft->setTextDatum(MC_DATUM);
    tft->fillRect(cx - 25, cy - 5, 50, 22, COLOR_GAUGE_INNER);
    
    // Color según RPM
    uint16_t textColor;
    float ratio = rpm / (float)maxRpm;
    if (ratio < 0.6f) {
        textColor = TFT_GREEN;
    } else if (ratio < 0.85f) {
        textColor = TFT_YELLOW;
    } else {
        textColor = TFT_RED;
    }
    
    tft->setTextColor(textColor, COLOR_GAUGE_INNER);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", (int)rpm);
    tft->drawString(buf, cx, cy + 5, 4);

    lastRpm = rpm;
}