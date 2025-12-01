#include "hud.h"
#include <Arduino.h>                // Para DEG_TO_RAD, millis, constrain
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>   // Librería táctil
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

// 🔒 v2.8.7: HSPI bus compartido entre TFT y Touch
// El XPT2046 debe usar el mismo bus SPI que el TFT (HSPI) para evitar conflictos
// que causan pantalla blanca al arrancar cuando touch está habilitado
#ifdef USE_HSPI_PORT
static SPIClass touchSpi = SPIClass(HSPI);
#else
static SPIClass& touchSpi = SPI;
#endif

static XPT2046_Touchscreen touch(PIN_TOUCH_CS, PIN_TOUCH_IRQ);

// 🔒 v2.8.6: Track touch initialization state for runtime checks
static bool touchInitialized = false;

// Touch calibration: Using constants from touch_map.h (included above)
// TouchCalibration::RAW_MIN, RAW_MAX, SCREEN_WIDTH, SCREEN_HEIGHT

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

    // 🔒 v2.8.6: TOUCH INITIALIZATION - Conditional based on config flag
    // Initialize touchInitialized to false before attempting
    touchInitialized = false;
    
#ifndef DISABLE_TOUCH
    if (cfg.touchEnabled) {
        // 🔒 v2.8.7: Usar HSPI para el touch (mismo bus que TFT)
        // Esto evita conflictos de bus SPI que causan pantalla blanca
        if (touch.begin(touchSpi)) {
            touch.setRotation(3);  // Match TFT rotation for proper touch mapping
            MenuHidden::initTouch(&touch);  // Pasar puntero táctil al menú oculto
            touchInitialized = true;
            Logger::info("Touchscreen XPT2046 inicializado OK (HSPI compartido)");
        } else {
            Logger::error("Touchscreen XPT2046 no detectado - deshabilitando touch para esta sesión");
            // NOTE: We don't persist this change to storage intentionally.
            // This allows touch to be retried on next boot in case it was a 
            // temporary issue (e.g., loose connection). If the user wants to
            // permanently disable touch, they can do so via the hidden menu
            // or by using the DISABLE_TOUCH compile flag.
            System::logError(760); // código reservado: fallo táctil
        }
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
static void drawCarBody() {
    // Only draw once (static background)
    if (carBodyDrawn) return;
    
    int cx = CAR_BODY_X + CAR_BODY_W / 2;  // Centro X del coche
    int cy = CAR_BODY_Y + CAR_BODY_H / 2;  // Centro Y del coche
    
    // === Sombra del coche (proyección 3D) ===
    tft.fillRoundRect(CAR_BODY_X + 3, CAR_BODY_Y + 3, CAR_BODY_W, CAR_BODY_H, 8, 0x1082);
    
    // === Carrocería principal con esquinas redondeadas ===
    tft.fillRoundRect(CAR_BODY_X, CAR_BODY_Y, CAR_BODY_W, CAR_BODY_H, 8, COLOR_CAR_BODY);
    
    // Gradiente de profundidad 3D
    // Highlight superior
    tft.drawFastHLine(CAR_BODY_X + 10, CAR_BODY_Y + 2, CAR_BODY_W - 20, COLOR_CAR_HIGHLIGHT);
    tft.drawFastHLine(CAR_BODY_X + 10, CAR_BODY_Y + 3, CAR_BODY_W - 20, COLOR_CAR_HIGHLIGHT);
    // Sombra inferior
    tft.drawFastHLine(CAR_BODY_X + 10, CAR_BODY_Y + CAR_BODY_H - 3, CAR_BODY_W - 20, COLOR_CAR_SHADOW);
    tft.drawFastHLine(CAR_BODY_X + 10, CAR_BODY_Y + CAR_BODY_H - 2, CAR_BODY_W - 20, COLOR_CAR_SHADOW);
    
    // Borde exterior con efecto metálico
    tft.drawRoundRect(CAR_BODY_X, CAR_BODY_Y, CAR_BODY_W, CAR_BODY_H, 8, COLOR_CAR_OUTLINE);
    tft.drawRoundRect(CAR_BODY_X + 1, CAR_BODY_Y + 1, CAR_BODY_W - 2, CAR_BODY_H - 2, 7, 0x4A49);
    
    // === Capó frontal (parte superior del coche - vista cenital) ===
    int hoodX = CAR_BODY_X + 20;
    int hoodW = CAR_BODY_W - 40;
    int hoodY = CAR_BODY_Y + 5;
    int hoodH = 25;
    
    // Área del capó
    tft.fillRoundRect(hoodX, hoodY, hoodW, hoodH, 4, COLOR_CAR_GRILLE);
    tft.drawRoundRect(hoodX, hoodY, hoodW, hoodH, 4, COLOR_CAR_OUTLINE);
    
    // Faros delanteros
    tft.fillRoundRect(hoodX + 5, hoodY + 5, 15, 10, 2, COLOR_HEADLIGHT);
    tft.fillRoundRect(hoodX + hoodW - 20, hoodY + 5, 15, 10, 2, COLOR_HEADLIGHT);
    
    // Parrilla central
    tft.fillRect(hoodX + 25, hoodY + 8, hoodW - 50, 8, 0x1082);
    // Líneas de parrilla
    for (int i = 0; i < 4; i++) {
        tft.drawFastVLine(hoodX + 30 + i * 12, hoodY + 8, 8, COLOR_CAR_OUTLINE);
    }
    
    // === Maletero trasero ===
    int trunkY = CAR_BODY_Y + CAR_BODY_H - 30;
    int trunkH = 25;
    
    tft.fillRoundRect(hoodX, trunkY, hoodW, trunkH, 4, COLOR_CAR_GRILLE);
    tft.drawRoundRect(hoodX, trunkY, hoodW, trunkH, 4, COLOR_CAR_OUTLINE);
    
    // Luces traseras
    tft.fillRoundRect(hoodX + 5, trunkY + 10, 18, 8, 2, COLOR_TAILLIGHT);
    tft.fillRoundRect(hoodX + hoodW - 23, trunkY + 10, 18, 8, 2, COLOR_TAILLIGHT);
    
    // === Zona de ventanas/techo ===
    int windowX = CAR_BODY_X + 25;
    int windowY = CAR_BODY_Y + 35;
    int windowW = CAR_BODY_W - 50;
    int windowH = CAR_BODY_H - 75;
    
    tft.fillRoundRect(windowX, windowY, windowW, windowH, 5, COLOR_CAR_WINDOW);
    tft.drawRoundRect(windowX, windowY, windowW, windowH, 5, COLOR_CAR_WINDOW_EDGE);
    
    // Pilares A y C (laterales de las ventanas)
    tft.drawFastVLine(windowX + 3, windowY + 5, windowH - 10, COLOR_CAR_OUTLINE);
    tft.drawFastVLine(windowX + windowW - 4, windowY + 5, windowH - 10, COLOR_CAR_OUTLINE);
    
    // Línea central del techo (eje del coche)
    tft.drawLine(cx, windowY + 10, cx, windowY + windowH - 10, 0x5ACB);
    
    // === Ejes de ruedas (estilo técnico) ===
    // Eje delantero
    tft.drawLine(X_FL, Y_FL, X_FR, Y_FR, 0x4A69);
    tft.drawLine(X_FL, Y_FL - 1, X_FR, Y_FR - 1, 0x3186);
    
    // Eje trasero
    tft.drawLine(X_RL, Y_RL, X_RR, Y_RR, 0x4A69);
    tft.drawLine(X_RL, Y_RL + 1, X_RR, Y_RR + 1, 0x3186);
    
    // Conexiones laterales (suspensión)
    tft.drawLine(X_FL, Y_FL, X_RL, Y_RL, 0x3186);
    tft.drawLine(X_FR, Y_FR, X_RR, Y_RR, 0x3186);
    
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

    // Icono de advertencia si hay errores almacenados
    Icons::drawErrorWarning();
    
    // Indicador de estado de sensores (LEDs I/T/W)
    Icons::drawSensorStatus(sensorCurrentOK, sensorTempOK, sensorWheelOK,
                           Sensors::NUM_CURRENTS, Sensors::NUM_TEMPS, Sensors::NUM_WHEELS);
    
    // Advertencia de temperatura crítica
    Icons::drawTempWarning(tempWarning, maxTemp);

    // Barra de pedal en la parte inferior
    drawPedalBar(pedalPercent);
    
#ifdef STANDALONE_DISPLAY
    // Draw demo button for easy hidden menu access
    drawDemoButton();
#endif

    // --- Lectura táctil usando touch_map ---
    // 🔒 v2.8.6: Solo leer touch si está habilitado e inicializado
    bool batteryTouch = false;
#ifdef STANDALONE_DISPLAY
    bool demoButtonTouched = false;
    static bool hiddenMenuJustActivated = false;
#endif

#ifndef DISABLE_TOUCH
    if (touchInitialized && cfg.touchEnabled && touch.touched()) {
        TS_Point p = touch.getPoint();
        // Map raw touch coordinates to screen coordinates using calibration constants
        int x = map(p.x, TouchCalibration::RAW_MIN, TouchCalibration::RAW_MAX, 0, TouchCalibration::SCREEN_WIDTH);
        int y = map(p.y, TouchCalibration::RAW_MIN, TouchCalibration::RAW_MAX, 0, TouchCalibration::SCREEN_HEIGHT);

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