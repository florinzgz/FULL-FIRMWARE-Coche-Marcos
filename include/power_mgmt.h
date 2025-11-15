#pragma once
#include <Arduino.h>

/**
 * @file power_mgmt.h
 * @brief Sistema de gestión de alimentación con módulo 4 relés SRD-05VDC-SL-C
 * 
 * ARQUITECTURA:
 * - Llave física corta/da paso a 12V principal
 * - Buck 12V→5V alimenta ESP32-S3 y módulo de relés
 * - Relé 1: Power Hold (mantiene buck activo tras apagar llave)
 * - Relé 2: 12V Auxiliares (sensores + encoder)
 * - Relé 3: 24V Potencia (motores tracción)
 * - Relé 4: Opcional (seguridad/interlock)
 * 
 * SECUENCIA ARRANQUE:
 * 1. Llave ON → Buck 5V enciende → ESP32 boot
 * 2. ESP32 activa Relé 1 (power hold) inmediatamente
 * 3. ESP32 activa Relé 2 (12V auxiliares) para alimentar motor dirección
 * 4. Busca centro volante con motor dirección RS390 12V (requiere 12V activo)
 * 5. Verifica sensores ruedas → Activa Relé 3 (24V motores tracción)
 * 
 * SECUENCIA APAGADO:
 * 1. Detecta llave OFF (GPIO 45 pull-up a GND)
 * 2. Mantiene Relé 1 activo (power hold)
 * 3. Reproduce audio apagado con DFPlayer
 * 4. Desactiva Relé 3 (24V motores)
 * 5. Desactiva Relé 2 (12V auxiliares)
 * 6. Delay 5 segundos → Desactiva Relé 1 → Buck apaga
 */

namespace PowerMgmt {

// Estados del sistema de alimentación
enum class PowerState {
    OFF,              // Sistema apagado
    POWER_HOLD,       // Power hold activo, esperando centrado
    CENTERING,        // Buscando centro volante
    AUX_POWER,        // 12V auxiliares activos
    FULL_POWER,       // Sistema completo encendido
    SHUTDOWN_START,   // Inicio secuencia apagado
    SHUTDOWN_AUDIO,   // Reproduciendo audio apagado
    SHUTDOWN_MOTORS,  // Cortando 24V motores
    SHUTDOWN_AUX,     // Cortando 12V auxiliares
    SHUTDOWN_FINAL    // Liberando power hold
};

/**
 * @brief Inicializa el sistema de gestión de alimentación
 * 
 * Configura GPIOs de relés y detección de llave.
 * Activa power hold inmediatamente para mantener buck activo.
 */
void init();

/**
 * @brief Actualiza el sistema de alimentación (llamar en loop)
 * 
 * Gestiona transiciones de estado, detección de llave OFF,
 * y secuencia de apagado temporizado no bloqueante.
 */
void update();

/**
 * @brief Activa relé de 12V auxiliares (sensores/encoder)
 * 
 * Nota: Este relé se activa automáticamente en init() para permitir
 * que el motor de dirección funcione durante la búsqueda de centro.
 * Esta función está disponible por si se necesita reactivar manualmente.
 */
void enableAuxPower();

/**
 * @brief Activa relé de 24V motores
 * 
 * Solo se activa tras verificar sensores OK.
 */
void enableMotorPower();

/**
 * @brief Desactiva relé de 24V motores
 */
void disableMotorPower();

/**
 * @brief Desactiva relé de 12V auxiliares
 */
void disableAuxPower();

/**
 * @brief Inicia secuencia de apagado controlado
 * 
 * Mantiene power hold activo durante 5 segundos mientras:
 * 1. Reproduce audio de apagado
 * 2. Corta alimentación de motores
 * 3. Corta alimentación de sensores
 * 4. Libera power hold → Buck se apaga
 */
void startShutdown();

/**
 * @brief Verifica si la llave está en posición ON
 * 
 * @return true si llave ON (GPIO 45 = LOW por pull-up)
 * @return false si llave OFF (GPIO 45 = HIGH)
 */
bool isKeyOn();

/**
 * @brief Obtiene estado actual del sistema
 * 
 * @return PowerState Estado actual
 */
PowerState getState();

/**
 * @brief Verifica si sistema tiene alimentación completa
 * 
 * @return true si 12V auxiliares Y 24V motores activos
 */
bool isFullPowerOn();

/**
 * @brief Notifica que volante está centrado
 * 
 * Llamar desde código de inicialización tras confirmar
 * que el motor de dirección encontró el punto cero.
 * Nota: El motor dirección requiere 12V auxiliares activo,
 * el cual se habilita automáticamente en init().
 */
void notifySteeringCentered();

/**
 * @brief Notifica que sensores de ruedas están OK
 * 
 * Llamar desde código de inicialización tras verificar
 * que los 4 sensores inductivos de ruedas responden.
 */
void notifyWheelSensorsOK();

} // namespace PowerMgmt
