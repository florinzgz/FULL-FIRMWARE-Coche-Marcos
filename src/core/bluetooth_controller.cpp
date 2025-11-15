#include "bluetooth_controller.h"
#include "config_storage.h"
#include "power_mgmt.h"
// #include "audio.h"  // Audio system not yet implemented
#include <Preferences.h>

// ⚠️ ESP32-S3 DOES NOT SUPPORT CLASSIC BLUETOOTH ⚠️
// BluetoothSerial requires classic Bluetooth (not available on ESP32-S3)
// ESP32-S3 only supports BLE (Bluetooth Low Energy)
// Bluetooth controller functionality is disabled for this hardware

// Static member initialization
// BluetoothSerial BluetoothController::btSerial;  // Not supported on ESP32-S3
ControllerState BluetoothController::state = {};
unsigned long BluetoothController::last_update_ms = 0;
String BluetoothController::paired_device_mac = "";

// Connection timeout (2 seconds)
#define BT_TIMEOUT_MS 2000
#define BT_UPDATE_INTERVAL_MS 100  // 10Hz

void BluetoothController::init() {
    // Bluetooth disabled on ESP32-S3 (classic BT not supported)
    Serial.println("⚠️ Bluetooth Controller: DISABLED (ESP32-S3 does not support classic Bluetooth)");
    Serial.println("   ESP32-S3 only supports BLE. Use BLE controller or switch to ESP32/ESP32-C3");
    
    // Initialize Bluetooth Serial - DISABLED FOR ESP32-S3
    // btSerial.begin("Smart Car ESP32");
    
    // Load paired device from EEPROM
    // loadPairing();
    
    // Initialize state
    state.connected = false;
    state.override_active = false;
    state.mode = OverrideMode::NONE;
    state.last_packet_ms = 0;
    state.override_start_ms = 0;
    
    // Zero out joystick values
    state.left_x = 0;
    state.left_y = 0;
    state.right_x = 0;
    state.right_y = 0;
    
    // Clear button states
    state.button_a = false;
    state.button_b = false;
    state.button_x = false;
    state.button_y = false;
    state.button_start = false;
    state.button_select = false;
}

void BluetoothController::update() {
    // Bluetooth disabled on ESP32-S3 - no updates needed
    return;
    
    /* DISABLED FOR ESP32-S3
    unsigned long now = millis();
    
    // Rate limit updates to 10Hz
    if (now - last_update_ms < BT_UPDATE_INTERVAL_MS) {
        return;
    }
    last_update_ms = now;
    
    // Check for incoming Bluetooth data
    if (btSerial.available()) {
        uint8_t buffer[32];
        size_t len = 0;
        
        while (btSerial.available() && len < sizeof(buffer)) {
            buffer[len++] = btSerial.read();
        }
        
        if (len > 0) {
            processPacket(buffer, len);
            state.last_packet_ms = now;
            state.connected = true;
        }
    }
    
    // Check connection timeout
    checkConnectionTimeout();
    */
}

bool BluetoothController::isConnected() {
    return state.connected;
}

bool BluetoothController::startPairing() {
    Serial.println("⚠️ Bluetooth pairing unavailable - ESP32-S3 does not support classic Bluetooth");
    // ESP32 Bluetooth is already discoverable - DISABLED FOR ESP32-S3
    return false;
}

void BluetoothController::disconnect() {
    // btSerial.disconnect();  // Disabled for ESP32-S3
    state.connected = false;
    clearOverride();
    Serial.println("Bluetooth controller disconnected (N/A on ESP32-S3)");
}

bool BluetoothController::isOverrideActive() {
    return state.override_active;
}

OverrideMode BluetoothController::getOverrideMode() {
    return state.mode;
}

void BluetoothController::clearOverride() {
    state.override_active = false;
    state.mode = OverrideMode::NONE;
    state.override_start_ms = 0;
}

int8_t BluetoothController::getSteeringOverride() {
    if (!state.override_active || state.mode == OverrideMode::EMERGENCY_STOP) {
        return 0;
    }
    return applyDeadzone(state.left_x);
}

int8_t BluetoothController::getThrottleOverride() {
    if (!state.override_active || state.mode == OverrideMode::EMERGENCY_STOP) {
        return 0;
    }
    return applyDeadzone(state.right_y);
}

bool BluetoothController::isEmergencyStop() {
    return state.mode == OverrideMode::EMERGENCY_STOP;
}

int BluetoothController::getGearOverride() {
    if (!state.override_active) {
        return 0; // No override
    }
    
    // X button forces Park
    if (state.button_x) {
        return 1; // Park
    }
    
    return 0; // No gear override
}

ControllerState& BluetoothController::getState() {
    return state;
}

void BluetoothController::savePairing() {
    Preferences prefs;
    prefs.begin("bt_ctrl", false);
    prefs.putString("mac", paired_device_mac);
    prefs.end();
}

void BluetoothController::loadPairing() {
    Preferences prefs;
    prefs.begin("bt_ctrl", true);
    paired_device_mac = prefs.getString("mac", "");
    prefs.end();
    
    if (paired_device_mac.length() > 0) {
        Serial.print("Loaded paired device MAC: ");
        Serial.println(paired_device_mac);
    }
}

void BluetoothController::processPacket(uint8_t* data, size_t len) {
    // 8BitDo Zero 2 sends HID gamepad reports
    // Simplified mapping: D-pad arrows = steering/throttle
    
    if (len < 8) return;
    
    // Parse D-pad values (byte 2 contains D-pad state)
    uint8_t dpad = data[2];
    
    // D-pad mapping (standard HID):
    // 0=up, 1=up-right, 2=right, 3=down-right, 4=down, 5=down-left, 6=left, 7=up-left, 8=neutral
    state.left_x = 0;   // Steering (left/right arrows)
    state.left_y = 0;
    state.right_x = 0;
    state.right_y = 0;  // Throttle (up/down arrows)
    
    switch(dpad) {
        case 0: // Arrow UP = Forward
            state.right_y = 100;  // Full throttle forward
            break;
        case 2: // Arrow RIGHT = Turn right
            state.left_x = 100;   // Turn right
            break;
        case 4: // Arrow DOWN = Reverse
            state.right_y = -100; // Full reverse
            break;
        case 6: // Arrow LEFT = Turn left
            state.left_x = -100;  // Turn left
            break;
        case 1: // Up-Right = Forward + Turn right
            state.right_y = 100;
            state.left_x = 100;
            break;
        case 3: // Down-Right = Reverse + Turn right
            state.right_y = -100;
            state.left_x = 100;
            break;
        case 5: // Down-Left = Reverse + Turn left
            state.right_y = -100;
            state.left_x = -100;
            break;
        case 7: // Up-Left = Forward + Turn left
            state.right_y = 100;
            state.left_x = -100;
            break;
        default: // Neutral
            state.left_x = 0;
            state.right_y = 0;
            break;
    }
    
    // Parse button states (byte 6-7, bit flags)
    uint16_t buttons = (data[7] << 8) | data[6];
    state.button_a = buttons & 0x0001;  // Emergency STOP
    state.button_b = buttons & 0x0002;  // Resume normal
    state.button_x = buttons & 0x0004;  // Safety: Force Park
    state.button_y = buttons & 0x0008;  // Safety: additional safety
    state.button_start = buttons & 0x0010;
    state.button_select = buttons & 0x0020;
    
    // Handle emergency stop button (A) - HIGHEST PRIORITY
    if (state.button_a) {
        handleEmergencyStop();
        return; // Don't process other inputs during emergency stop
    }
    
    // Handle resume button (B)
    if (state.button_b && state.override_active) {
        handleResume();
        return;
    }
    
    // Activate override if D-pad is being used
    if (!state.override_active && dpad != 8) {
        state.override_active = true;
        state.mode = OverrideMode::FULL;
        state.override_start_ms = millis();
        // Audio::playAlert(Audio::Alert::WARNING);  // Audio system not yet implemented
        Serial.println("Bluetooth override ACTIVE - remote control engaged");
    }
    
    // Clear override when D-pad returns to neutral (if not emergency stopped)
    if (state.override_active && dpad == 8 && state.mode != OverrideMode::EMERGENCY_STOP) {
        clearOverride();
        Serial.println("Bluetooth override cleared - D-pad neutral");
    }
}

void BluetoothController::handleEmergencyStop() {
    state.override_active = true;
    state.mode = OverrideMode::EMERGENCY_STOP;
    state.override_start_ms = millis();
    
    // Cut all motors immediately
    // PowerMgmt::emergencyStop();  // PowerMgmt emergency stop not yet implemented
    
    // Play emergency alert
    // Audio::playAlert(Audio::Alert::CRITICAL);  // Audio system not yet implemented
    
    Serial.println("EMERGENCY STOP activated via Bluetooth!");
}

void BluetoothController::handleResume() {
    clearOverride();
    // Audio::playAlert(Audio::Alert::INFO);  // Audio system not yet implemented
    Serial.println("Bluetooth override cleared - normal operation resumed");
}

int8_t BluetoothController::applyDeadzone(int8_t value, int8_t deadzone) {
    if (abs(value) < deadzone) {
        return 0;
    }
    return value;
}

void BluetoothController::checkConnectionTimeout() {
    if (!state.connected) return;
    
    unsigned long now = millis();
    
    // Check if we haven't received data in a while
    if (now - state.last_packet_ms > BT_TIMEOUT_MS) {
        Serial.println("Bluetooth connection timeout - controller battery may be low");
        state.connected = false;
        
        // IMPORTANT: Clear override but DON'T emergency stop
        // This allows driver to continue if controller battery dies
        if (state.override_active) {
            clearOverride();
            // Audio::playAlert(Audio::Alert::WARNING);  // Audio system not yet implemented
            Serial.println("Bluetooth override cleared - driver has full control");
            Serial.println("Controller disconnected - check battery if needed");
        }
    }
}
