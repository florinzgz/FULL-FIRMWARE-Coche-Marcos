#include "storage.h"
#include <Preferences.h>
#include "settings.h"
#include "logger.h"

static Preferences prefs;

// Global config variable
Storage::Config cfg;

static const char *kNamespace = "vehicle";
static const char *kKeyBlob   = "config";

void Storage::init() {
    if (!prefs.begin(kNamespace, false)) {
        Logger::warn("Storage init: fallo al abrir namespace");
    }
}

void Storage::defaults(Config &cfg) {
    // Pedal
    cfg.pedalMin = 200;           // ejemplo
    cfg.pedalMax = 3800;          // ejemplo
    cfg.pedalCurve = 0;

    // Freno regenerativo
    cfg.regenPercent = REGEN_DEFAULT;

    // INA226 shunts (coeficiente de conversión corriente)
    cfg.shuntCoeff[0] = 0.0010f;  // batería 100A
    cfg.shuntCoeff[1] = 0.0020f;  // rueda FL 50A
    cfg.shuntCoeff[2] = 0.0020f;  // rueda FR 50A
    cfg.shuntCoeff[3] = 0.0020f;  // rueda RL 50A
    cfg.shuntCoeff[4] = 0.0020f;  // rueda RR 50A
    cfg.shuntCoeff[5] = 0.0020f;  // dirección 50A

    // Dirección
    cfg.steerZeroOffset = 0;

    // HUD
    cfg.showTemps  = true;
    cfg.showEffort = true;

    // Módulos
    cfg.audioEnabled      = true;
    cfg.lightsEnabled     = true;
    cfg.multimediaEnabled = true;

    // Nuevos flags de tolerancia a fallos
    cfg.wheelSensorsEnabled    = true;
    cfg.tempSensorsEnabled     = true;
    cfg.currentSensorsEnabled  = true;
    cfg.steeringEnabled        = true;

    // Errores persistentes
    cfg.errorCount = 0;
    for(int i=0; i<Config::MAX_ERRORS; i++) {
        cfg.errors[i] = {0,0};
    }

    // versión y checksum
    cfg.version = kConfigVersion;
    cfg.checksum = computeChecksum(cfg);
}

uint32_t Storage::computeChecksum(const Config &cfg) {
    // Simple FNV-1a sobre campos (excluye checksum)
    const uint32_t FNV_OFFSET = 2166136261u;
    const uint32_t FNV_PRIME  = 16777619u;
    uint32_t h = FNV_OFFSET;

    auto mix = [&](const uint8_t *data, size_t len) {
        for(size_t i=0; i<len; ++i) { h ^= data[i]; h *= FNV_PRIME; }
    };

    mix((uint8_t*)&cfg.pedalMin, sizeof(cfg.pedalMin));
    mix((uint8_t*)&cfg.pedalMax, sizeof(cfg.pedalMax));
    mix((uint8_t*)&cfg.pedalCurve, sizeof(cfg.pedalCurve));
    mix((uint8_t*)&cfg.regenPercent, sizeof(cfg.regenPercent));
    mix((uint8_t*)&cfg.shuntCoeff[0], sizeof(cfg.shuntCoeff));
    mix((uint8_t*)&cfg.steerZeroOffset, sizeof(cfg.steerZeroOffset));
    mix((uint8_t*)&cfg.showTemps, sizeof(cfg.showTemps));
    mix((uint8_t*)&cfg.showEffort, sizeof(cfg.showEffort));
    mix((uint8_t*)&cfg.audioEnabled, sizeof(cfg.audioEnabled));
    mix((uint8_t*)&cfg.lightsEnabled, sizeof(cfg.lightsEnabled));
    mix((uint8_t*)&cfg.multimediaEnabled, sizeof(cfg.multimediaEnabled));

    // Nuevos flags
    mix((uint8_t*)&cfg.wheelSensorsEnabled, sizeof(cfg.wheelSensorsEnabled));
    mix((uint8_t*)&cfg.tempSensorsEnabled, sizeof(cfg.tempSensorsEnabled));
    mix((uint8_t*)&cfg.currentSensorsEnabled, sizeof(cfg.currentSensorsEnabled));
    mix((uint8_t*)&cfg.steeringEnabled, sizeof(cfg.steeringEnabled));

    // Errores persistentes
    mix((uint8_t*)&cfg.errorCount, sizeof(cfg.errorCount));
    mix((uint8_t*)&cfg.errors[0], sizeof(cfg.errors));

    mix((uint8_t*)&cfg.version, sizeof(cfg.version));
    return h;
}

void Storage::load(Config &cfg) {
    size_t len = prefs.getBytesLength(kKeyBlob);
    if(len != sizeof(Config)) {
        Logger::warn("Storage load: tamaño inválido, usando defaults");
        defaults(cfg);
        return;
    }
    prefs.getBytes(kKeyBlob, &cfg, sizeof(Config));

    // validar versión y checksum
    if(cfg.version != kConfigVersion) {
        Logger::warn("Storage load: versión inválida, usando defaults");
        defaults(cfg);
        return;
    }
    uint32_t chk = computeChecksum(cfg);
    if(chk != cfg.checksum) {
        Logger::warn("Storage load: checksum inválido, usando defaults");
        defaults(cfg);
        return;
    }
}

bool Storage::save(const Config &cfgIn) {
    Config tmp = cfgIn;
    tmp.version = kConfigVersion;
    tmp.checksum = computeChecksum(tmp);
    size_t written = prefs.putBytes(kKeyBlob, &tmp, sizeof(Config));
    if(written != sizeof(Config)) {
        Logger::errorf("Storage save: fallo al escribir (%u bytes)", written);
        return false;
    }
    return true;
}

void Storage::resetToFactory() {
    prefs.clear();
    Logger::warn("Storage: reset a valores de fábrica");
}