#pragma once
#include <stdint.h>
#include "pins.h"

// ============================================================================
// pwm_channels.h - PCA9685 PWM channel definitions and validation
// ============================================================================
// This header defines the PWM channel assignments for the three PCA9685
// drivers used in the vehicle control system:
// - FRONT (0x40): Front wheel motors FL/FR
// - REAR (0x41): Rear wheel motors RL/RR
// - STEERING (0x42): Steering motor
// ============================================================================

// ============================================================================
// PCA9685 Channel Limits
// ============================================================================
constexpr uint8_t PCA9685_MAX_CHANNEL = 15;  // PCA9685 has 16 channels (0-15)
constexpr uint8_t PCA9685_CHANNELS_PER_MOTOR = 2;  // Forward + Reverse per motor

// ============================================================================
// Front Motor Controller (PCA9685 @ 0x40)
// ============================================================================
namespace PWMChannels {
    namespace Front {
        constexpr uint8_t FL_FORWARD = PCA_FRONT_CH_FL_FWD;  // Channel 0
        constexpr uint8_t FL_REVERSE = PCA_FRONT_CH_FL_REV;  // Channel 1
        constexpr uint8_t FR_FORWARD = PCA_FRONT_CH_FR_FWD;  // Channel 2
        constexpr uint8_t FR_REVERSE = PCA_FRONT_CH_FR_REV;  // Channel 3
        constexpr uint8_t MAX_USED = 3;  // Highest channel used
    }
    
    // ============================================================================
    // Rear Motor Controller (PCA9685 @ 0x41)
    // ============================================================================
    namespace Rear {
        constexpr uint8_t RL_FORWARD = PCA_REAR_CH_RL_FWD;   // Channel 0
        constexpr uint8_t RL_REVERSE = PCA_REAR_CH_RL_REV;   // Channel 1
        constexpr uint8_t RR_FORWARD = PCA_REAR_CH_RR_FWD;   // Channel 2
        constexpr uint8_t RR_REVERSE = PCA_REAR_CH_RR_REV;   // Channel 3
        constexpr uint8_t MAX_USED = 3;  // Highest channel used
    }
    
    // ============================================================================
    // Steering Motor Controller (PCA9685 @ 0x42)
    // ============================================================================
    namespace Steering {
        constexpr uint8_t PWM_FORWARD = PCA_STEER_CH_PWM_FWD;  // Channel 0
        constexpr uint8_t PWM_REVERSE = PCA_STEER_CH_PWM_REV;  // Channel 1
        constexpr uint8_t MAX_USED = 1;  // Highest channel used
    }
}

// ============================================================================
// Validation Functions
// ============================================================================

/**
 * @brief Validate a PCA9685 channel number
 * 
 * @param channel Channel number to validate (0-15)
 * @return true if channel is within valid range
 */
static inline bool pwm_channel_valid(uint8_t channel) {
    return channel <= PCA9685_MAX_CHANNEL;
}

/**
 * @brief Validate a front motor PWM channel
 * 
 * @param channel Channel number to validate
 * @return true if channel is used by front motors
 */
static inline bool pwm_channel_valid_front(uint8_t channel) {
    return channel <= PWMChannels::Front::MAX_USED;
}

/**
 * @brief Validate a rear motor PWM channel
 * 
 * @param channel Channel number to validate
 * @return true if channel is used by rear motors
 */
static inline bool pwm_channel_valid_rear(uint8_t channel) {
    return channel <= PWMChannels::Rear::MAX_USED;
}

/**
 * @brief Validate a steering motor PWM channel
 * 
 * @param channel Channel number to validate
 * @return true if channel is used by steering motor
 */
static inline bool pwm_channel_valid_steering(uint8_t channel) {
    return channel <= PWMChannels::Steering::MAX_USED;
}
