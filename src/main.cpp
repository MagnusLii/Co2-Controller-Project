#include "FreeRTOS.h"

#include "IPStack.h"
#include "PicoI2C.h"
#include "api_tasks.h"
#include "connection_defines.h"
#include "connection_handler.h"
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

#include <climits>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>

#include "TLSConnect.hpp"
#include "connection_defines.h"


struct setup_params {
    std::shared_ptr<TLSConnect> con;
};

void setup_task(void *p) {
    auto pv = static_cast<setup_params*>(p);
    pv->con = std::make_shared<TLSConnect>(WIFI_SSID, WIFI_PASSWORD, THINGSPEAK_HOSTNAME, API_KEY, CERT2, sizeof(CERT2));
    vTaskSuspend(NULL);
}

int main() {
    stdio_init_all();
    std::cout << "start" << std::endl;
    std::shared_ptr<TLSConnect> con;
    setup_params pa{con};

    xTaskCreate(setup_task, "set", 500, &pa, tskIDLE_PRIORITY + 2, NULL);
    vTaskStartScheduler();
}