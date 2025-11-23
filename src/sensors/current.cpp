#include "current.h"
#include <Wire.h>
#include <INA226.h>
#include "logger.h"
#include "storage.h"
#include "settings.h"
#include "system.h"   // para logError()
#include "i2c_recovery.h"  // Sistema de recuperación I²C
#include "pins.h"     // 🔒 Para PIN_I2C_SDA y PIN_I2C_SCL

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

// Flag de inicialización global
static bool initialized = false;

// Selecciona canal del TCA9548A con recuperación automática y mutex
static void tcaSelect(uint8_t channel) {
    if(channel > 7) return;
    
    // 🔒 CORRECCIÓN CRÍTICA: Proteger acceso I2C con mutex
    if (i2cMutex != nullptr && xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Usar tcaSelectSafe en lugar de Wire directo
        if (!I2CRecovery::tcaSelectSafe(channel, TCA_ADDR)) {
            Logger::errorf("TCA select fail ch %d - recovery attempt", channel);
            I2CRecovery::recoverBus();  // Intentar recuperar bus
        }
        xSemaphoreGive(i2cMutex);
    } else {
        Logger::error("Current: mutex I2C timeout en tcaSelect");
    }
}

void Sensors::initCurrent() {
    // 🔒 CORRECCIÓN CRÍTICA: Crear mutex I2C si no existe
    if (i2cMutex == nullptr) {
        i2cMutex = xSemaphoreCreateMutex();
        if (i2cMutex == nullptr) {
            Logger::error("Current: No se pudo crear mutex I2C");
            System::logError(399);
            return;
        }
    }
    
    // 🔒 CORRECCIÓN CRÍTICA: Configurar pines I2C antes de begin()
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    bool allOk = true;

    for(int i=0; i<NUM_CURRENTS; i++) {
        // Create INA226 object for this channel
        ina[i] = new INA226(0x40);  // Address will be selected via TCA9548A multiplexer
        
        tcaSelect(i);
        if(!ina[i]->begin()) {
            Logger::errorf("INA226 init fail ch %d - retrying with recovery", i);
            
            // Intentar recuperación progresiva
            bool recovered = I2CRecovery::reinitSensor(i, 0x40, i);
            if (recovered && ina[i]->begin()) {
                Logger::infof("INA226 ch %d recovered!", i);
                sensorOk[i] = true;
            } else {
                System::logError(300+i);   // registrar fallo persistente
                sensorOk[i] = false;
                allOk = false;
            }
        } else {
            // Configurar shunt según canal:
            // Canal 4 = batería (100A), resto = motores (50A)
            float shuntOhm = (i == 4) ? SHUNT_BATTERY_OHM : SHUNT_MOTOR_OHM;
            float maxCurrent = (i == 4) ? MAX_CURRENT_BATTERY : MAX_CURRENT_MOTOR;
            
            // 🔒 CORRECCIÓN CRÍTICA: Descomentar calibración INA226
            // Calibrar para shunt CG FL-2C según canal
            ina[i]->configure(INA226_AVERAGES_1, INA226_BUS_CONV_TIME_1100US, 
                             INA226_SHUNT_CONV_TIME_1100US, INA226_MODE_SHUNT_BUS_CONT);
            ina[i]->calibrate(shuntOhm, maxCurrent);
            
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
}

void Sensors::updateCurrent() {
    uint32_t now = millis();
    if(now - lastUpdateMs < 50) return; // ~20 Hz
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

        tcaSelect(i);

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
    if(channel < NUM_CURRENTS) return lastCurrent[channel];
    return 0.0f;
}

float Sensors::getVoltage(int channel) {
    if(channel < NUM_CURRENTS) return lastVoltage[channel];
    return 0.0f;
}

float Sensors::getPower(int channel) {
    if(channel < NUM_CURRENTS) return lastPower[channel];
    return 0.0f;
}

float Sensors::getShuntVoltage(int channel) {
    if(channel < NUM_CURRENTS) return lastShunt[channel];
    return 0.0f;
}

bool Sensors::isCurrentSensorOk(int channel) {
    if(channel < NUM_CURRENTS) return sensorOk[channel];
    return false;
}

bool Sensors::currentInitOK() {
    return initialized;
}