#include "gauges.h"
#include "hud_layer.h" // 🚨 CRITICAL FIX: For RenderContext
#include "safe_draw.h" // 🚨 CRITICAL FIX: For coordinate-safe drawing
#include "settings.h"
#include "shadow_render.h" // Phase 3: Shadow mirroring support
#include <Arduino.h>       // para constrain(), snprintf, etc.
#include <cmath>           // Phase 10: for fabs()

static TFT_eSPI *tft = nullptr;

// Guardamos el último valor para poder borrar solo la aguja anterior
static float lastSpeed = -1;
static float lastRpm = -1;

// Constantes matemáticas
static const float DEG_TO_RAD_CONST =
    0.0174533f; // π/180 para conversión grados a radianes

// Constantes de configuración por defecto
static const int DEFAULT_MAX_KMH = 35;  // Velocidad máxima por defecto
static const int DEFAULT_MAX_RPM = 400; // RPM máxima por defecto

// Colores 3D para efectos de profundidad
static const uint16_t COLOR_GAUGE_OUTER = 0x4A69;     // Gris oscuro metálico
static const uint16_t COLOR_GAUGE_INNER = 0x2124;     // Negro profundo
static const uint16_t COLOR_GAUGE_RING = 0x7BCF;      // Plata brillante
static const uint16_t COLOR_GAUGE_HIGHLIGHT = 0xBDF7; // Blanco/plata claro
static const uint16_t COLOR_NEEDLE_BASE = 0xF800;     // Rojo principal
static const uint16_t COLOR_NEEDLE_TIP = 0xFFFF;      // Blanco punta
static const uint16_t COLOR_NEEDLE_SHADOW = 0x8000;   // Rojo oscuro sombra
static const uint16_t COLOR_SCALE_NORMAL = 0x07E0;    // Verde
static const uint16_t COLOR_SCALE_WARN = 0xFD20;      // Naranja
static const uint16_t COLOR_SCALE_DANGER = 0xF800;    // Rojo

// -----------------------
// Helpers mejorados
// -----------------------

// Dibujar arco grueso con efecto 3D
// Phase 6: Added target parameter for compositor mode
static void drawThickArc(int cx, int cy, int r, int thickness, uint16_t color,
                         float startAngle, float endAngle,
                         const HudLayer::RenderContext &ctx) {
  // 🚨 CRITICAL FIX: Use SafeDraw with RenderContext

  // Use TFT_eSPI's optimized drawArc() method for better performance
  for (int i = 0; i < thickness; i++) {
    // drawArc expects angles in degrees, and draws a ring segment with given
    // thickness
    SafeDraw::drawArc(ctx, cx, cy, r - i, r - i, (int)startAngle, (int)endAngle,
                      color, color, true);
#ifdef RENDER_SHADOW_MODE
    // Phase 3: Mirror to shadow sprite for validation
    SHADOW_MIRROR_drawArc(cx, cy, r - i, r - i, (int)startAngle, (int)endAngle,
                          color, color, true);
#endif
  }
}

// Dibujar marcas de escala con números
// Phase 6: Added target parameter for compositor mode
static void drawScaleMarks(int cx, int cy, int r, int maxValue, int step,
                           bool showNumbers,
                           const HudLayer::RenderContext &ctx) {
  // 🚨 CRITICAL FIX: Use SafeDraw with RenderContext
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (!drawTarget) return;

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
    SafeDraw::drawLine(ctx, x1, y1, x2, y2, color);
    SafeDraw::drawLine(ctx, x1 + 1, y1, x2 + 1, y2, color);
#ifdef RENDER_SHADOW_MODE
    // Phase 3: Mirror to shadow sprite
    SHADOW_MIRROR_drawLine(x1, y1, x2, y2, color);
    SHADOW_MIRROR_drawLine(x1 + 1, y1, x2 + 1, y2, color);
#endif

    // Número de escala
    if (showNumbers && (i % 2 == 0 || numMarks <= 6)) {
      int xNum = cx + (int)(cosf(rad) * (r - 25));
      int yNum = cy + (int)(sinf(rad) * (r - 25));
      drawTarget->setTextDatum(MC_DATUM);
      drawTarget->setTextColor(TFT_WHITE, TFT_BLACK);
      char buf[8];
      snprintf(buf, sizeof(buf), "%d", (int)value);
      SafeDraw::drawString(ctx, buf, xNum, yNum, 1);
#ifdef RENDER_SHADOW_MODE
      // Phase 3: Mirror text to shadow sprite
      SHADOW_MIRROR_setTextDatum(MC_DATUM);
      SHADOW_MIRROR_setTextColor(TFT_WHITE, TFT_BLACK);
      SHADOW_MIRROR_drawString(buf, xNum, yNum, 1);
#endif
    }
  }

  // Marcas menores (entre las principales)
  for (int i = 0; i <= numMarks * 2; i++) {
    if (i % 2 == 1) { // Solo marcas intermedias
      float value = i * step / 2.0f;
      float angle = -135.0f + (value / (float)maxValue) * 270.0f;
      float rad = angle * DEG_TO_RAD_CONST;

      int x1 = cx + (int)(cosf(rad) * (r - 5));
      int y1 = cy + (int)(sinf(rad) * (r - 5));
      int x2 = cx + (int)(cosf(rad) * (r - 10));
      int y2 = cy + (int)(sinf(rad) * (r - 10));
      SafeDraw::drawLine(ctx, x1, y1, x2, y2, TFT_DARKGREY);
#ifdef RENDER_SHADOW_MODE
      // Phase 3: Mirror minor marks to shadow sprite
      SHADOW_MIRROR_drawLine(x1, y1, x2, y2, TFT_DARKGREY);
#endif
    }
  }
}

// Aguja 3D con efecto de profundidad
// Phase 6: Added target parameter for compositor mode
static void drawNeedle3D(int cx, int cy, float value, float maxValue, int r,
                         bool erase, const HudLayer::RenderContext &ctx) {
  // 🚨 CRITICAL FIX: Use SafeDraw with RenderContext

  // 🔒 CORRECCIÓN ALTA: Proteger contra división por cero
  if (maxValue <= 0.0f) {
    maxValue = 1.0f; // Fallback seguro
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
    SafeDraw::fillTriangle(ctx, tipX, tipY, baseX1, baseY1, baseX2, baseY2,
                           TFT_BLACK);
    SafeDraw::fillCircle(ctx, cx, cy, 8, COLOR_GAUGE_INNER);
#ifdef RENDER_SHADOW_MODE
    // Phase 3: Mirror erase to shadow sprite
    SHADOW_MIRROR_fillTriangle(tipX, tipY, baseX1, baseY1, baseX2, baseY2,
                               TFT_BLACK);
    SHADOW_MIRROR_fillCircle(cx, cy, 8, COLOR_GAUGE_INNER);
#endif
  } else {
    // Sombra de la aguja (desplazada 2px)
    SafeDraw::fillTriangle(ctx, tipX + 1, tipY + 1, baseX1 + 1, baseY1 + 1,
                           baseX2 + 1, baseY2 + 1, COLOR_NEEDLE_SHADOW);

    // Aguja principal
    SafeDraw::fillTriangle(ctx, tipX, tipY, baseX1, baseY1, baseX2, baseY2,
                           COLOR_NEEDLE_BASE);

    // Línea central blanca (efecto brillo)
    int midX = cx + (int)(cosf(rad) * (r * 0.6f));
    int midY = cy + (int)(sinf(rad) * (r * 0.6f));
    SafeDraw::drawLine(ctx, cx, cy, midX, midY, COLOR_NEEDLE_TIP);

    // Centro de la aguja (círculo 3D)
    SafeDraw::fillCircle(ctx, cx, cy, 10, TFT_DARKGREY);
    SafeDraw::fillCircle(ctx, cx, cy, 8, COLOR_GAUGE_RING);
    SafeDraw::fillCircle(ctx, cx - 2, cy - 2, 3, COLOR_GAUGE_HIGHLIGHT);
#ifdef RENDER_SHADOW_MODE
    // Phase 3: Mirror needle drawing to shadow sprite
    SHADOW_MIRROR_fillTriangle(tipX + 1, tipY + 1, baseX1 + 1, baseY1 + 1,
                               baseX2 + 1, baseY2 + 1, COLOR_NEEDLE_SHADOW);
    SHADOW_MIRROR_fillTriangle(tipX, tipY, baseX1, baseY1, baseX2, baseY2,
                               COLOR_NEEDLE_BASE);
    SHADOW_MIRROR_drawLine(cx, cy, midX, midY, COLOR_NEEDLE_TIP);
    SHADOW_MIRROR_fillCircle(cx, cy, 10, TFT_DARKGREY);
    SHADOW_MIRROR_fillCircle(cx, cy, 8, COLOR_GAUGE_RING);
    SHADOW_MIRROR_fillCircle(cx - 2, cy - 2, 3, COLOR_GAUGE_HIGHLIGHT);
#endif
  }
}

// -----------------------
// Fondo estático mejorado con efecto 3D
// -----------------------
// Phase 6: Added target parameter for compositor mode
static void drawGaugeBackground(int cx, int cy, int maxValue, int step,
                                const char *unit,
                                const HudLayer::RenderContext &ctx) {
  // 🚨 CRITICAL FIX: Use SafeDraw with RenderContext
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (!drawTarget) return;

  int outerRadius = 68;
  int innerRadius = 55;

  // Fondo negro profundo
  SafeDraw::fillCircle(ctx, cx, cy, outerRadius + 5, TFT_BLACK);

  // Anillo exterior metálico (efecto 3D con gradiente)
  for (int r = outerRadius + 4; r >= outerRadius; r--) {
    uint16_t shade = (r == outerRadius + 4) ? COLOR_GAUGE_HIGHLIGHT
                     : (r == outerRadius)   ? COLOR_GAUGE_OUTER
                                            : COLOR_GAUGE_RING;
    SafeDraw::drawCircle(ctx, cx, cy, r, shade);
#ifdef RENDER_SHADOW_MODE
    // Phase 3: Mirror gauge rings to shadow sprite
    SHADOW_MIRROR_drawCircle(cx, cy, r, shade);
#endif
  }

  // Interior negro
  SafeDraw::fillCircle(ctx, cx, cy, innerRadius, COLOR_GAUGE_INNER);
#ifdef RENDER_SHADOW_MODE
  // Phase 3: Mirror gauge interior to shadow sprite
  SHADOW_MIRROR_fillCircle(cx, cy, innerRadius, COLOR_GAUGE_INNER);
#endif

  // Arco de escala coloreado (verde-amarillo-rojo)
  // Verde (0-60%)
  drawThickArc(cx, cy, outerRadius - 2, 3, COLOR_SCALE_NORMAL, -135.0f,
               -135.0f + 270.0f * 0.6f, ctx);
  // Amarillo (60-85%)
  drawThickArc(cx, cy, outerRadius - 2, 3, COLOR_SCALE_WARN,
               -135.0f + 270.0f * 0.6f, -135.0f + 270.0f * 0.85f, ctx);
  // Rojo (85-100%)
  drawThickArc(cx, cy, outerRadius - 2, 3, COLOR_SCALE_DANGER,
               -135.0f + 270.0f * 0.85f, 135.0f, ctx);

  // Marcas de escala con números
  drawScaleMarks(cx, cy, outerRadius, maxValue, step, true, ctx);

  // Etiqueta de unidad
  drawTarget->setTextDatum(MC_DATUM);
  drawTarget->setTextColor(TFT_CYAN, TFT_BLACK);
  SafeDraw::drawString(ctx, unit, cx, cy + 35, 1);
#ifdef RENDER_SHADOW_MODE
  // Phase 3: Mirror unit label to shadow sprite
  SHADOW_MIRROR_setTextDatum(MC_DATUM);
  SHADOW_MIRROR_setTextColor(TFT_CYAN, TFT_BLACK);
  SHADOW_MIRROR_drawString(unit, cx, cy + 35, 1);
#endif
}

// -----------------------
// API
// -----------------------
void Gauges::init(TFT_eSPI *display) {
  tft = display;
  // 🚨 CRITICAL FIX: Initialize SafeDraw
  SafeDraw::init(display);
  lastSpeed = -1;
  lastRpm = -1;
}

void Gauges::drawSpeed(int cx, int cy, float kmh, int maxKmh, float pedalPct,
                       TFT_eSprite *sprite) {
  // Phase 6: Support dual-mode rendering (sprite or TFT)
  // Safe cast: TFT_eSprite inherits from TFT_eSPI
  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx(sprite, true, 0, 0,
                              sprite ? sprite->width() : TFT_WIDTH,
                              sprite ? sprite->height() : TFT_HEIGHT);
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (!drawTarget) return;

  // 🔒 CORRECCIÓN ALTA: Clamp speed con límite superior seguro
  kmh = constrain(kmh, 0.0f,
                  min((float)maxKmh, 999.0f)); // Prevenir overflow visual

  // 🔒 Validar maxKmh para prevenir división por cero
  if (maxKmh <= 0) maxKmh = DEFAULT_MAX_KMH;

  // Calcular step de escala según maxKmh
  int step = (maxKmh <= 50) ? 5 : 10;

  // Redibujar fondo solo si es la primera vez
  if (lastSpeed < 0) {
    drawGaugeBackground(cx, cy, maxKmh, step, "km/h", ctx);
  } else {
    // Borrar aguja anterior
    drawNeedle3D(cx, cy, lastSpeed, (float)maxKmh, 50, true, ctx);
  }

  // Dibujar aguja nueva con efecto 3D
  drawNeedle3D(cx, cy, kmh, (float)maxKmh, 50, false, ctx);

  // Texto central grande con valor
  drawTarget->setTextDatum(MC_DATUM);
  // Use SafeDraw for coordinate translation
  SafeDraw::fillRect(ctx, cx - 25, cy - 5, 50, 22, COLOR_GAUGE_INNER);

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

  drawTarget->setTextColor(textColor, COLOR_GAUGE_INNER);
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", (int)kmh);
  SafeDraw::drawString(ctx, buf, cx, cy + 5, 4);
#ifdef RENDER_SHADOW_MODE
  // Phase 3: Mirror speed value text to shadow sprite
  SHADOW_MIRROR_setTextDatum(MC_DATUM);
  SHADOW_MIRROR_fillRect(cx - 25, cy - 5, 50, 22, COLOR_GAUGE_INNER);
  SHADOW_MIRROR_setTextColor(textColor, COLOR_GAUGE_INNER);
  SHADOW_MIRROR_drawString(buf, cx, cy + 5, 4);
#endif

  lastSpeed = kmh;
}

void Gauges::drawRPM(int cx, int cy, float rpm, int maxRpm,
                     TFT_eSprite *sprite) {
  // Phase 6: Support dual-mode rendering (sprite or TFT)
  // Safe cast: TFT_eSprite inherits from TFT_eSPI
  // 🚨 CRITICAL FIX: Create safe RenderContext
  HudLayer::RenderContext ctx(sprite, true, 0, 0,
                              sprite ? sprite->width() : TFT_WIDTH,
                              sprite ? sprite->height() : TFT_HEIGHT);
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (!drawTarget) return;

  // 🔒 CORRECCIÓN ALTA: Validar maxRpm para prevenir división por cero
  if (maxRpm <= 0) maxRpm = DEFAULT_MAX_RPM;

  rpm = constrain(rpm, 0.0f, (float)maxRpm);

  // Calcular step de escala según maxRpm
  int step = (maxRpm <= 500) ? 50 : 100;

  if (lastRpm < 0) {
    drawGaugeBackground(cx, cy, maxRpm, step, "RPM", ctx);
  } else {
    // Borrar aguja anterior
    drawNeedle3D(cx, cy, lastRpm, (float)maxRpm, 50, true, ctx);
  }

  // Dibujar aguja nueva con efecto 3D
  drawNeedle3D(cx, cy, rpm, (float)maxRpm, 50, false, ctx);

  // Texto central grande con valor
  drawTarget->setTextDatum(MC_DATUM);
  // Use SafeDraw for coordinate translation
  SafeDraw::fillRect(ctx, cx - 25, cy - 5, 50, 22, COLOR_GAUGE_INNER);

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

  drawTarget->setTextColor(textColor, COLOR_GAUGE_INNER);
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", (int)rpm);
  SafeDraw::drawString(ctx, buf, cx, cy + 5, 4);
#ifdef RENDER_SHADOW_MODE
  // Phase 3: Mirror RPM value text to shadow sprite
  SHADOW_MIRROR_setTextDatum(MC_DATUM);
  SHADOW_MIRROR_fillRect(cx - 25, cy - 5, 50, 22, COLOR_GAUGE_INNER);
  SHADOW_MIRROR_setTextColor(textColor, COLOR_GAUGE_INNER);
  SHADOW_MIRROR_drawString(buf, cx, cy + 5, 4);
#endif

  lastRpm = rpm;
}

// ============================================================================
// PHASE 10: RenderContext versions for granular dirty tracking
// ============================================================================

void Gauges::drawSpeed(int cx, int cy, float kmh, int maxKmh, float pedalPct,
                       HudLayer::RenderContext &ctx) {
  if (!ctx.isValid()) return;

  // Check if value changed significantly (> 0.5 km/h)
  bool changed = (lastSpeed < 0) || (fabs(kmh - lastSpeed) > 0.5f);

  // Call sprite version
  drawSpeed(cx, cy, kmh, maxKmh, pedalPct, ctx.sprite);

  // Mark dirty region if changed
  if (changed) {
    // Gauge bounding box: center ± (outerRadius + 5)
    const int gaugeRadius = 73; // outerRadius (68) + 5
    ctx.markDirty(cx - gaugeRadius, cy - gaugeRadius, gaugeRadius * 2,
                  gaugeRadius * 2);
  }
}

void Gauges::drawRPM(int cx, int cy, float rpm, int maxRpm,
                     HudLayer::RenderContext &ctx) {
  if (!ctx.isValid()) return;

  // Check if value changed significantly (> 5 RPM)
  bool changed = (lastRpm < 0) || (fabs(rpm - lastRpm) > 5.0f);

  // Call sprite version
  drawRPM(cx, cy, rpm, maxRpm, ctx.sprite);

  // Mark dirty region if changed
  if (changed) {
    // Gauge bounding box: center ± (outerRadius + 5)
    const int gaugeRadius = 73; // outerRadius (68) + 5
    ctx.markDirty(cx - gaugeRadius, cy - gaugeRadius, gaugeRadius * 2,
                  gaugeRadius * 2);
  }
}