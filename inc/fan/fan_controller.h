#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#include "register_handler.h"
#include <memory>

class FanController {
  public:
    FanController();

  private:
    std::shared_ptr<ModbusReadHandler> fan_count_handler;
    std::shared_ptr<ModbusWriteHandler> fan_speed_handler;
    std::shared_ptr<ModbusReadHandler> co2_handler;
};

#endif // FAN_CONTROLLER_H
