
#include "FreeRTOS.h"
#include "PicoI2C.h"
#include "TLSWrapper.h"
#include "connection_defines.h"
#include "device_registry.h"
#include "hardware_const.h"
#include "modbus_controller.h"
#include "pico/stdlib.h"
#include "read_runtime_ctr.cpp"
#include "rotary.hpp"
#include "screen.hpp"
#include "logger.hpp"
#include "task.h"
#include "task_defines.h"
#include "uart_instance.h"
#include <iostream>
#include <memory>

struct setup_params {
    shared_uart uart;
    shared_i2c i2c_0;
    shared_i2c i2c_1;
    shared_modbus modbus;
    std::shared_ptr<DeviceRegistry> registry;
    std::shared_ptr<TLSWrapper> connection;
    std::shared_ptr<Screen> screen;
    std::shared_ptr<Rotary> rotary;
    std::shared_ptr<Logger> logger;
};

void setup_task(void *pvParameters) {
    auto params = static_cast<setup_params *>(pvParameters);

    params->uart = std::make_shared<Uart_instance>(UART_NR, UART_BAUD, UART_TX_PIN, UART_RX_PIN, UART_BITS);
    params->i2c_0 = std::make_shared<PicoI2C>(I2C_0);
    params->i2c_1 = std::make_shared<PicoI2C>(I2C_1);

    params->logger = std::make_shared<Logger>(params->i2c_0);
    params->modbus = std::make_shared<ModbusCtrl>(params->uart);
    params->registry = std::make_shared<DeviceRegistry>(params->modbus, params->i2c_1);
    params->registry->set_initial_values(params->logger->co2_target_return, params->logger->fan_speed_return, false);

    params->screen = std::make_shared<Screen>(params->i2c_1);
    params->rotary = std::make_shared<Rotary>();

    //rice logger to wanted reading values
    params->registry->subscribe_to_handler(ReadingType::CO2_TARGET, params->logger->get_reading_queue_handle());
    params->registry->subscribe_to_handler(ReadingType::FAN_SPEED, params->logger->get_reading_queue_handle());


    // subsrice connection to all the reading values
    params->connection = std::make_shared<TLSWrapper>(WIFI_SSID, WIFI_PASSWORD, DEFAULT_COUNTRY_CODE);
    params->registry->subscribe_to_handler(ReadingType::CO2, params->connection->get_read_handle());
    params->registry->subscribe_to_handler(ReadingType::REL_HUMIDITY, params->connection->get_read_handle());
    params->registry->subscribe_to_handler(ReadingType::TEMPERATURE, params->connection->get_read_handle());
    params->registry->subscribe_to_handler(ReadingType::CO2_TARGET, params->connection->get_read_handle());
    params->registry->subscribe_to_handler(ReadingType::FAN_SPEED, params->connection->get_read_handle());
    params->connection->set_write_handle(params->registry->get_write_queue_handle());
    // subscribing screen to wanted reading values
    params->registry->subscribe_to_handler(ReadingType::CO2, params->screen->get_reading_queue_handle());
    params->registry->subscribe_to_handler(ReadingType::TEMPERATURE, params->screen->get_reading_queue_handle());
    params->registry->subscribe_to_handler(ReadingType::REL_HUMIDITY, params->screen->get_reading_queue_handle());
    params->registry->subscribe_to_handler(ReadingType::PRESSURE, params->screen->get_reading_queue_handle());
    params->registry->subscribe_to_handler(ReadingType::FAN_SPEED, params->screen->get_reading_queue_handle());
    params->registry->subscribe_to_handler(ReadingType::CO2_TARGET, params->screen->get_reading_queue_handle());
    params->rotary->add_subscriber(params->screen->get_control_queue_handle());

    vTaskSuspend(NULL);
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
    std::shared_ptr<Logger> logger;

    setup_params params{uart_i, i2c_0, i2c_1, mbctrl, registry, connection, screen, rotary, logger};

    xTaskCreate(setup_task, "SETUP", 512, &params, TaskPriority::ABSOLUTE, NULL);
    vTaskStartScheduler();
    for (;;);
}
