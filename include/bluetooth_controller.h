#ifndef BLUETOOTH_CONTROLLER_H
#define BLUETOOTH_CONTROLLER_H

#include <Arduino.h>

// ESP32-S3 does NOT support classic Bluetooth (only BLE)
// BluetoothSerial requires classic Bluetooth which is not available on ESP32-S3
// Bluetooth functionality is disabled for this hardware
// #include <BluetoothSerial.h>  // Not supported on ESP32-S3

// 8BitDo Zero 2 control mapping (SIMPLIFIED)
// D-PAD Arrows:
//   UP    = Forward (adelante)
//   DOWN  = Reverse (marcha atrás)
//   LEFT  = Turn left (girar izquierda)
//   RIGHT = Turn right (girar derecha)
// Buttons:
//   A = Emergency STOP (parada emergencia)
//   B = Resume normal operation (volver a control conductor)
//   X = Force Park (forzar marcha P por seguridad)
//   Y = Additional safety button
enum class BTButton {
    A = 0,      // Emergency STOP
    B = 1,      // Resume normal operation
    X = 2,      // Force Park (safety)
    Y = 3,      // Additional safety
    START = 4,  // Pair new controller
    SELECT = 5  // Disconnect
};

// Override modes
enum class OverrideMode {
    NONE = 0,           // Normal driver control
    EMERGENCY_STOP = 1, // Emergency stop active
    STEERING = 2,       // Steering override
    THROTTLE = 3,       // Throttle override
    FULL = 4            // Full remote control
};

struct ControllerState {
    bool connected;
    bool override_active;
    OverrideMode mode;
    
    // Joystick values (-100 to +100)
    int8_t left_x;   // Steering
    int8_t left_y;
    int8_t right_x;
    int8_t right_y;  // Throttle/Brake
    
    // Button states
    bool button_a;   // Emergency STOP
    bool button_b;   // Resume
    bool button_x;   // Force Park
    bool button_y;   // Toggle override
    bool button_start;
    bool button_select;
    
    // Timestamps
    unsigned long last_packet_ms;
    unsigned long override_start_ms;
};

class BluetoothController {
public:
    static void init();
    static void update();
    
    // Connection management
    static bool isConnected();
    static bool startPairing();
    static void disconnect();
    
    // Override control
    static bool isOverrideActive();
    static OverrideMode getOverrideMode();
    static void clearOverride();
    
    // Get controller values with priority override
    static int8_t getSteeringOverride();   // -100 to +100
    static int8_t getThrottleOverride();   // -100 to +100
    static bool isEmergencyStop();
    static int getGearOverride();          // 0=none, 1=P, 2=R, 3=N, 4=D1, 5=D2
    
    // State access
    static ControllerState& getState();
    
    // EEPROM persistence
    static void savePairing();
    static void loadPairing();

private:
    // static BluetoothSerial btSerial;  // Not supported on ESP32-S3 (classic BT only)
    static ControllerState state;
    static unsigned long last_update_ms;
    static String paired_device_mac;
    
    // Internal helpers
    static void processPacket(uint8_t* data, size_t len);
    static void handleEmergencyStop();
    static void handleResume();
    static int8_t applyDeadzone(int8_t value, int8_t deadzone = 10);
    static void checkConnectionTimeout();
};

#endif // BLUETOOTH_CONTROLLER_H
