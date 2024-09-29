
#include "FreeRTOS.h"

#include "DeviceRegistry.h"
#include "PicoOsUart.h"
#include "RegisterHandler.h"
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
#include "i2c_instance.h"

#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <limits.h>

extern "C" {
uint32_t read_runtime_ctr(void) { return timer_hw->timerawl; }
}

#define UART_TX_PIN 4
#define UART_RX_PIN 5

int main() {
    stdio_init_all();
    std::cout << "Booting..." << std::endl;

    shared_uart uart_i{std::make_shared<Uart_instance>(1, 9600, UART_TX_PIN, UART_RX_PIN, 2)};
    //shared_i2c i2c{std::make_shared<I2C_instance>(i2c1, I2C1_BAUD, I2C1_SDA, I2C1_SCL)};
    shared_modbus mbctrl{std::make_shared<ModbusCtrl>(uart_i)};

    DeviceRegistry registry{mbctrl};

    TestSubscriber sub1("sub1");
    TestSubscriber sub2("sub2");
    TestSubscriber sub3("sub3");
    TestSubscriber sub4("sub4");
    TestWriter writer("speeder", registry.get_write_queue_handle(WriteType::FAN_SPEED));

    registry.subscribe_to_handler(ReadingType::CO2, sub1.get_queue_handle());
    registry.subscribe_to_handler(ReadingType::TEMPERATURE, sub2.get_queue_handle());
    registry.subscribe_to_handler(ReadingType::REL_HUMIDITY, sub3.get_queue_handle());
    registry.subscribe_to_handler(ReadingType::FAN_COUNTER, sub4.get_queue_handle());

    vTaskStartScheduler();

    for (;;) {
    }
}


