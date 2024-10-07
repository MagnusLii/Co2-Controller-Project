#include "fan_controller.h"
#include "hardware/gpio.h"
#include "register_handler.h"
#include "task_defines.h"
#include <cmath>
#include <iostream>

#define COUNTER_CLOCKWISE 2
#define CLOCKWISE 1

FanController::FanController(QueueHandle_t fan_speed_q) : speed_queue(fan_speed_q) {
    this->reading_queue = xQueueCreate(5, sizeof(Reading));
    this->write_queue = xQueueCreate(20, sizeof(Command));

    xTaskCreate(fan_control_task, "FanController", 512, this, TaskPriority::MEDIUM, nullptr);
    xTaskCreate(fan_read_task, "FanReader", 512, this, TaskPriority::MEDIUM, nullptr);

    gpio_init(VALVE_PIN);
    gpio_pull_down(VALVE_PIN);

    this->close_valve = xTimerCreate("CloseValve", pdMS_TO_TICKS(2000), false, nullptr, close_valve_callback);
    this->clear_valve_block = xTimerCreate("ClearValveBlock", pdMS_TO_TICKS(60000), false, nullptr, clear_valve_block_callback);
    vTimerSetTimerID(clear_valve_block, this);
}

void FanController::set_initial_values(float co2_target, uint16_t fan_speed, bool is_manual) {
    this->co2_target = co2_target;
    this->speed = fan_speed;
    this->manual_mode = is_manual;
}

QueueHandle_t FanController::get_reading_queue_handle() const { return reading_queue; }

QueueHandle_t FanController::get_write_queue_handle() const { return write_queue; }

uint16_t FanController::get_speed() const { return speed; }

float FanController::get_co2_target() const { return co2_target; }

bool FanController::get_mode() const { return manual_mode; }

void FanController::fan_control() {
    set_speed(0);
    xTimerStart(clear_valve_block, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
    for (;;) {
        Reading reading;
        while (xQueueReceive(reading_queue, &reading, pdMS_TO_TICKS(10))) {
            if (reading.type == ReadingType::CO2) {
                co2 = reading.value.f32;
            } else if (reading.type == ReadingType::FAN_COUNTER) {
                is_fan_spinning(reading.value.u16);
                counter = reading.value.u16;
            }
        }
        if (!manual_mode) {
            if (co2 >= CO2_CRITICAL) {
                uint16_t new_speed = FAN_MAX;
                set_speed(new_speed);
            } else {
                adjust_speed();
            }
        }

        // TODO: Need to tune later with the actual machine
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void FanController::fan_read() {
    Command command;
    bool changed = false;
    while (true) {
        if (xQueueReceive(write_queue, &command, portMAX_DELAY)) {
            if (command.type == WriteType::CO2_TARGET && !manual_mode && changed) {
                changed = false;
                co2_target = command.value.f32;
                std::cout << "co2 target: " << command.value.f32 << std::endl;
            } else if (command.type == WriteType::FAN_SPEED && manual_mode && changed) {
                changed = false;
                set_speed(command.value.u16);
                std::cout << "speed target: " << command.value.u16 << std::endl;
            } else if (command.type == WriteType::MODE_SET) {
                changed = true;
                if (command.value.u16 == 0) manual_mode = false;
                if (command.value.u16 == 1) manual_mode = true;
            }
        }
    }
}

void FanController::set_speed(uint16_t new_speed) {
    speed = new_speed;
    xQueueSend(speed_queue, &speed, 0);
}

// Check if the fan is spinning - two consecutive 0s should mean it's not
void FanController::is_fan_spinning(const uint16_t &new_count) {
    if (new_count == counter && new_count == 0) {
        spinning = false;
        //std::cout << "Fan is not spinning" << std::endl;
    } else {
        spinning = true;
        //std::cout << "Fan is spinning" << std::endl;
    }
}

float FanController::distance_to_target() const { return co2 - co2_target; }

// Automatic adjustment of the fan speed
// TODO: Definitely need more tuning with real machine
void FanController::adjust_speed() {
    const float distance = distance_to_target();
    uint16_t new_speed;
    if (distance <= 0) {
        new_speed = 0;
        if (!valve_opened_recently) {
            gpio_put(VALVE_PIN, 1);
            valve_opened_recently = true;
            xTimerStart(close_valve, 0);
        }
    } else {
        new_speed = (1000/(2000-co2_target))*std::round(distance)+200;
    }


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

FanSpeedReadHandler::FanSpeedReadHandler(std::shared_ptr<FanController> fanctrl, const std::string &name) {
    this->reading = {ReadingType::FAN_SPEED, {0}};
    this->fanctrl = fanctrl;
    this->name = name;
    xTaskCreate(read_fanctrl_register_task, name.c_str(), 256, this, TaskPriority::MEDIUM, nullptr);
}

void FanSpeedReadHandler::get_reading() { reading.value.u16 = fanctrl->get_speed(); }

CO2TargetReadHandler::CO2TargetReadHandler(std::shared_ptr<FanController> fanctrl, const std::string &name) {
    this->reading = {ReadingType::CO2_TARGET, {0}};
    this->fanctrl = fanctrl;
    this->name = name;
    xTaskCreate(read_fanctrl_register_task, name.c_str(), 256, this, TaskPriority::MEDIUM, nullptr);
}

void CO2TargetReadHandler::get_reading() { reading.value.f32 = fanctrl->get_co2_target(); }

ModeReadHandler::ModeReadHandler(std::shared_ptr<FanController> fanctrl, const std::string &name) {
    this->reading = {ReadingType::MODE, {0}};
    this->fanctrl = fanctrl;
    this->name = name;
    xTaskCreate(read_fanctrl_register_task, name.c_str(), 256, this, TaskPriority::MEDIUM, nullptr);
}

void ModeReadHandler::get_reading() { reading.value.u16 = fanctrl->get_mode(); }


