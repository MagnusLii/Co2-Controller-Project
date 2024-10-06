#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#include "register_handler.h"

#include <memory>
#include <iostream>

#define CO2_MIN       0
#define CO2_MAX       1500
#define CO2_CRITICAL  2000
#define CO2_TOLERANCE 5
#define FAN_MIN       0
#define FAN_MAX       1000
#define FAN_STARTUP   300

class FanController {
  public:
    FanController(QueueHandle_t fan_speed_q);
    [[nodiscard]] QueueHandle_t get_reading_queue_handle() const;
    [[nodiscard]] QueueHandle_t get_write_queue_handle() const;
    uint16_t get_speed() const;
    float get_co2_target() const;
    bool get_mode() const;
    void set_initial_values(float co2_target, uint16_t fan_speed, bool is_manual);

  private:
    void fan_control();
    static void fan_control_task(void *pvParameters) {
        auto *fanctrl = static_cast<FanController *>(pvParameters);
        fanctrl->fan_control();
    }
    void fan_read();
    static void fan_read_task(void *pvParameters) {
        auto *fanctrl = static_cast<FanController *>(pvParameters);
        fanctrl->fan_read();
    }

    void set_speed(uint16_t speed);
    void is_fan_spinning(const uint16_t &new_count);
    float distance_to_target() const;
    void adjust_speed();

    QueueHandle_t reading_queue; // Receive readings from modbus registers
    QueueHandle_t write_queue;   // Receive write requests from other tasks
    QueueHandle_t speed_queue;   // This is where we send the fan speed changes -
                                 // Fan Speed RegisterHandler
    uint16_t speed = 0;
    uint16_t counter = 0;
    float co2 = 0.0;
    float co2_target;
    bool spinning;
    const uint16_t fan_starter = FAN_STARTUP;

    // TODO: Do we do this?
    bool manual_mode = false; // if true, stop all automagical fan adjustments
};

class FanCtrlReadHandler : public ReadRegisterHandler {
  protected:
    void read_fanctrl_register();
    static void read_fanctrl_register_task(void *pvParameters) {
        auto *handler = static_cast<FanCtrlReadHandler *>(pvParameters);
        handler->read_fanctrl_register();
    }

    std::shared_ptr<FanController> fanctrl;
};

class FanSpeedReadHandler : public FanCtrlReadHandler {
  public:
    FanSpeedReadHandler(std::shared_ptr<FanController> fanctrl, const std::string &name = "Fan Speed Read");
    void get_reading() override;
};

class CO2TargetReadHandler : public FanCtrlReadHandler {
  public:
    CO2TargetReadHandler(std::shared_ptr<FanController> fanctrl, const std::string &name = "CO2 Target Read");
    void get_reading() override;
};

class ModeReadHandler : public FanCtrlReadHandler {
  public:
    ModeReadHandler(std::shared_ptr<FanController> fanctrl, const std::string &name = "Mode Read");
    void get_reading() override;
};

#endif // FAN_CONTROLLER_H
