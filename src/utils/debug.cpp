#include "debug.h"
#include <stdarg.h>

static uint8_t gLevel = 1;

void Debug::setLevel(uint8_t lvl) { gLevel = lvl; }
uint8_t Debug::level() { return gLevel; }

static void vlogf(const char *prefix, const char *tag, const char *fmt, va_list args) {
  Serial.print(prefix);
  Serial.print(tag);
  Serial.print(": ");
  char buf[160];
  vsnprintf(buf, sizeof(buf), fmt, args);
  Serial.println(buf);
}

void Debug::info(const char *tag, const char *msg) {
  if (gLevel >= 1) {
    Serial.print("[INFO] ");
    Serial.print(tag);
    Serial.print(": ");
    Serial.println(msg);
  }
}

void Debug::infof(const char *tag, const char *fmt, ...) {
  if (gLevel >= 1) {
    va_list args;
    va_start(args, fmt);
    vlogf("[INFO] ", tag, fmt, args);
    va_end(args);
  }
}

void Debug::debug(const char *tag, const char *msg) {
  if (gLevel >= 2) {
    Serial.print("[DEBUG] ");
    Serial.print(tag);
    Serial.print(": ");
    Serial.println(msg);
  }
}

void Debug::debugf(const char *tag, const char *fmt, ...) {
  if (gLevel >= 2) {
    va_list args;
    va_start(args, fmt);
    vlogf("[DEBUG] ", tag, fmt, args);
    va_end(args);
  }
}

void Debug::warn(const char *tag, const char *msg) {
  Serial.print("[WARN] ");
  Serial.print(tag);
  Serial.print(": ");
  Serial.println(msg);
}

void Debug::error(const char *tag, const char *msg) {
  Serial.print("[ERROR] ");
  Serial.print(tag);
  Serial.print(": ");
  Serial.println(msg);
}

void Debug::hexdump(const char *tag, const uint8_t *data, size_t len) {
  Serial.print("[HEXDUMP] ");
  Serial.print(tag);
  Serial.print(": len=");
  Serial.println(len);
  for (size_t i = 0; i < len; ++i) {
    if (i % 16 == 0) Serial.printf("\n%04u: ", (unsigned)i);
    Serial.printf("%02X ", data[i]);
  }
  Serial.println();
}
