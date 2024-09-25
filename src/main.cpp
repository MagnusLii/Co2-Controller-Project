#include "FreeRTOS.h"
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

int main() {
    stdio_init_all();
    std::cout << "Boot" << std::endl;

    std::shared_ptr<PicoOsUart> uart{std::make_shared<PicoOsUart>(0, 0, 1, 9600)};
    std::shared_ptr<ModbusClient> mb_client{std::make_shared<ModbusClient>(uart)};

    ModbusRegister32 mb_read_reg_co2{mb_client, 240, 0, 2};
    ModbusRegister32 mb_read_reg_temp{mb_client, 241, 1, 2};
    ModbusRegister32 mb_read_reg_fan{mb_client, 1, 4, 2, false};
    ModbusRegister32 mb_write_reg_fan{mb_client, 1, 0, 2};

    for (;;) {
    }
}
