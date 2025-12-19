#pragma once

#include <stdint.h>

namespace BootGuard {
    // Ensure XSHUT lines that land on strapping pins are forced HIGH early in boot
    void applyXshutStrappingGuard();
}
