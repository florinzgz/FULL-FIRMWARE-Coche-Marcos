#include "sensors/car_sensors.h"
#include "logger.h"
#include "motor_controller.h"
#include "wheel_speed_sensor.h"
#include "battery_monitor.h"
#include "steering_sensor.h"
#include "temperature_sensor.h"
#include "current_sensor.h"
#include "settings.h"       // 🔒 v2.10.2: Para constantes TEMP_WARN_MOTOR, CURR_MAX_WHEEL, MAX_RPM

namespace Sensors {

void CarSensors::init() {
    Logger::log("Initializing car sensors...");
    // Initialize all sensors
}

void CarSensors::update() {
    // Update sensor readings
}

float CarSensors::getMotorTemp() {
    return TemperatureSensor::readMotorTemp();
}

float CarSensors::getWheelCurrent() {
    return CurrentSensor::readWheelCurrent();
}

int CarSensors::getRPM() {
    return WheelSpeedSensor::readRPM();
}

} // namespace Sensors
