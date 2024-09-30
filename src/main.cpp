
#include "FreeRTOS.h"

#include "DeviceRegistry.h"
#include "RegisterHandler.h"
#include "HardwareConst.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "modbus_register.h"
#include "pico/stdlib.h"
#include "semphr.h"
#include "ssd1306.h"
#include "task.h"
#include "modbus_controller.h"
#include "modbus_register.h"
#include "uart_instance.h"
#include "PicoI2C.h"

#include <cstdint>
#include <iostream>
#include <climits>
#include <map>
#include <memory>
#include <sstream>

extern "C" {
uint32_t read_runtime_ctr(void) { return timer_hw->timerawl; }
}

struct hw_setup_params {
    shared_uart uart;
    shared_modbus modbus;
    shared_i2c i2c;
    std::shared_ptr<DeviceRegistry> registry;
};

struct sub_setup_params {
    std::shared_ptr<DeviceRegistry> registry;
    std::shared_ptr<TestSubscriber> subscriber;
    std::shared_ptr<TestSubscriber> subscriber2;
    std::shared_ptr<TestSubscriber> subscriber3;
    std::shared_ptr<TestSubscriber> subscriber4;
    std::shared_ptr<TestSubscriber> subscriber5;
    std::shared_ptr<TestWriter> writer;
};

void setup_task(void *pvParameters);
void subscriber_setup_task(void *pvParameters);

int main() {
    stdio_init_all();
    std::cout << "Booting..." << std::endl;

    shared_uart uart_i; // {std::make_shared<Uart_instance>(UART_NR, UART_BAUD, UART_TX_PIN, UART_RX_PIN, 2)};
    shared_modbus mbctrl; // {std::make_shared<ModbusCtrl>(uart_i)};
    shared_i2c i2c_i; //{std::make_shared<PicoI2C>(I2C_NR)};

    auto registry = std::make_shared<DeviceRegistry>(); // {mbctrl, i2c_i};

    auto sub1 = std::make_shared<TestSubscriber>(); //("sub1");
    auto sub2 = std::make_shared<TestSubscriber>(); //("sub2");
    auto sub3 = std::make_shared<TestSubscriber>(); //("sub3");
    auto sub4 = std::make_shared<TestSubscriber>(); //("sub4");
    auto writer = std::make_shared<TestWriter>(); //("speeder", nullptr);
    auto sub5 = std::make_shared<TestSubscriber>(); //("pres");


    hw_setup_params hw_params{uart_i, mbctrl, i2c_i, registry};
    sub_setup_params sub_params{registry, sub1, sub2, sub3, sub4, sub5, writer};

    xTaskCreate(setup_task, "setup_task", 512, &hw_params, tskIDLE_PRIORITY + 5, NULL);
    xTaskCreate(subscriber_setup_task, "subscriber_setup_task", 512, &sub_params, tskIDLE_PRIORITY + 3, NULL);

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
    params->subscriber = std::make_shared<TestSubscriber>("CO2");
    params->subscriber2 = std::make_shared<TestSubscriber>("Temperature");
    params->subscriber3 = std::make_shared<TestSubscriber>("Humidity");
    params->subscriber4 = std::make_shared<TestSubscriber>("Fan counter");
    params->subscriber5 = std::make_shared<TestSubscriber>("Pressure");
    params->writer = std::make_shared<TestWriter>("Fan Speed", params->registry->get_write_queue_handle(WriteType::FAN_SPEED));
    params->registry->subscribe_to_handler(ReadingType::CO2, params->subscriber->get_queue_handle());
    params->registry->subscribe_to_handler(ReadingType::TEMPERATURE, params->subscriber2->get_queue_handle());
    params->registry->subscribe_to_handler(ReadingType::REL_HUMIDITY, params->subscriber3->get_queue_handle());
    params->registry->subscribe_to_handler(ReadingType::FAN_COUNTER, params->subscriber4->get_queue_handle());
    params->registry->subscribe_to_handler(ReadingType::PRESSURE, params->subscriber5->get_queue_handle());

    vTaskSuspend(nullptr);
}
