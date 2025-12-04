#include "hud.h"
#include <Arduino.h>                // Para DEG_TO_RAD, millis, constrain
#include <TFT_eSPI.h>
// 🔒 v2.8.8: Eliminada librería XPT2046_Touchscreen separada
// Ahora usamos el touch integrado de TFT_eSPI para evitar conflictos SPI
#include <SPI.h>                    // Para SPIClass HSPI
#include <math.h>                   // Para cosf, sinf

#include "gauges.h"
#include "wheels_display.h"
#include "icons.h"
#include "menu_hidden.h"
#include "touch_map.h"         // 👈 añadido
#include "display_types.h"     // For GearPosition enum
#include "sensors.h"           // Para SystemStatus de sensores

#include "pedal.h"
#include "steering.h"
#include "buttons.h"
#include "shifter.h"
#include "current.h"
#include "temperature.h"
#include "wheels.h"
#include "traction.h"
#include "system.h"
#include "logger.h"
#include "settings.h"
#include "storage.h"
#include "dfplayer.h"
#include "alerts.h"
#include "pins.h"

// ✅ Usar la instancia global de TFT_eSPI definida en hud_manager.cpp
extern TFT_eSPI tft;

// 🔒 v2.8.8: Touch integrado de TFT_eSPI
// Usamos tft.getTouch() en lugar de librería XPT2046_Touchscreen separada
// Esto evita conflictos de bus SPI que causaban pantalla blanca
static bool touchInitialized = false;

// Touch calibration: Using constants from touch_map.h (included above)
// TouchConstants::RAW_MIN, RAW_MAX, SCREEN_WIDTH, SCREEN_HEIGHT

// Demo mode button detection for STANDALONE_DISPLAY
#ifdef STANDALONE_DISPLAY
static uint32_t demoButtonPressStart = 0;
static bool demoButtonWasPressed = false;
static const uint32_t DEMO_BUTTON_LONG_PRESS_MS = 1500;  // 1.5 seconds for long press

// Demo button position (bottom right corner, visible area)
static const int DEMO_BTN_X1 = 400;
static const int DEMO_BTN_Y1 = 260;
static const int DEMO_BTN_X2 = 475;
static const int DEMO_BTN_Y2 = 295;
#endif

// Botón virtual para giro sobre eje (axis rotation toggle)
static bool axisRotationEnabled = false;
static bool lastAxisRotationState = false;
static const int AXIS_BTN_X1 = 245;
static const int AXIS_BTN_Y1 = 250;
static const int AXIS_BTN_X2 = 315;
static const int AXIS_BTN_Y2 = 295;

// Layout 480x320 (rotación 3 → 480x320 landscape)
// Note: Rotation 3 used for ST7796S 4-inch display in horizontal orientation
static const int X_SPEED = 70;   // Left gauge - moved left for car visualization
static const int Y_SPEED = 175;
static const int X_RPM   = 410;  // Right gauge - moved right for car visualization
static const int Y_RPM   = 175;

// Zonas ruedas - centered between gauges
// Car body center: X=240, Y=175
static const int X_FL = 195;     // Front left wheel - inner from center
static const int Y_FL = 115;     // Front wheels - top
static const int X_FR = 285;     // Front right wheel - inner from center
static const int Y_FR = 115;
static const int X_RL = 195;     // Rear left wheel
static const int Y_RL = 235;     // Rear wheels - bottom
static const int X_RR = 285;     // Rear right wheel
static const int Y_RR = 235;

// Car body dimensions (centered at 240, 175)
static const int CAR_BODY_X = 175;      // Car body left edge
static const int CAR_BODY_Y = 100;      // Car body top edge
static const int CAR_BODY_W = 130;      // Car body width
static const int CAR_BODY_H = 150;      // Car body height

// Car body detail dimensions (used in drawCarBody)
static const int CAR_HOOD_OFFSET = 15;           // Hood/trunk X offset from body edge
static const int CAR_HOOD_WIDTH_REDUCTION = 30;  // Hood/trunk width reduction (2x offset)
static const int CAR_HOOD_HEIGHT = 20;           // Hood and trunk area height
static const int CAR_CENTER_LINE_MARGIN = 25;    // Margin from body edge for center line

// Wheel placeholder radius (used in init)
static const int WHEEL_PLACEHOLDER_RADIUS = 10;

static bool carBodyDrawn = false;       // Track if car body needs redraw
static float lastSteeringAngle = -999.0f;  // Cache para ángulo del volante

extern Storage::Config cfg;   // acceso a flags

void HUD::init() {
    // ✅ NO llamar a tft.init() aquí - ya está inicializado en HUDManager::init()
    // Usamos la instancia global compartida de TFT_eSPI
    
    // Clear screen and prepare for dashboard
    tft.fillScreen(TFT_BLACK);

    // Initialize dashboard components
    // CRITICAL: These must be called after tft is initialized and rotation is set
    Gauges::init(&tft);
    WheelsDisplay::init(&tft);
    Icons::init(&tft);
    MenuHidden::init(&tft);  // MenuHidden stores tft pointer, must be non-null

    // 🔒 v2.9.0: TOUCH INITIALIZATION - Using stored calibration or defaults
    touchInitialized = false;
    
#ifndef DISABLE_TOUCH
    if (cfg.touchEnabled) {
        // Use stored calibration if available, otherwise use defaults
        uint16_t calData[5];
        
        if (cfg.touchCalibrated) {
            // Validate calibration data before use
            if (cfg.touchCalibration[0] < cfg.touchCalibration[1] &&  // minX < maxX
                cfg.touchCalibration[2] < cfg.touchCalibration[3] &&  // minY < maxY
                cfg.touchCalibration[1] <= 4095 &&                   // maxX within range
                cfg.touchCalibration[3] <= 4095) {                   // maxY within range
                // Use saved calibration from storage
                for (int i = 0; i < 5; i++) {
                    calData[i] = cfg.touchCalibration[i];
                }
                Logger::infof("Touch: Using stored calibration [%d, %d, %d, %d, %d]",
                             calData[0], calData[1], calData[2], calData[3], calData[4]);
            } else {
                Logger::warn("Touch: Invalid stored calibration, using defaults");
                cfg.touchCalibrated = false;
                // Use default calibration values from touch_map.h
                calData[0] = (uint16_t)TouchConstants::RAW_MIN;   // 200
                calData[1] = (uint16_t)TouchConstants::RAW_MAX;   // 3900
                calData[2] = (uint16_t)TouchConstants::RAW_MIN;   // 200
                calData[3] = (uint16_t)TouchConstants::RAW_MAX;   // 3900
                calData[4] = 3;  // Rotation para coincidir con tft.setRotation(3)
                Logger::info("Touch: Using default calibration values");
            }
        } else {
            // Use default calibration values from touch_map.h
            calData[0] = (uint16_t)TouchConstants::RAW_MIN;   // 200
            calData[1] = (uint16_t)TouchConstants::RAW_MAX;   // 3900
            calData[2] = (uint16_t)TouchConstants::RAW_MIN;   // 200
            calData[3] = (uint16_t)TouchConstants::RAW_MAX;   // 3900
            calData[4] = 3;  // Rotation para coincidir con tft.setRotation(3)
            Logger::info("Touch: Using default calibration values");
        }
        
        tft.setTouch(calData);
        touchInitialized = true;
        Logger::info("Touchscreen XPT2046 integrado TFT_eSPI inicializado OK");
    } else {
        Logger::info("Touchscreen deshabilitado en configuración");
    }
#else
    Logger::info("Touch deshabilitado por compilación (DISABLE_TOUCH)");
#endif

    Logger::info("HUD init OK - Display ST7796S ready");
    
    // Draw initial dashboard elements
    // Show title and status - car visualization will appear when update() is called
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(1);
    tft.drawString("Mercedes AMG GT", 240, 50, 4);
    
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Esperando sensores...", 240, 80, 2);
    
    // Draw placeholder car outline to show display is working
    // Using named constants for consistency
    tft.drawRect(CAR_BODY_X, CAR_BODY_Y, CAR_BODY_W, CAR_BODY_H, TFT_DARKGREY);
    tft.drawCircle(X_FL, Y_FL, WHEEL_PLACEHOLDER_RADIUS, TFT_DARKGREY);
    tft.drawCircle(X_FR, Y_FR, WHEEL_PLACEHOLDER_RADIUS, TFT_DARKGREY);
    tft.drawCircle(X_RL, Y_RL, WHEEL_PLACEHOLDER_RADIUS, TFT_DARKGREY);
    tft.drawCircle(X_RR, Y_RR, WHEEL_PLACEHOLDER_RADIUS, TFT_DARKGREY);
    
    // Reset carBodyDrawn flag so it will be redrawn on first update
    carBodyDrawn = false;
}

void HUD::showLogo() {
    tft.fillScreen(TFT_BLACK);
    carBodyDrawn = false;  // Reset flag so car body will be redrawn after logo
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Mercedes AMG", 240, 120, 4);
    tft.drawString("Sistema de arranque", 240, 170, 2);
}

void HUD::showReady() {
    tft.fillScreen(TFT_BLACK);
    carBodyDrawn = false;  // Reset flag so car body will be redrawn
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("READY", 240, 40, 4);
}

void HUD::showError() {
    tft.fillScreen(TFT_BLACK);
    carBodyDrawn = false;  // Reset flag so car body will be redrawn
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("ERROR", 240, 40, 4);
}

#ifdef STANDALONE_DISPLAY
// Estado de progreso para visualización del botón DEMO
static float demoButtonProgress = 0.0f;

// Dibujar botón DEMO para acceso fácil al menú oculto
// Muestra progreso visual durante la pulsación larga
static void drawDemoButton() {
    // Solo dibujar si el menú oculto no está activo
    if (MenuHidden::isActive()) return;
    
    int btnW = DEMO_BTN_X2 - DEMO_BTN_X1;
    int btnH = DEMO_BTN_Y2 - DEMO_BTN_Y1;
    int centerX = (DEMO_BTN_X1 + DEMO_BTN_X2) / 2;
    int centerY = (DEMO_BTN_Y1 + DEMO_BTN_Y2) / 2;
    
    // Draw button background (más brillante para mayor visibilidad)
    uint16_t bgColor = demoButtonWasPressed ? 0x001F : TFT_NAVY;  // Azul más claro si pulsado
    tft.fillRoundRect(DEMO_BTN_X1, DEMO_BTN_Y1, btnW, btnH, 5, bgColor);
    
    // Borde con efecto 3D
    uint16_t borderColor = demoButtonWasPressed ? TFT_WHITE : TFT_CYAN;
    tft.drawRoundRect(DEMO_BTN_X1, DEMO_BTN_Y1, btnW, btnH, 5, borderColor);
    tft.drawRoundRect(DEMO_BTN_X1 + 1, DEMO_BTN_Y1 + 1, btnW - 2, btnH - 2, 4, borderColor);
    
    // Barra de progreso si está siendo pulsado
    if (demoButtonWasPressed && demoButtonProgress > 0.0f) {
        int progressW = (int)(btnW * demoButtonProgress);
        tft.fillRect(DEMO_BTN_X1 + 2, DEMO_BTN_Y2 - 6, progressW - 4, 4, TFT_GREEN);
    }
    
    // Draw button text
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, bgColor);
    tft.drawString("MENU", centerX, centerY - 4, 2);
    tft.setTextColor(TFT_CYAN, bgColor);
    tft.drawString("Pulsar 1.5s", centerX, centerY + 10, 1);
}

// Verificar si el toque está en el área del botón DEMO (con margen extra para facilitar el toque)
static bool isTouchInDemoButton(int x, int y) {
    // Añadir margen de 10px para facilitar el toque
    const int MARGIN = 10;
    return (x >= DEMO_BTN_X1 - MARGIN && x <= DEMO_BTN_X2 + MARGIN && 
            y >= DEMO_BTN_Y1 - MARGIN && y <= DEMO_BTN_Y2 + MARGIN);
}
#endif

// Dibujar botón de giro sobre eje (axis rotation toggle)
static void drawAxisRotationButton() {
    // Solo redibujar si hay cambio de estado
    if (axisRotationEnabled == lastAxisRotationState) return;
    lastAxisRotationState = axisRotationEnabled;
    
    int btnW = AXIS_BTN_X2 - AXIS_BTN_X1;
    int btnH = AXIS_BTN_Y2 - AXIS_BTN_Y1;
    int centerX = (AXIS_BTN_X1 + AXIS_BTN_X2) / 2;
    int centerY = (AXIS_BTN_Y1 + AXIS_BTN_Y2) / 2;
    
    // Colores del botón
    const uint16_t COLOR_BOX_BG = 0x2104;
    const uint16_t COLOR_BOX_HIGHLIGHT = 0x6B6D;
    const uint16_t COLOR_BOX_SHADOW = 0x1082;
    const uint16_t COLOR_ACTIVE = 0x07E0;    // Verde brillante cuando activo
    
    // Sombra 3D
    tft.fillRoundRect(AXIS_BTN_X1 + 2, AXIS_BTN_Y1 + 2, btnW, btnH, 5, COLOR_BOX_SHADOW);
    
    // Fondo del botón
    uint16_t bgColor = axisRotationEnabled ? 0x0320 : COLOR_BOX_BG;  // Verde oscuro cuando activo
    tft.fillRoundRect(AXIS_BTN_X1, AXIS_BTN_Y1, btnW, btnH, 5, bgColor);
    
    // Borde con efecto 3D
    tft.drawRoundRect(AXIS_BTN_X1, AXIS_BTN_Y1, btnW, btnH, 5, 0x4A49);
    
    // Highlight superior
    tft.drawFastHLine(AXIS_BTN_X1 + 5, AXIS_BTN_Y1 + 2, btnW - 10, COLOR_BOX_HIGHLIGHT);
    tft.drawFastHLine(AXIS_BTN_X1 + 5, AXIS_BTN_Y1 + 3, btnW - 10, COLOR_BOX_HIGHLIGHT);
    
    // Sombra inferior
    tft.drawFastHLine(AXIS_BTN_X1 + 5, AXIS_BTN_Y2 - 3, btnW - 10, COLOR_BOX_SHADOW);
    tft.drawFastHLine(AXIS_BTN_X1 + 5, AXIS_BTN_Y2 - 2, btnW - 10, COLOR_BOX_SHADOW);
    
    // Icono de rotación (flechas circulares)
    int iconCx = centerX;
    int iconCy = centerY - 6;
    int iconR = 8;
    
    uint16_t arrowColor = axisRotationEnabled ? TFT_WHITE : TFT_DARKGREY;
    
    // Dibujar arco con flechas (simulando rotación)
    tft.drawCircle(iconCx, iconCy, iconR, arrowColor);
    tft.drawCircle(iconCx, iconCy, iconR - 1, arrowColor);
    
    // Flechas de dirección
    tft.fillTriangle(iconCx + iconR - 2, iconCy - 4, 
                     iconCx + iconR + 4, iconCy, 
                     iconCx + iconR - 2, iconCy + 4, arrowColor);
    tft.fillTriangle(iconCx - iconR + 2, iconCy - 4, 
                     iconCx - iconR - 4, iconCy, 
                     iconCx - iconR + 2, iconCy + 4, arrowColor);
    
    // Texto "GIRO" debajo
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(arrowColor, bgColor);
    tft.drawString("GIRO", centerX, centerY + 12, 1);
    tft.setTextDatum(TL_DATUM);
}

// Verificar si el toque está en el área del botón de giro
static bool isTouchInAxisButton(int x, int y) {
    return (x >= AXIS_BTN_X1 && x <= AXIS_BTN_X2 && 
            y >= AXIS_BTN_Y1 && y <= AXIS_BTN_Y2);
}

// Toggle axis rotation state and update traction system
void HUD::toggleAxisRotation() {
    axisRotationEnabled = !axisRotationEnabled;
    lastAxisRotationState = !axisRotationEnabled;  // Force redraw
    
    // Actualizar sistema de tracción con el nuevo estado
    Traction::setAxisRotation(axisRotationEnabled, 30.0f);  // 30% velocidad por defecto
    
    Logger::info(axisRotationEnabled ? "Axis rotation enabled" : "Axis rotation disabled");
}

// Get axis rotation state
bool HUD::isAxisRotationEnabled() {
    return axisRotationEnabled;
}

// Colores 3D para carrocería del coche
static const uint16_t COLOR_CAR_BODY = 0x2945;       // Gris azulado oscuro
static const uint16_t COLOR_CAR_HIGHLIGHT = 0x4A69;  // Highlight superior
static const uint16_t COLOR_CAR_SHADOW = 0x18E3;     // Sombra inferior
static const uint16_t COLOR_CAR_OUTLINE = 0x6B6D;    // Borde exterior
static const uint16_t COLOR_CAR_WINDOW = 0x39C7;     // Ventanas
static const uint16_t COLOR_CAR_WINDOW_EDGE = 0x5ACB; // Borde ventanas
static const uint16_t COLOR_CAR_GRILLE = 0x2104;     // Parrilla frontal
static const uint16_t COLOR_HEADLIGHT = 0xFFE0;      // Faros (amarillo brillante)
static const uint16_t COLOR_TAILLIGHT = 0xF800;      // Luces traseras (rojo)

// Colores para barra de aceleración
static const uint16_t COLOR_BAR_BG = 0x1082;         // Fondo barra (oscuro)
static const uint16_t COLOR_BAR_BORDER = 0x2104;     // Borde barra
static const uint16_t COLOR_REF_MARKS = 0x4208;      // Marcas de referencia

// Draw car body outline connecting the four wheels
// This creates a visual representation of the vehicle in the center
// 🔒 v2.8.8: Diseño mejorado - Vista cenital más realista de un coche
static void drawCarBody() {
    // Only draw once (static background)
    if (carBodyDrawn) return;
    
    int cx = CAR_BODY_X + CAR_BODY_W / 2;  // Centro X del coche (240)
    int cy = CAR_BODY_Y + CAR_BODY_H / 2;  // Centro Y del coche (175)
    
    // Dimensiones del coche (proporciones más realistas)
    int bodyW = CAR_BODY_W - 10;      // 120px ancho
    int bodyH = CAR_BODY_H;           // 150px largo
    int frontTaper = 25;               // Estrechamiento frontal
    int rearTaper = 15;                // Estrechamiento trasero
    
    // === SOMBRA GENERAL DEL COCHE ===
    // Sombra proyectada (offset 4px)
    int sx = 4, sy = 4;
    
    // Forma aerodinámica del coche (polígono de 8 puntos)
    // Puntos del coche (sentido horario desde frontal izquierdo)
    int carPoints[16] = {
        // Frente (más estrecho - capó aerodinámico)
        cx - bodyW/2 + frontTaper, CAR_BODY_Y + 5,           // 0: Frontal izquierdo
        cx + bodyW/2 - frontTaper, CAR_BODY_Y + 5,           // 1: Frontal derecho
        // Lateral derecho
        cx + bodyW/2, CAR_BODY_Y + 35,                        // 2: Ensanchamiento delantero derecho
        cx + bodyW/2, CAR_BODY_Y + bodyH - 35,                // 3: Ensanchamiento trasero derecho
        // Trasera (un poco más estrecha)
        cx + bodyW/2 - rearTaper, CAR_BODY_Y + bodyH - 5,    // 4: Trasero derecho
        cx - bodyW/2 + rearTaper, CAR_BODY_Y + bodyH - 5,    // 5: Trasero izquierdo
        // Lateral izquierdo
        cx - bodyW/2, CAR_BODY_Y + bodyH - 35,                // 6: Ensanchamiento trasero izquierdo
        cx - bodyW/2, CAR_BODY_Y + 35                         // 7: Ensanchamiento delantero izquierdo
    };
    
    // Dibujar sombra del coche
    for (int i = 0; i < 7; i++) {
        tft.fillTriangle(
            cx + sx, cy + sy,
            carPoints[i*2] + sx, carPoints[i*2+1] + sy,
            carPoints[(i+1)*2] + sx, carPoints[(i+1)*2+1] + sy,
            0x1082
        );
    }
    tft.fillTriangle(cx + sx, cy + sy, carPoints[14] + sx, carPoints[15] + sy, carPoints[0] + sx, carPoints[1] + sy, 0x1082);
    
    // === CARROCERÍA PRINCIPAL ===
    // Dibujar cuerpo del coche (relleno con triángulos desde el centro)
    for (int i = 0; i < 7; i++) {
        tft.fillTriangle(
            cx, cy,
            carPoints[i*2], carPoints[i*2+1],
            carPoints[(i+1)*2], carPoints[(i+1)*2+1],
            COLOR_CAR_BODY
        );
    }
    tft.fillTriangle(cx, cy, carPoints[14], carPoints[15], carPoints[0], carPoints[1], COLOR_CAR_BODY);
    
    // Borde exterior del coche
    for (int i = 0; i < 7; i++) {
        tft.drawLine(carPoints[i*2], carPoints[i*2+1], carPoints[(i+1)*2], carPoints[(i+1)*2+1], COLOR_CAR_OUTLINE);
    }
    tft.drawLine(carPoints[14], carPoints[15], carPoints[0], carPoints[1], COLOR_CAR_OUTLINE);
    
    // Highlights laterales (efecto 3D)
    tft.drawLine(carPoints[0], carPoints[1], carPoints[2], carPoints[3], COLOR_CAR_HIGHLIGHT);  // Lateral delantero derecho
    tft.drawLine(carPoints[14], carPoints[15], carPoints[12], carPoints[13], COLOR_CAR_HIGHLIGHT);  // Lateral delantero izquierdo
    
    // === CAPÓ FRONTAL ===
    int hoodX = cx - 30;
    int hoodY = CAR_BODY_Y + 8;
    int hoodW = 60;
    int hoodH = 28;
    
    // Forma del capó (trapezoidal)
    tft.fillTriangle(hoodX + 5, hoodY, hoodX + hoodW - 5, hoodY, hoodX + hoodW, hoodY + hoodH, COLOR_CAR_GRILLE);
    tft.fillTriangle(hoodX + 5, hoodY, hoodX + hoodW, hoodY + hoodH, hoodX, hoodY + hoodH, COLOR_CAR_GRILLE);
    tft.drawLine(hoodX + 5, hoodY, hoodX + hoodW - 5, hoodY, COLOR_CAR_OUTLINE);
    
    // Faros delanteros (forma de gota más realista)
    tft.fillRoundRect(hoodX - 8, hoodY + 8, 18, 12, 3, COLOR_HEADLIGHT);
    tft.drawRoundRect(hoodX - 8, hoodY + 8, 18, 12, 3, 0x8410);
    tft.fillRoundRect(hoodX + hoodW - 10, hoodY + 8, 18, 12, 3, COLOR_HEADLIGHT);
    tft.drawRoundRect(hoodX + hoodW - 10, hoodY + 8, 18, 12, 3, 0x8410);
    
    // Parrilla central con detalle
    tft.fillRoundRect(hoodX + 15, hoodY + 15, hoodW - 30, 10, 2, 0x1082);
    tft.drawRoundRect(hoodX + 15, hoodY + 15, hoodW - 30, 10, 2, 0x4A69);
    // Barras de parrilla
    for (int i = 0; i < 5; i++) {
        tft.drawFastVLine(hoodX + 20 + i * 5, hoodY + 17, 6, 0x6B6D);
    }
    
    // === PARABRISAS Y TECHO ===
    int windshieldY = CAR_BODY_Y + 40;
    int windshieldH = 25;
    int roofY = windshieldY + windshieldH;
    int roofH = 35;
    int rearWindowY = roofY + roofH;
    int rearWindowH = 20;
    
    // Parabrisas (cristal frontal)
    int wsX1 = cx - 35, wsX2 = cx + 35;
    int wsY1 = windshieldY, wsY2 = windshieldY + windshieldH;
    tft.fillRect(wsX1, wsY1, wsX2 - wsX1, wsY2 - wsY1, 0x2945);  // Cristal oscuro
    tft.drawRect(wsX1, wsY1, wsX2 - wsX1, wsY2 - wsY1, 0x5ACB);
    // Reflejo del parabrisas
    tft.drawLine(wsX1 + 5, wsY1 + 3, wsX1 + 20, wsY1 + 3, 0x7BEF);
    tft.drawLine(wsX1 + 5, wsY1 + 4, wsX1 + 15, wsY1 + 4, 0x6B6D);
    
    // Techo (zona central)
    tft.fillRect(cx - 40, roofY, 80, roofH, COLOR_CAR_BODY);
    tft.drawRect(cx - 40, roofY, 80, roofH, 0x4A69);
    // Línea central del techo
    tft.drawFastVLine(cx, roofY + 5, roofH - 10, 0x5ACB);
    // Techo solar (opcional - pequeño rectángulo oscuro)
    tft.fillRect(cx - 15, roofY + 8, 30, 18, 0x2104);
    tft.drawRect(cx - 15, roofY + 8, 30, 18, 0x4A69);
    
    // Luneta trasera
    tft.fillRect(cx - 32, rearWindowY, 64, rearWindowH, 0x2945);
    tft.drawRect(cx - 32, rearWindowY, 64, rearWindowH, 0x5ACB);
    // Reflejo
    tft.drawLine(cx - 27, rearWindowY + 3, cx - 12, rearWindowY + 3, 0x7BEF);
    
    // === MALETERO TRASERO ===
    int trunkY = CAR_BODY_Y + bodyH - 32;
    int trunkH = 25;
    
    tft.fillRoundRect(cx - 35, trunkY, 70, trunkH, 4, COLOR_CAR_GRILLE);
    tft.drawRoundRect(cx - 35, trunkY, 70, trunkH, 4, COLOR_CAR_OUTLINE);
    
    // Luces traseras (más detalladas)
    tft.fillRoundRect(cx - 48, trunkY + 5, 20, 14, 3, COLOR_TAILLIGHT);
    tft.drawRoundRect(cx - 48, trunkY + 5, 20, 14, 3, 0x8000);
    tft.fillRoundRect(cx + 28, trunkY + 5, 20, 14, 3, COLOR_TAILLIGHT);
    tft.drawRoundRect(cx + 28, trunkY + 5, 20, 14, 3, 0x8000);
    
    // Placa de matrícula
    tft.fillRect(cx - 12, trunkY + 12, 24, 8, TFT_WHITE);
    tft.drawRect(cx - 12, trunkY + 12, 24, 8, TFT_DARKGREY);
    
    // === RETROVISORES LATERALES ===
    // Retrovisor izquierdo
    tft.fillRoundRect(cx - bodyW/2 - 12, CAR_BODY_Y + 45, 15, 8, 2, COLOR_CAR_BODY);
    tft.drawRoundRect(cx - bodyW/2 - 12, CAR_BODY_Y + 45, 15, 8, 2, COLOR_CAR_OUTLINE);
    tft.fillRect(cx - bodyW/2 - 10, CAR_BODY_Y + 47, 8, 4, 0x2945);  // Espejo
    
    // Retrovisor derecho
    tft.fillRoundRect(cx + bodyW/2 - 3, CAR_BODY_Y + 45, 15, 8, 2, COLOR_CAR_BODY);
    tft.drawRoundRect(cx + bodyW/2 - 3, CAR_BODY_Y + 45, 15, 8, 2, COLOR_CAR_OUTLINE);
    tft.fillRect(cx + bodyW/2 + 2, CAR_BODY_Y + 47, 8, 4, 0x2945);  // Espejo
    
    // === PUERTAS (líneas sutiles) ===
    // Puerta delantera izquierda
    tft.drawLine(cx - 38, windshieldY + 5, cx - 38, rearWindowY - 5, 0x4A69);
    // Puerta trasera izquierda  
    tft.drawLine(cx - 38, roofY + 5, cx - 38, rearWindowY + 15, 0x4A69);
    // Puerta delantera derecha
    tft.drawLine(cx + 38, windshieldY + 5, cx + 38, rearWindowY - 5, 0x4A69);
    // Puerta trasera derecha
    tft.drawLine(cx + 38, roofY + 5, cx + 38, rearWindowY + 15, 0x4A69);
    
    // Manijas de puertas
    tft.fillRect(cx - 45, roofY + 2, 6, 3, 0x8410);
    tft.fillRect(cx - 45, roofY + roofH - 8, 6, 3, 0x8410);
    tft.fillRect(cx + 39, roofY + 2, 6, 3, 0x8410);
    tft.fillRect(cx + 39, roofY + roofH - 8, 6, 3, 0x8410);
    
    // === SISTEMA DE TRACCIÓN - CARDANES Y EJES ===
    // Colores para el sistema de transmisión
    const uint16_t COLOR_AXLE = 0x6B6D;        // Gris medio para ejes
    const uint16_t COLOR_AXLE_DARK = 0x4208;   // Gris oscuro para sombras
    const uint16_t COLOR_CARDAN = 0x8410;      // Gris claro para cardanes
    const uint16_t COLOR_DIFF = 0x7BEF;        // Gris brillante para diferenciales
    
    // Posición del diferencial central (debajo del techo)
    int diffCenterY = cy + 5;
    
    // === DIFERENCIAL CENTRAL (caja de transferencia) ===
    tft.fillRoundRect(cx - 8, diffCenterY - 6, 16, 12, 3, COLOR_DIFF);
    tft.drawRoundRect(cx - 8, diffCenterY - 6, 16, 12, 3, COLOR_AXLE_DARK);
    tft.fillCircle(cx, diffCenterY, 3, COLOR_AXLE_DARK);
    
    // === EJE DELANTERO (conecta ruedas FL y FR) ===
    // Diferencial delantero
    int diffFrontY = Y_FL;
    tft.fillRoundRect(cx - 6, diffFrontY - 5, 12, 10, 2, COLOR_DIFF);
    tft.drawRoundRect(cx - 6, diffFrontY - 5, 12, 10, 2, COLOR_AXLE_DARK);
    
    // Semi-ejes delanteros (del diferencial a las ruedas)
    // Eje izquierdo
    tft.drawLine(cx - 6, diffFrontY, X_FL + 18, Y_FL, COLOR_AXLE);
    tft.drawLine(cx - 6, diffFrontY + 1, X_FL + 18, Y_FL + 1, COLOR_AXLE_DARK);
    // Eje derecho
    tft.drawLine(cx + 6, diffFrontY, X_FR - 18, Y_FR, COLOR_AXLE);
    tft.drawLine(cx + 6, diffFrontY + 1, X_FR - 18, Y_FR + 1, COLOR_AXLE_DARK);
    
    // Juntas homocinéticas delanteras (círculos pequeños en las ruedas)
    tft.fillCircle(X_FL + 18, Y_FL, 4, COLOR_CARDAN);
    tft.drawCircle(X_FL + 18, Y_FL, 4, COLOR_AXLE_DARK);
    tft.fillCircle(X_FR - 18, Y_FR, 4, COLOR_CARDAN);
    tft.drawCircle(X_FR - 18, Y_FR, 4, COLOR_AXLE_DARK);
    
    // === EJE TRASERO (conecta ruedas RL y RR) ===
    // Diferencial trasero
    int diffRearY = Y_RL;
    tft.fillRoundRect(cx - 6, diffRearY - 5, 12, 10, 2, COLOR_DIFF);
    tft.drawRoundRect(cx - 6, diffRearY - 5, 12, 10, 2, COLOR_AXLE_DARK);
    
    // Semi-ejes traseros
    // Eje izquierdo
    tft.drawLine(cx - 6, diffRearY, X_RL + 18, Y_RL, COLOR_AXLE);
    tft.drawLine(cx - 6, diffRearY - 1, X_RL + 18, Y_RL - 1, COLOR_AXLE_DARK);
    // Eje derecho
    tft.drawLine(cx + 6, diffRearY, X_RR - 18, Y_RR, COLOR_AXLE);
    tft.drawLine(cx + 6, diffRearY - 1, X_RR - 18, Y_RR - 1, COLOR_AXLE_DARK);
    
    // Juntas homocinéticas traseras
    tft.fillCircle(X_RL + 18, Y_RL, 4, COLOR_CARDAN);
    tft.drawCircle(X_RL + 18, Y_RL, 4, COLOR_AXLE_DARK);
    tft.fillCircle(X_RR - 18, Y_RR, 4, COLOR_CARDAN);
    tft.drawCircle(X_RR - 18, Y_RR, 4, COLOR_AXLE_DARK);
    
    // === ÁRBOL DE TRANSMISIÓN (cardán central) ===
    // Conecta diferencial central con delantero
    tft.drawLine(cx, diffCenterY - 6, cx, diffFrontY + 5, COLOR_AXLE);
    tft.drawLine(cx - 1, diffCenterY - 6, cx - 1, diffFrontY + 5, COLOR_AXLE_DARK);
    tft.drawLine(cx + 1, diffCenterY - 6, cx + 1, diffFrontY + 5, COLOR_AXLE);
    
    // Junta cardán frontal
    tft.fillCircle(cx, diffFrontY + 5, 3, COLOR_CARDAN);
    tft.drawCircle(cx, diffFrontY + 5, 3, COLOR_AXLE_DARK);
    
    // Conecta diferencial central con trasero
    tft.drawLine(cx, diffCenterY + 6, cx, diffRearY - 5, COLOR_AXLE);
    tft.drawLine(cx - 1, diffCenterY + 6, cx - 1, diffRearY - 5, COLOR_AXLE_DARK);
    tft.drawLine(cx + 1, diffCenterY + 6, cx + 1, diffRearY - 5, COLOR_AXLE);
    
    // Junta cardán trasera
    tft.fillCircle(cx, diffRearY - 5, 3, COLOR_CARDAN);
    tft.drawCircle(cx, diffRearY - 5, 3, COLOR_AXLE_DARK);
    
    carBodyDrawn = true;
}

// Colores para el volante
static const uint16_t COLOR_WHEEL_RIM = 0x8410;      // Gris oscuro para el aro
static const uint16_t COLOR_WHEEL_SPOKE = 0xA514;    // Gris medio para los radios
static const uint16_t COLOR_WHEEL_CENTER = 0x4A49;   // Gris para el centro
static const uint16_t COLOR_WHEEL_HIGHLIGHT = 0xC618; // Highlight 3D

// Dibujar volante gráfico con rotación y mostrar ángulo en grados
static void drawSteeringWheel(float angleDeg) {
    // Posición: centro del coche
    const int cx = CAR_BODY_X + CAR_BODY_W / 2;  // Centro X del coche (240)
    const int cy = CAR_BODY_Y + CAR_BODY_H / 2;  // Centro Y del coche (175)
    const int wheelRadius = 25;   // Radio del volante
    const int innerRadius = 18;   // Radio interior (entre aro y centro)
    const int centerRadius = 8;   // Radio del centro del volante
    
    // Solo redibujar si hay cambio significativo (>0.5 grados)
    if (fabs(angleDeg - lastSteeringAngle) < 0.5f) return;
    lastSteeringAngle = angleDeg;
    
    // Limpiar área del volante
    tft.fillCircle(cx, cy, wheelRadius + 5, TFT_BLACK);
    
    // Convertir ángulo a radianes
    float rad = angleDeg * DEG_TO_RAD;
    
    // Color según ángulo: verde si centrado, amarillo si girado, rojo si muy girado
    uint16_t rimColor;
    float absAngle = fabs(angleDeg);
    if (absAngle < 5.0f) {
        rimColor = TFT_GREEN;       // Centrado
    } else if (absAngle < 20.0f) {
        rimColor = TFT_CYAN;        // Ligeramente girado
    } else if (absAngle <= 35.0f) {
        rimColor = TFT_YELLOW;      // Girado moderadamente
    } else {
        rimColor = TFT_ORANGE;      // Muy girado
    }
    
    // === Dibujar aro exterior del volante ===
    // Sombra del aro
    tft.drawCircle(cx + 1, cy + 1, wheelRadius, 0x2104);
    tft.drawCircle(cx + 1, cy + 1, wheelRadius - 1, 0x2104);
    // Aro principal
    tft.drawCircle(cx, cy, wheelRadius, rimColor);
    tft.drawCircle(cx, cy, wheelRadius - 1, rimColor);
    tft.drawCircle(cx, cy, wheelRadius - 2, COLOR_WHEEL_RIM);
    
    // === Dibujar 3 radios del volante (rotados según ángulo) ===
    for (int i = 0; i < 3; i++) {
        // Ángulos de los radios: 0°, 120°, 240° + rotación del volante
        float spokeAngle = rad + (i * 120.0f * DEG_TO_RAD);
        
        // Puntos inicio (cerca del centro) y fin (en el aro)
        int x1 = cx + (int)(cosf(spokeAngle) * centerRadius);
        int y1 = cy + (int)(sinf(spokeAngle) * centerRadius);
        int x2 = cx + (int)(cosf(spokeAngle) * innerRadius);
        int y2 = cy + (int)(sinf(spokeAngle) * innerRadius);
        
        // Dibujar radio con grosor
        tft.drawLine(x1, y1, x2, y2, COLOR_WHEEL_SPOKE);
        tft.drawLine(x1 - 1, y1, x2 - 1, y2, COLOR_WHEEL_SPOKE);
        tft.drawLine(x1 + 1, y1, x2 + 1, y2, COLOR_WHEEL_RIM);
    }
    
    // === Dibujar centro del volante (hub) ===
    // Sombra 3D
    tft.fillCircle(cx + 1, cy + 1, centerRadius, 0x2104);
    // Centro principal
    tft.fillCircle(cx, cy, centerRadius, COLOR_WHEEL_CENTER);
    tft.drawCircle(cx, cy, centerRadius, COLOR_WHEEL_RIM);
    // Highlight 3D
    tft.fillCircle(cx - 2, cy - 2, 2, COLOR_WHEEL_HIGHLIGHT);
    
    // === Indicador de posición 12 en punto (para referencia visual) ===
    // Línea que marca el "arriba" del volante rotado
    float topAngle = rad - (90.0f * DEG_TO_RAD);  // -90° es arriba en coords de pantalla
    int topX = cx + (int)(cosf(topAngle) * (wheelRadius - 5));
    int topY = cy + (int)(sinf(topAngle) * (wheelRadius - 5));
    tft.fillCircle(topX, topY, 3, rimColor);
    
    // === Mostrar grados debajo del volante ===
    const int textY = cy + wheelRadius + 12;
    tft.fillRect(cx - 25, textY - 6, 50, 14, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(rimColor, TFT_BLACK);
    char buf[16];
    snprintf(buf, sizeof(buf), "%+.0f\xB0", angleDeg);  // Formato: +0° o -15° (0xB0 = símbolo grado)
    tft.drawString(buf, cx, textY, 2);
}

// DEPRECATED: Mantener compatibilidad con nombre anterior
// TODO: Eliminar en próxima versión mayor, usar drawSteeringWheel() directamente
static void drawSteeringAngle(float angleDeg) {
    drawSteeringWheel(angleDeg);
}

void HUD::drawPedalBar(float pedalPercent) {
    const int y = 300;       // Posición vertical
    const int height = 18;   // Altura de la barra
    const int width = 480;   // Ancho total de pantalla
    const int radius = 4;    // Radio de esquinas redondeadas

    if (pedalPercent < 0.0f) {
        // Pedal inválido → barra vacía con "--"
        tft.fillRoundRect(0, y, width, height, radius, COLOR_BAR_BG);
        tft.drawRoundRect(0, y, width, height, radius, TFT_DARKGREY);
        tft.setTextDatum(MC_DATUM);
        tft.setTextColor(TFT_DARKGREY, COLOR_BAR_BG);
        tft.drawString("-- PEDAL --", 240, y + height/2, 2);
        return;
    }

    // Mapear porcentaje a ancho de barra (0-100% → 0-480px)
    int barWidth = map((int)pedalPercent, 0, 100, 0, width);
    if (barWidth > width) barWidth = width;

    // Colores con gradientes para efecto 3D
    uint16_t colorMain, colorLight, colorDark;
    if (pedalPercent > 95.0f) {
        colorMain = TFT_RED;
        colorLight = 0xFBEF;  // Rojo claro
        colorDark = 0x8000;   // Rojo oscuro
    } else if (pedalPercent > 80.0f) {
        colorMain = TFT_YELLOW;
        colorLight = 0xFFE0;  // Amarillo claro
        colorDark = 0x8400;   // Amarillo oscuro
    } else {
        colorMain = 0x07E0;   // Verde
        colorLight = 0x87F0;  // Verde claro
        colorDark = 0x03E0;   // Verde oscuro
    }

    // Fondo de barra con efecto 3D (hundido)
    tft.fillRoundRect(0, y, width, height, radius, COLOR_BAR_BG);
    tft.drawRoundRect(0, y, width, height, radius, COLOR_BAR_BORDER);
    
    // Barra de progreso principal
    if (barWidth > 0) {
        tft.fillRoundRect(0, y, barWidth, height, radius, colorMain);
        
        // Efecto 3D: línea clara arriba
        tft.drawFastHLine(2, y + 2, barWidth - 4, colorLight);
        tft.drawFastHLine(2, y + 3, barWidth - 4, colorLight);
        
        // Efecto 3D: línea oscura abajo
        tft.drawFastHLine(2, y + height - 3, barWidth - 4, colorDark);
        tft.drawFastHLine(2, y + height - 2, barWidth - 4, colorDark);
        
        // Borde de la barra activa
        tft.drawRoundRect(0, y, barWidth, height, radius, colorDark);
    }

    // Texto con porcentaje en el centro (con sombra)
    tft.setTextDatum(MC_DATUM);
    // Sombra
    tft.setTextColor(0x0000);
    char txt[16];
    snprintf(txt, sizeof(txt), "ACCEL %d%%", (int)pedalPercent);
    tft.drawString(txt, 241, y + height/2 + 1, 2);
    // Texto principal
    tft.setTextColor(TFT_WHITE);
    tft.drawString(txt, 240, y + height/2, 2);
    
    // Marcas de referencia (25%, 50%, 75%)
    tft.drawFastVLine(width / 4, y + 2, height - 4, COLOR_REF_MARKS);
    tft.drawFastVLine(width / 2, y + 2, height - 4, COLOR_REF_MARKS);
    tft.drawFastVLine(width * 3 / 4, y + 2, height - 4, COLOR_REF_MARKS);
}

void HUD::update() {
    // 🔒 v2.8.4: Diagnóstico visual - píxeles rojos para localizar bloqueos de render
#ifdef DEBUG_RENDER
    // Fase 1: entrada a update()
    tft.drawPixel(1, 0, TFT_RED);
#endif
    
#ifdef STANDALONE_DISPLAY
    // STANDALONE MODE: Use static simulated sensor values
    // NOTE: Animation is handled by main.cpp loop() which updates HUDManager with CarData
    // This block provides fallback values when HUD::update() is called directly
    
    float speedKmh = 12.0f;          // Static fallback speed
    float rpm = 850.0f;              // Static fallback RPM
    float pedalPercent = 50.0f;      // Static fallback pedal position
    float steerAngleFL = 0.0f;       // Static fallback steering
    float steerAngleFR = 0.0f;
    Shifter::Gear gear = Shifter::Gear::P;
    System::State sys = System::State::READY;
    
    // Static wheel temperatures
    float wheelTempFL = 42.0f;
    float wheelTempFR = 42.0f;
    float wheelTempRL = 40.0f;
    float wheelTempRR = 40.0f;
    
    // Static motor effort
    float wheelEffortFL = 30.0f;
    float wheelEffortFR = 30.0f;
    float wheelEffortRL = 28.0f;
    float wheelEffortRR = 28.0f;
    
    // Simulated button state (needed for MenuHidden)
    Buttons::State btns;
    btns.batteryIcon = false;
    btns.lights = false;
    btns.multimedia = false;
    btns.mode4x4 = false;
    
    // Static feature states
    bool lights = false;
    bool multimedia = false;
    bool mode4x4 = true;
    bool eco = false;
    
    // Simulated sensor status for standalone mode (all OK)
    uint8_t sensorCurrentOK = 6;
    uint8_t sensorTempOK = 5;
    uint8_t sensorWheelOK = 4;
    bool tempWarning = false;
    float maxTemp = 42.0f;
    
    // Simulated ambient temperature
    float ambientTemp = 22.0f;
    
#else
    auto pedal = Pedal::get();
    auto steer = Steering::get();
    auto sh    = Shifter::get();
    auto sys   = System::getState();
    auto tr    = Traction::get();
    auto btns  = Buttons::get();

    // Velocidad global = media ruedas delanteras
    float vFL = cfg.wheelSensorsEnabled ? Sensors::getWheelSpeed(0) : 0.0f;
    float vFR = cfg.wheelSensorsEnabled ? Sensors::getWheelSpeed(1) : 0.0f;
    float speedKmh = (vFL + vFR) * 0.5f;
    if(speedKmh > MAX_SPEED_KMH) speedKmh = MAX_SPEED_KMH;

    // Tacómetro proporcional a velocidad (PLACEHOLDER)
    // TODO: Replace with actual RPM from motor encoder/controller when sensors connected
    // Current implementation: RPM = (speed/maxSpeed) * maxRPM for visualization only
    float rpm = (speedKmh / MAX_SPEED_KMH) * MAX_RPM;
    if(rpm > MAX_RPM) rpm = MAX_RPM;
    
    float pedalPercent = pedal.valid ? pedal.percent : -1.0f;
    float steerAngleFL = cfg.steeringEnabled ? steer.angleFL : 0.0f;
    float steerAngleFR = cfg.steeringEnabled ? steer.angleFR : 0.0f;
    float wheelTempFL = cfg.tempSensorsEnabled ? tr.w[Traction::FL].tempC : -999.0f;
    float wheelTempFR = cfg.tempSensorsEnabled ? tr.w[Traction::FR].tempC : -999.0f;
    float wheelTempRL = cfg.tempSensorsEnabled ? tr.w[Traction::RL].tempC : -999.0f;
    float wheelTempRR = cfg.tempSensorsEnabled ? tr.w[Traction::RR].tempC : -999.0f;
    float wheelEffortFL = cfg.currentSensorsEnabled ? tr.w[Traction::FL].effortPct : -1.0f;
    float wheelEffortFR = cfg.currentSensorsEnabled ? tr.w[Traction::FR].effortPct : -1.0f;
    float wheelEffortRL = cfg.currentSensorsEnabled ? tr.w[Traction::RL].effortPct : -1.0f;
    float wheelEffortRR = cfg.currentSensorsEnabled ? tr.w[Traction::RR].effortPct : -1.0f;
    Shifter::Gear gear = sh.gear;
    bool lights = cfg.lightsEnabled && btns.lights;
    bool multimedia = cfg.multimediaEnabled && btns.multimedia;
    bool mode4x4 = tr.enabled4x4;
    bool eco = pedal.percent > 0 && !btns.mode4x4;
    
    // Obtener estado de sensores para indicadores
    Sensors::SystemStatus sensorStatus = Sensors::getSystemStatus();
    uint8_t sensorCurrentOK = sensorStatus.currentSensorsOK;
    uint8_t sensorTempOK = sensorStatus.temperatureSensorsOK;
    uint8_t sensorWheelOK = sensorStatus.wheelSensorsOK;
    bool tempWarning = sensorStatus.temperatureWarning;
    float maxTemp = sensorStatus.maxTemperature;
    
    // Temperatura ambiente desde sensor DS18B20 #4
    float ambientTemp = cfg.tempSensorsEnabled ? Sensors::getTemperature(4) : 22.0f;
#endif

#ifdef DEBUG_RENDER
    // 🔒 v2.8.4: Fase 2 - después de obtener datos de sensores
    tft.drawPixel(2, 0, TFT_RED);
#endif
    
    // Draw car body outline (once, static background)
    drawCarBody();
    
#ifdef DEBUG_RENDER
    // 🔒 v2.8.4: Fase 3 - después de dibujar carrocería
    tft.drawPixel(3, 0, TFT_RED);
#endif

    // Render gauges (ya optimizados internamente)
    Gauges::drawSpeed(X_SPEED, Y_SPEED, speedKmh, MAX_SPEED_KMH, pedalPercent);
    
#ifdef DEBUG_RENDER
    // 🔒 v2.8.4: Fase 4 - después de gauge velocidad
    tft.drawPixel(4, 0, TFT_RED);
#endif
    
    Gauges::drawRPM(X_RPM, Y_RPM, rpm, MAX_RPM);
    
#ifdef DEBUG_RENDER
    // 🔒 v2.8.4: Fase 5 - después de gauge RPM
    tft.drawPixel(5, 0, TFT_RED);
#endif

    // Ruedas (optimizado: solo redibuja si cambian ángulo/temp/esfuerzo)
    // Usar -999.0f para temp y -1.0f para effort cuando sensores deshabilitados
    WheelsDisplay::drawWheel(X_FL, Y_FL, steerAngleFL, wheelTempFL, wheelEffortFL);
    WheelsDisplay::drawWheel(X_FR, Y_FR, steerAngleFR, wheelTempFR, wheelEffortFR);
    WheelsDisplay::drawWheel(X_RL, Y_RL, 0.0f, wheelTempRL, wheelEffortRL);
    WheelsDisplay::drawWheel(X_RR, Y_RR, 0.0f, wheelTempRR, wheelEffortRR);
    
#ifdef DEBUG_RENDER
    // 🔒 v2.8.4: Fase 6 - después de dibujar ruedas
    tft.drawPixel(6, 0, TFT_RED);
#endif;
    
    // Mostrar ángulo del volante en grados (promedio de FL/FR)
    float avgSteerAngle = (steerAngleFL + steerAngleFR) / 2.0f;
    drawSteeringAngle(avgSteerAngle);

    // Iconos y estados
    Icons::drawSystemState(sys);
    Icons::drawGear(gear);
    Icons::drawFeatures(lights, multimedia, mode4x4, eco);
#ifdef STANDALONE_DISPLAY
    Icons::drawBattery(24.5f);  // Simulated battery voltage
#else
    Icons::drawBattery(cfg.currentSensorsEnabled ? Sensors::getVoltage(0) : 0.0f);
#endif

    // Temperatura ambiente (esquina superior derecha, debajo de batería)
    Icons::drawAmbientTemp(ambientTemp);

    // Icono de advertencia si hay errores almacenados
    Icons::drawErrorWarning();
    
    // Indicador de estado de sensores (LEDs I/T/W)
    Icons::drawSensorStatus(sensorCurrentOK, sensorTempOK, sensorWheelOK,
                           Sensors::NUM_CURRENTS, Sensors::NUM_TEMPS, Sensors::NUM_WHEELS);
    
    // Advertencia de temperatura crítica
    Icons::drawTempWarning(tempWarning, maxTemp);

    // Barra de pedal en la parte inferior
    drawPedalBar(pedalPercent);
    
    // Botón de giro sobre eje (axis rotation)
    drawAxisRotationButton();
    
#ifdef STANDALONE_DISPLAY
    // Draw demo button for easy hidden menu access
    drawDemoButton();
#endif

    // --- Lectura táctil usando touch_map ---
    // 🔒 v2.8.8: Solo leer touch si está habilitado e inicializado (usando TFT_eSPI integrado)
    bool batteryTouch = false;
#ifdef STANDALONE_DISPLAY
    bool demoButtonTouched = false;
    static bool hiddenMenuJustActivated = false;
#endif

#ifndef DISABLE_TOUCH
    uint16_t touchX = 0, touchY = 0;
    // 🔒 v2.8.8: Usar tft.getTouch() del touch integrado de TFT_eSPI
    if (touchInitialized && cfg.touchEnabled && tft.getTouch(&touchX, &touchY)) {
        int x = (int)touchX;
        int y = (int)touchY;

#ifdef STANDALONE_DISPLAY
        // Check demo button touch with long press detection
        // Reset long press timer if touch moves outside button area
        bool touchInButton = isTouchInDemoButton(x, y) && !MenuHidden::isActive();
        if (touchInButton) {
            uint32_t now = millis();
            if (!demoButtonWasPressed) {
                demoButtonPressStart = now;
                demoButtonWasPressed = true;
                demoButtonProgress = 0.0f;
                Logger::info("Demo button touched - hold 1.5s for menu");
            } else {
                // Actualizar progreso visual
                uint32_t elapsed = now - demoButtonPressStart;
                demoButtonProgress = (float)elapsed / (float)DEMO_BUTTON_LONG_PRESS_MS;
                if (demoButtonProgress > 1.0f) demoButtonProgress = 1.0f;
                
                if (elapsed >= DEMO_BUTTON_LONG_PRESS_MS) {
                    // Long press detected - activate hidden menu directly
                    demoButtonTouched = true;
                    hiddenMenuJustActivated = true;
                    demoButtonWasPressed = false;
                    demoButtonProgress = 0.0f;
                    Logger::info("Demo button long press - activating hidden menu");
                }
            }
        } else {
            // Reset if touch moved outside button area
            demoButtonWasPressed = false;
            demoButtonProgress = 0.0f;
        }
#endif

        // Check axis rotation button touch (simple toggle on touch)
        static bool axisButtonWasPressed = false;
        if (isTouchInAxisButton(x, y)) {
            if (!axisButtonWasPressed) {
                axisButtonWasPressed = true;
                toggleAxisRotation();
            }
        } else {
            axisButtonWasPressed = false;
        }

        TouchAction act = getTouchedZone(x, y);
        switch (act) {
            case TouchAction::Battery:
                batteryTouch = true;
                break;
            case TouchAction::Lights:
                Logger::info("Toque en icono luces");
                break;
            case TouchAction::Multimedia:
                Logger::info("Toque en icono multimedia");
                break;
            case TouchAction::Mode4x4:
                Logger::info("Toque en icono 4x4");
                break;
            case TouchAction::Warning:
                Logger::info("Toque en icono warning");
                break;
            default:
                break;
        }
    } else {
#ifdef STANDALONE_DISPLAY
        demoButtonWasPressed = false;
        demoButtonProgress = 0.0f;
#endif
    }
#endif  // DISABLE_TOUCH

#ifdef STANDALONE_DISPLAY
    // In demo mode, activate hidden menu directly on demo button long press
    // This bypasses the code entry mechanism for easier testing
    if (demoButtonTouched && hiddenMenuJustActivated) {
        MenuHidden::activateDirectly();
        hiddenMenuJustActivated = false;
    } else {
        // Normal battery touch handling
        MenuHidden::update(btns.batteryIcon || batteryTouch);
    }
#else
    // Menú oculto: botón físico o toque en batería
    MenuHidden::update(btns.batteryIcon || batteryTouch);
#endif
}