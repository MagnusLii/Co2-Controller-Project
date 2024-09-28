
#ifndef REGISTERHANDLER_H_
#define REGISTERHANDLER_H_

#include "FreeRTOS.h"
#include "modbus_register.h"
#include "portmacro.h"
#include "queue.h"
#include "timers.h"
#include "uart_instance.h"

#include <algorithm>
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
        //int16_t i16;
    } value;
};

class ReadRegisterHandler {
  public:
    virtual ~ReadRegisterHandler() = default;
    void add_subscriber(QueueHandle_t subscriber);
    void remove_subscriber(QueueHandle_t subscriber);
    void send_reading();
    ReadingType get_type();

  protected:
    Reading reading{ReadingType::UNSET, {0}};
    TickType_t last_reading = 0;
    std::string name = "";
    TimerHandle_t send_timer = nullptr;

    const uint16_t send_interval = 1000;
    const uint16_t reading_interval = 1000;

    static void send_reading_timer_callback(TimerHandle_t xTimer) {
        ReadRegisterHandler *handler =
            static_cast<ReadRegisterHandler *>(pvTimerGetTimerID(xTimer));
        handler->send_reading();
    }

  private:
    virtual void get_reading() = 0;

    std::vector<QueueHandle_t> subscribers;
};

class ModbusReadHandler : public ReadRegisterHandler {
  public:
    ModbusReadHandler(shared_modbus mbctrl, ReadRegister* register_, ReadingType type, std::string name);
  /*
    ModbusReadHandler(shared_modbus client, uint8_t server_address, uint16_t register_address,
                      uint8_t nr_of_registers, bool holding_register, ReadingType type,
                      std::string name);
  */
  private:
    void get_reading() override;

    void mb_read();

    static void mb_read_task(void *pvParameters) {
        ModbusReadHandler *handler = static_cast<ModbusReadHandler *>(pvParameters);
        handler->mb_read();
    }

    shared_modbus controller;
    ReadRegister& reg;
};

// TODO:
class I2CReadHandler : public ReadRegisterHandler {
  public:
    I2CReadHandler() = default;

  private:
    void get_reading() override;

    // I2C Reg
};

class WriteRegisterHandler {
  public:
    virtual ~WriteRegisterHandler() = default;
    TaskHandle_t get_write_task_handle();

  protected:
    TaskHandle_t write_task_handle = nullptr;
    Reading reading{ReadingType::UNSET, {0}};
    std::string name = "";

  private:
    std::vector<QueueHandle_t> subscribers;
};

class ModbusWriteHandler : public WriteRegisterHandler {
  public:
    ModbusWriteHandler(shared_modbus client, uint8_t server_address, uint16_t register_address,
                       uint8_t nr_of_registers, ReadingType type, std::string name);

  private:
    void mb_write();

    static void mb_write_task(void *pvParameters) {
        ModbusWriteHandler *handler = static_cast<ModbusWriteHandler *>(pvParameters);
        handler->mb_write();
    }

    shared_modbus controller;
    WriteRegister reg;
};

#endif /* REGISTERHANDLER_H_ */
