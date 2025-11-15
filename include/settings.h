#pragma once

// ======================================================
// Umbrales de temperatura (grados Celsius)
constexpr int TEMP_MAX_MOTOR   = 80;
constexpr int TEMP_WARN_MOTOR  = 65;

// ======================================================
// Umbrales de corriente (amperios)
constexpr int CURR_MAX_WHEEL   = 50;
constexpr int CURR_MAX_BATT    = 100;

// ======================================================
// Velocímetro y tacómetro (límites de visualización / seguridad)
constexpr int MAX_SPEED_KMH    = 35;   // km/h
constexpr int MAX_RPM          = 400;  // rpm

// ======================================================
// Freno regenerativo (porcentaje por defecto 0..100)
constexpr int REGEN_DEFAULT    = 30;

// ======================================================
// Comprobaciones en tiempo de compilación
static_assert(TEMP_WARN_MOTOR < TEMP_MAX_MOTOR, "TEMP_WARN_MOTOR must be less than TEMP_MAX_MOTOR");
static_assert(CURR_MAX_WHEEL > 0, "CURR_MAX_WHEEL must be > 0");
static_assert(CURR_MAX_BATT > 0, "CURR_MAX_BATT must be > 0");
static_assert(MAX_SPEED_KMH > 0, "MAX_SPEED_KMH must be > 0");
static_assert(MAX_RPM > 0, "MAX_RPM must be > 0");
static_assert(REGEN_DEFAULT >= 0 && REGEN_DEFAULT <= 100, "REGEN_DEFAULT must be 0..100");