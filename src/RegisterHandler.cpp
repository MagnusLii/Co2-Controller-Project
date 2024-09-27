#include "RegisterHandler.h"

void ReadRegisterHandler::add_subscriber(QueueHandle_t subscriber) {
    subscribers.push_back(subscriber);
}

void ReadRegisterHandler::remove_subscriber(QueueHandle_t subscriber) {
    // Not sure if we really need this
    subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), subscriber),
                      subscribers.end());
}

void ReadRegisterHandler::send_reading() {
    for (auto subscriber : subscribers) {
        xQueueSend(subscriber, &reading, 0);
    }
}

ModbusReadHandler::ModbusReadHandler(shared_modbus client,
                                             uint8_t device_address, uint16_t register_address,
                                             uint8_t nr_of_registers, bool holding_register,
                                             ReadingType type) {
    this.regi
}

void ModbusReadHandler::get_reading() { reading.value.f32 = register.read_float(); }

void ModbusReadHandler::mb_read() {
    for (;;) {
        get_reading();
        vTaskDelay(1000); // TBD
    }
}
