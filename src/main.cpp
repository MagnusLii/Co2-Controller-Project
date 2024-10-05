
#include "FreeRTOS.h"

#include "IPStack.h"
#include "PicoI2C.h"
#include "api_tasks.h"
#include "connection_defines.h"
#include "connection_handler.h"
#include "device_registry.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware_const.h"
#include "modbus_controller.h"
#include "pico/stdlib.h"
#include "read_runtime_ctr.cpp"
#include "rotary.hpp"
#include "screen.hpp"
#include "semphr.h"
#include "ssd1306.h"
#include "task.h"
#include "task_defines.h"
#include "uart_instance.h"

#include <climits>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>

struct hw_setup_params {
    shared_uart uart;
    shared_modbus modbus;
    shared_i2c i2c;
    std::shared_ptr<DeviceRegistry> registry;
    std::shared_ptr<Screen> screen;
    std::shared_ptr<Rotary> rotary;
};

struct sub_setup_params {
    std::shared_ptr<DeviceRegistry> registry;
    ConnectionHandler *connection_handler;
    std::shared_ptr<Screen> screen;
    std::shared_ptr<Rotary> rotary;
};

void setup_task(void *pvParameters);
void subscriber_setup_task(void *pvParameters);
void test_task(void *param);

int main() {
    stdio_init_all();
    std::cout << "Booting..." << std::endl;

    shared_uart uart_i;                                 // {std::make_shared<Uart_instance>(UART_NR, UART_BAUD,
                                                        // UART_TX_PIN, UART_RX_PIN, 2)};
    shared_modbus mbctrl;                               // {std::make_shared<ModbusCtrl>(uart_i)};
    shared_i2c i2c_i;                                   //{std::make_shared<PicoI2C>(I2C_NR)};
    auto registry = std::make_shared<DeviceRegistry>(); // {mbctrl, i2c_i};

    std::shared_ptr<Screen> screen;
    std::shared_ptr<Rotary> rotary;

    hw_setup_params hw_params{uart_i, mbctrl, i2c_i, registry, screen, rotary};
    // sub_setup_params sub_params{registry, NULL, screen};

    xTaskCreate(setup_task, "setup_task", 512, &hw_params, TASK_PRIORITY_ABSOLUTE + 1, nullptr);
    // xTaskCreate(subscriber_setup_task, "subscriber_setup_task", 512,
    // &sub_params, tskIDLE_PRIORITY + 3, nullptr);

    // sendQueue = xQueueCreate(SEND_QUEUE_SIZE, sizeof(Message));
    // receiveQueue = xQueueCreate(RECEIVE_QUEUE_SIZE, sizeof(Message));
    // ConnectionHandler connHandler = ConnectionHandler();
    // xTaskCreate(test_task, "test", 1024, (void*) &connHandler,
    // TASK_PRIORITY_LOW, nullptr);

    vTaskStartScheduler();

    for (;;) {}
}

void setup_task(void *pvParameters) {
    const auto params = static_cast<hw_setup_params *>(pvParameters);
    params->uart = std::make_shared<Uart_instance>(UART_NR, UART_BAUD, UART_TX_PIN, UART_RX_PIN, UART_BITS);
    params->modbus = std::make_shared<ModbusCtrl>(params->uart);
    params->i2c = std::make_shared<PicoI2C>(I2C_1);
    params->registry->add_shared(params->modbus, params->i2c);

    params->screen = std::make_shared<Screen>(params->i2c);
    params->rotary = std::make_shared<Rotary>();
    // params->registry->subscribe_to_handler(ReadingType::CO2,
    // params->screen->get_queue_handle());

    sub_setup_params sub_params{params->registry, NULL, params->screen, params->rotary};
    xTaskCreate(subscriber_setup_task, "subscriber_setup_task", 512, &sub_params, TASK_PRIORITY_HIGH, nullptr);

    vTaskSuspend(nullptr);
}

void subscriber_setup_task(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(100));
    const auto params = static_cast<sub_setup_params *>(pvParameters);
    auto subscriber = std::make_shared<TestSubscriber>("CO2");
    auto subscriber2 = std::make_shared<TestSubscriber>("Temperature");
    auto subscriber3 = std::make_shared<TestSubscriber>("Humidity");
    auto subscriber4 = std::make_shared<TestSubscriber>("Fan counter");
    auto subscriber5 = std::make_shared<TestSubscriber>("Pressure");
    auto writer =
        std::make_shared<TestWriter>("Fan speed", params->registry->get_write_queue_handle(WriteType::FAN_SPEED));

    params->registry->subscribe_to_handler(ReadingType::CO2, subscriber->get_queue_handle());

    // params->registry->subscribe_to_handler(ReadingType::TEMPERATURE,
    // subscriber2->get_queue_handle());
    // params->registry->subscribe_to_handler(ReadingType::REL_HUMIDITY,
    // subscriber3->get_queue_handle());
    params->registry->subscribe_to_handler(ReadingType::FAN_COUNTER, subscriber4->get_queue_handle());
    // params->registry->subscribe_to_handler(ReadingType::PRESSURE,
    // subscriber5->get_queue_handle());
    // params->registry->subscribe_to_all(subscriber->get_queue_handle());

    params->registry->subscribe_to_all(params->screen->get_queue_handle());
    params->rotary->add_subscriber(params->screen->get_queue_handle());

    //auto connHandler = static_cast<ConnectionHandler *>(params->connection_handler);
    //xTaskCreate(fully_initialize_connhandler_task, "init IPStack", 1024, (void *)&connHandler, TASK_PRIORITY_ABSOLUTE,
    //            nullptr);

    cyw43_arch_deinit();
    printf("All done\n");
    return;
}

void tls_task(void *pvParameters) {

    std::string cert = TLS_CERTIFICATE;
    std::string ssid = WIFI_SSID;
    std::string password = WIFI_PASSWORD;
    uint32_t countryCode = DEFAULT_COUNTRY_CODE;

    // std::string request = "POST /talkbacks/" TALKBACK_ID "/commands/execute.json HTTP/1.1\r\n"
    //                   "Host: api.thingspeak.com\r\n"
    //                   "Content-Length: 24\r\n"
    //                   "Content-Type: application/x-www-form-urlencoded\r\n"
    //                   "\r\n"
    //                   "api_key=" TALK_BACK_API_KEY;

    std::string request = "GET /update?api_key=" API_KEY "&field1=410&field2=45.7 HTTP/1.1\r\n"
                      "Host: api.thingspeak.com\r\n"
                      "Connection: close\r\n"
                      "\r\n";

    TLSWrapper tlsWrapper(cert, ssid, password, countryCode);
    while (true) {
        tlsWrapper.send_request(THINGSPEAK_HOSTNAME, request);
        vTaskDelay(5000);
    }

    Message msg;

    float values[8] = {2.0, 3.0, 4.0, 1.0, 1.0, 2.0, 5.0, 2.0};

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        msg.length = connHandler->create_minimal_push_message(msg.data, values);
        DEBUG_printf("Message length: %d\n", msg.length);

        DEBUG_printf("\n-------------------------------------------------------"
                     "--------\n\n");
        for (int i = 0; i < msg.length; i++) {
            printf("%c", msg.data[i]);
        }

        DEBUG_printf("\n-------------------------------------------------------"
                     "--------\n\n");

        if (msg.length > 0) { xQueueSend(sendQueue, &msg, QUEUE_TIMEOUT); }
    }

    xQueueSend(sendQueue, &msg, QUEUE_TIMEOUT);
    vTaskDelete(NULL);
}
