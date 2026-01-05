# TOFSense-M S LiDAR Integration Guide

## Version: 2.12.0
## Date: 2026-01-05

## Overview

This document describes the integration of the TOFSense-M S LiDAR sensor into the ESP32-S3 vehicle firmware, replacing the previous VL53L5X I2C-based obstacle detection system.

## Hardware Changes

### Sensor Migration
- **Old System**: 2x VL53L5X sensors via I2C with PCA9548A multiplexer
- **New System**: 1x TOFSense-M S via UART1

### Pin Assignments

#### TOFSense-M S (UART1)
- **RX (GPIO 18)**: Receives data from sensor
- **TX (GPIO 17)**: Not used (sensor is output-only)
- **Baudrate**: 115200
- **Format**: 8N1 (8 data bits, no parity, 1 stop bit)

#### DFPlayer Mini Audio (UART2)
- **TX (GPIO 15)**: Sends commands to DFPlayer
- **RX (GPIO 16)**: Receives responses from DFPlayer
- **Baudrate**: 9600
- **Format**: 8N1

#### Freed GPIOs
- **GPIO 43**: Now used for wheel sensor RL (rear left)
- **GPIO 44**: Now used for wheel sensor RR (rear right)
- **GPIO 46**: Now free (was XSHUT for VL53L5X front)
- **GPIO 19**: Now used for LED_FRONT (moved from GPIO 18)

## TOFSense-M S UART Protocol

### Reference Documentation
Official manual: [TOFSense-M_User_Manual_V1.4_en.pdf](https://ftp.nooploop.com/software/products/tofsense_m/doc/TOFSense-M_User_Manual_V1.4_en.pdf)

### Packet Format

The TOFSense-M S continuously transmits 9-byte packets at ~100Hz:

```
Byte 0: Header      = 0x57 (fixed)
Byte 1: Function    = 0x00 (distance measurement)
Byte 2: Length      = 0x02 (2 bytes of data)
Byte 3: Data_L      = Distance low byte
Byte 4: Data_H      = Distance high byte
Byte 5-7: Reserved  = Additional data (not used)
Byte 8: Checksum    = Sum of bytes 0-7, masked with 0xFF
```

### Distance Calculation

```cpp
uint16_t distance_mm = (Data_H << 8) | Data_L;
```

### Checksum Validation

```cpp
uint8_t checksum = 0;
for (uint8_t i = 0; i < 8; i++) {
    checksum += packet[i];
}
checksum &= 0xFF;

if (checksum != packet[8]) {
    // Invalid packet
}
```

### Example Packet

```
0x57 0x00 0x02 0xE8 0x03 0x00 0x00 0x00 0x44
Header: 0x57
Function: 0x00
Length: 0x02
Distance_L: 0xE8 (232)
Distance_H: 0x03 (3)
Distance = (3 << 8) | 232 = 768 + 232 = 1000 mm (1 meter)
Checksum: 0x44 (0x57+0x00+0x02+0xE8+0x03+0x00+0x00+0x00 = 0x144 & 0xFF = 0x44)
```

## Software Architecture

### Configuration (obstacle_config.h)

Key constants:
- `NUM_SENSORS = 1`: Single sensor
- `UART_BAUDRATE = 115200`: Communication speed
- `UART_NUM = 1`: Hardware UART1
- `PACKET_LENGTH = 9`: Bytes per packet
- `DISTANCE_MAX = 12000`: Max range 12 meters
- `DISTANCE_INVALID = 65535`: Invalid reading marker

### Distance Thresholds

```cpp
DISTANCE_CRITICAL = 200mm   // 0-20cm: Emergency stop
DISTANCE_WARNING = 500mm    // 20-50cm: Brake assist
DISTANCE_CAUTION = 1000mm   // 50-100cm: Reduce speed
DISTANCE_MAX = 12000mm      // Maximum detection range
```

### Implementation (obstacle_detection.cpp)

#### Initialization Flow

1. Initialize UART1 with 115200 baud
2. Configure GPIO 18 (RX) and GPIO 17 (TX)
3. Wait 100ms for UART stabilization
4. Attempt to detect sensor by waiting for data (2 second timeout)
5. If data received, set `hardwarePresent = true`
6. Otherwise, enter placeholder/simulation mode

#### Update Loop

The `update()` function is called periodically from the main loop:

1. Read available bytes from UART
2. Search for packet header (0x57)
3. Accumulate 9 bytes into buffer
4. Validate checksum
5. Extract distance value
6. Apply calibration offset
7. Update sensor state and proximity level
8. Handle timeouts (no data received)

#### Error Handling

- **Checksum mismatch**: Increment error counter, log warning
- **Invalid header/function**: Discard packet, continue
- **Timeout (100ms)**: Mark sensor unhealthy
- **Too many errors**: Disable sensor after 10 consecutive failures

### API Functions

```cpp
void init();                                    // Initialize UART and sensor
void update();                                  // Process UART data (call in loop)
uint16_t getMinDistance(uint8_t sensorIdx);    // Get current distance (mm)
ObstacleLevel getProximityLevel(uint8_t idx);  // Get proximity level
bool isHealthy(uint8_t sensorIdx);             // Check sensor health
void getStatus(ObstacleStatus& status);        // Get complete system status
```

## Migration Notes

### Removed Components

1. **VL53L5X Library**: Removed from platformio.ini
2. **I2C Recovery**: No longer needed for obstacle detection
3. **PCA9548A Multiplexer**: Eliminated (was @ 0x71)
4. **XSHUT Pins**: GPIO 46 and 19 freed
5. **8x8 Grid Data**: Replaced with single distance value

### Simplified Architecture

- No I2C communication overhead
- No multiplexer channel switching
- Single distance measurement instead of 64 zones
- Cleaner, more robust UART protocol
- No strapping pin conflicts (GPIO 46 freed)

### Compatibility

The API remains largely compatible:
- `getMinDistance()` returns single sensor distance
- `getProximityLevel()` works as before
- `ObstacleStatus` structure unchanged
- Existing code using obstacle detection should work without modification

### Known Limitations

1. **Single Sensor**: Only front detection, no rear coverage
2. **No Grid Data**: Single distance point instead of 8x8 grid
3. **No Confidence Metric**: Sensor doesn't provide confidence values (assumed 100%)

## Testing and Validation

### Verification Steps

1. **UART Communication**: Monitor Serial output for "TOFSense-M S sensor detected"
2. **Distance Readings**: Check logs every 2 seconds for distance values
3. **Packet Validation**: Ensure no checksum errors in logs
4. **Timeout Handling**: Disconnect sensor to verify timeout warnings
5. **LED Control**: Verify front LEDs work on GPIO 19
6. **Audio**: Verify DFPlayer works on UART2 (GPIO 15/16)
7. **Wheel Sensors**: Verify rear wheel sensors work on GPIO 43/44

### Debug Output

Enable debug logging to see:
```
TOFSense: Distance = 1234 mm
TOFSense: Checksum mismatch (expected: 0xAB, got: 0xCD)
TOFSense: Communication timeout
```

## Hardware Connection

### TOFSense-M S Wiring

```
TOFSense-M S          ESP32-S3
-----------           --------
VCC (5V)      -->     5V
GND           -->     GND
TX (Output)   -->     GPIO 18 (RX)
RX (Input)    -->     Not connected (sensor is output-only)
```

### Important Notes

- **Power**: Sensor requires 5V, not 3.3V
- **Logic Levels**: Sensor TX output is 3.3V compatible
- **No TX Connection**: ESP32 GPIO 17 (TX) is not connected to sensor
- **Update Rate**: Sensor transmits at ~100Hz continuously

## Troubleshooting

### Sensor Not Detected

1. Check 5V power connection
2. Verify GPIO 18 is receiving data (use oscilloscope/logic analyzer)
3. Ensure baudrate is 115200
4. Check for physical damage to sensor or cable

### Invalid Checksums

1. Verify baudrate exactly 115200
2. Check for electrical noise on data line
3. Add pull-up resistor if signal quality poor
4. Shorten cable length if possible

### Timeout Errors

1. Power cycle the sensor
2. Check cable connections
3. Verify 5V power supply can provide adequate current
4. Look for firmware logs indicating sensor health

## Performance Characteristics

- **Update Rate**: ~100Hz (sensor transmits continuously)
- **Firmware Processing**: 15Hz effective update rate
- **Range**: 0.1m to 12m
- **Accuracy**: ±3cm (typical)
- **Response Time**: <10ms
- **Power Consumption**: <200mA @ 5V

## Future Enhancements

Potential improvements for future versions:

1. **Dual Sensor Support**: Add second TOFSense-M S for rear coverage
2. **Advanced Filtering**: Implement Kalman filter for smoother readings
3. **Multi-Zone**: Mount sensor on servo for scanning
4. **Data Logging**: Record distance history for analysis
5. **Adaptive Thresholds**: Adjust based on vehicle speed

## References

- [TOFSense-M S Product Page](https://www.nooploop.com/product/tofsense-m/)
- [User Manual v1.4](https://ftp.nooploop.com/software/products/tofsense_m/doc/TOFSense-M_User_Manual_V1.4_en.pdf)
- [ESP32-S3 UART Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/uart.html)

## Version History

- **v2.12.0** (2026-01-05): Initial TOFSense-M S integration, VL53L5X removal
- **v2.11.x**: Previous VL53L5X I2C-based system

---

*Document created as part of firmware v2.12.0 migration*
