#include "operation_modes.h"
#include "logger.h"

namespace SystemMode {
// Variable global para el modo actual
OperationMode currentMode = OperationMode::MODE_FULL;

void init() {
  currentMode = OperationMode::MODE_FULL;
  Logger::info("SystemMode: Initialized in MODE_FULL");
}

void setMode(OperationMode mode) {
  // Siempre loggear intentos de cambiar a MODE_SAFE para diagnóstico
  if (mode == OperationMode::MODE_SAFE) {
    if (currentMode == OperationMode::MODE_SAFE) {
      Logger::warn("SystemMode: Intento adicional de cambiar a SAFE (ya en "
                   "SAFE) - indica fallo crítico múltiple");
    } else {
      currentMode = mode;
      Logger::errorf("SystemMode: Cambiado a %s - fallo crítico detectado",
                     getModeName());
    }
  } else if (currentMode != mode) {
    currentMode = mode;
    Logger::infof("SystemMode: Changed to %s", getModeName());
  }
}

OperationMode getMode() { return currentMode; }

const char *getModeName() {
  switch (currentMode) {
  case OperationMode::MODE_FULL:
    return "FULL";
  case OperationMode::MODE_DEGRADED:
    return "DEGRADED";
  case OperationMode::MODE_SAFE:
    return "SAFE";
  case OperationMode::MODE_STANDALONE:
    return "STANDALONE";
  default:
    return "UNKNOWN";
  }
}
} // namespace SystemMode
