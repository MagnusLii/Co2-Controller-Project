#include "FreeRTOS.h"
#include "PicoOsUart.h"
#include "RegisterHandler.h"
#include "SubscriptionManager.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include "semphr.h"
#include "ssd1306.h"
#include "task.h"
#include <iostream>
#include <map>
#include <memory>
#include <sstream>

#include "modbus_controller.h"
#include "modbus_register.h"
#include "uart_instance.h"

extern "C" {
uint32_t read_runtime_ctr(void) { return timer_hw->timerawl; }
}

#define UART_TX_PIN 4
#define UART_RX_PIN 5

void init_modbus_handlers(SubscriptionManager &manager, shared_modbus mbctrl) {

}

int main() {
    stdio_init_all();
    std::cout << "Booting" << std::endl;

    shared_uart uart_i{std::make_shared<Uart_instance>(1, 9600, UART_TX_PIN, UART_RX_PIN,
                                                  2)}; // 1 for testbox 2 for fullscale
    shared_modbus mbctrl{std::make_shared<ModbusCtrl>(uart_i)};

    SubscriptionManager manager;

    //init_modbus_handlers(manager, mbctrl);

    //ReadRegister co2_register{mbctrl, 240, 0x0, 2};
    //ReadRegister temp_register{mbctrl, 241, 0x2, 2};

    //ModbusReadHandler co2(mbctrl, &co2_register, ReadingType::CO2, "co2");
    //ModbusReadHandler temp(mbctrl, &temp_register, ReadingType::TEMPERATURE, "temp");

    auto co2 = std::make_unique<ModbusReadHandler>(mbctrl, 240, 0x0, 2, true, ReadingType::CO2, "co2");
    auto temp = std::make_unique<ModbusReadHandler>(mbctrl, 241, 0x2, 2, true, ReadingType::TEMPERATURE, "temp");
    auto rh = std::make_unique<ModbusReadHandler>(mbctrl, 241, 0x0, 2, true, ReadingType::REL_HUMIDITY, "rh");
    auto fcount = std::make_unique<ModbusReadHandler>(mbctrl, 1, 0x4, 1, true, ReadingType::FAN_COUNTER, "fcounter");
    auto fspeed = std::make_unique<ModbusWriteHandler>(mbctrl, 1, 0x0, 1, WriteType::FAN_SPEED, "fspeed");

    manager.add_register_handler(ReadingType::CO2, std::move(co2));
    manager.add_register_handler(ReadingType::TEMPERATURE, std::move(temp));
    manager.add_register_handler(ReadingType::REL_HUMIDITY, std::move(rh));
    manager.add_register_handler(ReadingType::FAN_COUNTER, std::move(fcount));
    manager.add_register_handler(WriteType::FAN_SPEED, std::move(fspeed));

    TestSubscriber eeprom("CO2");
    TestSubscriber display("TEMPERATURE");
    TestSubscriber wifi("REL_HUMIDITY");
    TestSubscriber fan_fan("FAN_COUNTER");
    //TestWriter writer("Controller", manager);

    manager.subscribe_to_handler(ReadingType::CO2, eeprom.get_queue_handle());
    manager.subscribe_to_handler(ReadingType::TEMPERATURE, display.get_queue_handle());
    manager.subscribe_to_handler(ReadingType::REL_HUMIDITY, wifi.get_queue_handle());
    manager.subscribe_to_handler(ReadingType::FAN_COUNTER, fan_fan.get_queue_handle());

    vTaskStartScheduler();

    for (;;) {
    }
}
