#ifndef DEVICEREGISTRY_H_
#define DEVICEREGISTRY_H_

#include "FreeRTOS.h"
#include "queue.h"
#include "RegisterHandler.h"

#include <iostream>
#include <map>
#include <memory>
#include <string>

class DeviceRegistry { // Maybe rename to something something
  public:
    DeviceRegistry(shared_modbus mbctrl);
    void subscribe_to_handler(ReadingType type, QueueHandle_t receiver);
    QueueHandle_t get_write_queue_handle(WriteType type);
    void subscribe_to_all(QueueHandle_t receiver);
    void add_register_handler(ReadingType type, std::shared_ptr<ReadRegisterHandler> handler);
    void add_register_handler(WriteType type, std::shared_ptr<WriteRegisterHandler> handler);

  private:
    shared_modbus mbctrl;
    std::map<ReadingType, std::shared_ptr<ReadRegisterHandler>> read_handlers;
    std::map<WriteType, std::shared_ptr<WriteRegisterHandler>> write_handlers;
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
    TestWriter(std::string name, QueueHandle_t handle);
    void add_send_handle(QueueHandle_t handle);

private:
    void send();

    static void send_task(void *pvParameters) {
        auto *writer = static_cast<TestWriter *>(pvParameters);
        writer->send();
    }

    QueueHandle_t send_handle;
    QueueHandle_t relay_queue;
    std::string name;
};

void trap(Reading reading);

#endif /* DEVICEREGISTRY_H_ */

