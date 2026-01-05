# TOFSense-M S 8x8 Matrix Implementation - TODO

## Critical Issue Identified

The user correctly pointed out that TOFSense-M S is an **8x8 matrix sensor** (64 points), NOT a single-point sensor as initially implemented.

## Correct Specifications

**Hardware:**
- 8x8 matrix = 64 distance measurements per frame
- Range: 4 meters (NOT 12m)
- Field of view: 65°
- Update rate: ~15Hz
- UART: 921600 baud (NOT 115200)

**Protocol:**
- Frame size: 400 bytes (NOT 9 bytes)
- Header: `57 01 FF 00` (4 bytes, NOT just `0x57`)
- Protocol: NLink_TOFSense_M_Frame0
- Reference: TOFSense-M User Manual V3.0

## Frame Structure (400 bytes)

```
Byte 0-3:   Header (57 01 FF 00)
Byte 4:     ID (0x01)
Byte 5-6:   Length (0x0190 = 400, little-endian)
Byte 7-10:  System time (milliseconds, little-endian)
Byte 11-394: Matrix data (64 pixels × 6 bytes = 384 bytes)
  Each pixel (6 bytes):
    Byte 0-2: Distance (3 bytes, little-endian signed)
    Byte 3:   Signal strength  
    Byte 4:   Status
    Byte 5:   Reserved
Byte 395:   Checksum (sum of all bytes)
Byte 396-399: Reserved (4 bytes)
```

## Distance Conversion Formula

```cpp
// Extract 3-byte little-endian signed value
int32_t temp = (int32_t)(byte[0] | (byte[1] << 8) | (byte[2] << 16));

// Sign extend from 24-bit to 32-bit
if (temp & 0x800000) {
    temp |= 0xFF000000;
}

// Convert to millimeters
int32_t distanceMm = temp / 256;

// Negative values indicate invalid/out of range
if (distanceMm < 0 || distanceMm > 4000) {
    // Invalid pixel
}
```

## Implementation Status

### Completed ✅

1. **obstacle_config.h** - Updated with correct protocol constants
   - Baudrate: 921600
   - Frame size: 400 bytes
   - ZONES_PER_SENSOR: 64
   - DISTANCE_MAX: 4000mm
   - Frame header: {0x57, 0x01, 0xFF, 0x00}

2. **obstacle_detection.cpp** - Partial implementation
   - Frame buffer: 400 bytes
   - Header validation function
   - Distance parsing function (3-byte little-endian)
   - Frame parsing function (processes all 64 pixels)
   - Minimum distance calculation across all valid pixels

### In Progress ⏳

3. **obstacle_detection.cpp** - Complete UART update() function rewrite
   - Need to replace old 9-byte packet parsing
   - Implement 400-byte frame accumulation
   - Handle frame synchronization (find header in stream)
   - Process complete frames when received
   - Update sensor health tracking

### Not Started ❌

4. **obstacle_detection.cpp** - Additional functions
   - Update initialization to use correct baudrate
   - Remove old packet-based code completely

5. **HUD Updates**
   - Update distance range: 12m → 4m
   - Optional: Add 8x8 matrix visualization
   - Update proximity display for 64-point coverage

6. **Documentation**
   - Update TOFSENSE_INTEGRATION.md with 8x8 protocol
   - Update safety documentation for 64-point coverage
   - Add matrix visualization examples

## Key Implementation Notes

### Frame Synchronization

The update() function needs to:
1. Search for header bytes `57 01 FF 00` in UART stream
2. Accumulate 400 bytes starting from header
3. Validate checksum
4. Parse 64 distance values
5. Calculate minimum distance
6. Update sensor state

### Memory Considerations

- Frame buffer: 400 bytes (static allocation OK)
- Processing: ~15 frames/second
- Need efficient header search to avoid false synchronization

### Minimum Distance Strategy

With 64 points, we take the MINIMUM valid distance as the obstacle distance. This provides:
- Maximum safety (closest obstacle is detected)
- Better coverage (65° FOV with 64 points)
- Compatible with existing safety system (uses single minDistance value)

## Testing Requirements

1. Verify 921600 baud communication
2. Verify frame synchronization
3. Verify distance parsing (all 64 points)
4. Verify minimum distance calculation
5. Verify proximity levels work with 4m range
6. Verify fail-safe behavior still works

## References

- [TOFSense-M User Manual V3.0](https://ftp.nooploop.com/downloads/tofsense/TOFSense-M_User_Manual_V3.0_en.pdf)
- [Nooploop Protocol Center](https://support.nooploop.com/tofsense/protocol/)
- Product Description: "Sensor de Distancia LiDAR TOFSense-M S, telémetro, módulo de Radar láser, Campo de visión 3D de 65°, Matriz de 8 * 8 Puntos, UART, Can, STM32, APM/Pixhawk"
