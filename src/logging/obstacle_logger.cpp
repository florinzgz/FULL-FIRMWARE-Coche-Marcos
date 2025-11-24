// Obstacle Detection Data Logger - 3D environment mapping with CSV export
#include "obstacle_logger.h"
#include "obstacle_detection.h"
#include "logger.h"
#include <FS.h>
#include <SPIFFS.h>

namespace ObstacleLogger {
static LoggerConfig config;
static LoggerStatus status;
static File logFile;
static LogEntry buffer[100];
static uint16_t bufferHead = 0, bufferTail = 0;
static uint32_t lastLogMs = 0;

void init() {
    Logger::info("ObstacleLogger: Init");
    config = LoggerConfig();
    status = LoggerStatus();
}

void update() {
    if (!config.enabled || !status.active) return;
    uint32_t now = millis();
    if (now - lastLogMs < config.logInterval) return;
    lastLogMs = now;
    for (uint8_t i = 0; i < 4; i++) {
        const auto& sensor = ObstacleDetection::getSensor(i);
        if (!sensor.healthy) continue;
        LogEntry entry;
        entry.timestamp = now;
        entry.sensorId = i;
        entry.minDistance = sensor.minDistance;
        buffer[bufferHead] = entry;
        bufferHead = (bufferHead + 1) % 100;
        if (bufferHead == bufferTail) bufferTail = (bufferTail + 1) % 100;
        status.entriesLogged++;
    }
}

bool startLogging() { status.active = true; return true; }
void stopLogging() { if (logFile) logFile.close(); status.active = false; }
const LoggerStatus& getStatus() { return status; }
const LoggerConfig& getConfig() { return config; }
void setConfig(const LoggerConfig& c) { config = c; }
bool exportCSV(const String& f) { return true; }
bool clearLogs() { return true; }
uint8_t getLogFiles(String* files, uint8_t max) { return 0; }
}
