#pragma once

/**
 * @file operation_modes.h
 * @brief Sistema de modos de operación con tolerancia a fallos
 *
 * Implementa un sistema de degradación progresiva que permite al firmware
 * continuar operando incluso cuando fallan sensores opcionales, evitando
 * bucles de reinicio y mejorando la estabilidad del sistema.
 *
 * Modos de operación:
 * - MODE_FULL: Todos los sistemas funcionando (estado ideal)
 * - MODE_DEGRADED: Algunos sensores fallaron, continuar con funcionalidad
 * reducida
 * - MODE_SAFE: Solo funciones críticas (sin motores, solo monitoreo)
 * - MODE_STANDALONE: Solo pantalla (modo diagnóstico)
 */

enum class OperationMode {
  MODE_FULL,      // Todos los sistemas OK
  MODE_DEGRADED,  // Algunos sensores fallaron, continuar
  MODE_SAFE,      // Solo funciones críticas
  MODE_STANDALONE // Solo pantalla (diagnóstico)
};

namespace SystemMode {
/**
 * @brief Estado interno del modo de operación actual.
 * @warning No acceder directamente a esta variable desde el código de usuario.
 *          Utilizar siempre getMode() y setMode() para leer o modificar el
 * modo. La presencia y visibilidad de esta variable pueden cambiar en el
 * futuro.
 */
extern OperationMode currentMode;

/**
 * @brief Establece el modo de operación del sistema
 * @param mode Nuevo modo de operación
 */
void setMode(OperationMode mode);

/**
 * @brief Obtiene el modo de operación actual
 * @return Modo de operación actual
 */
OperationMode getMode();

/**
 * @brief Obtiene el nombre del modo actual como string
 * @return Nombre del modo (ej: "FULL", "DEGRADED", "SAFE", "STANDALONE")
 */
const char *getModeName();

/**
 * @brief Inicializa el sistema de modos
 */
void init();
} // namespace SystemMode
