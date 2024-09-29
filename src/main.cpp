
#include "FreeRTOS.h"

#include "DeviceRegistry.h"
#include "PicoOsUart.h"
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
// #include "i2c_instance.h"
#include "PicoI2C.h"

#include <cstdint>
#include <iostream>
#include <limits.h>
#include <map>
#include <memory>
#include <sstream>

extern "C" {
uint32_t read_runtime_ctr(void) { return timer_hw->timerawl; }
}


int main() {
    stdio_init_all();
    std::cout << "Booting..." << std::endl;

    shared_uart uart_i{std::make_shared<Uart_instance>(UART_NR, UART_BAUD, UART_TX_PIN, UART_RX_PIN, 2)};
    shared_modbus mbctrl{std::make_shared<ModbusCtrl>(uart_i)};
    shared_i2c i2c_i{std::make_shared<PicoI2C>(I2C_NR)};

    DeviceRegistry registry{mbctrl, i2c_i};

    TestSubscriber sub1("sub1");
    TestSubscriber sub2("sub2");
    TestSubscriber sub3("sub3");
    TestSubscriber sub4("sub4");
    TestWriter writer("speeder", registry.get_write_queue_handle(WriteType::FAN_SPEED));
    //TestSubscriber sub5("pres");

    registry.subscribe_to_handler(ReadingType::CO2, sub1.get_queue_handle());
    registry.subscribe_to_handler(ReadingType::TEMPERATURE, sub2.get_queue_handle());
    registry.subscribe_to_handler(ReadingType::REL_HUMIDITY, sub3.get_queue_handle());
    registry.subscribe_to_handler(ReadingType::FAN_COUNTER, sub4.get_queue_handle());
    //registry.subscribe_to_handler(ReadingType::PRESSURE, sub5.get_queue_handle());

    vTaskStartScheduler();

    for (;;) {
    }
}


