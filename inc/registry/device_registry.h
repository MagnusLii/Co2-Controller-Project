#ifndef DEVICEREGISTRY_H_
#define DEVICEREGISTRY_H_

#include "FreeRTOS.h"
#include "fan_controller.h"
#include "queue.h"
#include "register_handler.h"

#include <iostream>
#include <map>
#include <memory>
#include <string>

class DeviceRegistry { // Maybe rename to something something
  public:
    DeviceRegistry(shared_modbus mbctrl, shared_i2c i2c_i);
    void subscribe_to_handler(ReadingType type, QueueHandle_t receiver);
    QueueHandle_t get_write_queue_handle(WriteType type);
    void subscribe_to_all(QueueHandle_t receiver);
    void add_register_handler(std::shared_ptr<ReadRegisterHandler> handler, ReadingType type);
    void add_register_handler(std::shared_ptr<WriteRegisterHandler> handler, WriteType type);

  private:
    void initialize();
    static void initialize_task(void *pvParameters) {
        const auto dr = static_cast<DeviceRegistry *>(pvParameters);
        dr->initialize();
    }

    shared_modbus mbctrl;
    shared_i2c i2c;
    std::shared_ptr<FanController> fanctrl;
    std::map<ReadingType, std::shared_ptr<ReadRegisterHandler>> read_handlers;
    std::map<WriteType, std::shared_ptr<WriteRegisterHandler>> write_handlers;
};

// Not to be used in production
class TestSubscriber {
  public:
    TestSubscriber();
    explicit TestSubscriber(const std::string &name);
    [[nodiscard]] QueueHandle_t get_queue_handle() const;

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
    TestWriter();
    TestWriter(const std::string &name, QueueHandle_t handle);
    void add_send_handle(QueueHandle_t handle);

  private:
    void send() const;
    static void send_task(void *pvParameters) {
        const auto *writer = static_cast<TestWriter *>(pvParameters);
        writer->send();
    }

    QueueHandle_t send_handle;
    std::string name;
};

void trap(Reading reading);

#endif /* DEVICEREGISTRY_H_ */
