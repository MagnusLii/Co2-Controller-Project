
#ifndef REGISTERHANDLER_H_
#define REGISTERHANDLER_H_

#include "FreeRTOS.h"
#include "PicoI2C.h"
#include "modbus_register.h"
#include "portmacro.h"
#include "pressure_register.h"
#include "queue.h"
#include "timers.h"

#include <algorithm>
#include <string>
#include <vector>

enum class ReadingType {
    UNSET,
    CO2,
    TEMPERATURE,
    REL_HUMIDITY,
    FAN_SPEED,
    CO2_TARGET,
    FAN_COUNTER,
    PRESSURE,
    MODE
};

enum class WriteType {
    UNSET,
    CO2_TARGET,
    FAN_SPEED,
    MODE_SET,
    TOGGLE,
};

struct Reading {
    ReadingType type;
    union {
        uint32_t u32; // Float readings get initially pulled as uint32_t
        float f32;    // CO2, CO2 target, Temperature, Humidity
        int32_t i32;  // Probably not used
        uint16_t u16; // Fan speed, Fan counter, Mode set
        int16_t i16;  // Pressure
    } value;
};

struct Command {
    WriteType type;
    union {
        float f32;    // CO2 target
        uint16_t u16; // Fan speed
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

  protected:
    virtual void get_reading() = 0;

    Reading reading{ReadingType::UNSET, {0}};
    const uint16_t reading_interval = 1000; // ms

  private:
    std::vector<QueueHandle_t> subscribers;
};

class ModbusReadHandler final : public ReadRegisterHandler {
  public:
    ModbusReadHandler(shared_modbus controller, uint8_t device_address, uint16_t register_address,
                      uint8_t nr_of_registers, bool holding_register, ReadingType type,
                      const std::string &name = "ModbusReadHandler");

  private:
    void get_reading() override;

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

  protected:
    virtual void write_to_reg(uint32_t value) = 0;

    QueueHandle_t write_queue = nullptr;
    WriteType type = WriteType::UNSET;
};

class ModbusWriteHandler : public WriteRegisterHandler {
  public:
    ModbusWriteHandler(shared_modbus controller, uint8_t device_address, uint16_t register_address,
                       uint8_t nr_of_registers, WriteType type, const std::string &name = "ModbusWriteHandler");

  private:
    void write_to_reg(uint32_t value) override;

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

  private:
    void get_reading() override;

    void i2c_read();
    static void i2c_read_task(void *pvParameters) {
        auto *handler = static_cast<I2CHandler *>(pvParameters);
        handler->i2c_read();
    }

    shared_i2c i2c;
    // Only I2C dev reg we have so lets go and just specify it
    PressureRegister reg;
};

#endif /* REGISTERHANDLER_H_ */
