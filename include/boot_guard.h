#pragma once

#include <stdint.h>

namespace BootGuard {
    // Ensure XSHUT lines that land on strapping pins are forced HIGH early in boot
    void applyXshutStrappingGuard();
    
    // 🔒 v2.17.1: Boot counter for bootloop detection
    void initBootCounter();
    void incrementBootCounter();
    void clearBootCounter();
    uint8_t getBootCount();
    bool isBootloopDetected();  // Returns true if >3 boots in 60 seconds
    bool shouldEnterSafeMode(); // Returns true if bootloop detected
}
