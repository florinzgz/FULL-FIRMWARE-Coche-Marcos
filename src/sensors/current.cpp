#include "current.h"
#include <Wire.h>
#include <INA226.h>
#include "logger.h"
#include "storage.h"
#include "settings.h"
#include "system.h"   // para logError()
#include "i2c_recovery.h"  // Sistema de recuperación I²C
#include "pins.h"     // 🔒 Para PIN_I2C_SDA y PIN_I2C_SCL

#ifndef I2C_FREQUENCY
#error "I2C_FREQUENCY must be defined in build flags for I2C sensor operation"
#endif

// 🔒 CORRECCIÓN CRÍTICA: Mutex para proteger acceso I2C concurrente
static SemaphoreHandle_t i2cMutex = nullptr;

#define TCA_ADDR 0x70   // Dirección I2C del TCA9548A

extern Storage::Config cfg;

// INA226 sensors - will be initialized in initCurrent()
// Using pointers to avoid constructor issues with array initialization
// 6 sensores: 0-3 motores tracción (50A), 4 batería (100A), 5 motor dirección (50A)
static INA226* ina[Sensors::NUM_CURRENTS] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
static bool sensorOk[Sensors::NUM_CURRENTS];
static float lastCurrent[Sensors::NUM_CURRENTS];
static float lastVoltage[Sensors::NUM_CURRENTS];
static float lastPower[Sensors::NUM_CURRENTS];
static float lastShunt[Sensors::NUM_CURRENTS];

// Configuración shunts CG FL-2C externos
// Canal 4 (batería): 100A 0.5 Class, 75mV → R_shunt = 75mV/100A = 0.00075Ω
// Canales 0-3,5 (motores): 50A 0.5 Class, 75mV → R_shunt = 75mV/50A = 0.0015Ω
static constexpr float SHUNT_BATTERY_OHM = 0.00075f;  // 75mV @ 100A
static constexpr float SHUNT_MOTOR_OHM = 0.0015f;     // 75mV @ 50A
static constexpr float MAX_CURRENT_BATTERY = 100.0f;  // 100A
static constexpr float MAX_CURRENT_MOTOR = 50.0f;     // 50A

static uint32_t lastUpdateMs = 0;
// 🔒 CORRECCIÓN MEDIA: Constante para frecuencia de actualización
static constexpr uint32_t CURRENT_UPDATE_INTERVAL_MS = 50;  // 20 Hz

// Flag de inicialización global
static bool initialized = false;

// 🔒 CORRECCIÓN MEDIA: tcaSelect mejorado con validación y retry
// Selecciona canal del TCA9548A con recuperación automática y mutex
static bool tcaSelect(uint8_t channel) {
    if(channel > 7) {
        Logger::errorf("Current: canal TCA inválido %d", channel);
        return false;
    }
    
    // 🔒 CORRECCIÓN CRÍTICA: Proteger acceso I2C con mutex
    if (i2cMutex != nullptr && xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Mutex adquirido - acceso I2C protegido
        if (!I2CRecovery::tcaSelectSafe(channel, TCA_ADDR)) {
            Logger::errorf("TCA select fail ch %d - recovery attempt", channel);
            I2CRecovery::recoverBus();  // Intentar recuperar bus
            
            // 🔒 CORRECCIÓN: Retry después de recovery
            if (!I2CRecovery::tcaSelectSafe(channel, TCA_ADDR)) {
                Logger::errorf("TCA select fail ch %d después de recovery", channel);
                xSemaphoreGive(i2cMutex);  // Liberar mutex antes de salir
                return false;
            }
        }
        xSemaphoreGive(i2cMutex);  // Liberar mutex en éxito
        return true;
    } else {
        // 🔒 v2.4.2: NOTA - No hay race condition aquí.
        // Cuando xSemaphoreTake retorna pdFALSE (timeout), el mutex NO fue adquirido,
        // por lo tanto NO hay necesidad de llamar xSemaphoreGive.
        // El mutex sigue disponible para otros tasks.
        Logger::error("Current: mutex I2C timeout en tcaSelect");
        return false;
    }
}

void Sensors::initCurrent() {
    // DEPENDENCY: I2C bus MUST be initialized by I2CRecovery::init() before calling this function.
    // 🔒 CORRECCIÓN CRÍTICA: Crear mutex I2C si no existe
    if (i2cMutex == nullptr) {
        i2cMutex = xSemaphoreCreateMutex();
        if (i2cMutex == nullptr) {
            Logger::error("Current: No se pudo crear mutex I2C");
            System::logError(399);
            return;
        }
    }
    
    // Timeout para inicialización completa
    const uint32_t INIT_TIMEOUT_MS = 5000;
    uint32_t startTime = millis();
    
    bool allOk = true;

    for(int i=0; i<NUM_CURRENTS; i++) {
        // Verificar timeout global
        if (millis() - startTime > INIT_TIMEOUT_MS) {
            Logger::warn("INA226 init timeout - continuando con sensores parciales");
            break;
        }
        
        // 🔒 CRITICAL FIX: Prevent memory leak on repeated init
        // Delete existing INA226 objects before creating new ones
        if (ina[i] != nullptr) {
            delete ina[i];
            ina[i] = nullptr;
        }
        
        // Create INA226 object for this channel
        // Using nothrow to explicitly get nullptr on allocation failure
        ina[i] = new(std::nothrow) INA226(0x40);  // Address will be selected via TCA9548A multiplexer
        
        // 🔒 CRITICAL FIX: Check if allocation succeeded
        if (ina[i] == nullptr) {
            Logger::errorf("INA226 allocation failed ch %d", i);
            sensorOk[i] = false;
            allOk = false;
            continue;
        }
        
        tcaSelect(i);
        if(!ina[i]->begin()) {
            Logger::warnf("INA226 ch %d falló - continuando", i);
            sensorOk[i] = false;
            allOk = false;
            // NO llamar a System::logError() - solo warning
            // El sistema puede continuar sin este sensor específico
        } else {
            // Configurar shunt según canal:
            // Canal 4 = batería (100A), resto = motores (50A)
            float shuntOhm = (i == 4) ? SHUNT_BATTERY_OHM : SHUNT_MOTOR_OHM;
            float maxCurrent = (i == 4) ? MAX_CURRENT_BATTERY : MAX_CURRENT_MOTOR;
            
            // 🔒 CORRECCIÓN CRÍTICA: Usar API correcta de INA226 v0.6.x
            // Calibrar para shunt CG FL-2C según canal
            ina[i]->setAverage(INA226_1_SAMPLE);
            ina[i]->setBusVoltageConversionTime(INA226_1100_us);
            ina[i]->setShuntVoltageConversionTime(INA226_1100_us);
            int err = ina[i]->setMaxCurrentShunt(maxCurrent, shuntOhm);
            
            if (err != INA226_ERR_NONE) {
                Logger::errorf("INA226 ch%d calibration error: %d", i, err);
                sensorOk[i] = false;
                continue;
            }
            
            sensorOk[i] = true;
            Logger::infof("INA226 OK ch%d (%.4fΩ, %.0fA)", i, shuntOhm, maxCurrent);
        }
        lastCurrent[i] = 0.0f;
        lastVoltage[i] = 0.0f;
        lastPower[i]   = 0.0f;
        lastShunt[i]   = 0.0f;
    }
    // Desactivar todos los canales
    Wire.beginTransmission(TCA_ADDR);
    Wire.write(0x00);
    Wire.endTransmission();

    initialized = allOk;
    
    if (!allOk) {
        Logger::warn("INA226 init: algunos sensores no disponibles - modo degradado");
    }
}

void Sensors::updateCurrent() {
    uint32_t now = millis();
    // 🔒 CORRECCIÓN MEDIA: Usar constante en lugar de hardcode
    if(now - lastUpdateMs < CURRENT_UPDATE_INTERVAL_MS) return; // 20 Hz
    lastUpdateMs = now;

    if(!cfg.currentSensorsEnabled) {
        for(int i=0; i<NUM_CURRENTS; i++) {
            lastCurrent[i] = 0.0f;
            lastVoltage[i] = 0.0f;
            lastPower[i]   = 0.0f;
            lastShunt[i]   = 0.0f;
            sensorOk[i]    = false;
        }
        return;
    }

    for(int i=0; i<NUM_CURRENTS; i++) {
        // Check device state before reading
        const I2CRecovery::DeviceState& state = I2CRecovery::getDeviceState(i);
        
        if (!state.online) {
            // Sensor marcado offline, saltar
            sensorOk[i] = false;
            lastCurrent[i] = 0.0f;
            lastVoltage[i] = 0.0f;
            lastPower[i]   = 0.0f;
            lastShunt[i]   = 0.0f;
            continue;
        }
        
        if(!sensorOk[i] || !ina[i]) {
            // Intentar recuperación si hay tiempo desde último intento
            if (millis() >= state.nextRetryMs) {
                Logger::infof("INA226 ch %d attempting recovery", i);
                if (I2CRecovery::reinitSensor(i, 0x40, i) && ina[i]->begin()) {
                    sensorOk[i] = true;
                    Logger::infof("INA226 ch %d recovered!", i);
                }
            }
            continue; // Saltar si aún no está ok
        }

        // 🔒 CORRECCIÓN MEDIA: Validar éxito de tcaSelect antes de continuar
        if (!tcaSelect(i)) {
            Logger::errorf("Cannot select TCA channel %d, skipping", i);
            sensorOk[i] = false;
            I2CRecovery::markDeviceOffline(i);
            continue;
        }

        float c = ina[i]->getCurrent();
        float v = ina[i]->getBusVoltage();
        float p = ina[i]->getPower();
        float s = ina[i]->getShuntVoltage();

        // Validación y fallback con recuperación
        bool hasError = false;
        if(!isfinite(c)) { 
            c = 0.0f; 
            System::logError(310+i); 
            Logger::errorf("INA226 ch %d: corriente inválida", i);
            hasError = true;
        }
        if(!isfinite(v)) { 
            v = 0.0f; 
            System::logError(320+i); 
            Logger::errorf("INA226 ch %d: voltaje inválido", i);
            hasError = true;
        }
        if(!isfinite(p)) { 
            p = 0.0f; 
            System::logError(330+i); 
            Logger::errorf("INA226 ch %d: potencia inválida", i);
            hasError = true;
        }
        if(!isfinite(s)) { 
            s = 0.0f; 
            System::logError(340+i); 
            Logger::errorf("INA226 ch %d: shunt inválido", i);
            hasError = true;
        }
        
        // Si hubo errores, marcar sensor como no ok
        if (hasError) {
            sensorOk[i] = false;
            continue;
        }

        // Clamps
        c = constrain(c, -CURR_MAX_BATT, CURR_MAX_BATT);
        v = constrain(v, 0.0f, 80.0f);
        p = constrain(p, -4000.0f, 4000.0f); // ejemplo
        s = constrain(s, -100.0f, 100.0f);

        // Filtro EMA para suavizar
        lastCurrent[i] = lastCurrent[i] + 0.2f * (c - lastCurrent[i]);
        lastVoltage[i] = lastVoltage[i] + 0.2f * (v - lastVoltage[i]);
        lastPower[i]   = lastPower[i]   + 0.2f * (p - lastPower[i]);
        lastShunt[i]   = lastShunt[i]   + 0.2f * (s - lastShunt[i]);
    }
}

float Sensors::getCurrent(int channel) {
    // 🔒 v2.4.1: Validación de rango completa
    if(channel >= 0 && channel < NUM_CURRENTS) return lastCurrent[channel];
    return 0.0f;
}

float Sensors::getVoltage(int channel) {
    // 🔒 v2.4.1: Validación de rango completa
    if(channel >= 0 && channel < NUM_CURRENTS) return lastVoltage[channel];
    return 0.0f;
}

float Sensors::getPower(int channel) {
    // 🔒 v2.4.1: Validación de rango completa
    if(channel >= 0 && channel < NUM_CURRENTS) return lastPower[channel];
    return 0.0f;
}

float Sensors::getShuntVoltage(int channel) {
    // 🔒 v2.4.1: Validación de rango completa
    if(channel >= 0 && channel < NUM_CURRENTS) return lastShunt[channel];
    return 0.0f;
}

bool Sensors::isCurrentSensorOk(int channel) {
    // 🔒 v2.4.1: Validación de rango completa
    if(channel >= 0 && channel < NUM_CURRENTS) return sensorOk[channel];
    return false;
}

bool Sensors::currentInitOK() {
    return initialized;
}
