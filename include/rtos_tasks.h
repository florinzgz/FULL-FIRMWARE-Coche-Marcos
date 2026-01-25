// rtos_tasks.h - FreeRTOS task management for dual-core ESP32-S3
// Core 0: Safety-critical tasks (SafetyManager, ControlManager, PowerManager)
// Core 1: HUD and telemetry tasks
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

namespace RTOSTasks {

// Task priorities (higher number = higher priority)
// Core 0 priorities - safety critical
constexpr UBaseType_t PRIORITY_SAFETY_MANAGER = 5;
constexpr UBaseType_t PRIORITY_CONTROL_MANAGER = 4;
constexpr UBaseType_t PRIORITY_POWER_MANAGER = 3;

// Core 1 priorities
constexpr UBaseType_t PRIORITY_HUD_MANAGER = 2;
constexpr UBaseType_t PRIORITY_TELEMETRY_MANAGER = 1;

// Stack sizes (in bytes)
constexpr uint32_t STACK_SIZE_SAFETY = 4096;
constexpr uint32_t STACK_SIZE_CONTROL = 4096;
constexpr uint32_t STACK_SIZE_POWER = 3072;
constexpr uint32_t STACK_SIZE_HUD = 8192;      // Larger for display operations
constexpr uint32_t STACK_SIZE_TELEMETRY = 3072;

// Core assignments
constexpr BaseType_t CORE_CRITICAL = 0; // Core 0 for motor control
constexpr BaseType_t CORE_GENERAL = 1;  // Core 1 for HUD/telemetry

// Task handles
extern TaskHandle_t safetyTaskHandle;
extern TaskHandle_t controlTaskHandle;
extern TaskHandle_t powerTaskHandle;
extern TaskHandle_t hudTaskHandle;
extern TaskHandle_t telemetryTaskHandle;

// Initialization
bool init();

// Task functions
void safetyTask(void *parameter);
void controlTask(void *parameter);
void powerTask(void *parameter);
void hudTask(void *parameter);
void telemetryTask(void *parameter);

// Suspend/resume for critical operations
void suspendNonCriticalTasks();
void resumeNonCriticalTasks();

} // namespace RTOSTasks
