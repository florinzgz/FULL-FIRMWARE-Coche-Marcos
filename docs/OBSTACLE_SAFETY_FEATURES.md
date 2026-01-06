# Obstacle Detection Safety Features - v2.12.0

## Overview
This document describes the safety features implemented in the obstacle detection system after migration from VL53L5X to TOFSense-M S UART sensor.

## Critical Safety Features

### 1. Fail-Safe Behavior (CRITICAL)

**Problem**: What happens if the sensor stops responding while approaching an obstacle?

**Solution**: The system implements **fail-safe emergency braking**:

```cpp
// In obstacle_safety.cpp, update() function:
if (obstStatus.sensorsHealthy == 0) {
    // FAIL-SAFE: If sensors unhealthy, apply emergency brake
    state.emergencyBrakeApplied = true;
    state.collisionImminent = true;
    state.speedReductionFactor = 0.0f;  // FULL STOP
    Alerts::play(Audio::AUDIO_EMERGENCIA);
    Logger::error("SENSOR FAILURE: Emergency brake applied (fail-safe)");
    return;
}
```

**When Triggered**:
- Sensor timeout (no data for 100ms)
- Excessive errors (>10 consecutive packet errors)
- Sensor initialization failure
- Checksum validation failures

**Behavior**:
- Emergency brake applied immediately
- Speed reduction factor = 0.0 (full stop)
- Audio alert every 3 seconds
- Error logged to system
- **Vehicle will stop completely until sensor recovers**

### 2. Sensor Health Monitoring

The TOFSense-M S sensor health is continuously monitored:

**Timeout Detection** (`obstacle_detection.cpp:261-273`):
```cpp
if (now - lastPacketMs > UART_READ_TIMEOUT_MS && lastPacketMs > 0) {
    sensorData[SENSOR_FRONT].healthy = false;
    sensorData[SENSOR_FRONT].minDistance = DISTANCE_INVALID;
    sensorData[SENSOR_FRONT].proximityLevel = LEVEL_INVALID;
    Logger::warn("TOFSense: Communication timeout");
}
```
- **Timeout**: 100ms (sensor transmits at ~100Hz)
- **Action**: Mark sensor unhealthy, set distance to INVALID
- **Result**: Safety system applies emergency brake

**Error Counting** (`obstacle_detection.cpp:247-254`):
```cpp
if (sensorData[SENSOR_FRONT].errorCount > MAX_CONSECUTIVE_ERRORS) {
    sensorData[SENSOR_FRONT].healthy = false;
    Logger::warn("TOFSense: Too many errors, marking sensor unhealthy");
}
```
- **Threshold**: 10 consecutive errors
- **Triggers on**: Checksum failures, invalid headers, malformed packets
- **Result**: Sensor marked unhealthy, emergency brake applied

**Automatic Recovery** (`obstacle_detection.cpp:109-115`):
```cpp
// Sensor recovery: Mark as healthy when valid data received
if (!sensor.healthy) {
    Logger::info("TOFSense: Sensor recovered, marking healthy");
}
sensor.healthy = true;
sensor.errorCount = 0;
```
- **Condition**: Valid packet received with correct checksum
- **Action**: Sensor marked healthy, error counter reset
- **Result**: Safety system releases emergency brake

### 3. 5-Zone Obstacle Detection

The system classifies obstacles into 5 zones with graduated response:

| Zone | Distance | Action | Speed Factor |
|------|----------|--------|--------------|
| 5 | <20cm | **EMERGENCY** | 0% (full stop) |
| 4 | 20-50cm | **CRITICAL** | 10-40% (hard brake) |
| 3 | 50-100cm | **WARNING** | 40-70% (moderate brake) |
| 2 | 100-150cm | **CAUTION** | 85-100% (gentle brake) |
| 1 | 150-400cm | **ALERT** | 100% (audio only) |

**Zone 5 - Emergency (<20cm)**:
- Full stop (0% speed)
- Emergency brake applied
- Audio alert: EMERGENCIA
- Logged as emergency event

**Zone 4 - Critical (20-50cm)**:
- Hard braking (10-40% speed)
- Linear interpolation based on distance
- Audio alert: EMERGENCIA
- Collision imminent flag set

**Zone 3 - Warning (50-100cm)**:
- **Child Reaction Detection Active**
- If child reacts (reduces pedal): 70% speed (soft assist)
- If no reaction: 40% speed (moderate brake)
- Audio alert: ERROR_GENERAL

**Zone 2 - Caution (100-150cm)**:
- If no child reaction: 85-100% gentle brake
- If child reacts: 100% speed (trust child)
- Adaptive Cruise Control may have priority

**Zone 1 - Alert (150-400cm)**:
- 100% speed allowed
- Audio alert only (less frequent)

### 4. Child Reaction Detection

**Purpose**: Detect if the child (driver) is reacting to obstacles

**Implementation** (`obstacle_safety.cpp:137-153`):
```cpp
float pedalReduction = state.lastPedalValue - pedalState.percent;
if (pedalReduction > CHILD_REACTION_THRESHOLD) {
    // Child reduced pedal
    state.lastPedalReductionMs = now;
    state.childReactionDetected = true;
} else if (pedalReduction <= 0.0f) {
    // Pedal increased - clear reaction
    state.childReactionDetected = false;
} else if (now - state.lastPedalReductionMs > CHILD_REACTION_WINDOW_MS) {
    // Outside reaction window - clear
    state.childReactionDetected = false;
}
```

**Parameters**:
- **Threshold**: 10% pedal reduction
- **Window**: 500ms
- **Reset**: Immediate if pedal increases

**Effect on Zones**:
- **Zone 3**: If reaction detected → 70% speed (soft), else 40% (hard)
- **Zone 2**: If reaction detected → 100% speed (trust), else 85-100% (brake)

**Safety Note**: If child doesn't react in Zones 3-4, system applies stronger braking.

### 5. Checksum Validation

Every UART packet is validated (`obstacle_detection.cpp:49-76`):

```cpp
// Validate checksum
uint8_t expectedChecksum = calculateChecksum(packetBuffer, POS_CHECKSUM);
uint8_t receivedChecksum = packetBuffer[POS_CHECKSUM];

if (expectedChecksum != receivedChecksum) {
    Logger::warnf("TOFSense: Checksum mismatch");
    System::logError(ERROR_CODE_CHECKSUM);
    return false;  // Packet rejected
}
```

**On Checksum Failure**:
- Packet discarded
- Error counter incremented
- If >10 consecutive failures → sensor marked unhealthy
- Emergency brake applied by safety system

### 6. Protocol Validation

Each packet is validated for:
- **Header**: Must be 0x57
- **Function**: Must be 0x00 (distance measurement)
- **Length**: Must be 0x02 (2 bytes data)
- **Checksum**: Sum of bytes 0-7

Invalid packets are rejected and don't update sensor state.

### 7. Distance Calibration

Allows fine-tuning sensor readings:

```cpp
int32_t calibratedDistance = distanceMm + sensor.offsetMm;
if (calibratedDistance < 0) calibratedDistance = 0;
if (calibratedDistance > DISTANCE_MAX) calibratedDistance = DISTANCE_INVALID;
```

**Features**:
- Positive/negative offset support
- Bounds checking (0 to 12000mm)
- Per-sensor calibration (future-proof for multiple sensors)

## Integration with Other Systems

### Traction Control
- Reads `speedReductionFactor` from safety system
- Applies factor to commanded speed
- Factor = 0.0 → full stop, Factor = 1.0 → no reduction

### Adaptive Cruise Control
- Has priority in Zones 2-3 (100-150cm)
- Safety overrides in Zones 4-5 (<50cm)
- Coordinates through `accHasPriority` flag

### Audio Alerts
- EMERGENCIA: Zones 5, 4, sensor failure
- ERROR_GENERAL: Zones 3, 2, 1
- Throttled to prevent spam (1-3 second intervals)

### Logging
- All sensor failures logged
- Emergency events logged
- Timeout events logged every 5 seconds
- Distance logged every 2 seconds

## Testing Scenarios

### Scenario 1: Normal Approach
1. Vehicle approaching obstacle
2. Zone 1 (150-400cm): Audio alert
3. Zone 2 (100-150cm): Gentle brake if no child reaction
4. Zone 3 (50-100cm): Moderate brake if no child reaction
5. Zone 4 (20-50cm): Hard brake
6. Zone 5 (<20cm): Full stop

**Expected**: Smooth deceleration, child can override by lifting pedal

### Scenario 2: Child Not Reacting
1. Obstacle detected in Zone 3
2. Child maintains pedal (no reduction >10%)
3. System applies 40% speed (moderate brake)
4. If obstacle gets closer: Zones 4-5 apply harder braking
5. Emergency stop at <20cm

**Expected**: System intervenes progressively stronger

### Scenario 3: Sensor Failure During Approach
1. Vehicle approaching obstacle
2. Sensor stops responding (timeout after 100ms)
3. `sensorsHealthy` = 0
4. **FAIL-SAFE**: Emergency brake applied immediately
5. Vehicle stops completely
6. Audio alert: EMERGENCIA
7. When sensor recovers: Normal operation resumes

**Expected**: Immediate emergency stop on sensor failure

### Scenario 4: Sensor Recovers
1. Sensor was unhealthy (timeout/errors)
2. Emergency brake active
3. Valid packet received
4. Sensor marked healthy
5. Emergency brake released
6. Normal operation resumes

**Expected**: Automatic recovery when sensor returns

### Scenario 5: Intermittent Errors
1. Occasional checksum errors (<10 consecutive)
2. Valid packets in between
3. Sensor remains healthy
4. Error counter resets on valid packet

**Expected**: System tolerates occasional errors

### Scenario 6: Sustained Errors
1. 10+ consecutive checksum errors
2. Sensor marked unhealthy
3. Emergency brake applied
4. Must receive valid packet to recover

**Expected**: Protection against corrupted data stream

## Safety Guarantees

✅ **No Sensor = No Movement**: If sensor fails, vehicle stops  
✅ **Fail-Safe by Default**: System errs on side of caution  
✅ **Automatic Recovery**: No manual intervention needed when sensor returns  
✅ **Progressive Response**: Graduated braking based on distance  
✅ **Child Override**: Child can reduce braking by lifting pedal  
✅ **Data Validation**: All packets validated before use  
✅ **Error Tolerance**: Handles occasional errors gracefully  
✅ **Timeout Protection**: Detects communication loss quickly (100ms)  

## Migration from VL53L5X

### Preserved Safety Features
✅ Emergency braking on sensor failure  
✅ Progressive braking zones  
✅ Child reaction detection  
✅ Parking assist  
✅ Collision avoidance  
✅ Speed reduction factor system  

### Enhanced Safety Features
✅ **Faster timeout detection**: 100ms (vs multiple seconds on I2C)  
✅ **Explicit fail-safe**: Emergency brake on sensor failure  
✅ **Automatic recovery**: Sensor recovers when data returns  
✅ **Checksum validation**: Every packet validated  
✅ **Error counting**: Tracks consecutive errors  
✅ **Better logging**: Clear failure/recovery messages  

### Changes
- Single sensor (front only) instead of 2 sensors (front/rear)
- UART protocol instead of I2C
- Single distance point instead of 8x8 grid
- Rear obstacle detection not available (can add second sensor later)

## Configuration

### Constants (obstacle_config.h)
```cpp
UART_READ_TIMEOUT_MS = 100       // Sensor timeout
MAX_CONSECUTIVE_ERRORS = 10      // Error threshold
DISTANCE_CRITICAL = 200          // Emergency zone (<20cm)
DISTANCE_WARNING = 500           // Critical zone (<50cm)
DISTANCE_CAUTION = 1000          // Warning zone (<100cm)
```

### Safety Config (obstacle_safety.cpp)
```cpp
ZONE_5_THRESHOLD = 200           // <20cm Emergency
ZONE_4_THRESHOLD = 500           // 20-50cm Critical
ZONE_3_THRESHOLD = 1000          // 50-100cm Warning
ZONE_2_THRESHOLD = 1500          // 100-150cm Caution
ZONE_1_THRESHOLD = 4000          // 150-400cm Alert
CHILD_REACTION_THRESHOLD = 10.0  // 10% pedal reduction
CHILD_REACTION_WINDOW_MS = 500   // 500ms reaction window
```

## Conclusion

The TOFSense-M S implementation provides **robust fail-safe behavior** that ensures:
1. ✅ Sensor failures trigger emergency braking
2. ✅ Communication timeouts detected quickly
3. ✅ Invalid data rejected through checksum validation
4. ✅ Automatic recovery when sensor returns
5. ✅ Progressive braking based on obstacle distance
6. ✅ Child reaction detection for smart assistance

**The system is safer than before** with explicit fail-safe logic that stops the vehicle if sensor data is lost.
