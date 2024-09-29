
#ifndef REGISTERHANDLER_H_
#define REGISTERHANDLER_H_

#include "FreeRTOS.h"
#include "modbus_register.h"
#include "portmacro.h"
#include "queue.h"
#include "timers.h"
#include "uart_instance.h"
#include "PicoI2C.h"

#include <algorithm>
#include <pressure_register.h>
#include <string>
#include <vector>

enum class ReadingType { CO2, TEMPERATURE, REL_HUMIDITY, FAN_COUNTER, PRESSURE, UNSET };

enum class WriteType { FAN_SPEED, UNSET };

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

class ReadRegisterHandler {
  public:
    virtual ~ReadRegisterHandler() = default;
    void add_subscriber(QueueHandle_t subscriber);
    void remove_subscriber(QueueHandle_t subscriber);
    void send_reading();
    ReadingType get_type();
    std::string get_name();
    TimerHandle_t get_timer_handle();
    virtual void get_reading() = 0;

  protected:
    Reading reading{ReadingType::UNSET, {0}};
    TickType_t last_reading = 0;
    std::string name = "";
    TimerHandle_t send_timer = nullptr;

    const uint16_t send_interval = 5000;
    const uint16_t reading_interval = 2000;

    static void send_reading_timer_callback(TimerHandle_t xTimer) {
        ReadRegisterHandler *handler =
            static_cast<ReadRegisterHandler *>(pvTimerGetTimerID(xTimer));
        handler->send_reading();
    }

  private:
    std::vector<QueueHandle_t> subscribers;
};

class ModbusReadHandler : public ReadRegisterHandler {
  public:
    ModbusReadHandler(shared_modbus client, uint8_t server_address, uint16_t register_address,
                      uint8_t nr_of_registers, bool holding_register, ReadingType type,
                      std::string name);
    void get_reading() override;

  private:
    void mb_read();
    static void mb_read_task(void *pvParameters) {
        ModbusReadHandler *handler = static_cast<ModbusReadHandler *>(pvParameters);
        handler->mb_read();
    }
    //void read_register();

    shared_modbus controller;
    ReadRegister reg;
};


class WriteRegisterHandler {
  public:
    virtual ~WriteRegisterHandler() = default;
    QueueHandle_t get_write_queue_handle();
    virtual void write_to_reg(uint32_t value) = 0;
    std::string get_name();

  protected:
    QueueHandle_t write_queue = nullptr;
    WriteType type = WriteType::UNSET;
    std::string name = "";

  private:
    std::vector<QueueHandle_t> subscribers;
};

class ModbusWriteHandler : public WriteRegisterHandler {
  public:
    ModbusWriteHandler(shared_modbus client, uint8_t server_address, uint16_t register_address,
                       uint8_t nr_of_registers, WriteType type, std::string name);

    void write_to_reg(uint32_t value) override;

  private:
    void mb_write();
    static void mb_write_task(void *pvParameters) {
        ModbusWriteHandler *handler = static_cast<ModbusWriteHandler *>(pvParameters);
        handler->mb_write();
    }

    shared_modbus controller;
    WriteRegister reg;
};


// TODO:
class I2CHandler : public ReadRegisterHandler, public WriteRegisterHandler {
  public:
    explicit I2CHandler(shared_i2c i2c_i, uint8_t device_address);

    void get_reading() override;
    void write_to_reg(uint32_t value) override;
  private:
    void i2c_read();
    static void i2c_read_task(void *pvParameters) {
      I2CHandler *handler = static_cast<I2CHandler *>(pvParameters);
      handler->i2c_read();
    }

    shared_i2c i2c;
    PressureRegister reg; // Only I2C dev we have so let's just specify it
};

#endif /* REGISTERHANDLER_H_ */
