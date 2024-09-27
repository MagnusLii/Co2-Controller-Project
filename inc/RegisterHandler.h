
#ifndef REGISTERHANDLER_H_
#define REGISTERHANDLER_H_

#include "portmacro.h"
#include "queue.h"
#include "timers.h"
#include "uart_instance.h"
#include "modbus_register.h"

#include <algorithm>
#include <vector>

enum class ReadingType { CO2, TEMP, FAN_SPEED, FAN_COUNTER, PRESSURE, UNSET };

struct Reading {
    ReadingType type;
    union {
        uint32_t u32;
        float f32;
        int32_t i32;
    } value;
};

class ReadRegisterHandler {
  public:
    virtual ~ReadRegisterHandler() = default;
    void add_subscriber(QueueHandle_t subscriber);
    void remove_subscriber(QueueHandle_t subscriber);
    void send_reading();

  protected:
    Reading reading{ ReadingType::UNSET, 0};
    TickType_t last_reading = 0;

    static void send_reading_timer_callback(TimerHandle_t xTimer) {
        ReadRegisterHandler *handler = static_cast<ReadRegisterHandler *>(pvTimerGetTimerID(xTimer));
        handler->send_reading();
    }

  private:
    virtual void get_reading() = 0;

    std::vector<QueueHandle_t> subscribers;
};

class ModbusReadHandler : public ReadRegisterHandler {
  public:
    ModbusReadHandler(shared_modbus client, uint8_t server_address,
                          uint16_t register_address, uint8_t nr_of_registers, bool holding_register,
                          ReadingType type);

  private:
    void get_reading() override;

    void mb_read();

    static void mb_read_task(void *pvParameters) {
        ModbusReadHandler *handler = static_cast<ModbusReadHandler *>(pvParameters);
        handler->mb_read();
    }

    ReadRegister register;
};

// TODO:
class I2CReadHandler : public ReadRegisterHandler {
  public:
    I2CReadHandler() = default;

  private:
    void get_reading() override;

    // I2C Reg
};

#endif /* REGISTERHANDLER_H_ */
