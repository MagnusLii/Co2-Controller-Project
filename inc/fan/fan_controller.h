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
    [[nodiscard]] QueueHandle_t get_write_queue_handle() const;
    uint16_t get_speed();
    float get_co2_target();

  private:
    void fan_control();
    static void fan_control_task(void *pvParameters) {
        auto *fanctrl = static_cast<FanController *>(pvParameters);
        fanctrl->fan_control();
    }

    void set_speed(uint16_t speed);
    void is_fan_spinning(const uint16_t &new_count);
    float distance_to_target() const;

    QueueHandle_t reading_queue; // Used to receive readings from modbus registers
    QueueHandle_t write_queue; // We receive write requests from other tasks
    QueueHandle_t speed_queue; // This is where we send the fan speed changes - Fan Speed RegisterHandler
    uint16_t speed = 0;
    uint16_t counter = 0;
    float co2 = 0.0;
    union { uint32_t u32; float f32; } co2_target;
    bool spinning;

    // TODO: Do we do this?
    bool manual_mode = false; // if true, stop all automagical fan adjustments
};

// TODO: These next two could be almost completely merged if we want even more inheritance
class FanSpeedReadHandler : public ReadRegisterHandler {
  public:
    FanSpeedReadHandler(std::shared_ptr<FanController> fanctrl);
    void get_reading() override;

  private:
    void read_fan_speed();
    static void read_fan_speed_task(void *pvParameters) {
        auto *handler = static_cast<FanSpeedReadHandler *>(pvParameters);
        handler->read_fan_speed();
    }

    std::shared_ptr<FanController> fanctrl; 
};

class CO2TargetReadHandler : public ReadRegisterHandler {
  public:
    CO2TargetReadHandler(std::shared_ptr<FanController> fanctrl);
    void get_reading() override;

  private:
    void read_co2_target();
    static void read_co2_target_task(void *pvParameters) {
        auto *handler = static_cast<CO2TargetReadHandler *>(pvParameters);
        handler->read_co2_target();
    }

    std::shared_ptr<FanController> fanctrl; 
};

#endif // FAN_CONTROLLER_H
