#pragma once
#include <cstdio>
#include <cstdint>

/**
 * @file error_codes.h
 * @brief Centralized error codes and descriptions for the vehicle control system
 * @version 2.9.5
 * @date 2025-12-05
 * 
 * This file contains all error codes used in the firmware along with their
 * human-readable descriptions. These are displayed in the hidden menu when
 * viewing system errors.
 * 
 * Error Code Ranges:
 * - 100-199: Input controls (pedal, buttons)
 * - 200-299: Steering system (encoder, steering motor)
 * - 300-399: Current sensors (INA226)
 * - 400-499: Temperature sensors (DS18B20)
 * - 500-599: Wheel speed sensors
 * - 600-699: Relays and power system
 * - 700-799: Audio system (DFPlayer, alerts)
 * - 800-899: Traction system
 * - 900-999: Storage system (EEPROM)
 */

namespace ErrorCodes {

// ============================================================================
// ERROR CODE DEFINITIONS
// ============================================================================

// --- Input Controls (100-199) ---
constexpr uint16_t PEDAL_ERROR = 100;

// --- Steering System (200-299) ---
constexpr uint16_t STEERING_INIT_FAIL = 200;
constexpr uint16_t ENCODER_PINS_NOT_ASSIGNED = 201;
constexpr uint16_t ENCODER_NOT_CENTERED = 210;
constexpr uint16_t ENCODER_CENTER_FAIL = 211;
constexpr uint16_t ENCODER_TICKS_INVALID = 212;
constexpr uint16_t ENCODER_Z_TIMEOUT = 213;
constexpr uint16_t STEERING_PCA9685_NO_RESPONSE = 250;
constexpr uint16_t STEERING_OVERCURRENT = 251;
constexpr uint16_t STEERING_PWM_CHANNEL_INVALID = 252;

// --- Current Sensors (300-399) ---
constexpr uint16_t CURRENT_SENSOR_FL_FAIL = 300;
constexpr uint16_t CURRENT_SENSOR_FR_FAIL = 301;
constexpr uint16_t CURRENT_SENSOR_RL_FAIL = 302;
constexpr uint16_t CURRENT_SENSOR_RR_FAIL = 303;
constexpr uint16_t CURRENT_SENSOR_FL_CONFIG = 310;
constexpr uint16_t CURRENT_SENSOR_FR_CONFIG = 311;
constexpr uint16_t CURRENT_SENSOR_RL_CONFIG = 312;
constexpr uint16_t CURRENT_SENSOR_RR_CONFIG = 313;
constexpr uint16_t CURRENT_SENSOR_FL_VOLTAGE = 320;
constexpr uint16_t CURRENT_SENSOR_FR_VOLTAGE = 321;
constexpr uint16_t CURRENT_SENSOR_RL_VOLTAGE = 322;
constexpr uint16_t CURRENT_SENSOR_RR_VOLTAGE = 323;
constexpr uint16_t CURRENT_SENSOR_FL_CURRENT = 330;
constexpr uint16_t CURRENT_SENSOR_FR_CURRENT = 331;
constexpr uint16_t CURRENT_SENSOR_RL_CURRENT = 332;
constexpr uint16_t CURRENT_SENSOR_RR_CURRENT = 333;
constexpr uint16_t CURRENT_SENSOR_FL_POWER = 340;
constexpr uint16_t CURRENT_SENSOR_FR_POWER = 341;
constexpr uint16_t CURRENT_SENSOR_RL_POWER = 342;
constexpr uint16_t CURRENT_SENSOR_RR_POWER = 343;
constexpr uint16_t CURRENT_SENSORS_INIT_FAIL = 399;

// --- Temperature Sensors (400-499) ---
constexpr uint16_t TEMP_SENSOR_FL_NOT_FOUND = 400;
constexpr uint16_t TEMP_SENSOR_FR_NOT_FOUND = 401;
constexpr uint16_t TEMP_SENSOR_RL_NOT_FOUND = 402;
constexpr uint16_t TEMP_SENSOR_RR_NOT_FOUND = 403;
constexpr uint16_t TEMP_CONVERSION_TIMEOUT = 450;

// --- Wheel Speed Sensors (500-599) ---
constexpr uint16_t WHEEL_SENSOR_FL_NO_PULSES = 500;
constexpr uint16_t WHEEL_SENSOR_FR_NO_PULSES = 501;
constexpr uint16_t WHEEL_SENSOR_RL_NO_PULSES = 502;
constexpr uint16_t WHEEL_SENSOR_RR_NO_PULSES = 503;

// --- Relay and Power System (600-699) ---
constexpr uint16_t RELAY_SYSTEM_FAIL = 600;
constexpr uint16_t RELAY_SHUTDOWN_SEQUENCE_FAIL = 601;
constexpr uint16_t RELAY_STARTUP_SEQUENCE_FAIL = 602;
constexpr uint16_t RELAY_MAIN_FAIL = 603;
constexpr uint16_t RELAY_TRAC_FAIL = 604;
constexpr uint16_t RELAY_DIR_FAIL = 605;
constexpr uint16_t RELAY_SPARE_FAIL = 606;
constexpr uint16_t RELAY_STATE_TIMEOUT = 607;
constexpr uint16_t RELAY_STATE_INCONSISTENT = 608;
constexpr uint16_t RELAY_ERROR_DETECTION_FAIL = 650;
constexpr uint16_t RELAY_UNSPECIFIED_ERROR = 699;

// --- Audio System (700-799) ---
constexpr uint16_t DFPLAYER_INIT_FAIL = 700;
constexpr uint16_t DFPLAYER_COMM_ERROR = 701;
constexpr uint16_t DFPLAYER_ERROR_BASE = 702;  // 702 + DFPlayer error code
constexpr uint16_t ALERT_NOT_INITIALIZED = 720;
constexpr uint16_t ALERT_INVALID_TRACK = 721;
constexpr uint16_t ALERT_QUEUE_FULL = 722;
constexpr uint16_t AUDIO_QUEUE_INVALID_TRACK = 730;
constexpr uint16_t AUDIO_QUEUE_FULL = 731;
constexpr uint16_t AUDIO_DFPLAYER_NOT_READY = 732;
constexpr uint16_t BUTTONS_ERROR = 740;

// --- Traction System (800-899) ---
constexpr uint16_t TRACTION_DISTRIBUTION_ANOMALY = 800;
constexpr uint16_t TRACTION_DEMAND_INVALID = 801;
constexpr uint16_t TRACTION_ASYMMETRY_EXTREME = 802;
constexpr uint16_t TRACTION_MOTOR_FL_OVERCURRENT = 810;
constexpr uint16_t TRACTION_MOTOR_FR_OVERCURRENT = 811;
constexpr uint16_t TRACTION_MOTOR_RL_OVERCURRENT = 812;
constexpr uint16_t TRACTION_MOTOR_RR_OVERCURRENT = 813;
constexpr uint16_t TRACTION_MOTOR_FL_PWM_INVALID = 820;
constexpr uint16_t TRACTION_MOTOR_FR_PWM_INVALID = 821;
constexpr uint16_t TRACTION_MOTOR_RL_PWM_INVALID = 822;
constexpr uint16_t TRACTION_MOTOR_RR_PWM_INVALID = 823;
constexpr uint16_t TRACTION_PCA9685_FRONT_FAIL = 830;
constexpr uint16_t TRACTION_PCA9685_REAR_FAIL = 831;
constexpr uint16_t TRACTION_MCP23017_FAIL = 832;
constexpr uint16_t TRACTION_MCP23017_SHARED_FAIL = 833;

// --- Storage System (900-999) ---
constexpr uint16_t STORAGE_OPEN_FAIL = 970;
constexpr uint16_t STORAGE_AUTO_RESTORE = 975;
constexpr uint16_t STORAGE_WRITE_MAGIC_FAIL = 980;
constexpr uint16_t STORAGE_WRITE_CONFIG_FAIL = 981;
constexpr uint16_t STORAGE_FACTORY_RESET = 985;  // Informational, not an error

// ============================================================================
// ERROR DESCRIPTION FUNCTION
// ============================================================================

/**
 * @brief Get human-readable description for an error code
 * @param code Error code (100-999)
 * @return Pointer to constant string with error description
 * 
 * Returns a brief description of the error that can be displayed in the
 * hidden menu. If the code is not recognized, returns "Error desconocido".
 */
inline const char* getErrorDescription(uint16_t code) {
    // Input Controls (100-199)
    if (code == PEDAL_ERROR) return "Fallo sensor pedal";
    
    // Steering System (200-299)
    if (code == STEERING_INIT_FAIL) return "Encoder no responde";
    if (code == ENCODER_PINS_NOT_ASSIGNED) return "Pines encoder no asignados";
    if (code == ENCODER_NOT_CENTERED) return "Encoder sin centrado";
    if (code == ENCODER_CENTER_FAIL) return "Fallo centrado por Z";
    if (code == ENCODER_TICKS_INVALID) return "Ticks/vuelta invalido";
    if (code == ENCODER_Z_TIMEOUT) return "Timeout señal Z";
    if (code == STEERING_PCA9685_NO_RESPONSE) return "PCA9685 direccion no responde";
    if (code == STEERING_OVERCURRENT) return "Sobrecorriente motor direccion";
    if (code == STEERING_PWM_CHANNEL_INVALID) return "Canal PWM invalido";
    
    // Current Sensors (300-399)
    if (code >= 300 && code <= 303) {
        static const char* motorNames[] = {"FL", "FR", "RL", "RR"};
        thread_local char buf300[48];  // Increased buffer size for safety margin
        snprintf(buf300, sizeof(buf300), "INA226 %s fallo persistente", motorNames[code - 300]);
        return buf300;
    }
    if (code >= 310 && code <= 313) {
        static const char* motorNames[] = {"FL", "FR", "RL", "RR"};
        thread_local char buf310[48];  // Increased buffer size for safety margin
        snprintf(buf310, sizeof(buf310), "INA226 %s config error", motorNames[code - 310]);
        return buf310;
    }
    if (code >= 320 && code <= 323) {
        static const char* motorNames[] = {"FL", "FR", "RL", "RR"};
        thread_local char buf320[48];  // Increased buffer size for safety margin
        snprintf(buf320, sizeof(buf320), "INA226 %s voltaje error", motorNames[code - 320]);
        return buf320;
    }
    if (code >= 330 && code <= 333) {
        static const char* motorNames[] = {"FL", "FR", "RL", "RR"};
        thread_local char buf330[48];  // Increased buffer size for safety margin
        snprintf(buf330, sizeof(buf330), "INA226 %s corriente error", motorNames[code - 330]);
        return buf330;
    }
    if (code >= 340 && code <= 343) {
        static const char* motorNames[] = {"FL", "FR", "RL", "RR"};
        thread_local char buf340[48];  // Increased buffer size for safety margin
        snprintf(buf340, sizeof(buf340), "INA226 %s potencia error", motorNames[code - 340]);
        return buf340;
    }
    if (code == CURRENT_SENSORS_INIT_FAIL) return "Init sensores corriente fallo";
    
    // Temperature Sensors (400-499)
    if (code >= 400 && code <= 403) {
        static const char* motorNames[] = {"FL", "FR", "RL", "RR"};
        thread_local char buf400[48];  // Increased buffer size for safety margin
        snprintf(buf400, sizeof(buf400), "DS18B20 %s no encontrado", motorNames[code - 400]);
        return buf400;
    }
    if (code == TEMP_CONVERSION_TIMEOUT) return "Timeout conversion temperatura";
    
    // Wheel Sensors (500-599)
    if (code >= 500 && code <= 503) {
        static const char* wheelNames[] = {"FL", "FR", "RL", "RR"};
        thread_local char buf500[48];  // Increased buffer size for safety margin
        snprintf(buf500, sizeof(buf500), "Sensor rueda %s sin pulsos", wheelNames[code - 500]);
        return buf500;
    }
    
    // Relay System (600-699)
    if (code == RELAY_SYSTEM_FAIL) return "Fallo sistema reles";
    if (code == RELAY_SHUTDOWN_SEQUENCE_FAIL) return "Secuencia apagado fallo";
    if (code == RELAY_STARTUP_SEQUENCE_FAIL) return "Secuencia encendido fallo";
    if (code == RELAY_MAIN_FAIL) return "Rele MAIN fallo";
    if (code == RELAY_TRAC_FAIL) return "Rele TRAC fallo";
    if (code == RELAY_DIR_FAIL) return "Rele DIR fallo";
    if (code == RELAY_SPARE_FAIL) return "Rele SPARE fallo";
    if (code == RELAY_STATE_TIMEOUT) return "Timeout estado rele";
    if (code == RELAY_STATE_INCONSISTENT) return "Estado rele inconsistente";
    if (code == RELAY_ERROR_DETECTION_FAIL) return "Deteccion error rele fallo";
    if (code == RELAY_UNSPECIFIED_ERROR) return "Error rele no especificado";
    
    // Audio System (700-799)
    if (code == DFPLAYER_INIT_FAIL) return "DFPlayer init fallo";
    if (code == DFPLAYER_COMM_ERROR) return "DFPlayer comm error";
    if (code >= DFPLAYER_ERROR_BASE && code < 720) {
        thread_local char buf702[48];  // Increased buffer size for safety margin
        snprintf(buf702, sizeof(buf702), "DFPlayer error %d", code - DFPLAYER_ERROR_BASE);
        return buf702;
    }
    if (code == ALERT_NOT_INITIALIZED) return "Alertas sin inicializar";
    if (code == ALERT_INVALID_TRACK) return "Track alerta invalido";
    if (code == ALERT_QUEUE_FULL) return "Cola alertas llena";
    if (code == AUDIO_QUEUE_INVALID_TRACK) return "Track cola invalido";
    if (code == AUDIO_QUEUE_FULL) return "Cola audio llena";
    if (code == AUDIO_DFPLAYER_NOT_READY) return "DFPlayer no listo";
    if (code == BUTTONS_ERROR) return "Error botones";
    
    // Traction System (800-899)
    if (code == TRACTION_DISTRIBUTION_ANOMALY) return "Reparto traccion anomalo";
    if (code == TRACTION_DEMAND_INVALID) return "Demanda traccion invalida";
    if (code == TRACTION_ASYMMETRY_EXTREME) return "Asimetria extrema";
    if (code >= 810 && code <= 813) {
        static const char* motorNames[] = {"FL", "FR", "RL", "RR"};
        thread_local char buf810[48];  // Increased buffer size for safety margin
        snprintf(buf810, sizeof(buf810), "Motor %s sobrecorriente", motorNames[code - 810]);
        return buf810;
    }
    if (code >= 820 && code <= 823) {
        static const char* motorNames[] = {"FL", "FR", "RL", "RR"};
        thread_local char buf820[48];  // Increased buffer size for safety margin
        snprintf(buf820, sizeof(buf820), "Motor %s PWM invalido", motorNames[code - 820]);
        return buf820;
    }
    if (code == TRACTION_PCA9685_FRONT_FAIL) return "PCA9685 Front (0x40) fallo";
    if (code == TRACTION_PCA9685_REAR_FAIL) return "PCA9685 Rear (0x41) fallo";
    if (code == TRACTION_MCP23017_FAIL) return "MCP23017 (0x20) fallo";
    if (code == TRACTION_MCP23017_SHARED_FAIL) return "MCP23017 shared init fallo";
    
    // Storage System (900-999)
    if (code == STORAGE_OPEN_FAIL) return "Apertura storage fallo";
    if (code == STORAGE_AUTO_RESTORE) return "Restauracion automatica";
    if (code == STORAGE_WRITE_MAGIC_FAIL) return "Escritura magic fallo";
    if (code == STORAGE_WRITE_CONFIG_FAIL) return "Escritura config fallo";
    if (code == STORAGE_FACTORY_RESET) return "Reset fabrica (info)";
    
    // Unknown error
    return "Error desconocido";
}

/**
 * @brief Get longer error description for documentation or detailed display
 * @param code Error code (100-999)
 * @return Pointer to constant string with detailed error description
 */
inline const char* getErrorDetailedDescription(uint16_t code) {
    // This could be expanded in the future for more detailed error descriptions
    // For now, return the same as getErrorDescription
    return getErrorDescription(code);
}

} // namespace ErrorCodes
