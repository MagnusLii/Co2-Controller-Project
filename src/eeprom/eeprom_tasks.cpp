#include "FreeRTOS.h"

#include "device_registry.h"
#include "register_handler.h"
#include "task.h"
#include "logHandler.h"

#include <cstdint>
#include <iostream>
#include <climits>
#include <map>
#include <memory>
#include <sstream>
#include "queue.h"

struct sub_setup_params {
    std::shared_ptr<DeviceRegistry> registry;
    LogHandler* loghandler;
};

void write_to_eeprom_task(void *pvParams){
    vTaskDelay(1000);
    const auto sub_setup_param = static_cast<sub_setup_params*>(pvParams);
    auto params = sub_setup_param->loghandler;

    QueueHandle_t readingsQueue = xQueueCreate(10, sizeof (Reading));

    sub_setup_param->registry->subscribe_to_handler(ReadingType::CO2, readingsQueue);
    sub_setup_param->registry->subscribe_to_handler(ReadingType::TEMPERATURE, readingsQueue);
    sub_setup_param->registry->subscribe_to_handler(ReadingType::REL_HUMIDITY, readingsQueue);
    sub_setup_param->registry->subscribe_to_handler(ReadingType::PRESSURE, readingsQueue);

    Reading reading;
    reading.type = ReadingType::CO2;
    reading.value.f32 = 2;
    float values[4] = {0.0, 0.0, 0.0, 0.0};
    Reading received;
    for(;;){

        while (xQueueReceive(readingsQueue, &received, 0) == pdTRUE){
            switch (received.type) {
            case ReadingType::CO2:
                values[0] = received.value.f32;
                break;
            case ReadingType::TEMPERATURE:
                values[1] = received.value.f32;
                break;
            case ReadingType::REL_HUMIDITY:
                values[2] = received.value.f32;
                break;
            case ReadingType::PRESSURE:
                values[3] = received.value.f32;
                break;
            default:
                break;
            }
            /*
            std::cout << "CO2 " << values[0] << std::endl;
            std::cout << "temperature " << values[1] << std::endl;
            std::cout << "REL_HUMIDITY " << values[2] << std::endl;
            std::cout << "PRESSURE " << values[3] << std::endl;
             */

            sub_setup_param->loghandler->storeData(&values[0], &values[1], &values[2], values[3]);

            std::cout << "done " << std::endl;
            vTaskDelay(pdMS_TO_TICKS(1000));
            //vTaskDelete(nullptr);
        }
    }

}

void read_eeprom_task(void *param) {
    float aa, bb, cc;
    int16_t gg;
    int arr[4] = {0, 0, 0, 0};

    vTaskDelay(pdMS_TO_TICKS(1000));
    auto *logHandler = static_cast<LogHandler *>(param);
    logHandler->fetchData(&aa, &bb, &cc, &gg, arr);

    std::cout << "CO2 " << aa << std::endl;
    std::cout << "temperature " << bb << std::endl;
    std::cout << "REL_HUMIDITY " << cc << std::endl;
    std::cout << "PRESSURE " << gg << std::endl;
    vTaskSuspend(nullptr);

}