#include "../../include/menu_ina226_monitor.h"
#include "../../include/i2c_recovery.h"
#include "../../include/logger.h" // 🔒 CRITICAL FIX: Include logger for error reporting
#include "../../include/hud_layer.h" // For RenderContext
#include "../../include/safe_draw.h" // For SafeDraw::getDrawTarget

TFT_eSPI *MenuINA226Monitor::_tft = nullptr;
unsigned long MenuINA226Monitor::_lastUpdate = 0;
float MenuINA226Monitor::_peakCurrent = 0.0f;
float MenuINA226Monitor::_totalCurrentSum = 0.0f;
uint32_t MenuINA226Monitor::_sampleCount = 0;
MenuINA226Monitor::SensorData MenuINA226Monitor::_sensors[6] = {};

void MenuINA226Monitor::init(TFT_eSPI *tft) {
  _tft = tft;
  _lastUpdate = 0;
  resetStatistics();

  // Initialize sensor data
  for (int i = 0; i < 6; i++) {
    _sensors[i].current = 0.0f;
    _sensors[i].voltage = 0.0f;
    _sensors[i].online = false;
    _sensors[i].state = 2; // OFFLINE initially
  }

  draw();
}

void MenuINA226Monitor::draw() {
  // 🔒 CRITICAL FIX: Check _tft is not null before use
  if (_tft == nullptr) {
    Logger::error(999, "MenuINA226Monitor::draw() called with null TFT");
    return;
  }

  _tft->fillScreen(TFT_BLACK);
  _tft->setTextColor(TFT_WHITE, TFT_BLACK);
  _tft->setTextSize(2);
  _tft->setCursor(10, 5);
  _tft->print("INA226 Monitor");

  // Draw 6 sensor cards in 2x3 grid
  // Create RenderContext for direct screen drawing
  HudLayer::RenderContext ctx(nullptr, true, 0, 0, 480, 320);
  
  for (int i = 0; i < 6; i++) {
    uint16_t x = (i % 2) * (CARD_WIDTH + CARD_MARGIN) + 5;
    uint16_t y = (i / 2) * (CARD_HEIGHT + CARD_MARGIN) + 30;
    drawSensorCard(i, x, y, ctx);
  }

  drawStatistics();
  drawButtons();
}

void MenuINA226Monitor::update() {
  unsigned long now = millis();
  if (now - _lastUpdate < UPDATE_INTERVAL) return;
  _lastUpdate = now;

  updateSensorData();

  // Create RenderContext for direct screen drawing
  HudLayer::RenderContext ctx(nullptr, true, 0, 0, 480, 320);

  // Redraw sensor cards and statistics
  for (int i = 0; i < 6; i++) {
    uint16_t x = (i % 2) * (CARD_WIDTH + CARD_MARGIN) + 5;
    uint16_t y = (i / 2) * (CARD_HEIGHT + CARD_MARGIN) + 30;
    drawSensorCard(i, x, y, ctx);
  }

  drawStatistics();
}

void MenuINA226Monitor::handleTouch(uint16_t x, uint16_t y) {
  // Reset Stats button (10, BTN_Y, 150, 30)
  if (x >= 10 && x <= 160 && y >= BTN_Y && y <= BTN_Y + 30) {
    resetStatistics();
    _tft->fillRect(10, BTN_Y, 150, 30, TFT_ORANGE);
    // Non-blocking: visual feedback will persist until next draw cycle
    // Removed blocking delay(100)
    drawButtons();
    drawStatistics();
  }
  // Back button (170, BTN_Y, 140, 30)
  else if (x >= 170 && x <= 310 && y >= BTN_Y && y <= BTN_Y + 30) {
    // Return to menu (handled by caller)
  }
}

void MenuINA226Monitor::drawSensorCard(uint8_t index, uint16_t x, uint16_t y,
                                        const HudLayer::RenderContext &ctx) {
  // 🔒 CRITICAL FIX: Get draw target from RenderContext
  TFT_eSPI *drawTarget = SafeDraw::getDrawTarget(ctx);
  if (!drawTarget) return;

  // Card background
  drawTarget->fillRoundRect(x, y, CARD_WIDTH, CARD_HEIGHT, 5, TFT_DARKGREY);
  drawTarget->drawRoundRect(x, y, CARD_WIDTH, CARD_HEIGHT, 5, TFT_WHITE);

  // Sensor name
  drawTarget->setTextSize(1);
  drawTarget->setTextColor(TFT_CYAN, TFT_DARKGREY);
  drawTarget->setCursor(x + 5, y + 5);
  drawTarget->print(getSensorName(index));

  // Status indicator (LED)
  uint16_t statusColor = TFT_DARKGREY;
  if (_sensors[index].online) {
    if (_sensors[index].current > 50.0f)
      statusColor = TFT_RED;
    else if (_sensors[index].current > 40.0f)
      statusColor = TFT_YELLOW;
    else
      statusColor = TFT_GREEN;
  }
  drawTarget->fillCircle(x + CARD_WIDTH - 10, y + 10, 5, statusColor);

  // Current value
  drawTarget->setTextSize(2);
  uint16_t currentColor = getCurrentColor(_sensors[index].current);
  drawTarget->setTextColor(currentColor, TFT_DARKGREY);
  drawTarget->setCursor(x + 10, y + 35);
  if (_sensors[index].online) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1fA", _sensors[index].current);
    drawTarget->print(buf);
  } else {
    drawTarget->print("--.-A");
  }

  // Voltage value
  drawTarget->setTextSize(1);
  drawTarget->setTextColor(TFT_WHITE, TFT_DARKGREY);
  drawTarget->setCursor(x + 10, y + 60);
  if (_sensors[index].online) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1fV", _sensors[index].voltage);
    drawTarget->print(buf);
  } else {
    drawTarget->print("--.-V");
  }

  // State text
  drawTarget->setTextColor(TFT_LIGHTGREY, TFT_DARKGREY);
  drawTarget->setCursor(x + 10, y + 80);
  if (_sensors[index].state == 0)
    drawTarget->print("OK");
  else if (_sensors[index].state == 1)
    drawTarget->print("BACKOFF");
  else
    drawTarget->print("OFFLINE");
}

void MenuINA226Monitor::drawStatistics() {
  // 🔒 CRITICAL FIX: Check _tft is not null before use
  if (_tft == nullptr) return;

  // Stats panel background
  _tft->fillRect(5, STATS_Y, 310, 60, TFT_NAVY);
  _tft->drawRect(5, STATS_Y, 310, 60, TFT_WHITE);

  _tft->setTextSize(1);
  _tft->setTextColor(TFT_WHITE, TFT_NAVY);

  // Total current
  _tft->setCursor(10, STATS_Y + 5);
  _tft->print("Total:");
  _tft->setTextColor(TFT_YELLOW, TFT_NAVY);
  _tft->setCursor(10, STATS_Y + 20);
  float totalCurrent = 0.0f;
  for (int i = 0; i < 6; i++) {
    if (_sensors[i].online) totalCurrent += _sensors[i].current;
  }
  char buf[16];
  snprintf(buf, sizeof(buf), "%.1fA", totalCurrent);
  _tft->print(buf);

  // Peak current
  _tft->setTextColor(TFT_WHITE, TFT_NAVY);
  _tft->setCursor(85, STATS_Y + 5);
  _tft->print("Peak:");
  _tft->setTextColor(TFT_RED, TFT_NAVY);
  _tft->setCursor(85, STATS_Y + 20);
  snprintf(buf, sizeof(buf), "%.1fA", _peakCurrent);
  _tft->print(buf);

  // Average current
  _tft->setTextColor(TFT_WHITE, TFT_NAVY);
  _tft->setCursor(160, STATS_Y + 5);
  _tft->print("Avg:");
  _tft->setTextColor(TFT_CYAN, TFT_NAVY);
  _tft->setCursor(160, STATS_Y + 20);
  float avgCurrent = _sampleCount > 0 ? _totalCurrentSum / _sampleCount : 0.0f;
  snprintf(buf, sizeof(buf), "%.1fA", avgCurrent);
  _tft->print(buf);

  // Battery voltage
  _tft->setTextColor(TFT_WHITE, TFT_NAVY);
  _tft->setCursor(10, STATS_Y + 40);
  _tft->print("Battery:");
  _tft->setTextColor(TFT_GREEN, TFT_NAVY);
  _tft->setCursor(70, STATS_Y + 40);
  if (_sensors[4].online) { // Battery is index 4
    snprintf(buf, sizeof(buf), "%.1fV", _sensors[4].voltage);
    _tft->print(buf);
  } else {
    _tft->print("--.-V");
  }
}

void MenuINA226Monitor::drawButtons() {
  // 🔒 CRITICAL FIX: Check _tft is not null before use
  if (_tft == nullptr) return;

  // Reset Stats button
  _tft->fillRoundRect(10, BTN_Y, 150, 30, 5, TFT_ORANGE);
  _tft->drawRoundRect(10, BTN_Y, 150, 30, 5, TFT_WHITE);
  _tft->setTextSize(2);
  _tft->setTextColor(TFT_BLACK, TFT_ORANGE);
  _tft->setCursor(25, BTN_Y + 8);
  _tft->print("Reset Stats");

  // Back button
  _tft->fillRoundRect(170, BTN_Y, 140, 30, 5, TFT_RED);
  _tft->drawRoundRect(170, BTN_Y, 140, 30, 5, TFT_WHITE);
  _tft->setTextColor(TFT_WHITE, TFT_RED);
  _tft->setCursor(215, BTN_Y + 8);
  _tft->print("Back");
}

void MenuINA226Monitor::updateSensorData() {
  float totalCurrent = 0.0f;

  for (int i = 0; i < 6; i++) {
    // Check if sensor is online via I2C Recovery
    const I2CRecovery::DeviceState &state = I2CRecovery::getDeviceState(i);
    _sensors[i].state = state.online ? 1 : 0;
    _sensors[i].online = state.online;

    if (_sensors[i].online) {
      // Read current and voltage from Sensors module
      _sensors[i].current = Sensors::getCurrent(i);
      _sensors[i].voltage = Sensors::getVoltage(i);
      totalCurrent += _sensors[i].current;
    } else {
      _sensors[i].current = 0.0f;
      _sensors[i].voltage = 0.0f;
    }
  }

  // Update statistics
  if (totalCurrent > _peakCurrent) { _peakCurrent = totalCurrent; }

  _totalCurrentSum += totalCurrent;
  _sampleCount++;
}

uint16_t MenuINA226Monitor::getCurrentColor(float current) {
  if (current < 10.0f)
    return TFT_CYAN;
  else if (current < 25.0f)
    return TFT_GREEN;
  else if (current < 40.0f)
    return TFT_YELLOW;
  else if (current < 50.0f)
    return TFT_ORANGE;
  else
    return TFT_RED;
}

const char *MenuINA226Monitor::getSensorName(uint8_t index) {
  switch (index) {
  case 0:
    return "Motor FL";
  case 1:
    return "Motor FR";
  case 2:
    return "Motor RL";
  case 3:
    return "Motor RR";
  case 4:
    return "Battery";
  case 5:
    return "Steering";
  default:
    return "Unknown";
  }
}

void MenuINA226Monitor::resetStatistics() {
  _peakCurrent = 0.0f;
  _totalCurrentSum = 0.0f;
  _sampleCount = 0;
}
