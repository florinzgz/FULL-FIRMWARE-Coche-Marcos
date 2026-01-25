# FreeRTOS Multitasking Architecture v2.18.0

## Overview
This document describes the FreeRTOS multitasking architecture implemented for the ESP32-S3 based vehicle control system. The architecture separates safety-critical motor control tasks from HUD and telemetry tasks across two CPU cores.

## Hardware Configuration
- **MCU**: ESP32-S3 (N16R8) with 16MB Flash and 8MB PSRAM Octal
- **Core 0**: Dedicated to safety-critical motor control
- **Core 1**: Dedicated to HUD rendering and telemetry

## Task Architecture

### Core 0 Tasks (Safety-Critical)

#### 1. SafetyTask (Priority 5 - Highest)
- **Frequency**: 100 Hz (10ms cycle)
- **Stack Size**: 4096 bytes
- **Responsibilities**:
  - Monitor ABS system
  - Monitor TCS system
  - Monitor obstacle safety
  - **CRITICAL**: Heartbeat failsafe monitoring
  - Emergency motor stop on heartbeat timeout (>200ms)
- **Thread Safety**: Reads ControlState via mutex

#### 2. ControlTask (Priority 4)
- **Frequency**: 100 Hz (10ms cycle)
- **Stack Size**: 4096 bytes
- **Responsibilities**:
  - Update traction control
  - Update steering motor
  - Update relays
  - **CRITICAL**: Update heartbeat timestamp
- **Thread Safety**: Writes ControlState via mutex

#### 3. PowerTask (Priority 3)
- **Frequency**: 10 Hz (100ms cycle)
- **Stack Size**: 3072 bytes
- **Responsibilities**:
  - Update power management
  - Update sensor readings (I2C operations)
  - Implement non-blocking I2C with timeouts
- **Thread Safety**: Writes SensorData via mutex

### Core 1 Tasks (General Purpose)

#### 4. HUDTask (Priority 2)
- **Frequency**: 30 Hz (~33ms cycle)
- **Stack Size**: 8192 bytes (larger for display buffers)
- **Responsibilities**:
  - Update display rendering
  - Handle touch input
  - Display system status
- **Thread Safety**: Reads SensorData and ControlState via mutex
- **Memory**: Display buffers allocated in PSRAM

#### 5. TelemetryTask (Priority 1 - Lowest)
- **Frequency**: 10 Hz (100ms cycle)
- **Stack Size**: 3072 bytes
- **Responsibilities**:
  - Collect telemetry data
  - Process data logging
  - Network communication (if enabled)
- **Thread Safety**: Reads SensorData via mutex

## Thread-Safe Data Sharing

### SharedData Module
Provides mutex-protected access to shared data structures:

#### SensorData Structure
- Current sensors (6x INA226)
- Temperature sensors (4x DS18B20)
- Wheel sensors (4x hall effect)
- Input devices (pedal, steering, shifter, buttons)
- I2C bus health status
- Timestamps for staleness detection

#### ControlState Structure
- Motor activity status
- Target speed and steering
- **CRITICAL**: lastHeartbeat timestamp

### Mutex Usage
- `sensorDataMutex`: Protects SensorData access
- `controlStateMutex`: Protects ControlState access
- `i2cMutex`: Protects I2C bus access (existing)
- Timeout: 50ms for data access, prevents deadlocks

## Safety Features

### Heartbeat Failsafe
- **Requirement**: ControlManager must report activity within 200ms
- **Implementation**:
  - ControlTask updates `lastHeartbeat` timestamp every cycle (10ms)
  - SafetyTask monitors heartbeat age every cycle (10ms)
  - If heartbeat age > 200ms: EMERGENCY MOTOR STOP
- **Recovery**: Automatic when heartbeat resumes

### Non-Blocking I2C
- **Timeout**: 50ms per I2C operation
- **Error Handling**:
  - Track consecutive I2C errors
  - After 5 consecutive errors: Mark I2C bus as unhealthy
  - Log errors every 5 seconds (prevents log spam)
  - System continues operation with degraded sensor data
- **Benefits**: Prevents I2C device failure from freezing control loop

## Memory Optimization

### PSRAM Allocation Strategy
1. **Display Buffers**: Allocated in PSRAM using `MALLOC_CAP_SPIRAM`
2. **FastLED Buffers**: Allocated in PSRAM using `MALLOC_CAP_SPIRAM`
3. **Task Stacks**: Remain in internal RAM for performance
4. **Shared Data**: Remains in internal RAM for fast access

### Memory Layout
- **Internal RAM**: Task stacks, shared data, critical variables
- **PSRAM**: Display framebuffers, LED buffers, large arrays
- **Benefits**: 
  - More free internal RAM for task stacks
  - Reduced risk of stack overflow
  - Improved system stability

## Migration from Single-Threaded

### Key Changes
1. **main.cpp loop()**: Now yields to scheduler, minimal work
2. **Manager Updates**: Moved to dedicated FreeRTOS tasks
3. **Data Access**: All cross-core access uses mutexes
4. **Timing**: Precise task timing via `vTaskDelayUntil()`

### Backward Compatibility
- **STANDALONE_DISPLAY**: Mode still uses single-threaded loop
- **Safe Mode**: Disables FreeRTOS tasks, falls back to basic operation
- **Existing Code**: Managers unchanged, wrapped by tasks

## Performance Characteristics

### Response Times
- **Control Loop**: 10ms (100 Hz) guaranteed on Core 0
- **Safety Monitor**: 10ms (100 Hz) guaranteed on Core 0
- **Sensor Update**: 100ms (10 Hz) on Core 0
- **Display Update**: 33ms (~30 FPS) on Core 1
- **Heartbeat Check**: Every 10ms, timeout at 200ms

### CPU Load Distribution
- **Core 0**: ~60-70% (control + safety + sensors)
- **Core 1**: ~40-50% (HUD rendering + telemetry)
- **Benefits**: Balanced load, no single core overload

## Testing and Validation

### Verification Points
1. Task creation successful (all 5 tasks)
2. Core affinity correct (check with `xPortGetCoreID()`)
3. Heartbeat failsafe triggers at >200ms
4. I2C timeout doesn't freeze control loop
5. Memory usage within limits (monitor PSRAM/heap)
6. No race conditions (run for extended periods)

### Monitoring
- Periodic memory logging (every 30s)
- Heartbeat status logging (every 1s if failsafe active)
- I2C health status logging
- Task priority/core logging at startup

## Future Enhancements
1. Dynamic priority adjustment based on system state
2. Task performance profiling and optimization
3. Additional safety monitors (temperature, current)
4. Enhanced telemetry with task statistics

## Author
FreeRTOS architecture implemented in v2.18.0 for ESP32-S3 dual-core operation.
