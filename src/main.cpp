
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
#include "logHandler.h"
#include "eeprom.h"

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
};

struct sub_setup_params {
    std::shared_ptr<DeviceRegistry> registry;
    ConnectionHandler* connection_handler;
};

SemaphoreHandle_t lock;

void setup_task(void *pvParameters);
void subscriber_setup_task(void *pvParameters);
void test_task(void *param);
void write_eeprom_task(void *param);
void read_eeprom_task(void *param);

int main() {
    stdio_init_all();
    std::cout << "Booting..." << std::endl;

    lock = xSemaphoreCreateMutex();

    shared_uart uart_i; // {std::make_shared<Uart_instance>(UART_NR, UART_BAUD, UART_TX_PIN, UART_RX_PIN, 2)};
    shared_modbus mbctrl; // {std::make_shared<ModbusCtrl>(uart_i)};
    shared_i2c i2c_i; //{std::make_shared<PicoI2C>(I2C_NR)};

    auto registry = std::make_shared<DeviceRegistry>(); // {mbctrl, i2c_i};

    eeprom_init_i2c(i2c0, 1000000, 5);
    LogHandler logHandler;

    hw_setup_params hw_params{uart_i, mbctrl, i2c_i, registry};
    sub_setup_params sub_params{registry};

    xTaskCreate(setup_task, "setup_task", 512, &hw_params, tskIDLE_PRIORITY + 5, nullptr);
    xTaskCreate(subscriber_setup_task, "subscriber_setup_task", 512, &sub_params, tskIDLE_PRIORITY + 3, nullptr);

    sendQueue = xQueueCreate(SEND_QUEUE_SIZE, sizeof(Message));
    receiveQueue = xQueueCreate(RECEIVE_QUEUE_SIZE, sizeof(Message));

    ConnectionHandler connHandler = ConnectionHandler();

    xTaskCreate(write_eeprom_task, "write_eeprom_task", 512, &logHandler, tskIDLE_PRIORITY + 1, nullptr);
    xTaskCreate(read_eeprom_task, "read_eeprom_task", 512, &logHandler, tskIDLE_PRIORITY + 1, nullptr);

    //xTaskCreate(test_task, "test", 1024, (void*) &connHandler, TASK_PRIORITY_LOW, nullptr);

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

void write_eeprom_task(void *param) {
    float aa = 12.5f, bb = 100.5f, cc = 12.12f;
    for (;;) {
        if (lock != nullptr) {
            if (xSemaphoreTake(lock, (TickType_t) 10) == pdTRUE) {
                auto *logHandler = static_cast<LogHandler *>(param);
                logHandler->storeData(&aa, &bb, &cc, 127);
                //DEBUG_printf("\n write successful??? \n\n");

                xSemaphoreGive(lock);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }
        }
    }
}

void read_eeprom_task(void *param) {
    float aa, bb, cc;
    int16_t gg;
    int arr[4] = {0, 0, 0, 0};

    vTaskDelay(pdMS_TO_TICKS(1000));
    if (lock != nullptr) {
        if (xSemaphoreTake(lock, (TickType_t) 10) == pdTRUE) {
            auto *logHandler = static_cast<LogHandler *>(param);
            logHandler->fetchCredentials(&aa, &bb, &cc, &gg, arr);

            DEBUG_printf("\n%f \n"
             "%f \n"
             "%f \n"
             "%d \n", aa, bb, cc, gg);
            xSemaphoreGive(lock);
            vTaskSuspend(nullptr);
        }
    }
}
