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

void write_to_eeprom_task(void *pvParams){
    vTaskDelay(10000);
    const auto params = static_cast<DeviceRegistry*>(pvParams);
    //auto params = sub_setup_param->loghandler;

    QueueHandle_t readingsQueue = xQueueCreate(10, sizeof (Reading));

    params->subscribe_to_handler(ReadingType::CO2, readingsQueue);
    params->subscribe_to_handler(ReadingType::TEMPERATURE, readingsQueue);
    params->subscribe_to_handler(ReadingType::REL_HUMIDITY, readingsQueue);
    params->subscribe_to_handler(ReadingType::PRESSURE, readingsQueue);

    /*
    Reading reading;
    reading.type = ReadingType::CO2;
    reading.value.f32 = 2;
     */
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

            std::cout << "CO2 " << values[0] << std::endl;
            std::cout << "temperature " << values[1] << std::endl;
            std::cout << "REL_HUMIDITY " << values[2] << std::endl;
            std::cout << "PRESSURE " << values[3] << std::endl;

            storeData(&values[0], &values[1], &values[2], values[3]);

            std::cout << "done " << std::endl;
            //vTaskDelay(pdMS_TO_TICKS(5000));
            vTaskDelete(nullptr);
        }
    }

}

void read_eeprom_task(void *param) {
    float aa, bb, cc;
    int16_t gg;
    int arr[4] = {0, 0, 0, 0};

    vTaskDelay(pdMS_TO_TICKS(10000));
    auto *logHandler = static_cast<LogHandler *>(param);
    logHandler->fetchData(&aa, &bb, &cc, &gg, arr);

    std::cout << "CO2 " << aa << std::endl;
    std::cout << "temperature " << bb << std::endl;
    std::cout << "REL_HUMIDITY " << cc << std::endl;
    std::cout << "PRESSURE " << gg << std::endl;
    vTaskSuspend(nullptr);

}