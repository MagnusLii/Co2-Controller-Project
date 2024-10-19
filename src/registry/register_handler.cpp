#include "register_handler.h"
#include "task_defines.h"

#include <climits>

std::string RegisterHandler::get_name() { return name; }

// Add a subscribers queue handle to the send list (vector)
void ReadRegisterHandler::add_subscriber(QueueHandle_t subscriber) { subscribers.push_back(subscriber); }

// Remove a subscribers queue handle from the send list
// -- probably not going to be used ever
void ReadRegisterHandler::remove_subscriber(QueueHandle_t subscriber) {
    subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), subscriber), subscribers.end());
}

// Send the reading to all subscribers
void ReadRegisterHandler::send_reading() {
    for (const auto subscriber : subscribers) {
        xQueueSend(subscriber, &reading, 0);
    }
}

void ReadRegisterHandler::send_reading_from_isr() {
    for (const auto subscriber : subscribers) {
        xQueueSendFromISR(subscriber, &reading, 0);
    }
}

ReadingType ReadRegisterHandler::get_type() const { return reading.type; }

ModbusReadHandler::ModbusReadHandler(shared_modbus controller, const uint8_t device_address,
                                     const uint16_t register_address, const uint8_t nr_of_registers,
                                     const bool holding_register, const ReadingType type, const std::string &name)
    : reg(controller, device_address, register_address, nr_of_registers, holding_register) {
    this->controller = controller;
    this->reading.type = type;
    this->name = name;

    xTaskCreate(mb_read_task, name.c_str(), 256, this, TaskPriority::MEDIUM, nullptr);
}

void ModbusReadHandler::get_reading() { reading.value.u32 = reg.get32(); }

// Main modbus register read task
void ModbusReadHandler::mb_read() {
    for (;;) {
        const TickType_t start_time = xTaskGetTickCount();
        while (controller->isbusy()) {
            vTaskDelay(5);
        }
        reg.start_transfer();
        while (controller->isbusy()) {
            vTaskDelay(5);
        }
        get_reading();
        send_reading();
        TickType_t delay_time = start_time + reading_interval - xTaskGetTickCount();
        vTaskDelay(delay_time);
    }
}

QueueHandle_t WriteRegisterHandler::get_write_queue_handle() const { return write_queue; }

ModbusWriteHandler::ModbusWriteHandler(shared_modbus controller, const uint8_t device_address,
                                       const uint16_t register_address, const uint8_t nr_of_registers,
                                       const WriteType type, const std::string &name)
    : reg(controller, device_address, register_address, nr_of_registers) {
    this->controller = controller;
    this->type = type;
    this->name = name;
    this->write_queue = xQueueCreate(5, sizeof(uint32_t));

    xTaskCreate(mb_write_task, name.c_str(), 256, this, TaskPriority::MEDIUM, nullptr);
}

void ModbusWriteHandler::write_to_reg(uint32_t value) {
    while (controller->isbusy()) // Someone already using the modbus so wait
        vTaskDelay(5);
    reg.start_transfer(value);
}

// Main modbus register write task
// Waits for someone to send a value to the queue
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

// I2CHandler - read only
I2CHandler::I2CHandler(shared_i2c i2c_i, const uint8_t device_address, const ReadingType rtype, const std::string &name)
    : reg(i2c_i, device_address) {
    this->i2c = i2c_i;
    this->reading.type = rtype;
    this->name = name;

    xTaskCreate(i2c_read_task, name.c_str(), 256, this, TaskPriority::MEDIUM, nullptr);
}

void I2CHandler::get_reading() { reading.value.i16 = reg.read_register(); }

// Main i2c read task
void I2CHandler::i2c_read() {
    for (;;) {
        get_reading();
        send_reading();
        vTaskDelay(reading_interval);
    }
}
