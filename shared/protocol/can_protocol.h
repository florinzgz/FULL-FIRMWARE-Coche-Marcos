#ifndef SHARED_PROTOCOL_CAN_PROTOCOL_H
#define SHARED_PROTOCOL_CAN_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CAN_PROTOCOL_VERSION 1u

enum {
  CAN_ID_HELLO = 0x100u,
  CAN_ID_HEARTBEAT = 0x101u,
  CAN_ID_ERROR_FRAME = 0x102u,
  CAN_ID_CMD_SETPOINT = 0x200u,
  CAN_ID_CMD_MODE_REQUEST = 0x201u,
  CAN_ID_CMD_ACTUATOR_TEST = 0x202u,
  CAN_ID_TELEMETRY_STATE = 0x300u,
  CAN_ID_SAFETY_EVENT = 0x301u,
  CAN_ID_CONTROL_STATUS = 0x302u,
  CAN_ID_DIAG_SNAPSHOT = 0x303u
};

typedef struct __attribute__((packed)) {
  uint16_t protocol_version;
  uint16_t firmware_version;
  uint16_t capability_flags;
  uint16_t reserved;
} can_hello_t;

typedef struct __attribute__((packed)) {
  uint32_t uptime_ms;
  uint16_t status_flags;
  uint16_t reserved;
} can_heartbeat_t;

typedef struct __attribute__((packed)) {
  int16_t throttle;
  int16_t brake;
  int16_t steering;
  uint8_t mode_request;
  uint8_t reserved;
} can_cmd_setpoint_t;

typedef struct __attribute__((packed)) {
  uint16_t requested_mode;
  uint16_t profile_id;
  uint16_t reserved;
  uint16_t crc16;
} can_cmd_mode_request_t;

typedef struct __attribute__((packed)) {
  uint16_t test_id;
  uint16_t parameter;
  uint16_t duration_ms;
  uint16_t reserved;
} can_cmd_actuator_test_t;

typedef struct __attribute__((packed)) {
  uint16_t speed_cm_per_s;
  int16_t steering_deg_x10;
  uint16_t current_a_x10;
  uint16_t temp_c_x10;
} can_telemetry_state_t;

typedef struct __attribute__((packed)) {
  uint16_t fault_code;
  uint8_t fault_level;
  uint8_t inhibit_state;
  uint16_t detail;
  uint16_t reserved;
} can_safety_event_t;

typedef struct __attribute__((packed)) {
  uint16_t active_mode;
  uint16_t limits_flags;
  uint16_t torque_limit;
  uint16_t reserved;
} can_control_status_t;

typedef struct __attribute__((packed)) {
  uint16_t sensor_flags;
  uint16_t relay_state;
  uint16_t error_bitmap;
  uint16_t reserved;
} can_diag_snapshot_t;

#ifdef __cplusplus
}
#endif

#endif
