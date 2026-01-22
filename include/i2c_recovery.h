#pragma once
#include <Arduino.h>

/**
 * @file i2c_recovery.h
 * @brief Sistema de recuperación I²C con timeout, retry y bus recovery
 *
 * MECANISMOS DE ROBUSTEZ:
 * 1. Bus Recovery: Genera pulsos SCL para liberar líneas atascadas
 * 2. Timeout: Límite de tiempo para transacciones I²C
 * 3. Retry exponencial: 1, 2, 4, 8s (máx 30s)
 * 4. Skip sensor: Marca dispositivo como "offline" si falla repetidamente
 * 5. Re-init progresivo: Sensor → Canal multiplexor → Bus completo
 *
 * USO CON TCA9548A:
 * - Llamar I2CRecovery::init() una vez
 * - Usar I2CRecovery::tcaSelectSafe() en lugar de Wire.beginTransmission()
 * - Usar I2CRecovery::readBytesWithRetry() para lecturas robustas
 */

namespace I2CRecovery {

// Configuración de retry
constexpr uint8_t MAX_RETRIES = 3;         // Número máximo de reintentos
constexpr uint32_t MAX_BACKOFF_MS = 30000; // Backoff máximo: 30 segundos
constexpr uint32_t SKIP_SENSOR_AFTER_MS = 60000; // Marcar offline tras 1 min

// Estados de dispositivos I²C
struct DeviceState {
  bool online;                 // true si dispositivo responde
  uint8_t consecutiveFailures; // Fallos consecutivos
  uint32_t lastSuccessMs;      // Último éxito (millis)
  uint32_t lastAttemptMs;      // Último intento (millis)
  uint32_t nextRetryMs;        // Próximo reintento (backoff)
};

/**
 * @brief Inicializa el sistema de recuperación I²C
 *
 * Configura pines SDA/SCL para bus recovery y estados de dispositivos.
 */
void init();
bool isInitialized();

/**
 * @brief Selecciona canal TCA9548A con timeout y recovery
 *
 * @param channel Canal TCA9548A (0-7)
 * @param tcaAddr Dirección I²C del TCA9548A (default 0x70)
 * @return true si selección exitosa, false si timeout/fallo
 */
bool tcaSelectSafe(uint8_t channel, uint8_t tcaAddr = 0x70);

/**
 * @brief Lee bytes de dispositivo I²C con retry exponencial
 *
 * @param deviceAddr Dirección I²C del dispositivo
 * @param regAddr Dirección del registro a leer
 * @param buffer Buffer para almacenar datos leídos
 * @param length Cantidad de bytes a leer
 * @param deviceId ID único del dispositivo (0-15 para tracking)
 * @return true si lectura exitosa, false si fallo tras retries
 */
bool readBytesWithRetry(uint8_t deviceAddr, uint8_t regAddr, uint8_t *buffer,
                        uint8_t length, uint8_t deviceId);

/**
 * @brief Escribe bytes a dispositivo I²C con retry
 *
 * @param deviceAddr Dirección I²C del dispositivo
 * @param regAddr Dirección del registro a escribir
 * @param data Datos a escribir
 * @param length Cantidad de bytes a escribir
 * @param deviceId ID único del dispositivo (0-15 para tracking)
 * @return true si escritura exitosa, false si fallo
 */
bool writeBytesWithRetry(uint8_t deviceAddr, uint8_t regAddr,
                         const uint8_t *data, uint8_t length, uint8_t deviceId);

/**
 * @brief Recuperación activa del bus I²C
 *
 * Genera hasta 9 pulsos SCL para liberar líneas SDA/SCL atascadas.
 * Llamar cuando I²C no responde o tras timeout.
 *
 * @return true si bus recuperado (SDA/SCL HIGH), false si fallo
 */
bool recoverBus();

/**
 * @brief Re-inicializa un sensor específico
 *
 * Intenta restablecer comunicación con sensor:
 * 1. Re-init del sensor (soft reset si disponible)
 * 2. Re-init del canal TCA9548A
 * 3. Si falla, intenta bus recovery completo
 *
 * @param channel Canal TCA9548A (0-7)
 * @param deviceAddr Dirección I²C del dispositivo
 * @param deviceId ID único del dispositivo
 * @return true si re-init exitoso
 */
bool reinitSensor(uint8_t channel, uint8_t deviceAddr, uint8_t deviceId);

/**
 * @brief Verifica si un dispositivo está online
 *
 * @param deviceId ID único del dispositivo (0-15)
 * @return true si dispositivo responde
 */
bool isDeviceOnline(uint8_t deviceId);

/**
 * @brief Obtiene estado de un dispositivo
 *
 * @param deviceId ID único del dispositivo (0-15)
 * @return Estado del dispositivo
 */
const DeviceState &getDeviceState(uint8_t deviceId);

/**
 * @brief Resetea estadísticas de un dispositivo
 *
 * @param deviceId ID único del dispositivo (0-15)
 */
void resetDeviceState(uint8_t deviceId);

/**
 * @brief Verifica salud general del bus I²C
 *
 * @return true si al menos un dispositivo responde
 */
bool isBusHealthy();

/**
 * @brief Marca un dispositivo como offline manualmente
 *
 * @param deviceId ID único del dispositivo (0-15)
 */
void markDeviceOffline(uint8_t deviceId);

} // namespace I2CRecovery
