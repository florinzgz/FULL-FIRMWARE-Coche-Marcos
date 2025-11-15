#pragma once

// =======================
// Geometría del vehículo
// =======================
constexpr float WHEELBASE_M       = 0.95f;   // Distancia entre ejes (m)
constexpr float TRACK_WIDTH_M     = 0.70f;   // Ancho de vía (m)
constexpr float MAX_STEER_DEG     = 54.0f;   // Ángulo máximo de ruedas (Ackermann)

// =======================
// Dirección (volante + encoder)
// =======================
constexpr float MAX_STEER_WHEEL_DEG = 350.0f;        // Giro mecánico máximo del volante (total, grados)
constexpr int   ENCODER_PPR         = 1200;          // Pulsos por revolución del encoder (PPR)
constexpr float ENCODER_TICKS_PER_DEG = ENCODER_PPR / 360.0f; // ticks por grado (puede ser fraccional)

// =======================
// Motor de dirección (RS390 + reductora 1:50)
// =======================
constexpr int   STEER_MOTOR_RPM      = 6000;   // rpm nominal motor RS390
constexpr int   STEER_GEAR_RATIO     = 50;     // reductora 1:50
constexpr float STEER_OUTPUT_RPM     = STEER_MOTOR_RPM / static_cast<float>(STEER_GEAR_RATIO); // rpm salida
constexpr float STEER_OUTPUT_TURNS_S = STEER_OUTPUT_RPM / 60.0f; // vueltas/s salida

// =======================
// Motores de rueda (RS775 + reductora 1:75)
// =======================
constexpr int   WHEEL_MOTOR_RPM      = 15000;  // rpm nominal motor RS775
constexpr int   WHEEL_GEAR_RATIO     = 75;     // reductora 1:75
constexpr float WHEEL_OUTPUT_RPM     = WHEEL_MOTOR_RPM / static_cast<float>(WHEEL_GEAR_RATIO); // rpm salida

// =======================
// Ruedas
// =======================
constexpr float WHEEL_CIRCUM_MM      = 1100.0f; // Circunferencia real de rueda (mm)
constexpr float WHEEL_MAX_SPEED_MS   = (WHEEL_OUTPUT_RPM / 60.0f) * (WHEEL_CIRCUM_MM / 1000.0f); // m/s
constexpr float WHEEL_MAX_SPEED_KMH  = WHEEL_MAX_SPEED_MS * 3.6f; // km/h
constexpr int   PULSES_PER_REV       = 6;       // Pulsos por vuelta del sensor de rueda (inductivo)

// =======================
// Conversión y factores
// =======================
constexpr float MM_TO_KMH   = 0.0036f;       // mm/s → km/h  (3.6 / 1000 = 0.0036)
constexpr float DEG_TO_RAD  = 3.14159265358979323846f / 180.0f; // grados → radianes
constexpr float RAD_TO_DEG  = 180.0f / 3.14159265358979323846f; // radianes → grados

// =======================
// Filtros y suavizado
// =======================
constexpr float EMA_ALPHA   = 0.2f;           // Coeficiente filtro EMA por defecto

// =======================
// Validaciones en tiempo de compilación
// =======================
static_assert(ENCODER_PPR > 0, "ENCODER_PPR must be > 0");
static_assert(STEER_GEAR_RATIO > 0, "STEER_GEAR_RATIO must be > 0");
static_assert(WHEEL_GEAR_RATIO > 0, "WHEEL_GEAR_RATIO must be > 0");
static_assert(WHEEL_CIRCUM_MM > 0.0f, "WHEEL_CIRCUM_MM must be > 0");
static_assert(PULSES_PER_REV > 0, "PULSES_PER_REV must be > 0");