#include "icons.h"
#include "display_types.h" // para CACHE_UNINITIALIZED
#include "hud_layer.h"     // 🚨 CRITICAL FIX: For RenderContext
#include "logger.h"
#include "safe_draw.h"     // 🚨 CRITICAL FIX: For coordinate-safe drawing
#include "shadow_render.h" // Phase 3: Shadow mirroring support
#include "system.h"        // para consultar errores persistentes
#include <Arduino.h>       // para constrain()
#include <TFT_eSPI.h>
#include <math.h> // para fabs()

static TFT_eSPI *tft = nullptr;
static bool initialized = false;

// 🔒 IMPROVEMENT: Helper to validate initialization before drawing
// Reduces code duplication and ensures consistent error handling
static inline bool isValidForDrawing() {
  if (!initialized || tft == nullptr) {
    Logger::warn("Icons draw function called but not initialized");
    return false;
  }
  return true;
}

// Cache de último estado para evitar redibujos innecesarios
// NOTA: Usamos int con valor CACHE_UNINITIALIZED para forzar el primer dibujado
// incluso si el estado inicial es false (que se convierte a 0)
static System::State lastSysState = (System::State)CACHE_UNINITIALIZED;
static Shifter::Gear lastGear = (Shifter::Gear)CACHE_UNINITIALIZED;
// v2.14.0: Simplified - only mode4x4 and regen cached
static int lastMode4x4 =
    CACHE_UNINITIALIZED; // Usar int para permitir valor -1 (no inicializado)
static int lastRegen =
    CACHE_UNINITIALIZED; // Usar int para permitir valor -1 (no inicializado)
static float lastBattery = -999.0f;
static int lastErrorCount = CACHE_UNINITIALIZED;

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
  
  // 🚨 CRITICAL FIX: Initialize SafeDraw with TFT reference
  SafeDraw::init(tft);
}

// 🚨 CRITICAL FIX: Helper to convert sprite to RenderContext
// This allows gradual migration while maintaining safety
static inline HudLayer::RenderContext createContext(TFT_eSprite *sprite) {
  if (sprite) {
    // Create context for sprite rendering with default origin (0,0)
    // Note: For proper sprite positioning, origin should be set by caller
    return HudLayer::RenderContext(sprite, true, 0, 0, 
                                    sprite->width(), sprite->height());
  } else {
    // Create context for screen rendering
    return HudLayer::RenderContext(nullptr, true, 0, 0, 480, 320);
  }
}

void Icons::drawSystemState(System::State st, TFT_eSprite *sprite) {
  if (!isValidForDrawing()) return;
  if (st == lastSysState) return; // no cambio → no redibujar
  lastSysState = st;

  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx = createContext(sprite);

  const char *txt = "OFF";
  uint16_t col = TFT_WHITE;
  switch (st) {
  case System::PRECHECK:
    txt = "PRE";
    col = TFT_YELLOW;
    break;
  case System::READY:
    txt = "READY";
    col = TFT_GREEN;
    break;
  case System::RUN:
    txt = "RUN";
    col = TFT_CYAN;
    break;
  case System::ERROR:
    txt = "ERROR";
    col = TFT_RED;
    break;
  default:
    break;
  }
  
  // 🚨 CRITICAL FIX: Use SafeDraw for coordinate-safe rendering
  SafeDraw::fillRect(ctx, 5, 5, 80, 20, TFT_BLACK);
  
  // For text drawing, we need the draw target for setTextDatum/setTextColor
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (drawTarget) {
    drawTarget->setTextDatum(TL_DATUM);
    drawTarget->setTextColor(col, TFT_BLACK);
  }
  
  SafeDraw::drawString(ctx, txt, 10, 10, 2);
#ifdef RENDER_SHADOW_MODE
  // Phase 3.5: Mirror system state to shadow sprite
  SHADOW_MIRROR_fillRect(5, 5, 80, 20, TFT_BLACK);
  SHADOW_MIRROR_setTextDatum(TL_DATUM);
  SHADOW_MIRROR_setTextColor(col, TFT_BLACK);
  SHADOW_MIRROR_drawString(txt, 10, 10, 2);
#endif
}

// 🔒 v2.9.0: Indicador de marcha REUBICADO Y MEJORADO
// Posición: Centro de pantalla, debajo del triángulo warning (entre warning y
// coche) Diseño: 3D con efecto de profundidad y tamaño más grande
void Icons::drawGear(Shifter::Gear g, TFT_eSprite *sprite) {
  if (!isValidForDrawing()) return;
  
  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx = createContext(sprite);
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (!drawTarget) return;
  if (g == lastGear) return;
  lastGear = g;

  // ========================================
  // NUEVA POSICIÓN: Centro de pantalla, entre warning y coche
  // Centrado horizontalmente, Y = 48 (debajo del warning que termina en Y=40)
  // ========================================
  const int GEAR_PANEL_X =
      155; // Posición X del panel (más centrado para no tapar temperaturas)
  const int GEAR_PANEL_Y = 50;  // Debajo del triángulo warning
  const int GEAR_PANEL_W = 170; // Ancho total del panel (reducido de 200 a 170)
  const int GEAR_PANEL_H = 28;  // Alto del panel (reducido de 36 a 28)
  const int GEAR_ITEM_W =
      30; // Ancho de cada celda de marcha (reducido de 36 a 30)
  const int GEAR_ITEM_H = 22; // Alto de cada celda (reducido de 28 a 22)
  const int GEAR_SPACING = 4; // Espacio entre celdas

  // Colores 3D mejorados
  const uint16_t COLOR_PANEL_BG = 0x1082; // Fondo del panel (gris muy oscuro)
  const uint16_t COLOR_PANEL_BORDER = 0x4A49;    // Borde del panel
  const uint16_t COLOR_PANEL_HIGHLIGHT = 0x6B6D; // Highlight 3D superior
  const uint16_t COLOR_PANEL_SHADOW = 0x0841;    // Sombra 3D inferior

  const uint16_t COLOR_INACTIVE_BG = 0x2104;     // Fondo celda inactiva
  const uint16_t COLOR_INACTIVE_TEXT = 0x630C;   // Texto gris para inactivas
  const uint16_t COLOR_INACTIVE_BORDER = 0x3186; // Borde celda inactiva

  const uint16_t COLOR_ACTIVE_BG = 0x03EF; // Fondo azul brillante para activa
  const uint16_t COLOR_ACTIVE_TEXT = TFT_WHITE;  // Texto blanco brillante
  const uint16_t COLOR_ACTIVE_BORDER = TFT_CYAN; // Borde cyan para activa
  const uint16_t COLOR_ACTIVE_GLOW = 0x04FF;     // Efecto brillo

  const uint16_t COLOR_REVERSE_BG = 0xF800;   // Fondo rojo para Reversa
  const uint16_t COLOR_REVERSE_GLOW = 0xFBE0; // Brillo naranja para reversa

  // ========================================
  // Dibujar panel de fondo con efecto 3D
  // ========================================

  // Sombra exterior (efecto profundidad)
  drawTarget->fillRoundRect(GEAR_PANEL_X + 3, GEAR_PANEL_Y + 3, GEAR_PANEL_W,
                            GEAR_PANEL_H, 6, COLOR_PANEL_SHADOW);

  // Fondo principal del panel
  drawTarget->fillRoundRect(GEAR_PANEL_X, GEAR_PANEL_Y, GEAR_PANEL_W,
                            GEAR_PANEL_H, 6, COLOR_PANEL_BG);

  // Borde del panel
  drawTarget->drawRoundRect(GEAR_PANEL_X, GEAR_PANEL_Y, GEAR_PANEL_W,
                            GEAR_PANEL_H, 6, COLOR_PANEL_BORDER);

  // Highlight superior (efecto 3D)
  drawTarget->drawFastHLine(GEAR_PANEL_X + 8, GEAR_PANEL_Y + 2,
                            GEAR_PANEL_W - 16, COLOR_PANEL_HIGHLIGHT);
  drawTarget->drawFastHLine(GEAR_PANEL_X + 10, GEAR_PANEL_Y + 3,
                            GEAR_PANEL_W - 20, COLOR_PANEL_HIGHLIGHT);
#ifdef RENDER_SHADOW_MODE
  // Phase 3.5: Mirror gear panel to shadow sprite
  SHADOW_MIRROR_fillRoundRect(GEAR_PANEL_X + 3, GEAR_PANEL_Y + 3, GEAR_PANEL_W,
                              GEAR_PANEL_H, 6, COLOR_PANEL_SHADOW);
  SHADOW_MIRROR_fillRoundRect(GEAR_PANEL_X, GEAR_PANEL_Y, GEAR_PANEL_W,
                              GEAR_PANEL_H, 6, COLOR_PANEL_BG);
  SHADOW_MIRROR_drawRoundRect(GEAR_PANEL_X, GEAR_PANEL_Y, GEAR_PANEL_W,
                              GEAR_PANEL_H, 6, COLOR_PANEL_BORDER);
  SHADOW_MIRROR_drawFastHLine(GEAR_PANEL_X + 8, GEAR_PANEL_Y + 2,
                              GEAR_PANEL_W - 16, COLOR_PANEL_HIGHLIGHT);
  SHADOW_MIRROR_drawFastHLine(GEAR_PANEL_X + 10, GEAR_PANEL_Y + 3,
                              GEAR_PANEL_W - 20, COLOR_PANEL_HIGHLIGHT);
#endif

  // ========================================
  // Dibujar las 5 marchas: P R N D1 D2
  // ========================================
  const char *gears[] = {"P", "R", "N", "D1", "D2"};
  const Shifter::Gear gearValues[] = {Shifter::P, Shifter::R, Shifter::N,
                                      Shifter::D1, Shifter::D2};

  // Calcular posición inicial para centrar las 5 celdas
  int totalWidth = 5 * GEAR_ITEM_W + 4 * GEAR_SPACING;
  int startX = GEAR_PANEL_X + (GEAR_PANEL_W - totalWidth) / 2;
  int cellY = GEAR_PANEL_Y + (GEAR_PANEL_H - GEAR_ITEM_H) / 2;

  drawTarget->setTextDatum(MC_DATUM); // Centro para el texto

  for (int i = 0; i < 5; i++) {
    int cellX = startX + i * (GEAR_ITEM_W + GEAR_SPACING);
    bool isActive = (gearValues[i] == g);

    uint16_t bgColor, textColor, borderColor;

    if (isActive) {
      // Marcha activa - efecto brillante
      if (g == Shifter::R) {
        bgColor = COLOR_REVERSE_BG;
        borderColor = COLOR_REVERSE_GLOW;
      } else {
        bgColor = COLOR_ACTIVE_BG;
        borderColor = COLOR_ACTIVE_BORDER;
      }
      textColor = COLOR_ACTIVE_TEXT;

      // Efecto glow exterior para marcha activa
      drawTarget->drawRoundRect(
          cellX - 1, cellY - 1, GEAR_ITEM_W + 2, GEAR_ITEM_H + 2, 5,
          (g == Shifter::R) ? COLOR_REVERSE_GLOW : COLOR_ACTIVE_GLOW);
#ifdef RENDER_SHADOW_MODE
      SHADOW_MIRROR_drawRoundRect(
          cellX - 1, cellY - 1, GEAR_ITEM_W + 2, GEAR_ITEM_H + 2, 5,
          (g == Shifter::R) ? COLOR_REVERSE_GLOW : COLOR_ACTIVE_GLOW);
#endif
    } else {
      bgColor = COLOR_INACTIVE_BG;
      textColor = COLOR_INACTIVE_TEXT;
      borderColor = COLOR_INACTIVE_BORDER;
    }

    // Fondo de la celda con esquinas redondeadas
    drawTarget->fillRoundRect(cellX, cellY, GEAR_ITEM_W, GEAR_ITEM_H, 4,
                              bgColor);

    // Borde de la celda
    drawTarget->drawRoundRect(cellX, cellY, GEAR_ITEM_W, GEAR_ITEM_H, 4,
                              borderColor);

    // Efecto 3D interno: highlight superior
    if (isActive) {
      drawTarget->drawFastHLine(cellX + 4, cellY + 2, GEAR_ITEM_W - 8,
                                (g == Shifter::R) ? 0xFC10 : 0x07FF);
    }

    // Dibujar texto de la marcha (font 2 para que quepa en celdas más pequeñas)
    int textX = cellX + GEAR_ITEM_W / 2;
    int textY = cellY + GEAR_ITEM_H / 2;

    drawTarget->setTextColor(textColor, bgColor);
    drawTarget->drawString(gears[i], textX, textY,
                           2); // Font 2 para celdas más pequeñas
#ifdef RENDER_SHADOW_MODE
    // Phase 3.5: Mirror gear cell to shadow sprite
    SHADOW_MIRROR_fillRoundRect(cellX, cellY, GEAR_ITEM_W, GEAR_ITEM_H, 4,
                                bgColor);
    SHADOW_MIRROR_drawRoundRect(cellX, cellY, GEAR_ITEM_W, GEAR_ITEM_H, 4,
                                borderColor);
    if (isActive) {
      SHADOW_MIRROR_drawFastHLine(cellX + 4, cellY + 2, GEAR_ITEM_W - 8,
                                  (g == Shifter::R) ? 0xFC10 : 0x07FF);
    }
    SHADOW_MIRROR_setTextColor(textColor, bgColor);
    SHADOW_MIRROR_drawString(gears[i], textX, textY, 2);
#endif
  }
}

void Icons::drawFeatures(bool mode4x4, bool regenOn, TFT_eSprite *sprite) {
  // Phase 6.2: Support dual-mode rendering (sprite or TFT)
  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx = createContext(sprite);
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (!drawTarget) return;
  if (!isValidForDrawing()) return;
  // v2.14.0: Simplified - only 4x4 mode and REGEN
  // Convertir bool a int para comparación con cache (que puede ser -1 = no
  // inicializado)
  int iMode4x4 = mode4x4 ? 1 : 0;
  int iRegen = regenOn ? 1 : 0;
  if (iMode4x4 == lastMode4x4 && iRegen == lastRegen) return;

  // Colores para efectos 3D
  const uint16_t COLOR_BOX_BG = 0x2104;        // Fondo oscuro
  const uint16_t COLOR_BOX_BORDER = 0x4A49;    // Borde gris
  const uint16_t COLOR_BOX_HIGHLIGHT = 0x6B6D; // Highlight superior
  const uint16_t COLOR_BOX_SHADOW = 0x1082;    // Sombra inferior

  // Colores para cada icono cuando activo
  const uint16_t COLOR_4X4_ACTIVE = 0x0410;   // Cian oscuro (4x4 activo)
  const uint16_t COLOR_4X2_ACTIVE = 0x4100;   // Naranja oscuro (4x2 activo)
  const uint16_t COLOR_REGEN_ACTIVE = 0x0015; // Azul oscuro

  // Helper para dibujar cuadrado 3D con texto
  auto draw3DBox = [&](int x1, int y1, int x2, int y2, const char *text,
                       bool active, uint16_t activeColor) {
    int w = x2 - x1;
    int h = y2 - y1;
    int cx = (x1 + x2) / 2;
    int cy = (y1 + y2) / 2;

    // Sombra del cuadrado (offset 2px)
    SafeDraw::fillRoundRect(ctx, x1 + 2, y1 + 2, w, h, 5, COLOR_BOX_SHADOW);

    // Fondo del cuadrado
    uint16_t bgColor = active ? activeColor : COLOR_BOX_BG;
    SafeDraw::fillRoundRect(ctx, x1, y1, w, h, 5, bgColor);

    // Borde exterior
    drawTarget->drawRoundRect(x1, y1, w, h, 5, COLOR_BOX_BORDER);

    // Efecto 3D: highlight superior
    SafeDraw::drawFastHLine(ctx, x1 + 5, y1 + 2, w - 10, COLOR_BOX_HIGHLIGHT);
    SafeDraw::drawFastHLine(ctx, x1 + 5, y1 + 3, w - 10, COLOR_BOX_HIGHLIGHT);

    // Efecto 3D: sombra inferior interna
    SafeDraw::drawFastHLine(ctx, x1 + 5, y2 - 3, w - 10, COLOR_BOX_SHADOW);
    SafeDraw::drawFastHLine(ctx, x1 + 5, y2 - 2, w - 10, COLOR_BOX_SHADOW);

    // Texto centrado con sombra
    drawTarget->setTextDatum(MC_DATUM);
    // Sombra del texto
    drawTarget->setTextColor(TFT_BLACK, bgColor);
    SafeDraw::drawString(ctx, text, cx + 1, cy + 1, 2);
    // Texto principal
    uint16_t textColor = active ? TFT_WHITE : TFT_DARKGREY;
    drawTarget->setTextColor(textColor, bgColor);
    SafeDraw::drawString(ctx, text, cx, cy, 2);
    drawTarget->setTextDatum(TL_DATUM);
#ifdef RENDER_SHADOW_MODE
    // Phase 3.5: Mirror 3D box to shadow sprite
    SHADOW_MIRROR_fillRoundRect(x1 + 2, y1 + 2, w, h, 5, COLOR_BOX_SHADOW);
    SHADOW_MIRROR_fillRoundRect(x1, y1, w, h, 5, bgColor);
    SHADOW_MIRROR_drawRoundRect(x1, y1, w, h, 5, COLOR_BOX_BORDER);
    SHADOW_MIRROR_drawFastHLine(x1 + 5, y1 + 2, w - 10, COLOR_BOX_HIGHLIGHT);
    SHADOW_MIRROR_drawFastHLine(x1 + 5, y1 + 3, w - 10, COLOR_BOX_HIGHLIGHT);
    SHADOW_MIRROR_drawFastHLine(x1 + 5, y2 - 3, w - 10, COLOR_BOX_SHADOW);
    SHADOW_MIRROR_drawFastHLine(x1 + 5, y2 - 2, w - 10, COLOR_BOX_SHADOW);
    SHADOW_MIRROR_setTextDatum(MC_DATUM);
    SHADOW_MIRROR_setTextColor(TFT_BLACK, bgColor);
    SHADOW_MIRROR_drawString(text, cx + 1, cy + 1, 2);
    SHADOW_MIRROR_setTextColor(textColor, bgColor);
    SHADOW_MIRROR_drawString(text, cx, cy, 2);
    SHADOW_MIRROR_setTextDatum(TL_DATUM);
#endif
  };

  // 4x4 / 4x2 - Siempre muestra el estado activo con color diferente
  // 4x4 = cian, 4x2 = naranja (ambos siempre "activos" visualmente)
  if (iMode4x4 != lastMode4x4) {
    uint16_t mode4x4Color = mode4x4 ? COLOR_4X4_ACTIVE : COLOR_4X2_ACTIVE;
    draw3DBox(MODE4X4_X1, MODE4X4_Y1, MODE4X4_X2, MODE4X4_Y2,
              mode4x4 ? "4x4" : "4x2", true, mode4x4Color);
    lastMode4x4 = iMode4x4;
  }

  // v2.14.0: Lights and Media removed - cleaner HUD

  // Regenerativo - Usar helper draw3DBox
  if (iRegen != lastRegen) {
    // Limpiar área primero
    SafeDraw::fillRect(ctx, 400, 250, 75, 45, TFT_BLACK);
#ifdef RENDER_SHADOW_MODE
    SHADOW_MIRROR_fillRect(400, 250, 75, 45, TFT_BLACK);
#endif
    // Usar helper con posiciones fijas para REGEN
    draw3DBox(400, 250, 471, 291, "REGEN", regenOn, COLOR_REGEN_ACTIVE);
    lastRegen = iRegen;
  }
}

void Icons::drawBattery(float volts, TFT_eSprite *sprite) {
  // Phase 6.2: Support dual-mode rendering (sprite or TFT)
  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx = createContext(sprite);
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (!drawTarget) return;
  if (!isValidForDrawing()) return;
  volts = constrain(volts, 0.0f, 99.9f);
  if (fabs(volts - lastBattery) < 0.1f) return; // no cambio significativo
  lastBattery = volts;

  int x = BATTERY_X1;
  int y = BATTERY_Y1;
  int w = BATTERY_X2 - BATTERY_X1;
  int h = BATTERY_Y2 - BATTERY_Y1;

  // Limpiar área
  SafeDraw::fillRect(ctx, x, y, w, h, TFT_BLACK);

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
  SafeDraw::fillRoundRect(ctx, battX, battY, battW, battH, 3, 0x2104);
  drawTarget->drawRoundRect(battX, battY, battW, battH, 3, 0x6B6D);

  // Terminal positivo
  drawTarget->fillRect(battX + battW, battY + (battH - capH) / 2, capW, capH,
                       0x6B6D);

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
    drawTarget->fillRoundRect(battX + 3, battY + 3, fillW, battH - 6, 1,
                              fillColor);
    // Highlight 3D
    SafeDraw::drawFastHLine(ctx, battX + 3, battY + 3, fillW, 0xFFFF);
  }

  // Voltaje debajo del icono
  drawTarget->setTextDatum(TL_DATUM);
  drawTarget->setTextColor(fillColor, TFT_BLACK);
  char buf[10];
  snprintf(buf, sizeof(buf), "%.1fV", volts);
  SafeDraw::drawString(ctx, buf, battX, battY + battH + 3, 1);
#ifdef RENDER_SHADOW_MODE
  // Phase 3.5: Mirror battery indicator to shadow sprite
  SHADOW_MIRROR_fillRect(x, y, w, h, TFT_BLACK);
  SHADOW_MIRROR_fillRoundRect(battX, battY, battW, battH, 3, 0x2104);
  SHADOW_MIRROR_drawRoundRect(battX, battY, battW, battH, 3, 0x6B6D);
  SHADOW_MIRROR_fillRect(battX + battW, battY + (battH - capH) / 2, capW, capH,
                         0x6B6D);
  if (fillW > 0) {
    SHADOW_MIRROR_fillRoundRect(battX + 3, battY + 3, fillW, battH - 6, 1,
                                fillColor);
    SHADOW_MIRROR_drawFastHLine(battX + 3, battY + 3, fillW, 0xFFFF);
  }
  SHADOW_MIRROR_setTextDatum(TL_DATUM);
  SHADOW_MIRROR_setTextColor(fillColor, TFT_BLACK);
  SHADOW_MIRROR_drawString(buf, battX, battY + battH + 3, 1);
#endif
}

void Icons::drawErrorWarning(TFT_eSprite *sprite) {
  // Phase 6.2: Support dual-mode rendering (sprite or TFT)
  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx = createContext(sprite);
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (!drawTarget) return;
  if (!isValidForDrawing()) return;
  int count = System::getErrorCount();
  if (count == lastErrorCount) return;
  lastErrorCount = count;

  if (count > 0) {
    int midX = (WARNING_X1 + WARNING_X2) / 2;
    int topY = WARNING_Y1 + 8;
    int botY = WARNING_Y2 - 5;

    // Sombra del triángulo
    drawTarget->fillTriangle(WARNING_X1 + 2, botY + 2, WARNING_X2 + 2, botY + 2,
                             midX + 2, topY + 2, 0x8400);

    // Triángulo de warning con borde
    drawTarget->fillTriangle(WARNING_X1, botY, WARNING_X2, botY, midX, topY,
                             TFT_YELLOW);
    drawTarget->drawTriangle(WARNING_X1, botY, WARNING_X2, botY, midX, topY,
                             0x8400);

    // Signo de exclamación
    SafeDraw::fillRect(ctx, midX - 1, topY + 8, 3, 10, TFT_BLACK);
    SafeDraw::fillCircle(ctx, midX, botY - 5, 2, TFT_BLACK);

    // Contador de errores
    drawTarget->setTextColor(TFT_YELLOW, TFT_BLACK);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", count);
    SafeDraw::drawString(ctx, buf, WARNING_X2 + 5, WARNING_Y1 + 15, 2);
#ifdef RENDER_SHADOW_MODE
    // Phase 3.5: Mirror error warning to shadow sprite
    SHADOW_MIRROR_fillTriangle(WARNING_X1 + 2, botY + 2, WARNING_X2 + 2,
                               botY + 2, midX + 2, topY + 2, 0x8400);
    SHADOW_MIRROR_fillTriangle(WARNING_X1, botY, WARNING_X2, botY, midX, topY,
                               TFT_YELLOW);
    SHADOW_MIRROR_drawTriangle(WARNING_X1, botY, WARNING_X2, botY, midX, topY,
                               0x8400);
    SHADOW_MIRROR_fillRect(midX - 1, topY + 8, 3, 10, TFT_BLACK);
    SHADOW_MIRROR_fillCircle(midX, botY - 5, 2, TFT_BLACK);
    SHADOW_MIRROR_setTextColor(TFT_YELLOW, TFT_BLACK);
    SHADOW_MIRROR_drawString(buf, WARNING_X2 + 5, WARNING_Y1 + 15, 2);
#endif
  } else {
    drawTarget->fillRect(WARNING_X1, WARNING_Y1, (WARNING_X2 - WARNING_X1) + 40,
                         WARNING_Y2 - WARNING_Y1, TFT_BLACK);
#ifdef RENDER_SHADOW_MODE
    SHADOW_MIRROR_fillRect(WARNING_X1, WARNING_Y1,
                           (WARNING_X2 - WARNING_X1) + 40,
                           WARNING_Y2 - WARNING_Y1, TFT_BLACK);
#endif
  }
}

void Icons::drawSensorStatus(uint8_t currentOK, uint8_t tempOK, uint8_t wheelOK,
                             uint8_t currentTotal, uint8_t tempTotal,
                             uint8_t wheelTotal, TFT_eSprite *sprite) {
  // Phase 6.2: Support dual-mode rendering (sprite or TFT)
  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx = createContext(sprite);
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (!drawTarget) return;
  if (!isValidForDrawing()) return;

  // Solo redibujar si hay cambios (o primera vez)
  if (sensorsCacheInitialized && currentOK == lastCurrentOK &&
      tempOK == lastTempOK && wheelOK == lastWheelOK)
    return;

  lastCurrentOK = currentOK;
  lastTempOK = tempOK;
  lastWheelOK = wheelOK;
  sensorsCacheInitialized = true;

  // Limpiar área
  drawTarget->fillRect(SENSOR_STATUS_X1, SENSOR_STATUS_Y1,
                       SENSOR_STATUS_X2 - SENSOR_STATUS_X1,
                       SENSOR_STATUS_Y2 - SENSOR_STATUS_Y1, TFT_BLACK);

  // Helper para determinar color basado en proporción OK/Total
  auto getStatusColor = [](uint8_t ok, uint8_t total) -> uint16_t {
    if (total == 0) return TFT_DARKGREY; // Deshabilitado
    if (ok == total) return TFT_GREEN;   // Todos OK
    if (ok == 0) return TFT_RED;         // Todos fallidos
    return TFT_YELLOW;                   // Parcial
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
  drawTarget->fillCircle(startX + 1, ledY + 1, ledRadius,
                         getDarkColor(colorCurrent));
  // LED principal
  SafeDraw::fillCircle(ctx, startX, ledY, ledRadius, colorCurrent);
  // Highlight 3D (brillo)
  SafeDraw::fillCircle(ctx, startX - 2, ledY - 2, 2, 0xFFFF);
  // Borde
  SafeDraw::drawCircle(ctx, startX, ledY, ledRadius, getDarkColor(colorCurrent));

  drawTarget->setTextDatum(MC_DATUM);
  drawTarget->setTextColor(colorCurrent, TFT_BLACK);
  SafeDraw::drawString(ctx, "I", startX, textY, 1);

  // LED 2: Temperatura (DS18B20) con efecto 3D
  uint16_t colorTemp = getStatusColor(tempOK, tempTotal);
  drawTarget->fillCircle(startX + spacing + 1, ledY + 1, ledRadius,
                         getDarkColor(colorTemp));
  SafeDraw::fillCircle(ctx, startX + spacing, ledY, ledRadius, colorTemp);
  SafeDraw::fillCircle(ctx, startX + spacing - 2, ledY - 2, 2, 0xFFFF);
  drawTarget->drawCircle(startX + spacing, ledY, ledRadius,
                         getDarkColor(colorTemp));

  drawTarget->setTextColor(colorTemp, TFT_BLACK);
  SafeDraw::drawString(ctx, "T", startX + spacing, textY, 1);

  // LED 3: Ruedas con efecto 3D
  uint16_t colorWheel = getStatusColor(wheelOK, wheelTotal);
  drawTarget->fillCircle(startX + 2 * spacing + 1, ledY + 1, ledRadius,
                         getDarkColor(colorWheel));
  SafeDraw::fillCircle(ctx, startX + 2 * spacing, ledY, ledRadius, colorWheel);
  SafeDraw::fillCircle(ctx, startX + 2 * spacing - 2, ledY - 2, 2, 0xFFFF);
  drawTarget->drawCircle(startX + 2 * spacing, ledY, ledRadius,
                         getDarkColor(colorWheel));

  drawTarget->setTextColor(colorWheel, TFT_BLACK);
  SafeDraw::drawString(ctx, "W", startX + 2 * spacing, textY, 1);
#ifdef RENDER_SHADOW_MODE
  // Phase 3.5: Mirror sensor status to shadow sprite
  SHADOW_MIRROR_fillRect(SENSOR_STATUS_X1, SENSOR_STATUS_Y1,
                         SENSOR_STATUS_X2 - SENSOR_STATUS_X1,
                         SENSOR_STATUS_Y2 - SENSOR_STATUS_Y1, TFT_BLACK);
  // Current LED
  SHADOW_MIRROR_fillCircle(startX + 1, ledY + 1, ledRadius,
                           getDarkColor(colorCurrent));
  SHADOW_MIRROR_fillCircle(startX, ledY, ledRadius, colorCurrent);
  SHADOW_MIRROR_fillCircle(startX - 2, ledY - 2, 2, 0xFFFF);
  SHADOW_MIRROR_drawCircle(startX, ledY, ledRadius, getDarkColor(colorCurrent));
  SHADOW_MIRROR_setTextDatum(MC_DATUM);
  SHADOW_MIRROR_setTextColor(colorCurrent, TFT_BLACK);
  SHADOW_MIRROR_drawString("I", startX, textY, 1);
  // Temp LED
  SHADOW_MIRROR_fillCircle(startX + spacing + 1, ledY + 1, ledRadius,
                           getDarkColor(colorTemp));
  SHADOW_MIRROR_fillCircle(startX + spacing, ledY, ledRadius, colorTemp);
  SHADOW_MIRROR_fillCircle(startX + spacing - 2, ledY - 2, 2, 0xFFFF);
  SHADOW_MIRROR_drawCircle(startX + spacing, ledY, ledRadius,
                           getDarkColor(colorTemp));
  SHADOW_MIRROR_setTextColor(colorTemp, TFT_BLACK);
  SHADOW_MIRROR_drawString("T", startX + spacing, textY, 1);
  // Wheel LED
  SHADOW_MIRROR_fillCircle(startX + 2 * spacing + 1, ledY + 1, ledRadius,
                           getDarkColor(colorWheel));
  SHADOW_MIRROR_fillCircle(startX + 2 * spacing, ledY, ledRadius, colorWheel);
  SHADOW_MIRROR_fillCircle(startX + 2 * spacing - 2, ledY - 2, 2, 0xFFFF);
  SHADOW_MIRROR_drawCircle(startX + 2 * spacing, ledY, ledRadius,
                           getDarkColor(colorWheel));
  SHADOW_MIRROR_setTextColor(colorWheel, TFT_BLACK);
  SHADOW_MIRROR_drawString("W", startX + 2 * spacing, textY, 1);
#endif
}

void Icons::drawTempWarning(bool tempWarning, float maxTemp,
                            TFT_eSprite *sprite) {
  // Phase 6.2: Support dual-mode rendering (sprite or TFT)
  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx = createContext(sprite);
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (!drawTarget) return;
  if (!isValidForDrawing()) return;

  // Solo redibujar si hay cambios significativos
  if (tempWarning == lastTempWarning && fabs(maxTemp - lastMaxTemp) < 1.0f)
    return;

  lastTempWarning = tempWarning;
  lastMaxTemp = maxTemp;

  drawTarget->fillRect(TEMP_WARNING_X, TEMP_WARNING_Y, TEMP_WARNING_W,
                       TEMP_WARNING_H, TFT_BLACK);

  if (tempWarning) {
    // Mostrar advertencia de temperatura crítica
    drawTarget->setTextDatum(ML_DATUM);
    drawTarget->setTextColor(TFT_RED, TFT_BLACK);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.0fC!", maxTemp);
    drawTarget->drawString(buf, TEMP_WARNING_X + 5,
                           TEMP_WARNING_Y + TEMP_WARNING_H / 2, 2);
#ifdef RENDER_SHADOW_MODE
    // Phase 3.5: Mirror temp warning to shadow sprite
    SHADOW_MIRROR_fillRect(TEMP_WARNING_X, TEMP_WARNING_Y, TEMP_WARNING_W,
                           TEMP_WARNING_H, TFT_BLACK);
    SHADOW_MIRROR_setTextDatum(ML_DATUM);
    SHADOW_MIRROR_setTextColor(TFT_RED, TFT_BLACK);
    SHADOW_MIRROR_drawString(buf, TEMP_WARNING_X + 5,
                             TEMP_WARNING_Y + TEMP_WARNING_H / 2, 2);
#endif
  } else {
#ifdef RENDER_SHADOW_MODE
    SHADOW_MIRROR_fillRect(TEMP_WARNING_X, TEMP_WARNING_Y, TEMP_WARNING_W,
                           TEMP_WARNING_H, TFT_BLACK);
#endif
  }
}

// Cache para temperatura ambiente
static float lastAmbientTemp = -999.0f;

void Icons::drawAmbientTemp(float ambientTemp, TFT_eSprite *sprite) {
  // Phase 6.2: Support dual-mode rendering (sprite or TFT)
  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx = createContext(sprite);
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (!drawTarget) return;
  if (!isValidForDrawing()) return;

  // Solo redibujar si hay cambio significativo (>0.5°C)
  if (fabs(ambientTemp - lastAmbientTemp) < 0.5f) return;
  lastAmbientTemp = ambientTemp;

  // Limpiar área
  drawTarget->fillRect(AMBIENT_TEMP_X, AMBIENT_TEMP_Y, AMBIENT_TEMP_W,
                       AMBIENT_TEMP_H, TFT_BLACK);

  // Dibujar icono de termómetro pequeño
  int iconX = AMBIENT_TEMP_X + 2;
  int iconY = AMBIENT_TEMP_Y + 3;

  // Cuerpo del termómetro (vertical)
  SafeDraw::fillRoundRect(ctx, iconX, iconY, 6, 12, 2, TFT_CYAN);
  drawTarget->drawRoundRect(iconX, iconY, 6, 12, 2, TFT_DARKGREY);

  // Bulbo del termómetro
  SafeDraw::fillCircle(ctx, iconX + 3, iconY + 13, 4, TFT_CYAN);
  SafeDraw::drawCircle(ctx, iconX + 3, iconY + 13, 4, TFT_DARKGREY);

  // Texto de temperatura
  drawTarget->setTextDatum(ML_DATUM);

  // Color según temperatura
  uint16_t tempColor;
  if (ambientTemp < 10.0f) {
    tempColor = TFT_CYAN; // Frío
  } else if (ambientTemp < 25.0f) {
    tempColor = TFT_GREEN; // Confortable
  } else if (ambientTemp < 35.0f) {
    tempColor = TFT_YELLOW; // Cálido
  } else {
    tempColor = TFT_RED; // Caliente
  }

  drawTarget->setTextColor(tempColor, TFT_BLACK);
  char buf[10];
  snprintf(buf, sizeof(buf), "%.0fC", ambientTemp);
  drawTarget->drawString(buf, AMBIENT_TEMP_X + 14,
                         AMBIENT_TEMP_Y + AMBIENT_TEMP_H / 2, 2);
  drawTarget->setTextDatum(TL_DATUM);
#ifdef RENDER_SHADOW_MODE
  // Phase 3.5: Mirror ambient temp to shadow sprite
  SHADOW_MIRROR_fillRect(AMBIENT_TEMP_X, AMBIENT_TEMP_Y, AMBIENT_TEMP_W,
                         AMBIENT_TEMP_H, TFT_BLACK);
  SHADOW_MIRROR_fillRoundRect(iconX, iconY, 6, 12, 2, TFT_CYAN);
  SHADOW_MIRROR_drawRoundRect(iconX, iconY, 6, 12, 2, TFT_DARKGREY);
  SHADOW_MIRROR_fillCircle(iconX + 3, iconY + 13, 4, TFT_CYAN);
  SHADOW_MIRROR_drawCircle(iconX + 3, iconY + 13, 4, TFT_DARKGREY);
  SHADOW_MIRROR_setTextDatum(ML_DATUM);
  SHADOW_MIRROR_setTextColor(tempColor, TFT_BLACK);
  SHADOW_MIRROR_drawString(buf, AMBIENT_TEMP_X + 14,
                           AMBIENT_TEMP_Y + AMBIENT_TEMP_H / 2, 2);
  SHADOW_MIRROR_setTextDatum(TL_DATUM);
#endif
}

// ============================================================================
// PHASE 10: RenderContext versions for granular dirty tracking
// ============================================================================

void Icons::drawSystemState(System::State st, HudLayer::RenderContext &ctx) {
  if (!ctx.isValid()) return;

  // Check if changed
  bool changed = (st != lastSysState);

  // Call sprite version
  drawSystemState(st, ctx.sprite);

  // Mark dirty if changed
  if (changed) {
    // System state icon is centered at top - approximate 80x50 area
    ctx.markDirty(200, 0, 80, 50);
  }
}

void Icons::drawGear(Shifter::Gear g, HudLayer::RenderContext &ctx) {
  if (!ctx.isValid()) return;

  // Check if changed
  bool changed = (g != lastGear);

  // Call sprite version
  drawGear(g, ctx.sprite);

  // Mark dirty if changed
  if (changed) {
    // Gear display is at top center - approximate 100x60 area
    ctx.markDirty(190, 45, 100, 60);
  }
}

void Icons::drawFeatures(bool mode4x4, bool regenOn,
                         HudLayer::RenderContext &ctx) {
  if (!ctx.isValid()) return;

  // Convert to int for comparison with cache
  int m4x4 = mode4x4 ? 1 : 0;
  int regen = regenOn ? 1 : 0;

  // Check if changed
  bool changed = (m4x4 != lastMode4x4) || (regen != lastRegen);

  // Call sprite version
  drawFeatures(mode4x4, regenOn, ctx.sprite);

  // Mark dirty if changed
  if (changed) {
    // Mode4x4 icon area
    ctx.markDirty(MODE4X4_X1, MODE4X4_Y1, MODE4X4_X2 - MODE4X4_X1,
                  MODE4X4_Y2 - MODE4X4_Y1);
    // Regen icon is next to it - mark wider area
    ctx.markDirty(MODE4X4_X1, MODE4X4_Y1, 240 - MODE4X4_X1,
                  MODE4X4_Y2 - MODE4X4_Y1);
  }
}

void Icons::drawBattery(float volts, HudLayer::RenderContext &ctx) {
  if (!ctx.isValid()) return;

  // Check if changed significantly
  bool changed = fabs(volts - lastBattery) >= 0.1f;

  // Call sprite version
  drawBattery(volts, ctx.sprite);

  // Mark dirty if changed
  if (changed) {
    ctx.markDirty(BATTERY_X1, BATTERY_Y1, BATTERY_X2 - BATTERY_X1,
                  BATTERY_Y2 - BATTERY_Y1);
  }
}

void Icons::drawErrorWarning(HudLayer::RenderContext &ctx) {
  if (!ctx.isValid()) return;

  // Check if error count changed
  int count = System::getErrorCount();
  bool changed = (count != lastErrorCount);

  // Call sprite version (which updates cache)
  drawErrorWarning(ctx.sprite);

  // Mark warning area dirty only if changed
  if (changed) {
    ctx.markDirty(WARNING_X1, WARNING_Y1, WARNING_X2 - WARNING_X1,
                  WARNING_Y2 - WARNING_Y1);
  }
}

void Icons::drawSensorStatus(uint8_t currentOK, uint8_t tempOK, uint8_t wheelOK,
                             uint8_t currentTotal, uint8_t tempTotal,
                             uint8_t wheelTotal, HudLayer::RenderContext &ctx) {
  if (!ctx.isValid()) return;

  // Check if changed
  bool changed = (currentOK != lastCurrentOK) || (tempOK != lastTempOK) ||
                 (wheelOK != lastWheelOK);

  // Call sprite version
  drawSensorStatus(currentOK, tempOK, wheelOK, currentTotal, tempTotal,
                   wheelTotal, ctx.sprite);

  // Mark dirty if changed
  if (changed) {
    ctx.markDirty(SENSOR_STATUS_X1, SENSOR_STATUS_Y1,
                  SENSOR_STATUS_X2 - SENSOR_STATUS_X1,
                  SENSOR_STATUS_Y2 - SENSOR_STATUS_Y1);
  }
}

void Icons::drawTempWarning(bool tempWarning, float maxTemp,
                            HudLayer::RenderContext &ctx) {
  if (!ctx.isValid()) return;

  // Check if changed
  bool changed =
      (tempWarning != lastTempWarning) || (fabs(maxTemp - lastMaxTemp) >= 1.0f);

  // Call sprite version
  drawTempWarning(tempWarning, maxTemp, ctx.sprite);

  // Mark dirty if changed
  if (changed) {
    ctx.markDirty(TEMP_WARNING_X, TEMP_WARNING_Y, TEMP_WARNING_W,
                  TEMP_WARNING_H);
  }
}

void Icons::drawAmbientTemp(float ambientTemp, HudLayer::RenderContext &ctx) {
  if (!ctx.isValid()) return;

  // Check if changed significantly
  bool changed = fabs(ambientTemp - lastAmbientTemp) >= 0.5f;

  // Call sprite version
  drawAmbientTemp(ambientTemp, ctx.sprite);

  // Mark dirty if changed
  if (changed) {
    ctx.markDirty(AMBIENT_TEMP_X, AMBIENT_TEMP_Y, AMBIENT_TEMP_W,
                  AMBIENT_TEMP_H);
  }
}