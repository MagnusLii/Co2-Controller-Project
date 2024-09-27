#ifndef SUBSCRIPTIONMANAGER_H_
#define SUBSCRIPTIONMANAGER_H_

#include "FreeRTOS.h"
#include "queue.h"
#include "RegisterHandler.h"

#include <iostream>
#include <map>
#include <memory>
#include <string>

class SubscriptionManager {
  public:
    SubscriptionManager();
    void subscribe_to_handler(ReadingType type, QueueHandle_t receiver);
    void subscribe_to_all(QueueHandle_t receiver);
    void add_register_handler(ReadingType type, std::shared_ptr<ReadRegisterHandler>); // For testing purposes, probably not used in production

  private:
    std::map<ReadingType, std::shared_ptr<ReadRegisterHandler>> handlers;
};

// Not to used in production
class TestSubscriber {
  public:
    TestSubscriber(std::string name);
    QueueHandle_t get_queue_handle();

  private:
    void receive();

    static void receive_task(void *pvParameters) {
        TestSubscriber *subscriber = static_cast<TestSubscriber *>(pvParameters);
        subscriber->receive();
    }

    QueueHandle_t receiver;
    std::string name;
};

#endif /* SUBSCRIPTIONMANAGER_H_ */

