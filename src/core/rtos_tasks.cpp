// rtos_tasks.cpp - FreeRTOS task implementation for dual-core operation
#include "rtos_tasks.h"
#include "logger.h"
#include "managers/ControlManager.h"
#include "managers/HUDManager.h"
#include "managers/PowerManager.h"
#include "managers/SafetyManager.h"
#include "managers/SensorManager.h"
#include "managers/TelemetryManager.h"
#include "shared_data.h"
#include "watchdog.h"

namespace RTOSTasks {

// Task handles
TaskHandle_t safetyTaskHandle = nullptr;
TaskHandle_t controlTaskHandle = nullptr;
TaskHandle_t powerTaskHandle = nullptr;
TaskHandle_t hudTaskHandle = nullptr;
TaskHandle_t telemetryTaskHandle = nullptr;

bool init() {
  Logger::info("RTOSTasks: Creating FreeRTOS tasks for dual-core operation");

  BaseType_t result;

  // Core 0 tasks - Safety critical
  result = xTaskCreatePinnedToCore(
      safetyTask,           // Task function
      "SafetyTask",         // Name
      STACK_SIZE_SAFETY,    // Stack size
      nullptr,              // Parameters
      PRIORITY_SAFETY_MANAGER, // Priority
      &safetyTaskHandle,    // Handle
      CORE_CRITICAL         // Core 0
  );
  if (result != pdPASS) {
    Logger::error("RTOSTasks: Failed to create SafetyTask");
    return false;
  }

  result = xTaskCreatePinnedToCore(
      controlTask,          // Task function
      "ControlTask",        // Name
      STACK_SIZE_CONTROL,   // Stack size
      nullptr,              // Parameters
      PRIORITY_CONTROL_MANAGER, // Priority
      &controlTaskHandle,   // Handle
      CORE_CRITICAL         // Core 0
  );
  if (result != pdPASS) {
    Logger::error("RTOSTasks: Failed to create ControlTask");
    return false;
  }

  result = xTaskCreatePinnedToCore(
      powerTask,            // Task function
      "PowerTask",          // Name
      STACK_SIZE_POWER,     // Stack size
      nullptr,              // Parameters
      PRIORITY_POWER_MANAGER, // Priority
      &powerTaskHandle,     // Handle
      CORE_CRITICAL         // Core 0
  );
  if (result != pdPASS) {
    Logger::error("RTOSTasks: Failed to create PowerTask");
    return false;
  }

  // Core 1 tasks - HUD and telemetry
  result = xTaskCreatePinnedToCore(
      hudTask,              // Task function
      "HUDTask",            // Name
      STACK_SIZE_HUD,       // Stack size
      nullptr,              // Parameters
      PRIORITY_HUD_MANAGER, // Priority
      &hudTaskHandle,       // Handle
      CORE_GENERAL          // Core 1
  );
  if (result != pdPASS) {
    Logger::error("RTOSTasks: Failed to create HUDTask");
    return false;
  }

  result = xTaskCreatePinnedToCore(
      telemetryTask,        // Task function
      "TelemetryTask",      // Name
      STACK_SIZE_TELEMETRY, // Stack size
      nullptr,              // Parameters
      PRIORITY_TELEMETRY_MANAGER, // Priority
      &telemetryTaskHandle, // Handle
      CORE_GENERAL          // Core 1
  );
  if (result != pdPASS) {
    Logger::error("RTOSTasks: Failed to create TelemetryTask");
    return false;
  }

  Logger::info("RTOSTasks: All tasks created successfully");
  Logger::infof("RTOSTasks: Core 0 (critical): Safety(%d), Control(%d), Power(%d)",
                PRIORITY_SAFETY_MANAGER, PRIORITY_CONTROL_MANAGER,
                PRIORITY_POWER_MANAGER);
  Logger::infof("RTOSTasks: Core 1 (general): HUD(%d), Telemetry(%d)",
                PRIORITY_HUD_MANAGER, PRIORITY_TELEMETRY_MANAGER);

  return true;
}

void safetyTask(void *parameter) {
  (void)parameter;
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t frequency = pdMS_TO_TICKS(10); // 100 Hz

  Logger::info("SafetyTask: Started on Core 0");

  while (true) {
    // Update safety systems with heartbeat monitoring
    SafetyManager::updateWithHeartbeat();

    // Feed watchdog from safety task
    Watchdog::feed();

    // Wait for next cycle
    vTaskDelayUntil(&lastWakeTime, frequency);
  }
}

void controlTask(void *parameter) {
  (void)parameter;
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t frequency = pdMS_TO_TICKS(10); // 100 Hz

  Logger::info("ControlTask: Started on Core 0");

  while (true) {
    // Update control systems
    ControlManager::update();

    // Update heartbeat in shared data
    SharedData::ControlState state;
    if (SharedData::readControlState(state)) {
      state.lastHeartbeat = millis();
      SharedData::writeControlState(state);
    }

    // Wait for next cycle
    vTaskDelayUntil(&lastWakeTime, frequency);
  }
}

void powerTask(void *parameter) {
  (void)parameter;
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t frequency = pdMS_TO_TICKS(100); // 10 Hz

  Logger::info("PowerTask: Started on Core 0");

  while (true) {
    // Update power management
    PowerManager::update();

    // Update sensor data with non-blocking I2C (runs at lower priority)
    SensorManager::updateNonBlocking();

    // Wait for next cycle
    vTaskDelayUntil(&lastWakeTime, frequency);
  }
}

void hudTask(void *parameter) {
  (void)parameter;
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t frequency = pdMS_TO_TICKS(33); // ~30 FPS

  Logger::info("HUDTask: Started on Core 1");

  while (true) {
    // Update HUD display
    HUDManager::update();

    // Wait for next cycle
    vTaskDelayUntil(&lastWakeTime, frequency);
  }
}

void telemetryTask(void *parameter) {
  (void)parameter;
  TickType_t lastWakeTime = xTaskGetTickCount();
  const TickType_t frequency = pdMS_TO_TICKS(100); // 10 Hz

  Logger::info("TelemetryTask: Started on Core 1");

  while (true) {
    // Update telemetry
    TelemetryManager::update();

    // Wait for next cycle
    vTaskDelayUntil(&lastWakeTime, frequency);
  }
}

void suspendNonCriticalTasks() {
  if (hudTaskHandle != nullptr) {
    vTaskSuspend(hudTaskHandle);
  }
  if (telemetryTaskHandle != nullptr) {
    vTaskSuspend(telemetryTaskHandle);
  }
  Logger::info("RTOSTasks: Non-critical tasks suspended");
}

void resumeNonCriticalTasks() {
  if (hudTaskHandle != nullptr) {
    vTaskResume(hudTaskHandle);
  }
  if (telemetryTaskHandle != nullptr) {
    vTaskResume(telemetryTaskHandle);
  }
  Logger::info("RTOSTasks: Non-critical tasks resumed");
}

} // namespace RTOSTasks
