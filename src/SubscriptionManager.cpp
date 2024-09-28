#include "SubscriptionManager.h"

#include <sys/unistd.h>

SubscriptionManager::SubscriptionManager() {}

void SubscriptionManager::subscribe_to_handler(ReadingType type, QueueHandle_t receiver) {
    if (read_handlers.find(type) != read_handlers.end()) {
        read_handlers[type]->add_subscriber(receiver);
        std::cout << "Subscriber added to " << read_handlers[type]->get_name() << std::endl;
    } else {
        std::cout << "Handler not found" << std::endl;
    }
}

TaskHandle_t SubscriptionManager::subscribe_to_handler(WriteType type) {
    if (write_handlers.find(type) != write_handlers.end()) {
        return write_handlers[type]->get_write_task_handle();
    } else {
        std::cout << "Handler not found" << std::endl;
        return nullptr;
    }
}

void SubscriptionManager::subscribe_to_all(QueueHandle_t receiver) {
    std::cout << "All subscribers added" << std::endl;
    for (auto &handler : read_handlers) {
        handler.second->add_subscriber(receiver);
    }
}

void SubscriptionManager::add_register_handler(ReadingType type,
                                               std::unique_ptr<ReadRegisterHandler> handler) {
    read_handlers[type] = std::move(handler); // Overwrites if already exists
    std::cout << "Read handler added" << std::endl;
}

void SubscriptionManager::add_register_handler(WriteType type,
                                               std::unique_ptr<WriteRegisterHandler> handler) {
    write_handlers[type] = std::move(handler);
    std::cout << "Write handler added" << std::endl;
}

TestSubscriber::TestSubscriber(std::string name) : name(name) {
    receiver = xQueueCreate(10, sizeof(Reading));
    xTaskCreate(receive_task, name.c_str(), 256, this, tskIDLE_PRIORITY + 1, nullptr);
}

void TestSubscriber::receive() {
    for (;;) {
        Reading reading;
        if (xQueueReceive(receiver, &reading, pdMS_TO_TICKS(500))) {
            if (reading.type != ReadingType::UNSET) {
                union {
                    uint32_t u;
                    float f;
                    int32_t i;
                    uint16_t s;
                } value;
                value.u = reading.value.u32;
                value.f = reading.value.f32;
                value.s = reading.value.u16;
                value.i = reading.value.i32;
            }
            /*
                if (reading.type == ReadingType::FAN_COUNTER) {
                    std::cout << name << ": " << reading.value.u16 << std::endl;
                } else {
                    std::cout << name << ": " << reading.value.f32 << std::endl;
                }
            */
        }
    }
}

QueueHandle_t TestSubscriber::get_queue_handle() { return receiver; }

TestWriter::TestWriter(std::string name, SubscriptionManager& manager) : name(name) {
    this->send_handle = manager.subscribe_to_handler(WriteType::FAN_SPEED);
    xTaskCreate(send_task, name.c_str(), 256, this, tskIDLE_PRIORITY + 1, nullptr);
}

void TestWriter::add_send_handle(TaskHandle_t handle) { send_handle = handle; }

void TestWriter::send() {
    uint32_t value = 500;
    for (;;) {
        xTaskNotify(send_handle, value, eSetValueWithOverwrite);
        value = 0;
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
