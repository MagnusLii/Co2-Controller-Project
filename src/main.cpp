
#include "FreeRTOS.h"

#include "device_registry.h"
#include "register_handler.h"
#include "HardwareConst.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include "semphr.h"
#include "ssd1306.h"
#include "task.h"
#include "modbus_controller.h"
#include "uart_instance.h"
#include "PicoI2C.h"
#include "connection_handler.h"
#include "IPStack.h"
#include <read_runtime_ctr.cpp>
#include "connection_defines.h"
#include "api_tasks.h"
#include "task_defines.h"

#include "screen.hpp"

#include <cstdint>
#include <iostream>
#include <climits>
#include <map>
#include <memory>
#include <sstream>

struct hw_setup_params {
    shared_uart uart;
    shared_modbus modbus;
    shared_i2c i2c;
    std::shared_ptr<DeviceRegistry> registry;
    std::shared_ptr<Screen> screen;
};

struct sub_setup_params {
    std::shared_ptr<DeviceRegistry> registry;
    ConnectionHandler* connection_handler;
    std::shared_ptr<Screen> screen;
};

void setup_task(void *pvParameters);
void subscriber_setup_task(void *pvParameters);
void test_task(void *param);

int main() {
    stdio_init_all();
    std::cout << "Booting..." << std::endl;


    

    shared_uart uart_i; // {std::make_shared<Uart_instance>(UART_NR, UART_BAUD, UART_TX_PIN, UART_RX_PIN, 2)};
    shared_modbus mbctrl; // {std::make_shared<ModbusCtrl>(uart_i)};
    shared_i2c i2c_i; //{std::make_shared<PicoI2C>(I2C_NR)};
    auto registry = std::make_shared<DeviceRegistry>(); // {mbctrl, i2c_i};

    std::shared_ptr<Screen> screen;
    hw_setup_params hw_params{uart_i, mbctrl, i2c_i, registry, screen};
    sub_setup_params sub_params{registry, NULL, screen};
    xTaskCreate(setup_task, "setup_task", 512, &hw_params, tskIDLE_PRIORITY + 5, nullptr);
    // xTaskCreate(subscriber_setup_task, "subscriber_setup_task", 512, &sub_params, tskIDLE_PRIORITY + 3, nullptr);

    // sendQueue = xQueueCreate(SEND_QUEUE_SIZE, sizeof(Message));
    // receiveQueue = xQueueCreate(RECEIVE_QUEUE_SIZE, sizeof(Message));
    // ConnectionHandler connHandler = ConnectionHandler();
    // xTaskCreate(test_task, "test", 1024, (void*) &connHandler, TASK_PRIORITY_LOW, nullptr);

    vTaskStartScheduler();

    for (;;) {
    }
}

void setup_task(void *pvParameters) {
    const auto params = static_cast<hw_setup_params*>(pvParameters);
    params->uart = std::make_shared<Uart_instance>(UART_NR, UART_BAUD, UART_TX_PIN, UART_RX_PIN, 2);
    params->modbus = std::make_shared<ModbusCtrl>(params->uart);
    params->i2c = std::make_shared<PicoI2C>(I2C_NR);
    params->registry->add_shared(params->modbus, params->i2c);

    params->screen = std::make_shared<Screen>(params->i2c);
    // params->registry->subscribe_to_handler(ReadingType::CO2, params->screen->get_queue_handle());

    sub_setup_params sub_params{params->registry, NULL, params->screen};
    xTaskCreate(subscriber_setup_task, "subscriber_setup_task", 512, &sub_params, tskIDLE_PRIORITY + 3, nullptr);

    vTaskSuspend(nullptr);
}

void subscriber_setup_task(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(100));
    const auto params = static_cast<sub_setup_params*>(pvParameters);
    auto subscriber = std::make_shared<TestSubscriber>("CO2");
    auto subscriber2 = std::make_shared<TestSubscriber>("Temperature");
    auto subscriber3 = std::make_shared<TestSubscriber>("Humidity");
    auto subscriber4 = std::make_shared<TestSubscriber>("Fan counter");
    auto subscriber5 = std::make_shared<TestSubscriber>("Pressure");
    auto writer = std::make_shared<TestWriter>("Fan Speed", params->registry->get_write_queue_handle(WriteType::FAN_SPEED));
    params->registry->subscribe_to_handler(ReadingType::CO2, subscriber->get_queue_handle());
    params->registry->subscribe_to_handler(ReadingType::TEMPERATURE, subscriber2->get_queue_handle());
    params->registry->subscribe_to_handler(ReadingType::REL_HUMIDITY, subscriber3->get_queue_handle());
    params->registry->subscribe_to_handler(ReadingType::FAN_COUNTER, subscriber4->get_queue_handle());
    params->registry->subscribe_to_handler(ReadingType::PRESSURE, subscriber5->get_queue_handle());

    params->registry->subscribe_to_all(params->screen->get_queue_handle());

    auto connHandler = static_cast<ConnectionHandler*>(params->connection_handler);
    xTaskCreate(fully_initialize_connhandler_task, "init IPStack", 1024, (void *) &connHandler, TASK_PRIORITY_ABSOLUTE, nullptr);

    vTaskSuspend(nullptr);
}

void test_task(void *param) {
    ConnectionHandler *connHandler = static_cast<ConnectionHandler *>(param);
    while (connHandler->isConnected() != true && connHandler->isIPStackInitialized() != true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    Message msg;

    float values[8] = {2.0, 3.0, 4.0, 1.0, 1.0, 2.0, 5.0, 2.0};

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        msg.length = connHandler->create_minimal_push_message(msg.data, values);
        DEBUG_printf("Message length: %d\n", msg.length);

        DEBUG_printf("\n---------------------------------------------------------------\n\n");
        for (int i = 0; i < msg.length; i++) {
            printf("%c", msg.data[i]);
        }

        DEBUG_printf("\n---------------------------------------------------------------\n\n");

        if (msg.length > 0) {
            xQueueSend(sendQueue, &msg, QUEUE_TIMEOUT);
        }
    }


    xQueueSend(sendQueue, &msg, QUEUE_TIMEOUT);
    vTaskDelete(NULL);
}
