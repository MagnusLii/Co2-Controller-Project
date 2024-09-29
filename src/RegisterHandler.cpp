#include "RegisterHandler.h"
#include "modbus_register.h"
#include "projdefs.h"

#include <climits>
#include <iostream>
#include <ostream>

/*
void ReadRegisterHandler::add_subscriber(QueueHandle_t subscriber) {
    subscribers.push_back(subscriber);
}

void ReadRegisterHandler::remove_subscriber(QueueHandle_t subscriber) {
    subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), subscriber),
                      subscribers.end());
}
void ReadRegisterHandler::send_reading() {
    // std::cout << "Sending reading..." << std::endl;
    get_reading();
    for (auto subscriber : subscribers) {
        xQueueSend(subscriber, &reading, 0);
    }
}
*/
ReadingType ReadRegisterHandler::get_type() { return reading.type; }

std::string ReadRegisterHandler::get_name() { return name; }

TimerHandle_t ReadRegisterHandler::get_timer_handle() { return read_timer; }

/* Test
ModbusReadHandler::ModbusReadHandler(shared_modbus mbctrl, ReadRegister* register_, ReadingType
type, std::string name) : reg(*register_) { this->controller = mbctrl; this->reading.type = type;
    this->name = name;

    xTaskCreate(mb_read_task, name.c_str(), 256, this, tskIDLE_PRIORITY + 1, NULL);
    this->send_timer = xTimerCreate(name.c_str(), pdMS_TO_TICKS(send_interval), pdTRUE, this,
                                    send_reading_timer_callback);
}
*/
ModbusReadHandler::ModbusReadHandler(shared_modbus client, uint8_t device_address,
                                     uint16_t register_address, uint8_t nr_of_registers,
                                     bool holding_register, ReadingType type, std::string name)
    : reg(client, device_address, register_address, nr_of_registers, holding_register) {
    this->controller = client; // <- I wasted so much time because of this damn thing
    this->reading.type = type;
    this->name = name;

    this->read_timer = xTimerCreate(name.c_str(), pdMS_TO_TICKS(reading_interval), pdTRUE, this, read_register_callback);
    xTimerStart(this->read_timer, 0); // START IMMEDIATELY
    /*
        xTaskCreate(mb_read_task, name.c_str(), 256, this, tskIDLE_PRIORITY + 1, NULL);
        this->send_timer = xTimerCreate(name.c_str(), pdMS_TO_TICKS(send_interval), pdTRUE, this,
                                        send_reading_timer_callback);
                                        */
}

Reading ModbusReadHandler::get_reading() {
    return reading;
}

void ModbusReadHandler::read_register() {
    reg.start_transfer();
    while (controller->isbusy())
        vTaskDelay(pdMS_TO_TICKS(10));
    reading.value.u32 = reg.get32();
}

/*
void ModbusReadHandler::mb_read() {
    if (send_timer != nullptr) xTimerStart(send_timer, pdMS_TO_TICKS(2000));
    for (;;) {
        // TickType_t start = xTaskGetTickCount();
        while (controller->isbusy())
            vTaskDelay(5);
        // std::cout << "Starting read..." << std::endl;
        reg.start_transfer();
        vTaskDelay(pdMS_TO_TICKS(reading_interval)); // TBD
    }
}
*/

TaskHandle_t WriteRegisterHandler::get_write_task_handle() { return write_task_handle; }

ModbusWriteHandler::ModbusWriteHandler(shared_modbus client, uint8_t device_address,
                                       uint16_t register_address, uint8_t nr_of_registers,
                                       WriteType type, std::string name)
    : reg(client, device_address, register_address, nr_of_registers) {
    this->controller = client;
    this->type = type;
    this->name = name;

    // xTaskCreate(mb_write_task, name.c_str(), 256, this, tskIDLE_PRIORITY + 1,
    // &write_task_handle);
}

void ModbusWriteHandler::write_to_reg(uint32_t value) {
    while(controller->isbusy()) // Someone already using the modbus
        vTaskDelay(5);
    reg.start_transfer(value);
}

/*
void ModbusWriteHandler::mb_write() {
    uint32_t received_value = 0;

    for (;;) {
        if (xTaskNotifyWait(0, ULONG_MAX, &received_value, portMAX_DELAY) == pdTRUE) {
            while (controller->isbusy())
                vTaskDelay(5);
            reg.start_transfer(received_value);
        }
    }
}
*/

std::string WriteRegisterHandler::get_name() { return name; }
