cat >> apply_corrections.sh << 'EOF'

# ============================================================================
# CORRECCIÓN 4: include/relays.h
# ============================================================================
echo ""
echo "🔧 CORRECCIÓN 4: include/relays.h"
echo "----------------------------------"

cat > include/relays.h << 'RELAYS_H_EOF'
#pragma once
#include <Arduino.h>

namespace Relays {
    struct State {
        bool mainOn;      // Relé principal (power hold)
        bool tractionOn;  // Relé 12V auxiliares
        bool steeringOn;  // Relé 24V motores tracción
        bool lightsOn;    // Estado luces
        bool mediaOn;     // Estado multimedia
    };

    // API existente
    void init();
    void enablePower();
    void disablePower();
    void update();
    const State& get();
    bool initOK();

    // Control individual
    void setLights(bool on);
    void setMedia(bool on);
    
    // NUEVAS FUNCIONES para monitoreo secuencia
    bool isPowerSequenceComplete();
    uint8_t getPowerSequenceProgress();  // 0-100%
    
    // Control individual de relés
    void setRelay(uint8_t relay_id, bool state);
    bool getRelayState(uint8_t relay_id);
    
    // Diagnóstico de relés
    bool selfTest();
    void emergencyStop();
}
RELAYS_H_EOF

echo "✅ include/relays.h actualizado"
EOF