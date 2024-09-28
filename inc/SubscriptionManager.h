#ifndef SUBSCRIPTIONMANAGER_H_
#define SUBSCRIPTIONMANAGER_H_

#include "FreeRTOS.h"
#include "queue.h"
#include "RegisterHandler.h"

#include <iostream>
#include <map>
#include <memory>
#include <string>

class SubscriptionManager { // Maybe rename to something something registry?
  public:
    SubscriptionManager();
    void subscribe_to_handler(ReadingType type, QueueHandle_t receiver);
    TaskHandle_t subscribe_to_handler(WriteType type);
    void subscribe_to_all(QueueHandle_t receiver);
    void add_register_handler(ReadingType type, std::unique_ptr<ReadRegisterHandler> handler);
    void add_register_handler(WriteType type, std::unique_ptr<WriteRegisterHandler> handler);

  private:
    std::map<ReadingType, std::unique_ptr<ReadRegisterHandler>> read_handlers;
    std::map<WriteType, std::unique_ptr<WriteRegisterHandler>> write_handlers;
};

// Not to be used in production
class TestSubscriber {
  public:
    TestSubscriber(std::string name);
    QueueHandle_t get_queue_handle();

  private:
    void receive();

    static void receive_task(void *pvParameters) {
        auto *subscriber = static_cast<TestSubscriber *>(pvParameters);
        subscriber->receive();
    }

    QueueHandle_t receiver;
    std::string name;
};

class TestWriter {
public:
    TestWriter(std::string name, SubscriptionManager& manager);
    void add_send_handle(TaskHandle_t handle);

private:
    void send();

    static void send_task(void *pvParameters) {
        auto *writer = static_cast<TestWriter *>(pvParameters);
        writer->send();
    }

    TaskHandle_t send_handle;
    std::string name;
};

#endif /* SUBSCRIPTIONMANAGER_H_ */

