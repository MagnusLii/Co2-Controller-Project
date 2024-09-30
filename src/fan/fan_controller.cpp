#include "fan_controller.h"
#include "task_defines.h"

FanController::FanController(QueueHandle_t fan_speed_q, uint32_t prev_co2_target) : speed_queue(fan_speed_q) {
    this->reading_queue = xQueueCreate(5, sizeof(Reading));
    this->co2_target.u32 = prev_co2_target;

    xTaskCreate(fan_control_task, "FanController", 512, this, TASK_PRIORITY_MEDIUM, NULL);
}

QueueHandle_t FanController::get_reading_queue_handle() const { return reading_queue; }

void FanController::fan_control() {
    for (;;) {
        Reading reading;
        if (xQueueReceive(reading_queue, &reading, pdMS_TO_TICKS(100))) {
            if (reading.type == ReadingType::CO2) {
                co2 = reading.value.f32;
            } else if (reading.type == ReadingType::FAN_COUNTER) {
                is_fan_spinning(reading.value.u16);
                counter = reading.value.u16;
            }
        }

        if (co2 >= CO2_CRITICAL) {
            speed = FAN_MAX;
            set_speed(speed);
        } else if (co2 > co2_target.f32) { // Probably need some tolerance value for these
            // TODO: Some automagic adjustments
        } else if (co2 < co2_target.f32) {
            // TODO: Some automagic adjustments
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
