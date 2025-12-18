#include "i2c_recovery.h"
#include "logger.h"
#include "pins.h"
#include "watchdog.h"
#include <Wire.h>

namespace I2CRecovery {

// Estados de hasta 16 dispositivos I²C
constexpr uint8_t MAX_DEVICES = 16;
static DeviceState devices[MAX_DEVICES];

// Pines I²C (de pins.h)
static uint8_t pinSDA = PIN_I2C_SDA;
static uint8_t pinSCL = PIN_I2C_SCL;

void init() {
    // 🔒 v2.11.1: Initialize Wire FIRST to prevent IPC stack issues
    // Wire.begin() must be called early to avoid conflicts with other init code
    Serial.println("[I2CRecovery] Initializing I2C bus...");
    Wire.begin(pinSDA, pinSCL);
    Wire.setClock(400000);  // 400 kHz as configured in platformio.ini
    Serial.println("[I2CRecovery] I2C bus initialized");
    
    // Inicializar estados
    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
        devices[i].online = true;  // Asumir online hasta que fallen
        devices[i].consecutiveFailures = 0;
        devices[i].lastSuccessMs = millis();
        devices[i].lastAttemptMs = 0;
        devices[i].nextRetryMs = 0;
    }
    
    Serial.println("[I2CRecovery] Sistema recuperación I²C inicializado");
}

bool recoverBus() {
    Serial.println("[I2CRecovery] WARNING: Iniciando bus recovery");
    
    // 1. Terminar transacción I²C actual
    Wire.end();
    // Non-blocking: use delayMicroseconds for hardware settling (< 1ms total)
    delayMicroseconds(100);
    
    // 2. Configurar pines como GPIO para generar pulsos manualmente
    pinMode(pinSDA, INPUT_PULLUP);
    pinMode(pinSCL, INPUT_PULLUP);
    delayMicroseconds(100);
    
    // 3. Verificar estado inicial
    bool sdaStuck = (digitalRead(pinSDA) == LOW);
    bool sclStuck = (digitalRead(pinSCL) == LOW);
    
    if (!sdaStuck && !sclStuck) {
        Logger::info("I2CRecovery: Bus OK - SDA/SCL HIGH, no necesita recovery");
        Wire.begin(pinSDA, pinSCL);
        return true;
    }
    
    Logger::infof("Bus stuck - SDA:%d SCL:%d", 
                   digitalRead(pinSDA), digitalRead(pinSCL));
    
    // 4. Generar hasta 9 pulsos SCL para liberar SDA
    pinMode(pinSCL, OUTPUT);
    for (int i = 0; i < 9; i++) {
        digitalWrite(pinSCL, LOW);
        delayMicroseconds(5);
        digitalWrite(pinSCL, HIGH);
        delayMicroseconds(5);
        
        // Verificar si SDA se liberó
        if (digitalRead(pinSDA) == HIGH) {
            Serial.printf("[I2CRecovery] Bus liberado tras %d pulsos SCL\n", i + 1);
            break;
        }
    }
    
    // 5. Generar condición STOP (SDA LOW→HIGH mientras SCL HIGH)
    pinMode(pinSDA, OUTPUT);
    digitalWrite(pinSDA, LOW);
    delayMicroseconds(5);
    digitalWrite(pinSCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(pinSDA, HIGH);
    delayMicroseconds(5);
    
    // 6. Verificar recovery exitoso
    pinMode(pinSDA, INPUT_PULLUP);
    pinMode(pinSCL, INPUT_PULLUP);
    delayMicroseconds(100);
    
    bool recovered = (digitalRead(pinSDA) == HIGH) && (digitalRead(pinSCL) == HIGH);
    
    // 7. Re-inicializar Wire
    Wire.begin(pinSDA, pinSCL);
    Wire.setClock(100000);  // 100 kHz (modo estándar)
    
    if (recovered) {
        Serial.println("[I2CRecovery] Bus recovery exitoso");
    } else {
        Logger::error("I2CRecovery: Bus recovery FALLIDO - Hardware problem");
    }
    
    return recovered;
}

bool tcaSelectSafe(uint8_t channel, uint8_t tcaAddr) {
    if (channel > 7) {
        Serial.printf("[I2CRecovery] ERROR: TCA channel inválido: %d\n", channel);
        return false;
    }
    
    // Timeout para beginTransmission + write + endTransmission
    constexpr uint32_t TIMEOUT_MS = 100;
    uint32_t startMs = millis();
    
    Wire.beginTransmission(tcaAddr);
    Wire.write(1 << channel);
    uint8_t result = Wire.endTransmission();
    
    uint32_t elapsed = millis() - startMs;
    
    // Feed watchdog tras operación I²C
    Watchdog::feed();
    
    if (result != 0) {
        Serial.printf("[I2CRecovery] ERROR: TCA select ch%d falló (error %d)\n", channel, result);
        
        // Intentar bus recovery
        if (recoverBus()) {
            // Retry tras recovery
            Wire.beginTransmission(tcaAddr);
            Wire.write(1 << channel);
            result = Wire.endTransmission();
            
            if (result == 0) {
                Serial.printf("[I2CRecovery] TCA select ch%d OK tras recovery\n", channel);
                return true;
            }
        }
        return false;
    }
    
    if (elapsed > 50) {
        Serial.printf("[I2CRecovery] TCA select ch%d lento (%lums)\n", channel, elapsed);
    }
    
    return true;
}

bool readBytesWithRetry(uint8_t deviceAddr, uint8_t regAddr, 
                         uint8_t* buffer, uint8_t length, uint8_t deviceId) {
    if (deviceId >= MAX_DEVICES) {
        Logger::errorf("deviceId inválido: %d", deviceId);
        return false;
    }
    
    DeviceState& dev = devices[deviceId];
    uint32_t now = millis();
    
    // Verificar backoff exponencial
    if (now < dev.nextRetryMs) {
        // Aún en período de backoff, no intentar
        return false;
    }
    
    // Verificar si dispositivo debe estar offline (>1 min sin responder)
    if ((now - dev.lastSuccessMs) > SKIP_SENSOR_AFTER_MS) {
        if (dev.online) {
            dev.online = false;
            Logger::warn("I2CRecovery: Dispositivo marcado OFFLINE (1 min sin respuesta)");
        }
        // Aún intentar (backoff permitirá retry esporádico)
    }
    
    dev.lastAttemptMs = now;
    
    // Intentar lectura con timeout
    constexpr uint32_t TIMEOUT_MS = 100;
    
    for (uint8_t retry = 0; retry <= MAX_RETRIES; retry++) {
        if (retry > 0) {
            Logger::infof("I2C retry %d/%d (dev 0x%02X)", 
                           retry, MAX_RETRIES, deviceAddr);
            // Non-blocking: use delayMicroseconds for minimal settling time
            delayMicroseconds(500);  // 0.5ms instead of 50ms
        }
        
        uint32_t startMs = millis();
        
        // Escribir dirección de registro
        Wire.beginTransmission(deviceAddr);
        Wire.write(regAddr);
        uint8_t result = Wire.endTransmission(false);  // Repeated start
        
        if (result != 0) {
            if ((millis() - startMs) > TIMEOUT_MS) {
                Serial.printf("[I2CRecovery] ERROR: I2C timeout escribiendo reg (dev 0x%02X)\n", deviceAddr);
                continue;  // Retry
            }
            continue;
        }
        
        // Leer datos
        startMs = millis();
        uint8_t received = Wire.requestFrom(deviceAddr, length);
        
        if (received != length) {
            if ((millis() - startMs) > TIMEOUT_MS) {
                Serial.printf("[I2CRecovery] ERROR: I2C timeout leyendo (dev 0x%02X)\n", deviceAddr);
            }
            continue;  // Retry
        }
        
        // Copiar datos al buffer
        for (uint8_t i = 0; i < length; i++) {
            buffer[i] = Wire.read();
        }
        
        // Feed watchdog tras operación exitosa
        Watchdog::feed();
        
        // ÉXITO
        dev.online = true;
        dev.consecutiveFailures = 0;
        dev.lastSuccessMs = now;
        dev.nextRetryMs = 0;  // Sin backoff
        
        return true;
    }
    
    // FALLO tras MAX_RETRIES
    dev.consecutiveFailures++;
    
    // Calcular backoff exponencial: 1s, 2s, 4s, 8s, ... (máx 30s)
    uint8_t exponent = (dev.consecutiveFailures - 1 > 5) ? 5 : (dev.consecutiveFailures - 1);
    uint32_t backoff = 1000 * (1 << exponent);
    if (backoff > MAX_BACKOFF_MS) backoff = MAX_BACKOFF_MS;
    dev.nextRetryMs = now + backoff;
    
    Logger::errorf("I2C fallo dev 0x%02X (fallos:%d, backoff:%lus)", 
                   deviceAddr, dev.consecutiveFailures, backoff / 1000);
    
    // Intentar bus recovery si fallos >= 3
    if (dev.consecutiveFailures >= 3) {
        Logger::warn("I2CRecovery: Intentando bus recovery...");
        recoverBus();
    }
    
    return false;
}

bool writeBytesWithRetry(uint8_t deviceAddr, uint8_t regAddr,
                          const uint8_t* data, uint8_t length, uint8_t deviceId) {
    if (deviceId >= MAX_DEVICES) {
        Logger::errorf("deviceId inválido: %d", deviceId);
        return false;
    }
    
    DeviceState& dev = devices[deviceId];
    uint32_t now = millis();
    
    // Verificar backoff
    if (now < dev.nextRetryMs) {
        return false;
    }
    
    dev.lastAttemptMs = now;
    
    // Intentar escritura con retry
    for (uint8_t retry = 0; retry <= MAX_RETRIES; retry++) {
        if (retry > 0) {
            // Non-blocking: use delayMicroseconds for minimal settling time
            delayMicroseconds(500);  // 0.5ms instead of 50ms
        }
        
        Wire.beginTransmission(deviceAddr);
        Wire.write(regAddr);
        Wire.write(data, length);
        uint8_t result = Wire.endTransmission();
        
        Watchdog::feed();
        
        if (result == 0) {
            // ÉXITO
            dev.online = true;
            dev.consecutiveFailures = 0;
            dev.lastSuccessMs = now;
            dev.nextRetryMs = 0;
            return true;
        }
    }
    
    // FALLO
    dev.consecutiveFailures++;
    uint8_t exponent = (dev.consecutiveFailures - 1 > 5) ? 5 : (dev.consecutiveFailures - 1);
    uint32_t backoff = 1000 * (1 << exponent);
    if (backoff > MAX_BACKOFF_MS) backoff = MAX_BACKOFF_MS;
    dev.nextRetryMs = now + backoff;
    
    Logger::errorf("I2C write fallo dev 0x%02X (backoff:%lus)", 
                   deviceAddr, backoff / 1000);
    
    if (dev.consecutiveFailures >= 3) {
        recoverBus();
    }
    
    return false;
}

bool reinitSensor(uint8_t channel, uint8_t deviceAddr, uint8_t deviceId) {
    Serial.println("[I2CRecovery] Re-init sensor...");
    
    // 1. Intentar re-seleccionar canal TCA9548A
    if (!tcaSelectSafe(channel)) {
        Serial.printf("[ERROR] Re-init: TCA select ch%d falló\n", channel);
        
        // 2. Intentar bus recovery completo
        if (!recoverBus()) {
            return false;
        }
        
        // Retry select tras recovery
        if (!tcaSelectSafe(channel)) {
            return false;
        }
    }
    
    // 3. Intentar ping al dispositivo
    Wire.beginTransmission(deviceAddr);
    uint8_t result = Wire.endTransmission();
    
    if (result == 0) {
        Serial.println("[INFO] I2CRecovery: Sensor re-init exitoso");
        
        // Resetear estado
        if (deviceId < MAX_DEVICES) {
            resetDeviceState(deviceId);
        }
        return true;
    }
    
    Serial.printf("[ERROR] Sensor re-init falló (ping error %d)\n", result);
    return false;
}

bool isDeviceOnline(uint8_t deviceId) {
    if (deviceId >= MAX_DEVICES) return false;
    return devices[deviceId].online;
}

const DeviceState& getDeviceState(uint8_t deviceId) {
    static DeviceState dummy = {false, 0, 0, 0, 0};
    if (deviceId >= MAX_DEVICES) return dummy;
    return devices[deviceId];
}

void resetDeviceState(uint8_t deviceId) {
    if (deviceId >= MAX_DEVICES) return;
    
    devices[deviceId].online = true;
    devices[deviceId].consecutiveFailures = 0;
    devices[deviceId].lastSuccessMs = millis();
    devices[deviceId].lastAttemptMs = 0;
    devices[deviceId].nextRetryMs = 0;
    
    Serial.printf("[DEBUG] Device %d estado reseteado\n", deviceId);
}

bool isBusHealthy() {
    uint8_t onlineCount = 0;
    for (uint8_t i = 0; i < MAX_DEVICES; i++) {
        if (devices[i].online) onlineCount++;
    }
    return (onlineCount > 0);
}

void markDeviceOffline(uint8_t deviceId) {
    if (deviceId >= MAX_DEVICES) return;
    
    devices[deviceId].online = false;
    devices[deviceId].consecutiveFailures++;
    
    // Calcular backoff exponencial para evitar reintentos frecuentes
    uint8_t exponent = (devices[deviceId].consecutiveFailures - 1 > 5) ? 5 : (devices[deviceId].consecutiveFailures - 1);
    uint32_t backoff = 1000 * (1 << exponent);
    if (backoff > MAX_BACKOFF_MS) backoff = MAX_BACKOFF_MS;
    devices[deviceId].nextRetryMs = millis() + backoff;
    
    Serial.printf("[DEBUG] Device %d marcado OFFLINE (backoff:%lus)\n", deviceId, backoff / 1000);
}

} // namespace I2CRecovery
