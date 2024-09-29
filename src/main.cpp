#include "DeviceRegistry.h"
#include "FreeRTOS.h"
#include "PicoOsUart.h"
#include "RegisterHandler.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "modbus_register.h"
#include "pico/stdlib.h"
#include "semphr.h"
#include "ssd1306.h"
#include "task.h"
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>

#include "modbus_controller.h"
#include "modbus_register.h"
#include "uart_instance.h"

#include <limits.h>

extern "C" {
uint32_t read_runtime_ctr(void) { return timer_hw->timerawl; }
}

#define UART_TX_PIN 4
#define UART_RX_PIN 5

int main() {
    stdio_init_all();
    std::cout << "Booting" << std::endl;

    shared_uart uart_i{std::make_shared<Uart_instance>(1, 9600, UART_TX_PIN, UART_RX_PIN, 2)};
    shared_modbus mbctrl{std::make_shared<ModbusCtrl>(uart_i)};

    DeviceRegistry registry;
    
    auto co2 = std::make_shared<ModbusReadHandler>(mbctrl, 240, 0x0, 2, true, ReadingType::CO2, "CO2");
    auto temp = std::make_shared<ModbusReadHandler>(mbctrl, 241, 0x2, 2, true, ReadingType::TEMPERATURE, "Temp");
    auto hum = std::make_shared<ModbusReadHandler>(mbctrl, 241, 0x0, 2, true, ReadingType::REL_HUMIDITY, "Hum");
    auto fan = std::make_shared<ModbusReadHandler>(mbctrl, 1, 4, 1, false, ReadingType::FAN_COUNTER, "Fan Counter");
    //auto speed = std::make_shared<ModbusWriteHandler>(mbctrl, 1, 0, 1, WriteType::FAN_SPEED, "Fan Speed");

    registry.add_register_handler(ReadingType::CO2, co2);
    registry.add_register_handler(ReadingType::TEMPERATURE, temp);
    registry.add_register_handler(ReadingType::REL_HUMIDITY, hum);
    registry.add_register_handler(ReadingType::FAN_COUNTER, fan);
    //registry.add_register_handler(WriteType::FAN_SPEED, speed);

    TestSubscriber sub1("sub1");
    TestSubscriber sub2("sub2");
    TestSubscriber sub3("sub3");
    TestSubscriber sub4("sub4");
    //TestWriter writer("speeder", registry.subscribe_to_handler(WriteType::FAN_SPEED));

    registry.subscribe_to_handler(ReadingType::CO2, sub1.get_queue_handle());
    registry.subscribe_to_handler(ReadingType::TEMPERATURE, sub2.get_queue_handle());
    registry.subscribe_to_handler(ReadingType::REL_HUMIDITY, sub3.get_queue_handle());
    registry.subscribe_to_handler(ReadingType::FAN_COUNTER, sub4.get_queue_handle());


    vTaskStartScheduler();

    for (;;) {
    }
}


