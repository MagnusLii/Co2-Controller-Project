#include "RegisterHandler.h"
#include "modbus_register.h"
#include "projdefs.h"

#include <climits>
#include <iostream>
#include <ostream>

// Add a subscribers queue handle to the send list (vector)
void ReadRegisterHandler::add_subscriber(QueueHandle_t subscriber) {
    subscribers.push_back(subscriber);
}

// Remove a subscribers queue handle from the send list -- probably not used ever
void ReadRegisterHandler::remove_subscriber(QueueHandle_t subscriber) {
    subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), subscriber),
                      subscribers.end());
}

// Send the reading to all subscribers
void ReadRegisterHandler::send_reading() {
    // std::cout << "Sending reading..." << reading.value.f32 << std::endl;
    for (auto subscriber : subscribers) {
        xQueueSend(subscriber, &reading, 0);
    }
}

ReadingType ReadRegisterHandler::get_type() { return reading.type; }

std::string ReadRegisterHandler::get_name() { return name; }

// TimerHandle_t ReadRegisterHandler::get_timer_handle() { return send_timer; }

ModbusReadHandler::ModbusReadHandler(shared_modbus client, uint8_t device_address,
                                     uint16_t register_address, uint8_t nr_of_registers,
                                     bool holding_register, ReadingType type, std::string name)
    : reg(client, device_address, register_address, nr_of_registers, holding_register) {
    this->controller = client; // <- I wasted so much time because of this damn thing
    this->reading.type = type;
    this->name = name;

    xTaskCreate(mb_read_task, name.c_str(), 256, this, tskIDLE_PRIORITY + 2, NULL);

    /* Unused - switched to a different approach
    this->send_timer = xTimerCreate(name.c_str(), pdMS_TO_TICKS(send_interval), pdTRUE, this,
                                    send_reading_timer_callback);
    */
}

void ModbusReadHandler::get_reading() { reading.value.u32 = reg.get32(); }

/* Unused - switched to a consolidated approach
void ModbusReadHandler::read_register() {
    reg.start_transfer();
    while (controller->isbusy())
        vTaskDelay(pdMS_TO_TICKS(10));
    reading.value.u32 = reg.get32();
}
*/

// Main modbus register read task
void ModbusReadHandler::mb_read() {
    // if (send_timer != nullptr) xTimerStart(send_timer, pdMS_TO_TICKS(send_interval));
    for (;;) {
        TickType_t start_time = xTaskGetTickCount();
        while (controller->isbusy()) {
            vTaskDelay(5);
        }
        // std::cout << "Starting read..." << std::endl;
        reg.start_transfer();
        while (controller->isbusy()) {
            vTaskDelay(5);
        }
        get_reading();
        send_reading();
        TickType_t delay_time = start_time + reading_interval - xTaskGetTickCount(); // TBD
        vTaskDelay(delay_time);                                                      // TBD
    }
}

QueueHandle_t WriteRegisterHandler::get_write_queue_handle() { return write_queue; }

ModbusWriteHandler::ModbusWriteHandler(shared_modbus client, uint8_t device_address,
                                       uint16_t register_address, uint8_t nr_of_registers,
                                       WriteType type, std::string name)
    : reg(client, device_address, register_address, nr_of_registers) {
    this->controller = client;
    this->type = type;
    this->name = name;
    this->write_queue = xQueueCreate(5, sizeof(uint32_t));

    xTaskCreate(mb_write_task, name.c_str(), 256, this, tskIDLE_PRIORITY + 2, NULL);
}

void ModbusWriteHandler::write_to_reg(uint32_t value) {
    while (controller->isbusy()) // Someone already using the modbus so wait
        vTaskDelay(5);
    reg.start_transfer(value);
}

// Main modbus register write task
// Waits for someone to send value to the queue
void ModbusWriteHandler::mb_write() {
    uint32_t received_value = 0;

    for (;;) {
        if (xQueueReceive(write_queue, &received_value, portMAX_DELAY)) {
            while (controller->isbusy())
                vTaskDelay(5);
            reg.start_transfer(received_value);
        }
    }
}

std::string WriteRegisterHandler::get_name() { return name; }

// I2CHandler - read only for now (I don't think we need to write?)
I2CHandler::I2CHandler(shared_i2c i2c_i, uint8_t device_address, ReadingType rtype,
                       std::string name)
    : reg(i2c_i, device_address) {
    this->i2c = i2c_i;
    this->reading.type = rtype;
    this->name = name;

    xTaskCreate(i2c_read_task, "i2c_read_task", 256, this, tskIDLE_PRIORITY + 2, NULL);
}

void I2CHandler::get_reading() {
    reading.value.i16 = reg.read_register();
    // std::cout << reading.value.i16 << std::endl;
}

// Main i2c read task
void I2CHandler::i2c_read() {
    for (;;) {
        get_reading();
        send_reading();
        vTaskDelay(reading_interval);
    }
}
