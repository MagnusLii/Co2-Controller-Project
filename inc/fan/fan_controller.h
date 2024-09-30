#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#include "register_handler.h"

#include <memory>

#define CO2_MIN 0
#define CO2_MAX 1500
#define CO2_CRITICAL 2000
#define FAN_MIN 0
#define FAN_MAX 1000

class FanController {
  public:
    FanController(QueueHandle_t fan_speed_q, uint32_t prev_co2_target);
    [[nodiscard]] QueueHandle_t get_reading_queue_handle() const;

  private:
    void fan_control();
    static void fan_control_task(void *pvParameters) {
        auto *fanctrl = static_cast<FanController *>(pvParameters);
        fanctrl->fan_control();
    }

    void set_speed(uint16_t speed);
    void is_fan_spinning(const uint16_t &new_count);
    float distance_to_target() const;

    QueueHandle_t reading_queue; // Used to receive readings from modbus regs
    QueueHandle_t speed_queue; // Write handlers queue that we will send fan speeds to
    uint16_t speed = 0;
    uint16_t counter = 0;
    float co2 = 0.0;
    union { uint32_t u32; float f32; } co2_target;
    bool spinning;
};

#endif // FAN_CONTROLLER_H
