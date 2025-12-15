#include "logger.h"
#include <Arduino.h>
#include <cstdarg>
#include <cstdio>
#include "system.h"   // opcional: para logError persistente

namespace Logger {

static bool serialReady = false;

void init() {
    Serial.begin(115200);
    serialReady = true;
    // Opcional: esperar a que Serial esté listo en placas con USB nativo
    // unsigned long start = millis();
    // while (!Serial && (millis() - start < 2000)) { delay(1); }
    if(!Serial) {
        warn("Serial no reporta disponibilidad inmediata");
    } else {
        // 🔒 v2.10.7: Confirm logger initialization
        info("Logger init: Serial comunicación establecida");
    }
}

// --- Helpers internos ---
static inline void safePrint(const char *prefix, const char *msg) {
    if (!serialReady || msg == nullptr) return;
    Serial.print(prefix);
    Serial.println(msg);
}

static inline void vformat(char *buf, size_t size, const char *fmt, va_list ap) {
    if (buf == nullptr || size == 0) return;
    vsnprintf(buf, size, fmt, ap);
    buf[size - 1] = '\0'; // 🔒 asegurar terminación
}

// --- Implementaciones base ---
void info(const char *msg) {
    safePrint("[INFO] ", msg);
}

void warn(const char *msg) {
    safePrint("[WARN] ", msg);
}

void error(uint16_t code, const char *msg) {
    if (!serialReady) return;
    Serial.print("[ERROR ");
    Serial.print(code);
    Serial.print("] ");
    if (msg) Serial.println(msg);
    else Serial.println("(null)");

    // Opcional: registrar también en log persistente
    if(code != 0) {
        System::logError(code);
    }
}

// --- Helpers de formato ---
void infof(const char *fmt, ...) {
    constexpr size_t BUF_SZ = 256;
    char buf[BUF_SZ];
    va_list ap;
    va_start(ap, fmt);
    vformat(buf, BUF_SZ, fmt, ap);
    va_end(ap);
    info(buf);
}

void warnf(const char *fmt, ...) {
    constexpr size_t BUF_SZ = 256;
    char buf[BUF_SZ];
    va_list ap;
    va_start(ap, fmt);
    vformat(buf, BUF_SZ, fmt, ap);
    va_end(ap);
    warn(buf);
}

void errorf(uint16_t code, const char *fmt, ...) {
    constexpr size_t BUF_SZ = 256;
    char buf[BUF_SZ];
    va_list ap;
    va_start(ap, fmt);
    vformat(buf, BUF_SZ, fmt, ap);
    va_end(ap);
    error(code, buf);
}

void errorf(const char *fmt, ...) {
    constexpr size_t BUF_SZ = 256;
    char buf[BUF_SZ];
    va_list ap;
    va_start(ap, fmt);
    vformat(buf, BUF_SZ, fmt, ap);
    va_end(ap);
    error(999, buf); // coherente con logger.h
}

// --- Debug functions ---
// Nivel global de debug (en debug.cpp hay uno similar, usamos level 2 = DEBUG)
static uint8_t debugLevel = 2;

void debug(const char *msg) {
    if (debugLevel >= 2) {
        safePrint("[DEBUG] ", msg);
    }
}

void debugf(const char *fmt, ...) {
    if (debugLevel < 2) return;
    constexpr size_t BUF_SZ = 256;
    char buf[BUF_SZ];
    va_list ap;
    va_start(ap, fmt);
    vformat(buf, BUF_SZ, fmt, ap);
    va_end(ap);
    debug(buf);
}

} // namespace Logger