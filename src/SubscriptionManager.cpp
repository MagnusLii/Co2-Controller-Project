#include "SubscriptionManager.h"

SubscriptionManager::SubscriptionManager() {}

void SubscriptionManager::subscribe_to_handler(ReadingType type, QueueHandle_t receiver) {
    if (handlers.find(type) != handlers.end()) {
        handlers[type]->add_subscriber(receiver);
        std::cout << "Subscriber added" << std::endl;
    } else {
        std::cout << "Handler not found" << std::endl;
    }
}

void SubscriptionManager::subscribe_to_all(QueueHandle_t receiver) {
    // TODO
}

void SubscriptionManager::add_register_handler(ReadingType type,
                                               std::shared_ptr<ReadRegisterHandler> handler) {
    handlers[type] = handler; // Overwrites if already exists
    std::cout << "Handler added" << std::endl;
}


TestSubscriber::TestSubscriber(std::string name) : name(name) {
    receiver = xQueueCreate(10, sizeof(Reading));
    xTaskCreate(receive_task, name.c_str(), 256, this, tskIDLE_PRIORITY + 1, nullptr);
}

void TestSubscriber::receive() {
    for (;;) {
        Reading reading;
        if (xQueueReceive(receiver, &reading, pdMS_TO_TICKS(100))) {
            std::cout << name << ": " << reading.value.f32 << std::endl;
        }
    }
}

QueueHandle_t TestSubscriber::get_queue_handle() { return receiver; }
