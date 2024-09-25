
#ifndef REGISTERHANDLER_H_
#define REGISTERHANDLER_H_

#include "ModbusRegister_32.h"
#include "PicoOsUart.h"
#include "queue.h"

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
    void send_reading(Reading reading);
    virtual void get_reading() = 0;

  private:
    std::vector<QueueHandle_t> subscribers;
};

class ModbusRegisterHandler : public RegisterHandler {
  public:
    ModbusRegisterHandler(std::shared_ptr<ModbusClient> client, uint8_t server_address,
                          uint16_t register_address, uint8_t nr_of_registers, bool holding_register,
                          ReadingType type);

  private:
    void get_reading() override;

    Reading reading;
    ModbusRegister32 modbus_register;
};

class I2CRegisterHandler : public RegisterHandler {
  public:
    I2CRegisterHandler() = default;

  private:
    void get_reading() override;

    Reading reading;
};

#endif /* REGISTERHANDLER_H_ */
