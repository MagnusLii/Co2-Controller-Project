#include "RegisterHandler.h"

void RegisterHandler::add_subscriber(QueueHandle_t subscriber) {
    subscribers.push_back(subscriber);
}

void RegisterHandler::remove_subscriber(QueueHandle_t subscriber) {
    // Not sure if we really need this
    subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), subscriber),
                      subscribers.end());
}

void RegisterHandler::send_reading() {
    for (auto subscriber : subscribers) {
        xQueueSend(subscriber, &reading, 0);
    }
}

ModbusRegisterHandler::ModbusRegisterHandler(std::shared_ptr<ModbusClient> client,
                                             uint8_t server_address, uint16_t register_address,
                                             uint8_t nr_of_registers, bool holding_register,
                                             ReadingType type)
    : modbus_register(client, server_address, register_address, nr_of_registers, holding_register) {

}

void ModbusRegisterHandler::get_reading() { reading.value.f32 = modbus_register.read_float(); }

void ModbusRegisterHandler::mb_read() {
    for (;;) {
        get_reading();
        vTaskDelay(1000); // TBD
    }
}
