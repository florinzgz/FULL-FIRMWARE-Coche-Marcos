#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdarg>

namespace Logger {

// ---------------------------------------------------------------------------
// API base: implementaciones en logger.cpp
// ---------------------------------------------------------------------------
void init();
void info(const char *msg);
void warn(const char *msg);
void error(uint16_t code, const char *msg);

// ---------------------------------------------------------------------------
// Sobrecarga añadida: permite llamar Logger::error("mensaje")
// sin necesidad de pasar un código explícito.
// Usa un código genérico (999) por defecto.
// ---------------------------------------------------------------------------
inline void error(const char *msg) {
    error(999, msg);
}

// ---------------------------------------------------------------------------
// Wrappers con formato estilo printf
// Uso: Logger::infof("Value=%u", v);
// ---------------------------------------------------------------------------
void infof(const char *fmt, ...);
void warnf(const char *fmt, ...);

// ---------------------------------------------------------------------------
// errorf con código y sin código
// ---------------------------------------------------------------------------
void errorf(uint16_t code, const char *fmt, ...);
void errorf(const char *fmt, ...);

// ---------------------------------------------------------------------------
// Debug functions: solo emiten si nivel de log es DEBUG (2+)
// ---------------------------------------------------------------------------
void debug(const char *msg);
void debugf(const char *fmt, ...);

} // namespace Logger