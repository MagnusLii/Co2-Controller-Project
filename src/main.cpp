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

// void read_old(void *pvParameters) {
//     auto reg = static_cast<ReadRegister *>(pvParameters);
//     while (true) {
//         reg->start_transfer();
//         while (mbctrl->isbusy())
//             vTaskDelay(10); // IMPORTANT if modbus controller is busy it means the transfers are
//             not
//                             // compeleted yet
//         std::cout << reg->get_float() << std::endl;
//         vTaskDelay(1000);
//     }
// }

int main() {
    stdio_init_all();
    std::cout << "Booting" << std::endl;

    shared_uart u{std::make_shared<Uart_instance>(1, 9600, UART_TX_PIN, UART_RX_PIN,
                                                  2)}; // 1 for testbox 2 for fullscale
    shared_modbus mbctrl{std::make_shared<ModbusCtrl>(u)};

    auto co2_handler = std::make_shared<ModbusReadHandler>(mbctrl, 240, 0x0, 2, true, ReadingType::CO2, "co2");
    auto temp_handler = std::make_shared<ModbusReadHandler>(mbctrl, 241, 0x2, 2, true, ReadingType::TEMPERATURE, "temp");
    auto rh_handler = std::make_shared<ModbusReadHandler>(mbctrl, 241, 0x0, 2, true, ReadingType::REL_HUMIDITY, "rh");
    auto fcounter_handler = std::make_shared<ModbusReadHandler>(mbctrl, 1, 0x4, 2, true, ReadingType::FAN_COUNTER, "fcounter");

    SubscriptionManager sm;
    TestSubscriber eeprom("EEPROM");
    TestSubscriber display("Display");

    sm.add_register_handler(ReadingType::CO2, co2_handler);
    sm.add_register_handler(ReadingType::TEMPERATURE, temp_handler);

    sm.subscribe_to_handler(ReadingType::CO2, eeprom.get_queue_handle());
    sm.subscribe_to_handler(ReadingType::TEMPERATURE, eeprom.get_queue_handle());
    sm.subscribe_to_handler(ReadingType::CO2, display.get_queue_handle());
    sm.subscribe_to_handler(ReadingType::TEMPERATURE, display.get_queue_handle());

    // ReadRegister co2(mbctrl, 240, 0x0, 2);
    // ReadRegister temp(mbctrl, 241, 0x2, 2);

    // xTaskCreate(read_old, "read_co2", 256, &co2, tskIDLE_PRIORITY + 1, nullptr);
    // xTaskCreate(read_old, "read_temp", 256, &temp, tskIDLE_PRIORITY + 1, nullptr);

    // xTaskCreate(read_task, "read_task", 256, (void *) &co2, tskIDLE_PRIORITY + 1, nullptr);

    // xTaskCreate(read_task_16, "read_task_16", 256, (void *) &co, tskIDLE_PRIORITY + 1, nullptr);

    vTaskStartScheduler();

    for (;;) {
    }
}

