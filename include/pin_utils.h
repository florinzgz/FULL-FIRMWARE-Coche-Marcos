#pragma once
#include <stdint.h>

// ============================================================================
// pin_utils.h - Utility functions for ESP32-S3 GPIO validation
// ============================================================================
// This header provides safe pin validation helpers for ESP32-S3, including:
//   - pin_is_reserved(): checks if a pin is reserved/strapping/JTAG
//   - pin_is_valid_gpio(): checks if a pin is within valid GPIO range
//   - pin_is_safe_input(): checks if a pin is safe for general input use
//   - pin_is_safe_output(): checks if a pin is safe for general output use
// These functions help avoid reserved, strapping, and JTAG pins for general use.
// ============================================================================

/**
 * @brief Check if a GPIO pin is reserved/strapping pin on ESP32-S3
 * 
 * Reserved pins that should be avoided for general I/O:
 * - GPIO 0:  Boot mode (HIGH=SPI Boot, LOW=Download mode)
 * - GPIO 3:  JTAG (avoid if using JTAG debugging)
 * - GPIO 45: VDD_SPI voltage select
 * - GPIO 46: Boot mode / ROM log
 * - GPIO 43: UART0 TX (reserved for USB/Serial)
 * - GPIO 44: UART0 RX (reserved for USB/Serial)
 * 
 * @param gpio GPIO number to check
 * @return true if pin is reserved/strapping, false if safe for general use
 */
static inline bool pin_is_reserved(uint8_t gpio) {
    switch (gpio) {
        case 0:   // Boot mode
        case 3:   // JTAG
        case 43:  // UART0 TX
        case 44:  // UART0 RX
        case 45:  // VDD_SPI voltage
        case 46:  // Boot mode / ROM log
            return true;
        default:
            return false;
    }
}

/**
 * @brief Check if a GPIO is within valid range for ESP32-S3
 * 
 * ESP32-S3 has GPIOs 0-48 available, but not all are exposed on all boards.
 * 
 * @param gpio GPIO number to check
 * @return true if within valid ESP32-S3 GPIO range
 */
static inline bool pin_is_valid_gpio(uint8_t gpio) {
    return gpio <= 48;
}

/**
 * @brief Check if a GPIO is safe for general input use
 * 
 * Excludes strapping pins and reserved pins that could cause boot issues
 * or interfere with JTAG/USB functionality.
 * 
 * @param gpio GPIO number to check
 * @return true if safe for input use
 */
static inline bool pin_is_safe_input(uint8_t gpio) {
    if (!pin_is_valid_gpio(gpio)) return false;
    if (pin_is_reserved(gpio)) return false;
    return true;
}

/**
 * @brief Check if a GPIO is safe for general output use
 * 
 * Excludes input-only pins (34-39 on ESP32, but ESP32-S3 has different constraints)
 * and reserved/strapping pins.
 * 
 * @param gpio GPIO number to check
 * @return true if safe for output use
 */
static inline bool pin_is_safe_output(uint8_t gpio) {
    if (!pin_is_valid_gpio(gpio)) return false;
    if (pin_is_reserved(gpio)) return false;
    return true;
}
