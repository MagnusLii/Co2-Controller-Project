#include "DeviceRegistry.h"

DeviceRegistry::DeviceRegistry() {}

void DeviceRegistry::subscribe_to_handler(ReadingType type, QueueHandle_t receiver) {
    if (read_handlers.find(type) != read_handlers.end()) {
        read_handlers[type]->add_subscriber(receiver);
        std::cout << "Subscriber added to " << read_handlers[type]->get_name() << std::endl;
    } else {
        std::cout << "Handler not found" << std::endl;
    }
}

QueueHandle_t DeviceRegistry::subscribe_to_handler(WriteType type) {
    if (write_handlers.find(type) != write_handlers.end()) {
        std::cout << "Subscriber given handle to " << write_handlers[type]->get_name() << std::endl;
        return write_handlers[type]->get_write_queue_handle();
    } else {
        std::cout << "Handler not found" << std::endl;
        return nullptr;
    }
}

void DeviceRegistry::subscribe_to_all(QueueHandle_t receiver) {
    std::cout << "All subscribers added" << std::endl;
    for (auto &handler : read_handlers) {
        handler.second->add_subscriber(receiver);
    }
}

void DeviceRegistry::add_register_handler(ReadingType type,
                                          std::shared_ptr<ReadRegisterHandler> handler) {
    for (auto const &handler_pair : read_handlers) {
        if (handler_pair.first == type) {
            std::cout << "Handler already exists" << std::endl;
            return;
        }
    }
    read_handlers[type] = handler;
    std::cout << "Read handler added" << std::endl;
}

void DeviceRegistry::add_register_handler(WriteType type,
                                          std::shared_ptr<WriteRegisterHandler> handler) {
    write_handlers[type] = handler;
    std::cout << "Write handler added" << std::endl;
}

TestSubscriber::TestSubscriber(std::string name) : name(name) {
    receiver = xQueueCreate(10, sizeof(Reading));
    xTaskCreate(receive_task, name.c_str(), 256, this, tskIDLE_PRIORITY + 2, nullptr);
}

void TestSubscriber::receive() {
    for (;;) {
        Reading reading;
        if (xQueueReceive(receiver, &reading, pdMS_TO_TICKS(500))) {
            trap(reading);
            if (reading.type == ReadingType::FAN_COUNTER) {
                std::cout << name << ": " << reading.value.u16 << std::endl;
            } else {
                std::cout << name << ": " << reading.value.f32 << std::endl;
            }
        }
    }
}

QueueHandle_t TestSubscriber::get_queue_handle() { return receiver; }

TestWriter::TestWriter(std::string name, QueueHandle_t handle) {
    this->name = std::move(name);
    this->send_handle = handle;

    xTaskCreate(send_task, name.c_str(), 256, this, tskIDLE_PRIORITY + 2, nullptr);
}

void TestWriter::add_send_handle(QueueHandle_t handle) { send_handle = handle; }

void TestWriter::send() {
    uint32_t msg = 400;
    for (;;) {
        xQueueSend(send_handle, &msg, 0);
        msg = 0;
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

void trap(Reading reading) {
    std::cout << reading.value.u16 << std::endl;
}