#include "fan_controller.h"
#include "task_defines.h"
#include <iostream>

FanController::FanController(QueueHandle_t fan_speed_q, float prev_co2_target) : speed_queue(fan_speed_q) {
    this->reading_queue = xQueueCreate(5, sizeof(Reading));
    this->write_queue = xQueueCreate(5, sizeof(Command));
    this->co2_target.f32 = prev_co2_target;

    xTaskCreate(fan_control_task, "FanController", 512, this, TASK_PRIORITY_MEDIUM, NULL);
}

QueueHandle_t FanController::get_reading_queue_handle() const { return reading_queue; }

QueueHandle_t FanController::get_write_queue_handle() const { return write_queue; }

uint16_t FanController::get_speed() { return speed; }

float FanController::get_co2_target() { return co2_target.f32; }

void FanController::fan_control() {
    speed = 1000;
    set_speed(1000);
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

        Command command; // TODO: If we decide to do automatic mode only then we can change this
        if (xQueueReceive(write_queue, &command, pdMS_TO_TICKS(10))) {
            if (command.type == WriteType::CO2_TARGET) {
                co2_target.f32 = reading.value.f32;
            } else if (command.type == WriteType::FAN_SPEED && manual_mode) {
                set_speed(command.value.u32);
            }
        }

        if (co2 >= CO2_CRITICAL) {
            speed = FAN_MAX;
            set_speed(speed);
        } else {
            adjust_speed(); // tmp
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void FanController::set_speed(uint16_t speed) { xQueueSend(speed_queue, &speed, 0); }

// Check if the fan is spinning - two consecutive 0s should mean it's not spinning
void FanController::is_fan_spinning(const uint16_t &new_count) {
    if (new_count == counter && new_count == 0) {
        spinning = false;
    } else {
        spinning = true;
    }
}

float FanController::distance_to_target() const { return co2_target.f32 - co2; }

void FanController::adjust_speed() {
    float distance = distance_to_target();

    if (abs(distance) < CO2_TOLERANCE) {
        return;
    }

    float adjustment = distance * FAN_MAX / CO2_MAX;
    speed += adjustment / 2;
    if (speed > FAN_MAX) {
        speed = FAN_MAX;
    } else if (speed < FAN_MIN) {
        speed = FAN_MIN;
    }
    set_speed(speed);
    std::cout << "Adjusting fan speed to " << speed << std::endl;
}

FanSpeedReadHandler::FanSpeedReadHandler(std::shared_ptr<FanController> fanctrl) : fanctrl(fanctrl) {
    this->reading = {ReadingType::FAN_SPEED, {0}};
    xTaskCreate(read_fan_speed_task, "FanSpeedReadHandler", 256, this, TASK_PRIORITY_MEDIUM, NULL);
}

void FanSpeedReadHandler::get_reading() {
    reading.value.u16 = fanctrl->get_speed(); 
}

void FanSpeedReadHandler::read_fan_speed() {
    for (;;) {
        get_reading();
        send_reading();
        vTaskDelay(pdMS_TO_TICKS(reading_interval));
    }
}

CO2TargetReadHandler::CO2TargetReadHandler(std::shared_ptr<FanController> fanctrl) : fanctrl(fanctrl) {
    this->reading = {ReadingType::CO2_TARGET, {0}};
    xTaskCreate(read_co2_target_task, "CO2TargetReadHandler", 256, this, TASK_PRIORITY_MEDIUM, NULL);
}

void CO2TargetReadHandler::get_reading() {
    reading.value.f32 = fanctrl->get_co2_target();
}

void CO2TargetReadHandler::read_co2_target() {
    for (;;) {
        get_reading();
        send_reading();
        vTaskDelay(pdMS_TO_TICKS(reading_interval));
    }
}
