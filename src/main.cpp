#include "FreeRTOS.h"
#include "ModbusRegister.h"
#include "ModbusRegister_32.h"
#include "RegisterHandler.h"
#include "PicoOsUart.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "pico/stdlib.h"
#include "semphr.h"
#include "ssd1306.h"
#include "task.h"
#include <iostream>
#include <sstream>

#include "uart_instance.h"
#include "modbus_controller.h"
#include "modbus_register.h"

extern "C" {
    uint32_t read_runtime_ctr(void) {
        return timer_hw->timerawl;
    }
}

#define UART_TX_PIN 4
#define UART_RX_PIN 5

shared_uart u{std::make_shared<Uart_instance>(1, 9600, UART_TX_PIN, UART_RX_PIN, 2)}; // 1 for testbox 2 for fullscale
shared_modbus mbctrl{std::make_shared<ModbusCtrl>(u)};

void read_task(void *pvParameters) {
    auto reg = static_cast<ModbusRegister32 *>(pvParameters);
    float f = 0;
    while (true) {
        f = reg->read_float();
        vTaskDelay(2000);
        std::cout << "Reading float: " << f << std::endl;
    }
}

void read_task_16(void *pvParameters) {
    auto reg = static_cast<ModbusRegister *>(pvParameters);
    uint16_t u = 0;
    while (true) {
        u = reg->read();
        vTaskDelay(2000);
        std::cout << "Reading uint16: " << u << std::endl;
    }
}

void read_old(void *pvParameters) {
    auto reg = static_cast<ReadRegister *>(pvParameters);
    while (true) {
        reg->start_transfer();
        while (mbctrl->isbusy())
            vTaskDelay(10); // IMPORTANT if modbus controller is busy it means the transfers are not compeleted yet
        std::cout << reg->get_float() << std::endl;
        vTaskDelay(1000);
    }
}

int main() {
    stdio_init_all();
    std::cout << "Booting" << std::endl;

    //std::shared_ptr<PicoOsUart> uart{std::make_shared<PicoOsUart>(1, 4, 5, 9600, 1)};
    //std::shared_ptr<ModbusClient> mb_client{std::make_shared<ModbusClient>(uart)};

    //ModbusRegister co(mb_client, 240, 0x101);
    //ModbusRegister32 co2(mb_client, 240, 0, 2);


    //ModbusRegister32 temp(mb_client, 241, 0x2, 2);
    //ModbusRegister32 rh(mb_client, 241, 0x0, 2);

    ReadRegister co2(mbctrl, 240, 0x0, 2);
    ReadRegister temp(mbctrl, 241, 0x2, 2);

    xTaskCreate(read_old, "read_co2", 256, &co2, tskIDLE_PRIORITY + 1, nullptr);
    xTaskCreate(read_old, "read_temp", 256, &temp, tskIDLE_PRIORITY + 1, nullptr);

    //xTaskCreate(read_task, "read_task", 256, (void *) &co2, tskIDLE_PRIORITY + 1, nullptr);

    //xTaskCreate(read_task_16, "read_task_16", 256, (void *) &co, tskIDLE_PRIORITY + 1, nullptr);

    vTaskStartScheduler();

    for (;;) {
    }
}

