#pragma once
#include <cstdint>

namespace Audio {

// Clase mínima que declara la API usada por el proyecto.
class DFPlayer {
public:
  static void init();
  static void play(uint16_t track);
  static void update();

  // Estado de inicialización
  static bool initOK();
};

// Wrapper para mantener la API uniforme con el resto de módulos
inline bool initOK() { return DFPlayer::initOK(); }

} // namespace Audio