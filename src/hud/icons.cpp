#include "icons.h"
#include "system.h"   // para consultar errores persistentes
#include "logger.h"
#include <TFT_eSPI.h>
#include <Arduino.h>  // para constrain()
#include <math.h>     // para fabs()

static TFT_eSPI *tft = nullptr;
static bool initialized = false;

// Cache de último estado para evitar redibujos innecesarios
static System::State lastSysState = (System::State)-1;
static Shifter::Gear lastGear = (Shifter::Gear)-1;
static bool lastLights = false;
static bool lastMedia = false;
static bool lastMode4x4 = false;
static bool lastRegen = false;
static float lastBattery = -999.0f;
static int lastErrorCount = -1;

// Cache para estado de sensores
static uint8_t lastCurrentOK = 0;
static uint8_t lastTempOK = 0;
static uint8_t lastWheelOK = 0;
static bool lastTempWarning = false;
static float lastMaxTemp = -999.0f;
static bool sensorsCacheInitialized = false;

void Icons::init(TFT_eSPI *display) {
    tft = display;
    initialized = true;
    // Reset cache de sensores con flag de inicialización
    lastCurrentOK = 0;
    lastTempOK = 0;
    lastWheelOK = 0;
    lastTempWarning = false;
    lastMaxTemp = -999.0f;
    sensorsCacheInitialized = false;
    Logger::info("Icons init OK");
}

void Icons::drawSystemState(System::State st) {
    if(!initialized) return;
    if(st == lastSysState) return; // no cambio → no redibujar
    lastSysState = st;

    const char* txt = "OFF";
    uint16_t col = TFT_WHITE;
    switch(st){
        case System::PRECHECK: txt="PRE";   col=TFT_YELLOW; break;
        case System::READY:    txt="READY"; col=TFT_GREEN;  break;
        case System::RUN:      txt="RUN";   col=TFT_CYAN;   break;
        case System::ERROR:    txt="ERROR"; col=TFT_RED;    break;
        default: break;
    }
    tft->fillRect(5, 5, 80, 20, TFT_BLACK);
    tft->setTextDatum(TL_DATUM);
    tft->setTextColor(col, TFT_BLACK);
    tft->drawString(txt, 10, 10, 2);
}

// 🔒 v2.9.0: Indicador de marcha REUBICADO Y MEJORADO
// Posición: Centro de pantalla, debajo del triángulo warning (entre warning y coche)
// Diseño: 3D con efecto de profundidad y tamaño más grande
void Icons::drawGear(Shifter::Gear g) {
    if(!initialized) return;
    if(g == lastGear) return;
    lastGear = g;

    // ========================================
    // NUEVA POSICIÓN: Centro de pantalla, entre warning y coche
    // Centrado horizontalmente, Y = 48 (debajo del warning que termina en Y=40)
    // ========================================
    const int GEAR_PANEL_X = 140;   // Posición X del panel (centrado ~140-340 = 200px ancho)
    const int GEAR_PANEL_Y = 48;    // Debajo del triángulo warning
    const int GEAR_PANEL_W = 200;   // Ancho total del panel
    const int GEAR_PANEL_H = 36;    // Alto del panel
    const int GEAR_ITEM_W = 36;     // Ancho de cada celda de marcha
    const int GEAR_ITEM_H = 28;     // Alto de cada celda
    const int GEAR_SPACING = 4;     // Espacio entre celdas
    
    // Colores 3D mejorados
    const uint16_t COLOR_PANEL_BG = 0x1082;       // Fondo del panel (gris muy oscuro)
    const uint16_t COLOR_PANEL_BORDER = 0x4A49;   // Borde del panel
    const uint16_t COLOR_PANEL_HIGHLIGHT = 0x6B6D; // Highlight 3D superior
    const uint16_t COLOR_PANEL_SHADOW = 0x0841;   // Sombra 3D inferior
    
    const uint16_t COLOR_INACTIVE_BG = 0x2104;    // Fondo celda inactiva
    const uint16_t COLOR_INACTIVE_TEXT = 0x630C;  // Texto gris para inactivas
    const uint16_t COLOR_INACTIVE_BORDER = 0x3186; // Borde celda inactiva
    
    const uint16_t COLOR_ACTIVE_BG = 0x03EF;      // Fondo azul brillante para activa
    const uint16_t COLOR_ACTIVE_TEXT = TFT_WHITE; // Texto blanco brillante
    const uint16_t COLOR_ACTIVE_BORDER = TFT_CYAN; // Borde cyan para activa
    const uint16_t COLOR_ACTIVE_GLOW = 0x04FF;    // Efecto brillo
    
    const uint16_t COLOR_REVERSE_BG = 0xF800;     // Fondo rojo para Reversa
    const uint16_t COLOR_REVERSE_GLOW = 0xFBE0;   // Brillo naranja para reversa
    
    // ========================================
    // Dibujar panel de fondo con efecto 3D
    // ========================================
    
    // Sombra exterior (efecto profundidad)
    tft->fillRoundRect(GEAR_PANEL_X + 3, GEAR_PANEL_Y + 3, GEAR_PANEL_W, GEAR_PANEL_H, 6, COLOR_PANEL_SHADOW);
    
    // Fondo principal del panel
    tft->fillRoundRect(GEAR_PANEL_X, GEAR_PANEL_Y, GEAR_PANEL_W, GEAR_PANEL_H, 6, COLOR_PANEL_BG);
    
    // Borde del panel
    tft->drawRoundRect(GEAR_PANEL_X, GEAR_PANEL_Y, GEAR_PANEL_W, GEAR_PANEL_H, 6, COLOR_PANEL_BORDER);
    
    // Highlight superior (efecto 3D)
    tft->drawFastHLine(GEAR_PANEL_X + 8, GEAR_PANEL_Y + 2, GEAR_PANEL_W - 16, COLOR_PANEL_HIGHLIGHT);
    tft->drawFastHLine(GEAR_PANEL_X + 10, GEAR_PANEL_Y + 3, GEAR_PANEL_W - 20, COLOR_PANEL_HIGHLIGHT);
    
    // ========================================
    // Dibujar las 5 marchas: P R N D1 D2
    // ========================================
    const char* gears[] = {"P", "R", "N", "D1", "D2"};
    const Shifter::Gear gearValues[] = {Shifter::P, Shifter::R, Shifter::N, Shifter::D1, Shifter::D2};
    
    // Calcular posición inicial para centrar las 5 celdas
    int totalWidth = 5 * GEAR_ITEM_W + 4 * GEAR_SPACING;
    int startX = GEAR_PANEL_X + (GEAR_PANEL_W - totalWidth) / 2;
    int cellY = GEAR_PANEL_Y + (GEAR_PANEL_H - GEAR_ITEM_H) / 2;
    
    tft->setTextDatum(MC_DATUM);  // Centro para el texto
    
    for(int i = 0; i < 5; i++) {
        int cellX = startX + i * (GEAR_ITEM_W + GEAR_SPACING);
        bool isActive = (gearValues[i] == g);
        
        uint16_t bgColor, textColor, borderColor;
        
        if(isActive) {
            // Marcha activa - efecto brillante
            if(g == Shifter::R) {
                bgColor = COLOR_REVERSE_BG;
                borderColor = COLOR_REVERSE_GLOW;
            } else {
                bgColor = COLOR_ACTIVE_BG;
                borderColor = COLOR_ACTIVE_BORDER;
            }
            textColor = COLOR_ACTIVE_TEXT;
            
            // Efecto glow exterior para marcha activa
            tft->drawRoundRect(cellX - 1, cellY - 1, GEAR_ITEM_W + 2, GEAR_ITEM_H + 2, 5, 
                              (g == Shifter::R) ? COLOR_REVERSE_GLOW : COLOR_ACTIVE_GLOW);
        } else {
            bgColor = COLOR_INACTIVE_BG;
            textColor = COLOR_INACTIVE_TEXT;
            borderColor = COLOR_INACTIVE_BORDER;
        }
        
        // Fondo de la celda con esquinas redondeadas
        tft->fillRoundRect(cellX, cellY, GEAR_ITEM_W, GEAR_ITEM_H, 4, bgColor);
        
        // Borde de la celda
        tft->drawRoundRect(cellX, cellY, GEAR_ITEM_W, GEAR_ITEM_H, 4, borderColor);
        
        // Efecto 3D interno: highlight superior
        if(isActive) {
            tft->drawFastHLine(cellX + 4, cellY + 2, GEAR_ITEM_W - 8, 
                              (g == Shifter::R) ? 0xFC10 : 0x07FF);
        }
        
        // Dibujar texto de la marcha (tamaño grande: font 4)
        int textX = cellX + GEAR_ITEM_W / 2;
        int textY = cellY + GEAR_ITEM_H / 2;
        
        tft->setTextColor(textColor, bgColor);
        tft->drawString(gears[i], textX, textY, 4);  // Font 4 = más grande
    }
}

void Icons::drawFeatures(bool lights, bool media, bool mode4x4, bool regenOn) {
    if(!initialized) return;
    if(lights==lastLights && media==lastMedia && mode4x4==lastMode4x4 && regenOn==lastRegen) return;

    // Colores para efectos 3D
    const uint16_t COLOR_BOX_BG = 0x2104;        // Fondo oscuro
    const uint16_t COLOR_BOX_BORDER = 0x4A49;    // Borde gris
    const uint16_t COLOR_BOX_HIGHLIGHT = 0x6B6D; // Highlight superior
    const uint16_t COLOR_BOX_SHADOW = 0x1082;    // Sombra inferior
    
    // Colores para cada icono cuando activo
    const uint16_t COLOR_4X4_ACTIVE = 0x0410;    // Cian oscuro (4x4 activo)
    const uint16_t COLOR_4X2_ACTIVE = 0x4100;    // Naranja oscuro (4x2 activo)
    const uint16_t COLOR_LIGHTS_ACTIVE = 0x4200; // Amarillo oscuro
    const uint16_t COLOR_MEDIA_ACTIVE = 0x0320;  // Verde oscuro
    const uint16_t COLOR_REGEN_ACTIVE = 0x0015;  // Azul oscuro
    
    // Helper para dibujar cuadrado 3D con texto
    auto draw3DBox = [&](int x1, int y1, int x2, int y2, const char* text, bool active, uint16_t activeColor) {
        int w = x2 - x1;
        int h = y2 - y1;
        int cx = (x1 + x2) / 2;
        int cy = (y1 + y2) / 2;
        
        // Sombra del cuadrado (offset 2px)
        tft->fillRoundRect(x1 + 2, y1 + 2, w, h, 5, COLOR_BOX_SHADOW);
        
        // Fondo del cuadrado
        uint16_t bgColor = active ? activeColor : COLOR_BOX_BG;
        tft->fillRoundRect(x1, y1, w, h, 5, bgColor);
        
        // Borde exterior
        tft->drawRoundRect(x1, y1, w, h, 5, COLOR_BOX_BORDER);
        
        // Efecto 3D: highlight superior
        tft->drawFastHLine(x1 + 5, y1 + 2, w - 10, COLOR_BOX_HIGHLIGHT);
        tft->drawFastHLine(x1 + 5, y1 + 3, w - 10, COLOR_BOX_HIGHLIGHT);
        
        // Efecto 3D: sombra inferior interna
        tft->drawFastHLine(x1 + 5, y2 - 3, w - 10, COLOR_BOX_SHADOW);
        tft->drawFastHLine(x1 + 5, y2 - 2, w - 10, COLOR_BOX_SHADOW);
        
        // Texto centrado con sombra
        tft->setTextDatum(MC_DATUM);
        // Sombra del texto
        tft->setTextColor(TFT_BLACK, bgColor);
        tft->drawString(text, cx + 1, cy + 1, 2);
        // Texto principal
        uint16_t textColor = active ? TFT_WHITE : TFT_DARKGREY;
        tft->setTextColor(textColor, bgColor);
        tft->drawString(text, cx, cy, 2);
        tft->setTextDatum(TL_DATUM);
    };

    // 4x4 / 4x2 - Siempre muestra el estado activo con color diferente
    // 4x4 = cian, 4x2 = naranja (ambos siempre "activos" visualmente)
    if(mode4x4 != lastMode4x4) {
        uint16_t mode4x4Color = mode4x4 ? COLOR_4X4_ACTIVE : COLOR_4X2_ACTIVE;
        draw3DBox(MODE4X4_X1, MODE4X4_Y1, MODE4X4_X2, MODE4X4_Y2, 
                  mode4x4 ? "4x4" : "4x2", true, mode4x4Color);
        lastMode4x4 = mode4x4;
    }

    // Luces - Amarillo cuando activo
    if(lights != lastLights) {
        draw3DBox(LIGHTS_X1, LIGHTS_Y1, LIGHTS_X2, LIGHTS_Y2, 
                  "LUCES", lights, COLOR_LIGHTS_ACTIVE);
        lastLights = lights;
    }

    // Multimedia - Verde cuando activo
    if(media != lastMedia) {
        draw3DBox(MEDIA_X1, MEDIA_Y1, MEDIA_X2, MEDIA_Y2, 
                  "MEDIA", media, COLOR_MEDIA_ACTIVE);
        lastMedia = media;
    }

    // Regenerativo - Usar helper draw3DBox
    if(regenOn != lastRegen) {
        // Limpiar área primero
        tft->fillRect(400, 250, 75, 45, TFT_BLACK);
        // Usar helper con posiciones fijas para REGEN
        draw3DBox(400, 250, 471, 291, "REGEN", regenOn, COLOR_REGEN_ACTIVE);
        lastRegen = regenOn;
    }
}

void Icons::drawBattery(float volts) {
    if(!initialized) return;
    volts = constrain(volts, 0.0f, 99.9f);
    if(fabs(volts - lastBattery) < 0.1f) return; // no cambio significativo
    lastBattery = volts;

    int x = BATTERY_X1;
    int y = BATTERY_Y1;
    int w = BATTERY_X2 - BATTERY_X1;
    int h = BATTERY_Y2 - BATTERY_Y1;
    
    // Limpiar área
    tft->fillRect(x, y, w, h, TFT_BLACK);
    
    // Calcular porcentaje de batería (asumiendo 20V-28V como rango)
    float percent = constrain((volts - 20.0f) / 8.0f * 100.0f, 0.0f, 100.0f);
    
    // Dibujar icono de batería 3D
    int battX = x + 5;
    int battY = y + 5;
    int battW = 35;
    int battH = 20;
    int capW = 4;
    int capH = 10;
    
    // Cuerpo de la batería con efecto 3D
    tft->fillRoundRect(battX, battY, battW, battH, 3, 0x2104);
    tft->drawRoundRect(battX, battY, battW, battH, 3, 0x6B6D);
    
    // Terminal positivo
    tft->fillRect(battX + battW, battY + (battH - capH) / 2, capW, capH, 0x6B6D);
    
    // Relleno según nivel
    int fillW = (int)((battW - 6) * percent / 100.0f);
    uint16_t fillColor;
    if (percent < 20.0f) {
        fillColor = TFT_RED;
    } else if (percent < 50.0f) {
        fillColor = TFT_YELLOW;
    } else {
        fillColor = TFT_GREEN;
    }
    
    if (fillW > 0) {
        tft->fillRoundRect(battX + 3, battY + 3, fillW, battH - 6, 1, fillColor);
        // Highlight 3D
        tft->drawFastHLine(battX + 3, battY + 3, fillW, 0xFFFF);
    }
    
    // Voltaje debajo del icono
    tft->setTextDatum(TL_DATUM);
    tft->setTextColor(fillColor, TFT_BLACK);
    char buf[10];
    snprintf(buf, sizeof(buf), "%.1fV", volts);
    tft->drawString(buf, battX, battY + battH + 3, 1);
}

void Icons::drawErrorWarning() {
    if(!initialized) return;
    int count = System::getErrorCount();
    if(count == lastErrorCount) return;
    lastErrorCount = count;

    if(count > 0) {
        int midX = (WARNING_X1 + WARNING_X2) / 2;
        int topY = WARNING_Y1 + 8;
        int botY = WARNING_Y2 - 5;
        
        // Sombra del triángulo
        tft->fillTriangle(WARNING_X1 + 2, botY + 2, WARNING_X2 + 2, botY + 2, midX + 2, topY + 2, 0x8400);
        
        // Triángulo de warning con borde
        tft->fillTriangle(WARNING_X1, botY, WARNING_X2, botY, midX, topY, TFT_YELLOW);
        tft->drawTriangle(WARNING_X1, botY, WARNING_X2, botY, midX, topY, 0x8400);
        
        // Signo de exclamación
        tft->fillRect(midX - 1, topY + 8, 3, 10, TFT_BLACK);
        tft->fillCircle(midX, botY - 5, 2, TFT_BLACK);
        
        // Contador de errores
        tft->setTextColor(TFT_YELLOW, TFT_BLACK);
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", count);
        tft->drawString(buf, WARNING_X2 + 5, WARNING_Y1 + 15, 2);
    } else {
        tft->fillRect(WARNING_X1, WARNING_Y1, (WARNING_X2 - WARNING_X1) + 40, WARNING_Y2 - WARNING_Y1, TFT_BLACK);
    }
}

void Icons::drawSensorStatus(uint8_t currentOK, uint8_t tempOK, uint8_t wheelOK,
                            uint8_t currentTotal, uint8_t tempTotal, uint8_t wheelTotal) {
    if(!initialized) return;
    
    // Solo redibujar si hay cambios (o primera vez)
    if(sensorsCacheInitialized && 
       currentOK == lastCurrentOK && tempOK == lastTempOK && wheelOK == lastWheelOK) return;
    
    lastCurrentOK = currentOK;
    lastTempOK = tempOK;
    lastWheelOK = wheelOK;
    sensorsCacheInitialized = true;
    
    // Limpiar área
    tft->fillRect(SENSOR_STATUS_X1, SENSOR_STATUS_Y1, 
                  SENSOR_STATUS_X2 - SENSOR_STATUS_X1, 
                  SENSOR_STATUS_Y2 - SENSOR_STATUS_Y1, TFT_BLACK);
    
    // Helper para determinar color basado en proporción OK/Total
    auto getStatusColor = [](uint8_t ok, uint8_t total) -> uint16_t {
        if (total == 0) return TFT_DARKGREY;  // Deshabilitado
        if (ok == total) return TFT_GREEN;     // Todos OK
        if (ok == 0) return TFT_RED;           // Todos fallidos
        return TFT_YELLOW;                     // Parcial
    };
    
    // Helper para color más oscuro (sombra LED)
    auto getDarkColor = [](uint16_t color) -> uint16_t {
        if (color == TFT_GREEN) return 0x03E0;
        if (color == TFT_RED) return 0x8000;
        if (color == TFT_YELLOW) return 0x8400;
        return 0x2104;
    };
    
    // Posiciones para los 3 indicadores LED
    const int ledRadius = 7;
    const int startX = SENSOR_STATUS_X1 + 20;
    const int ledY = SENSOR_STATUS_Y1 + 14;
    const int textY = SENSOR_STATUS_Y1 + 28;
    const int spacing = 38;
    
    // LED 1: Corriente (INA226) con efecto 3D
    uint16_t colorCurrent = getStatusColor(currentOK, currentTotal);
    // Sombra del LED
    tft->fillCircle(startX + 1, ledY + 1, ledRadius, getDarkColor(colorCurrent));
    // LED principal
    tft->fillCircle(startX, ledY, ledRadius, colorCurrent);
    // Highlight 3D (brillo)
    tft->fillCircle(startX - 2, ledY - 2, 2, 0xFFFF);
    // Borde
    tft->drawCircle(startX, ledY, ledRadius, getDarkColor(colorCurrent));
    
    tft->setTextDatum(MC_DATUM);
    tft->setTextColor(colorCurrent, TFT_BLACK);
    tft->drawString("I", startX, textY, 1);
    
    // LED 2: Temperatura (DS18B20) con efecto 3D
    uint16_t colorTemp = getStatusColor(tempOK, tempTotal);
    tft->fillCircle(startX + spacing + 1, ledY + 1, ledRadius, getDarkColor(colorTemp));
    tft->fillCircle(startX + spacing, ledY, ledRadius, colorTemp);
    tft->fillCircle(startX + spacing - 2, ledY - 2, 2, 0xFFFF);
    tft->drawCircle(startX + spacing, ledY, ledRadius, getDarkColor(colorTemp));
    
    tft->setTextColor(colorTemp, TFT_BLACK);
    tft->drawString("T", startX + spacing, textY, 1);
    
    // LED 3: Ruedas con efecto 3D
    uint16_t colorWheel = getStatusColor(wheelOK, wheelTotal);
    tft->fillCircle(startX + 2*spacing + 1, ledY + 1, ledRadius, getDarkColor(colorWheel));
    tft->fillCircle(startX + 2*spacing, ledY, ledRadius, colorWheel);
    tft->fillCircle(startX + 2*spacing - 2, ledY - 2, 2, 0xFFFF);
    tft->drawCircle(startX + 2*spacing, ledY, ledRadius, getDarkColor(colorWheel));
    
    tft->setTextColor(colorWheel, TFT_BLACK);
    tft->drawString("W", startX + 2*spacing, textY, 1);
}

void Icons::drawTempWarning(bool tempWarning, float maxTemp) {
    if(!initialized) return;
    
    // Solo redibujar si hay cambios significativos
    if(tempWarning == lastTempWarning && fabs(maxTemp - lastMaxTemp) < 1.0f) return;
    
    lastTempWarning = tempWarning;
    lastMaxTemp = maxTemp;
    
    tft->fillRect(TEMP_WARNING_X, TEMP_WARNING_Y, TEMP_WARNING_W, TEMP_WARNING_H, TFT_BLACK);
    
    if(tempWarning) {
        // Mostrar advertencia de temperatura crítica
        tft->setTextDatum(ML_DATUM);
        tft->setTextColor(TFT_RED, TFT_BLACK);
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0fC!", maxTemp);
        tft->drawString(buf, TEMP_WARNING_X + 5, TEMP_WARNING_Y + TEMP_WARNING_H/2, 2);
    }
}

// Cache para temperatura ambiente
static float lastAmbientTemp = -999.0f;

void Icons::drawAmbientTemp(float ambientTemp) {
    if(!initialized) return;
    
    // Solo redibujar si hay cambio significativo (>0.5°C)
    if(fabs(ambientTemp - lastAmbientTemp) < 0.5f) return;
    lastAmbientTemp = ambientTemp;
    
    // Limpiar área
    tft->fillRect(AMBIENT_TEMP_X, AMBIENT_TEMP_Y, AMBIENT_TEMP_W, AMBIENT_TEMP_H, TFT_BLACK);
    
    // Dibujar icono de termómetro pequeño
    int iconX = AMBIENT_TEMP_X + 2;
    int iconY = AMBIENT_TEMP_Y + 3;
    
    // Cuerpo del termómetro (vertical)
    tft->fillRoundRect(iconX, iconY, 6, 12, 2, TFT_CYAN);
    tft->drawRoundRect(iconX, iconY, 6, 12, 2, TFT_DARKGREY);
    
    // Bulbo del termómetro
    tft->fillCircle(iconX + 3, iconY + 13, 4, TFT_CYAN);
    tft->drawCircle(iconX + 3, iconY + 13, 4, TFT_DARKGREY);
    
    // Texto de temperatura
    tft->setTextDatum(ML_DATUM);
    
    // Color según temperatura
    uint16_t tempColor;
    if (ambientTemp < 10.0f) {
        tempColor = TFT_CYAN;       // Frío
    } else if (ambientTemp < 25.0f) {
        tempColor = TFT_GREEN;      // Confortable
    } else if (ambientTemp < 35.0f) {
        tempColor = TFT_YELLOW;     // Cálido
    } else {
        tempColor = TFT_RED;        // Caliente
    }
    
    tft->setTextColor(tempColor, TFT_BLACK);
    char buf[10];
    snprintf(buf, sizeof(buf), "%.0fC", ambientTemp);
    tft->drawString(buf, AMBIENT_TEMP_X + 14, AMBIENT_TEMP_Y + AMBIENT_TEMP_H/2, 2);
    tft->setTextDatum(TL_DATUM);
}