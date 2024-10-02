
#ifndef REGISTERHANDLER_H_
#define REGISTERHANDLER_H_

#include "FreeRTOS.h"
#include "modbus_register.h"
#include "portmacro.h"
#include "queue.h"
#include "timers.h"
#include "uart_instance.h"
#include "PicoI2C.h"
#include "pressure_register.h"

#include <algorithm>
#include <string>
#include <vector>

enum class ReadingType { CO2, CO2_TARGET, TEMPERATURE, REL_HUMIDITY, FAN_COUNTER, FAN_SPEED, PRESSURE, ROT_SW, CW, CCW, UNSET };

enum class WriteType { CO2_TARGET, FAN_SPEED, UNSET };

struct Reading {
    ReadingType type;
    union {
        uint32_t u32;
        float f32;
        int32_t i32;
        uint16_t u16;
        int16_t i16;
    } value;
};

// TODO: Pretty sure we need to implement something like this if we want a manual fan speed adjust mode in addition to the required CO2 target based mode
struct Command {
    WriteType type;
    union {
        uint32_t u32; // Fan speed?
        float f32; // CO2 target
    } value;
};

class RegisterHandler {
  public:
    virtual ~RegisterHandler() = default;
    std::string get_name();

  protected:
    std::string name;
};

class ReadRegisterHandler : public RegisterHandler {
  public:
    void add_subscriber(QueueHandle_t subscriber);
    void remove_subscriber(QueueHandle_t subscriber);
    void send_reading();
    void send_reading_from_isr();
    [[nodiscard]] ReadingType get_type() const;
    virtual void get_reading() = 0;

  protected:
    Reading reading{ReadingType::UNSET, {0}};

    const uint16_t reading_interval = 1000; // ms

  private:
    std::vector<QueueHandle_t> subscribers;
};

class ModbusReadHandler final : public ReadRegisterHandler {
  public:
    ModbusReadHandler(shared_modbus controller, uint8_t device_address, uint16_t register_address,
                      uint8_t nr_of_registers, bool holding_register, ReadingType type,
                      const std::string& name = "ModbusReadHandler");
    void get_reading() override;

  private:
    void mb_read();
    static void mb_read_task(void *pvParameters) {
        auto *handler = static_cast<ModbusReadHandler *>(pvParameters);
        handler->mb_read();
    }

    shared_modbus controller;
    ReadRegister reg;
};


class WriteRegisterHandler : public RegisterHandler {
  public:
    [[nodiscard]] QueueHandle_t get_write_queue_handle() const;
    virtual void write_to_reg(uint32_t value) = 0;

  protected:
    QueueHandle_t write_queue = nullptr;
    WriteType type = WriteType::UNSET;
};

class ModbusWriteHandler : public WriteRegisterHandler {
  public:
    ModbusWriteHandler(shared_modbus controller, uint8_t device_address, uint16_t register_address,
                       uint8_t nr_of_registers, WriteType type, const std::string& name = "ModbusWriteHandler");

    void write_to_reg(uint32_t value) override;

  private:
    void mb_write();
    static void mb_write_task(void *pvParameters) {
        auto *handler = static_cast<ModbusWriteHandler *>(pvParameters);
        handler->mb_write();
    }

    shared_modbus controller;
    WriteRegister reg;
};

class I2CHandler final : public ReadRegisterHandler {
  public:
    I2CHandler(shared_i2c i2c_i, uint8_t device_address, ReadingType rtype = ReadingType::UNSET,
               const std::string &name = "I2CHandler");

    void get_reading() override;
  private:
    void i2c_read();
    static void i2c_read_task(void *pvParameters) {
      auto *handler = static_cast<I2CHandler *>(pvParameters);
      handler->i2c_read();
    }

    shared_i2c i2c;
    PressureRegister reg; // Only I2C dev reg we have so lets go and just specify it
};

#endif /* REGISTERHANDLER_H_ */
