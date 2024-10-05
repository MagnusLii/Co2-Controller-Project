
#include "FreeRTOS.h"

#include "PicoI2C.h"
#include "connection_defines.h"
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
#include "TLSWrapper.h"

#include <climits>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>


struct setup_params {
    shared_uart uart;
    shared_i2c i2c_0;
    shared_i2c i2c_1;
    shared_modbus modbus;
    std::shared_ptr<DeviceRegistry> registry;
    std::shared_ptr<TLSWrapper> connection;
    std::shared_ptr<Screen> screen;
    std::shared_ptr<Rotary> rotary;
};

void setup_task(void *pvParameters) {
    auto params = static_cast<setup_params*>(pvParameters);


    params->uart = std::make_shared<Uart_instance>(UART_NR, UART_BAUD, UART_TX_PIN, UART_RX_PIN, UART_BITS);
    params->i2c_0 = std::make_shared<PicoI2C>(I2C_0);
    params->i2c_1 = std::make_shared<PicoI2C>(I2C_1);

    params->modbus = std::make_shared<ModbusCtrl>(params->uart);
    params->registry = std::make_shared<DeviceRegistry>(params->modbus, params->i2c_1);

    params->screen = std::make_shared<Screen>(params->i2c_1);
    params->rotary = std::make_shared<Rotary>();

    params->connection = std::make_shared<TLSWrapper>(WIFI_SSID, WIFI_PASSWORD, DEFAULT_COUNTRY_CODE);

    // SUBSRAIB TO MY CHANNEL

}

int main() {
    stdio_init_all();
    std::cout << "Booting..." << std::endl;

    shared_uart uart_i;
    shared_i2c i2c_0;
    shared_i2c i2c_1;

    shared_modbus mbctrl;
    std::shared_ptr<DeviceRegistry> registry;

    std::shared_ptr<TLSWrapper> connection;

    std::shared_ptr<Screen> screen;

    std::shared_ptr<Rotary> rotary;

    setup_params params{};

    xTaskCreate(setup_task, "SETUP", 256, &params, TaskPriority::ABSOLUTE, NULL);


    vTaskStartScheduler();
    for (;;) {}
}
