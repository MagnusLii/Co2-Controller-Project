#include "RegisterHandler.h"
#include "modbus_register.h"

#include <iostream>
#include <ostream>

void ReadRegisterHandler::add_subscriber(QueueHandle_t subscriber) {
    subscribers.push_back(subscriber);
}

void ReadRegisterHandler::remove_subscriber(QueueHandle_t subscriber) {
    // Not sure if we really need this
    subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), subscriber),
                      subscribers.end());
}

void ReadRegisterHandler::send_reading() {
    //std::cout << "Sending reading..." << std::endl;
    get_reading();
    for (auto subscriber : subscribers) {
        xQueueSend(subscriber, &reading, 0);
    }
}

ReadingType ReadRegisterHandler::get_type() {
    return reading.type;
}

ModbusReadHandler::ModbusReadHandler(shared_modbus client, uint8_t device_address,
                                     uint16_t register_address, uint8_t nr_of_registers,
                                     bool holding_register, ReadingType type, std::string name)
    : reg(client, device_address, register_address, nr_of_registers, holding_register) {
    this->controller = client; // <- I wasted so much time because of this damn thing
    this->reading.type = type;
    this->name = name;

    xTaskCreate(mb_read_task, name.c_str(), 256, this, tskIDLE_PRIORITY + 1, NULL);
    this->send_timer = xTimerCreate(name.c_str(), pdMS_TO_TICKS(send_interval), pdTRUE, this, send_reading_timer_callback);
}

void ModbusReadHandler::get_reading() { reading.value.f32 = reg.get_float(); }

void ModbusReadHandler::mb_read() {
    if (send_timer != nullptr) xTimerStart(send_timer, 0);
    for (;;) {
        TickType_t start = xTaskGetTickCount();
        while (controller->isbusy()) vTaskDelay(5);
        //std::cout << "Starting read..." << std::endl;
        reg.start_transfer();
        vTaskDelay(pdMS_TO_TICKS(reading_interval)); // TBD
    }
}
