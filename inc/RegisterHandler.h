
#ifndef REGISTERHANDLER_H_
#define REGISTERHANDLER_H_

#include "ModbusRegister_32.h"
#include "PicoOsUart.h"
#include "portmacro.h"
#include "queue.h"
#include "timers.h"

#include <algorithm>
#include <vector>

enum class ReadingType { CO2, TEMP, FAN_SPEED, FAN_COUNTER, PRESSURE };

struct Reading {
    ReadingType type;
    union {
        uint32_t u32;
        float f32;
        int32_t i32;
    } value;
};

class RegisterHandler {
  public:
    void add_subscriber(QueueHandle_t subscriber);
    void remove_subscriber(QueueHandle_t subscriber);
    void send_reading();

  protected:
    Reading reading;
    TickType_t last_reading = 0;

    static void send_reading_timer_callback(TimerHandle_t xTimer) {
        RegisterHandler *handler = static_cast<RegisterHandler *>(pvTimerGetTimerID(xTimer));
        handler->send_reading();
    }

  private:
    virtual void get_reading() = 0;

    std::vector<QueueHandle_t> subscribers;
};

class ModbusRegisterHandler : public RegisterHandler {
  public:
    ModbusRegisterHandler(std::shared_ptr<ModbusClient> client, uint8_t server_address,
                          uint16_t register_address, uint8_t nr_of_registers, bool holding_register,
                          ReadingType type);

  private:
    void get_reading() override;

    void mb_read();

    static void mb_read_task(void *pvParameters) {
        ModbusRegisterHandler *handler = static_cast<ModbusRegisterHandler *>(pvParameters);
        handler->mb_read();
    }

    ModbusRegister32 modbus_register;

};

// TODO:
class I2CRegisterHandler : public RegisterHandler {
  public:
    I2CRegisterHandler() = default;

  private:
    void get_reading() override;

    // I2C Reg
};

#endif /* REGISTERHANDLER_H_ */
