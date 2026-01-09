#pragma once
#include <Arduino.h>

namespace Debug {
// Nivel global de log por Serial (0=silencio, 1=INFO, 2=DEBUG)
void setLevel(uint8_t lvl);
uint8_t level();

// Trazas
void info(const char *tag, const char *msg);
void infof(const char *tag, const char *fmt, ...);

void debug(const char *tag, const char *msg);
void debugf(const char *tag, const char *fmt, ...);

void warn(const char *tag, const char *msg);
void error(const char *tag, const char *msg);

// Hex dump
void hexdump(const char *tag, const uint8_t *data, size_t len);
} // namespace Debug