#include "wheels_display.h"
#include "hud_layer.h" // For RenderContext
#include "logger.h"
#include "safe_draw.h" // 🚨 CRITICAL FIX: Safe coordinate-translated drawing
#include "settings.h"
#include "shadow_render.h" // Phase 3: Shadow mirroring support
#include <Arduino.h>       // para constrain() y DEG_TO_RAD
#include <TFT_eSPI.h>
#include <math.h> // para fabs()

// Puntero global a la pantalla
static TFT_eSPI *tft = nullptr;
static bool initialized = false;

// Colores 3D para ruedas
static const uint16_t COLOR_WHEEL_OUTER = 0x3186;     // Gris oscuro borde
static const uint16_t COLOR_WHEEL_INNER = 0x4A49;     // Gris medio llanta
static const uint16_t COLOR_WHEEL_TREAD = 0x2104;     // Negro neumático
static const uint16_t COLOR_WHEEL_HIGHLIGHT = 0x6B4D; // Highlight 3D
static const uint16_t COLOR_WHEEL_SHADOW = 0x1082;    // Sombra 3D
static const uint16_t COLOR_HUB_CENTER = 0x9CF3;      // Centro plateado
static const uint16_t COLOR_HUB_BOLT = 0xC618;        // Tornillos brillantes
static const uint16_t COLOR_INFO_BG =
    0x2104; // Fondo de info (temperatura/esfuerzo)
static const uint16_t COLOR_BAR_BG = 0x1082; // Fondo de barras de progreso

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
  if (cy < 175) {              // Ruedas delanteras (y < centro)
    return (cx < 240) ? 0 : 1; // FL=0, FR=1
  } else {                     // Ruedas traseras (y >= centro)
    return (cx < 240) ? 2 : 3; // RL=2, RR=3
  }
}

// Helpers: colores según temperatura
static uint16_t colorByTemp(float t) {
  if (t < TEMP_WARN_MOTOR) return TFT_GREEN;
  if (t < TEMP_MAX_MOTOR) return TFT_YELLOW;
  return TFT_RED;
}

// Helpers: colores según esfuerzo
static uint16_t colorByEffort(float e) {
  if (e < 60.0f) return TFT_GREEN;
  if (e < 85.0f) return TFT_YELLOW;
  return TFT_RED;
}

// Dibujar rueda 3D con efecto de profundidad
// 🔒 v2.8.8: Ruedas en orientación vertical con marcas de neumático realistas
// 🚨 CRITICAL FIX: Now uses RenderContext for safe coordinate translation
static void drawWheel3D(int screenCX, int screenCY, float angleDeg,
                        const HudLayer::RenderContext &ctx) {
  if (!ctx.sprite && !tft) return;

  int w = 18, h = 40; // Dimensiones: rueda vertical (ancho < alto)
  float rad = angleDeg * DEG_TO_RAD;

  // Calcular puntos de los vértices del rectángulo rotado
  int dx = (int)(cosf(rad) * w / 2);
  int dy = (int)(sinf(rad) * w / 2);
  int ex = (int)(-sinf(rad) * h / 2);
  int ey = (int)(cosf(rad) * h / 2);

  // 4 esquinas del rectángulo (en coordenadas de pantalla)
  int x0 = screenCX - dx - ex, y0 = screenCY - dy - ey;
  int x1 = screenCX + dx - ex, y1 = screenCY + dy - ey;
  int x2 = screenCX + dx + ex, y2 = screenCY + dy + ey;
  int x3 = screenCX - dx + ex, y3 = screenCY - dy + ey;

  // Fondo negro para limpiar (cuadrado que cubre la rueda rotada)
  int clearSize = h + 8; // h es la dimensión más larga

  // 🚨 CRITICAL FIX: Use SafeDraw for coordinate translation
  SafeDraw::fillRect(ctx, screenCX - clearSize / 2, screenCY - clearSize / 2,
                     clearSize, clearSize, TFT_BLACK);
#ifdef RENDER_SHADOW_MODE
  // Phase 3: Mirror wheel clear area to shadow sprite
  SHADOW_MIRROR_fillRect(screenCX - clearSize / 2, screenCY - clearSize / 2,
                         clearSize, clearSize, TFT_BLACK);
#endif

  // 🚨 CRITICAL FIX: For complex shapes (triangles), translate coordinates
  // manually
  if (ctx.sprite) {
    // Convert all points to sprite-local coordinates
    int16_t local_x0 = ctx.toLocalX(x0 + 2);
    int16_t local_y0 = ctx.toLocalY(y0 + 2);
    int16_t local_x1 = ctx.toLocalX(x1 + 2);
    int16_t local_y1 = ctx.toLocalY(y1 + 2);
    int16_t local_x2 = ctx.toLocalX(x2 + 2);
    int16_t local_y2 = ctx.toLocalY(y2 + 2);
    int16_t local_x3 = ctx.toLocalX(x3 + 2);
    int16_t local_y3 = ctx.toLocalY(y3 + 2);

    // Shadow triangles (offset +2 for 3D effect)
    ctx.sprite->fillTriangle(local_x0, local_y0, local_x1, local_y1, local_x2,
                             local_y2, COLOR_WHEEL_SHADOW);
    ctx.sprite->fillTriangle(local_x0, local_y0, local_x2, local_y2, local_x3,
                             local_y3, COLOR_WHEEL_SHADOW);
  } else {
    // Drawing to screen - use screen coordinates directly
    SafeDraw::fillTriangle(ctx, x0 + 2, y0 + 2, x1 + 2, y1 + 2, x2 + 2, y2 + 2,
                           COLOR_WHEEL_SHADOW);
    SafeDraw::fillTriangle(ctx, x0 + 2, y0 + 2, x2 + 2, y2 + 2, x3 + 2, y3 + 2,
                           COLOR_WHEEL_SHADOW);
  }
#ifdef RENDER_SHADOW_MODE
  // Phase 3.5: Mirror wheel shadow to shadow sprite
  SHADOW_MIRROR_fillTriangle(x0 + 2, y0 + 2, x1 + 2, y1 + 2, x2 + 2, y2 + 2,
                             COLOR_WHEEL_SHADOW);
  SHADOW_MIRROR_fillTriangle(x0 + 2, y0 + 2, x2 + 2, y2 + 2, x3 + 2, y3 + 2,
                             COLOR_WHEEL_SHADOW);
#endif

  // Neumático exterior (banda de rodadura)
  SafeDraw::fillTriangle(ctx, x0, y0, x1, y1, x2, y2, COLOR_WHEEL_TREAD);
  SafeDraw::fillTriangle(ctx, x0, y0, x2, y2, x3, y3, COLOR_WHEEL_TREAD);
#ifdef RENDER_SHADOW_MODE
  // Phase 3.5: Mirror wheel tread to shadow sprite
  SHADOW_MIRROR_fillTriangle(x0, y0, x1, y1, x2, y2, COLOR_WHEEL_TREAD);
  SHADOW_MIRROR_fillTriangle(x0, y0, x2, y2, x3, y3, COLOR_WHEEL_TREAD);
#endif

  // Borde exterior brillante
  SafeDraw::drawLine(ctx, x0, y0, x1, y1, COLOR_WHEEL_OUTER);
  SafeDraw::drawLine(ctx, x1, y1, x2, y2, COLOR_WHEEL_OUTER);
  SafeDraw::drawLine(ctx, x2, y2, x3, y3, COLOR_WHEEL_OUTER);
  SafeDraw::drawLine(ctx, x3, y3, x0, y0, COLOR_WHEEL_OUTER);
#ifdef RENDER_SHADOW_MODE
  // Phase 3.5: Mirror wheel border to shadow sprite
  SHADOW_MIRROR_drawLine(x0, y0, x1, y1, COLOR_WHEEL_OUTER);
  SHADOW_MIRROR_drawLine(x1, y1, x2, y2, COLOR_WHEEL_OUTER);
  SHADOW_MIRROR_drawLine(x2, y2, x3, y3, COLOR_WHEEL_OUTER);
  SHADOW_MIRROR_drawLine(x3, y3, x0, y0, COLOR_WHEEL_OUTER);
#endif

  // 🔒 v2.8.8: Marcas de neumático (líneas transversales simulando banda de
  // rodadura) Dibujar 5 marcas equidistantes a lo largo de la rueda
  const int NUM_TREADS = 5;
  // Reutilizar COLOR_WHEEL_SHADOW para las marcas oscuras (consistencia)
  const uint16_t COLOR_TREAD_HIGHLIGHT = COLOR_WHEEL_OUTER; // Highlight sutil

  for (int i = 1; i <= NUM_TREADS; i++) {
    // Calcular posición proporcional a lo largo de la rueda
    float t = (float)i / (NUM_TREADS + 1);

    // Punto central de esta marca (interpolando entre extremos de cada borde)
    float left_x = x0 + t * (x3 - x0);
    float right_x = x1 + t * (x2 - x1);
    int mcx = (int)((left_x + right_x) / 2);
    float left_y = y0 + t * (y3 - y0);
    float right_y = y1 + t * (y2 - y1);
    int mcy = (int)((left_y + right_y) / 2);

    // Puntos de la línea transversal (perpendicular al eje de la rueda)
    int treadHalf = w / 2 - 2; // Un poco más corto que el ancho total
    int tx1 = mcx - (int)(cosf(rad) * treadHalf);
    int ty1 = mcy - (int)(sinf(rad) * treadHalf);
    int tx2 = mcx + (int)(cosf(rad) * treadHalf);
    int ty2 = mcy + (int)(sinf(rad) * treadHalf);

    // Dibujar marca con efecto 3D (línea oscura + highlight perpendicular)
    SafeDraw::drawLine(ctx, tx1, ty1, tx2, ty2, COLOR_WHEEL_SHADOW);
    // Small perpendicular offset for highlight
    int offset_x = (int)roundf(sinf(rad)); // perpendicular to wheel axis
    int offset_y = (int)roundf(-cosf(rad));
    SafeDraw::drawLine(ctx, tx1 + offset_x, ty1 + offset_y, tx2 + offset_x,
                       ty2 + offset_y, COLOR_TREAD_HIGHLIGHT);
#ifdef RENDER_SHADOW_MODE
    // Phase 3.5: Mirror tread marks to shadow sprite
    SHADOW_MIRROR_drawLine(tx1, ty1, tx2, ty2, COLOR_WHEEL_SHADOW);
    SHADOW_MIRROR_drawLine(tx1 + offset_x, ty1 + offset_y, tx2 + offset_x,
                           ty2 + offset_y, COLOR_TREAD_HIGHLIGHT);
#endif
  }

  // Llanta interior (más clara)
  int innerScale = 55; // 55% del tamaño para dejar ver más neumático
  int ix0 = cx - dx * innerScale / 100 - ex * innerScale / 100;
  int iy0 = cy - dy * innerScale / 100 - ey * innerScale / 100;
  int ix1 = cx + dx * innerScale / 100 - ex * innerScale / 100;
  int iy1 = cy + dy * innerScale / 100 - ey * innerScale / 100;
  int ix2 = cx + dx * innerScale / 100 + ex * innerScale / 100;
  int iy2 = cy + dy * innerScale / 100 + ey * innerScale / 100;
  int ix3 = cx - dx * innerScale / 100 + ex * innerScale / 100;
  int iy3 = cy - dy * innerScale / 100 + ey * innerScale / 100;

  SafeDraw::fillTriangle(ctx, ix0, iy0, ix1, iy1, ix2, iy2, COLOR_WHEEL_INNER);
  SafeDraw::fillTriangle(ctx, ix0, iy0, ix2, iy2, ix3, iy3, COLOR_WHEEL_INNER);
#ifdef RENDER_SHADOW_MODE
  // Phase 3.5: Mirror inner wheel to shadow sprite
  SHADOW_MIRROR_fillTriangle(ix0, iy0, ix1, iy1, ix2, iy2, COLOR_WHEEL_INNER);
  SHADOW_MIRROR_fillTriangle(ix0, iy0, ix2, iy2, ix3, iy3, COLOR_WHEEL_INNER);
#endif

  // Centro de la rueda (hub) con efecto 3D
  SafeDraw::fillCircle(ctx, cx, cy, 5, COLOR_HUB_CENTER);
  SafeDraw::drawCircle(ctx, cx, cy, 5, COLOR_WHEEL_OUTER);

  // Punto de luz central (highlight)
  SafeDraw::fillCircle(ctx, cx - 1, cy - 1, 1, COLOR_HUB_BOLT);
#ifdef RENDER_SHADOW_MODE
  // Phase 3.5: Mirror hub to shadow sprite
  SHADOW_MIRROR_fillCircle(cx, cy, 5, COLOR_HUB_CENTER);
  SHADOW_MIRROR_drawCircle(cx, cy, 5, COLOR_WHEEL_OUTER);
  SHADOW_MIRROR_fillCircle(cx - 1, cy - 1, 1, COLOR_HUB_BOLT);
#endif

  // Flecha de dirección mejorada (solo para ruedas que giran)
  if (fabs(angleDeg) > 0.1f) {
    int arrowLen = 22; // Un poco más larga para ruedas verticales
    int ax = cx + (int)(cosf(rad) * arrowLen);
    int ay = cy + (int)(sinf(rad) * arrowLen);

    // Línea principal
    SafeDraw::drawLine(ctx, cx, cy, ax, ay, TFT_CYAN);

    // Punta de flecha
    float arrowAngle1 = rad + 2.5f;
    float arrowAngle2 = rad - 2.5f;
    int arrowSize = 5;
    int ax1 = ax - (int)(cosf(arrowAngle1) * arrowSize);
    int ay1 = ay - (int)(sinf(arrowAngle1) * arrowSize);
    int ax2 = ax - (int)(cosf(arrowAngle2) * arrowSize);
    int ay2 = ay - (int)(sinf(arrowAngle2) * arrowSize);

    SafeDraw::drawLine(ctx, ax, ay, ax1, ay1, TFT_CYAN);
    SafeDraw::drawLine(ctx, ax, ay, ax2, ay2, TFT_CYAN);
#ifdef RENDER_SHADOW_MODE
    // Phase 3.5: Mirror direction arrow to shadow sprite
    SHADOW_MIRROR_drawLine(cx, cy, ax, ay, TFT_CYAN);
    SHADOW_MIRROR_drawLine(ax, ay, ax1, ay1, TFT_CYAN);
    SHADOW_MIRROR_drawLine(ax, ay, ax2, ay2, TFT_CYAN);
#endif
  }
}

void WheelsDisplay::init(TFT_eSPI *display) {
  tft = display;
  SafeDraw::init(tft); // 🚨 CRITICAL FIX: Initialize SafeDraw
  initialized = true;
  // Resetear cache de todas las ruedas
  for (int i = 0; i < 4; i++) {
    wheelCaches[i].lastAngle = -999.0f;
    wheelCaches[i].lastTemp = -999.0f;
    wheelCaches[i].lastEffort = -999.0f;
  }
  Logger::info("WheelsDisplay init OK");
}

void WheelsDisplay::drawWheel(int cx, int cy, float angleDeg, float tempC,
                              float effortPct, TFT_eSprite *sprite) {
  if (!initialized) {
    Logger::warn("WheelsDisplay drawWheel() llamado sin init");
    return;
  }

  // Phase 6.3: Support dual-mode rendering (sprite or TFT)
  // Safe cast: TFT_eSprite inherits from TFT_eSPI
  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx(sprite, true, 0, 0,
                              sprite ? sprite->width() : 480,
                              sprite ? sprite->height() : 320);
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (!drawTarget) return;

  // Clamp valores
  effortPct = constrain(effortPct, 0.0f, 100.0f);
  tempC = constrain(tempC, -40.0f, 150.0f);

  // 🔒 v2.8.4: Cache por rueda - solo redibujar si hay cambios significativos
  int wheelIdx = getWheelIndex(cx, cy);
  WheelCache &cache = wheelCaches[wheelIdx];

  bool angleChanged = fabs(angleDeg - cache.lastAngle) > 0.5f;
  bool tempChanged = fabs(tempC - cache.lastTemp) > 0.5f;
  bool effortChanged = fabs(effortPct - cache.lastEffort) > 0.5f;

  // Si nada cambió, no redibujar
  if (!angleChanged && !tempChanged && !effortChanged) { return; }

  // Actualizar cache
  cache.lastAngle = angleDeg;
  cache.lastTemp = tempC;
  cache.lastEffort = effortPct;

  // Dibujar rueda 3D mejorada
  drawWheel3D(cx, cy, angleDeg, drawTarget);

  // Temperatura encima con fondo semitransparente
  SafeDraw::fillRoundRect(ctx, cx - 26, cy - 36, 52, 16, 3, COLOR_INFO_BG);
  SafeDraw::drawRoundRect(ctx, cx - 26, cy - 36, 52, 16, 3, TFT_DARKGREY);
  drawTarget->setTextDatum(MC_DATUM);
  char buf[16];

  // Si temperatura es -999, mostrar "--" (sensor deshabilitado)
  if (tempC < -998.0f) {
    drawTarget->setTextColor(TFT_DARKGREY, COLOR_INFO_BG);
    snprintf(buf, sizeof(buf), "-- C");
  } else {
    drawTarget->setTextColor(colorByTemp(tempC), COLOR_INFO_BG);
    snprintf(buf, sizeof(buf), "%d C", (int)tempC);
  }
  SafeDraw::drawString(ctx, buf, cx, cy - 28, 2);
#ifdef RENDER_SHADOW_MODE
  // Phase 3: Mirror temperature display to shadow sprite
  SHADOW_MIRROR_fillRoundRect(cx - 26, cy - 36, 52, 16, 3, COLOR_INFO_BG);
  SHADOW_MIRROR_drawRoundRect(cx - 26, cy - 36, 52, 16, 3, TFT_DARKGREY);
  SHADOW_MIRROR_setTextDatum(MC_DATUM);
  if (tempC < -998.0f) {
    SHADOW_MIRROR_setTextColor(TFT_DARKGREY, COLOR_INFO_BG);
  } else {
    SHADOW_MIRROR_setTextColor(colorByTemp(tempC), COLOR_INFO_BG);
  }
  SHADOW_MIRROR_drawString(buf, cx, cy - 28, 2);
#endif

  // Esfuerzo debajo con fondo semitransparente
  SafeDraw::fillRoundRect(ctx, cx - 26, cy + 18, 52, 16, 3, COLOR_INFO_BG);
  SafeDraw::drawRoundRect(ctx, cx - 26, cy + 18, 52, 16, 3, TFT_DARKGREY);

  // Si esfuerzo es -1, mostrar "--" (sensor deshabilitado)
  if (effortPct < 0.0f) {
    drawTarget->setTextColor(TFT_DARKGREY, COLOR_INFO_BG);
    snprintf(buf, sizeof(buf), "-- %%");
  } else {
    drawTarget->setTextColor(colorByEffort(effortPct), COLOR_INFO_BG);
    snprintf(buf, sizeof(buf), "%d%%", (int)effortPct);
  }
  SafeDraw::drawString(ctx, buf, cx, cy + 26, 2);
#ifdef RENDER_SHADOW_MODE
  // Phase 3: Mirror effort display to shadow sprite
  SHADOW_MIRROR_fillRoundRect(cx - 26, cy + 18, 52, 16, 3, COLOR_INFO_BG);
  SHADOW_MIRROR_drawRoundRect(cx - 26, cy + 18, 52, 16, 3, TFT_DARKGREY);
  if (effortPct < 0.0f) {
    SHADOW_MIRROR_setTextColor(TFT_DARKGREY, COLOR_INFO_BG);
  } else {
    SHADOW_MIRROR_setTextColor(colorByEffort(effortPct), COLOR_INFO_BG);
  }
  SHADOW_MIRROR_drawString(buf, cx, cy + 26, 2);
#endif

  // Barra de esfuerzo 3D (solo si hay valor válido)
  int barW = 48, barH = 8;
  int barX = cx - barW / 2;
  int barY = cy + 36;

  // Fondo de barra con efecto 3D
  SafeDraw::fillRoundRect(ctx, barX, barY, barW, barH, 2, COLOR_BAR_BG);
  SafeDraw::drawRoundRect(ctx, barX, barY, barW, barH, 2, TFT_DARKGREY);
#ifdef RENDER_SHADOW_MODE
  // Phase 3: Mirror effort bar background to shadow sprite
  SHADOW_MIRROR_fillRoundRect(barX, barY, barW, barH, 2, COLOR_BAR_BG);
  SHADOW_MIRROR_drawRoundRect(barX, barY, barW, barH, 2, TFT_DARKGREY);
#endif

  if (effortPct >= 0.0f) {
    int filled = (int)((effortPct / 100.0f) * (barW - 4));
    if (filled > 0) {
      uint16_t barColor = colorByEffort(effortPct);
      SafeDraw::fillRoundRect(ctx, barX + 2, barY + 2, filled, barH - 4, 1,
                              barColor);
      // Highlight superior
      SafeDraw::drawFastHLine(ctx, barX + 2, barY + 2, filled, 0xFFFF);
#ifdef RENDER_SHADOW_MODE
      // Phase 3: Mirror effort bar fill to shadow sprite
      SHADOW_MIRROR_fillRoundRect(barX + 2, barY + 2, filled, barH - 4, 1,
                                  barColor);
      SHADOW_MIRROR_drawFastHLine(barX + 2, barY + 2, filled, 0xFFFF);
#endif
    }
  }
}

// ============================================================================
// PHASE 10: RenderContext version for granular dirty tracking
// ============================================================================

void WheelsDisplay::drawWheel(int cx, int cy, float angleDeg, float tempC,
                              float effortPct, HudLayer::RenderContext &ctx) {
  if (!ctx.isValid()) return;

  // Get wheel index for cache
  int wheelIdx = getWheelIndex(cx, cy);
  if (wheelIdx < 0) {
    // Invalid wheel position, fallback to sprite version
    drawWheel(cx, cy, angleDeg, tempC, effortPct, ctx.sprite);
    return;
  }

  WheelCache &cache = wheelCaches[wheelIdx];

  // Check if anything changed
  bool angleChanged = fabs(angleDeg - cache.lastAngle) > 0.5f;
  bool tempChanged = fabs(tempC - cache.lastTemp) > 0.5f;
  bool effortChanged = fabs(effortPct - cache.lastEffort) > 0.5f;
  bool changed = angleChanged || tempChanged || effortChanged;

  // Call sprite version
  drawWheel(cx, cy, angleDeg, tempC, effortPct, ctx.sprite);

  // Mark dirty if changed
  if (changed) {
    // Wheel bounding box: approx 60x80 pixels
    // Temperature is above wheel, effort bar below
    const int wheelW = 60;
    const int wheelH = 80;
    ctx.markDirty(cx - wheelW / 2, cy - 40, wheelW, wheelH);
  }
}