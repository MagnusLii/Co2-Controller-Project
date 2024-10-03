#include "fan_controller.h"
#include "task_defines.h"
#include <iostream>

FanController::FanController(QueueHandle_t fan_speed_q, float prev_co2_target) : speed_queue(fan_speed_q) {
    this->reading_queue = xQueueCreate(5, sizeof(Reading));
    this->write_queue = xQueueCreate(5, sizeof(Command));
    this->co2_target = prev_co2_target;

    xTaskCreate(fan_control_task, "FanController", 512, this, TASK_PRIORITY_MEDIUM, nullptr);
}

QueueHandle_t FanController::get_reading_queue_handle() const { return reading_queue; }

QueueHandle_t FanController::get_write_queue_handle() const { return write_queue; }

uint16_t FanController::get_speed() const { return speed; }

float FanController::get_co2_target() const { return co2_target; }

void FanController::fan_control() {
    set_speed(0);
    vTaskDelay(pdMS_TO_TICKS(500));
    for (;;) {
        Reading reading;
        if (xQueueReceive(reading_queue, &reading, pdMS_TO_TICKS(10))) {
            if (reading.type == ReadingType::CO2) {
                co2 = reading.value.f32;
            } else if (reading.type == ReadingType::FAN_COUNTER) {
                is_fan_spinning(reading.value.u16);
                counter = reading.value.u16;
            }
        }

        Command command;
        if (xQueueReceive(write_queue, &command, pdMS_TO_TICKS(10))) {
            if (command.type == WriteType::CO2_TARGET && !manual_mode) {
                co2_target = command.value.f32;
            } else if (command.type == WriteType::FAN_SPEED && manual_mode) {
                set_speed(command.value.u16);
            }
        }

        if (co2 >= CO2_CRITICAL) {
            uint16_t new_speed = FAN_MAX;
            set_speed(new_speed);
        } else {
            adjust_speed();
        }

        // TODO: Need to tune later with the actual machine
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void FanController::set_speed(uint16_t new_speed) {
    speed = new_speed;
    xQueueSend(speed_queue, &speed, 0);
}

// Check if the fan is spinning - two consecutive 0s should mean it's not
// spinning
void FanController::is_fan_spinning(const uint16_t &new_count) {
    if (new_count == counter && new_count == 0) {
        spinning = false;
    } else {
        spinning = true;
    }
}

float FanController::distance_to_target() const { return co2 - co2_target; }

// Automatic adjustment of the fan speed
// TODO: Might need more tuning with real machine
void FanController::adjust_speed() {
    const float distance = distance_to_target();

    if (std::abs(distance) < CO2_TOLERANCE) { return; }

    const float adjustment = distance * FAN_MAX / CO2_MAX;
    uint16_t new_speed = speed + adjustment / 2;
    if (new_speed >= FAN_MAX) {
        if (speed == FAN_MAX) { return; }
        new_speed = FAN_MAX;
    } else if (new_speed <= FAN_MIN) {
        if (speed == FAN_MIN) { return; }
        new_speed = FAN_MIN;
    }
    set_speed(new_speed);
    std::cout << "Adjusting fan speed to " << speed << std::endl;
}

void FanCtrlReadHandler::read_fanctrl_register() {
    for (;;) {
        get_reading();
        send_reading();
        vTaskDelay(pdMS_TO_TICKS(reading_interval));
    }
}

FanSpeedReadHandler::FanSpeedReadHandler(std::shared_ptr<FanController> fanctrl) {
    this->reading = {ReadingType::FAN_SPEED, {0}};
    this->fanctrl = fanctrl;
    xTaskCreate(read_fanctrl_register_task, "FanSpeedReadHandler", 256, this, TASK_PRIORITY_MEDIUM, nullptr);
}

void FanSpeedReadHandler::get_reading() { reading.value.u16 = fanctrl->get_speed(); }

CO2TargetReadHandler::CO2TargetReadHandler(std::shared_ptr<FanController> fanctrl) {
    this->reading = {ReadingType::CO2_TARGET, {0}};
    this->fanctrl = fanctrl;
    xTaskCreate(read_fanctrl_register_task, "CO2TargetReadHandler", 256, this, TASK_PRIORITY_MEDIUM, nullptr);
}

void CO2TargetReadHandler::get_reading() { reading.value.f32 = fanctrl->get_co2_target(); }
