#include "pico/stdlib.h"
#include <iostream>
#include <sstream>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hardware/gpio.h"
#include "PicoOsUart.h"
#include "ssd1306.h"
#include "hardware/timer.h"
#include "connection_handler.h"
#include "IPStack.h"
#include <read_runtime_ctr.cpp>
#include "connection_defines.c"
#include "api_tasks.h"



void test_task(void *param) {
    ConnectionHandler *connHandler = static_cast<ConnectionHandler *>(param);

    for(;;) {

        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

int main() {
    stdio_init_all();
    
    sendQueue = xQueueCreate(SEND_QUEUE_SIZE, sizeof(Message));
    receiveQueue = xQueueCreate(RECEIVE_QUEUE_SIZE, sizeof(Message));

    ConnectionHandler connHandler = ConnectionHandler();

    xTaskCreate(send_data_task, "send", 1024, (void *) &connHandler, tskIDLE_PRIORITY + 1, nullptr);
    xTaskCreate(receive_data_task, "receive", 1024, (void *) &connHandler, tskIDLE_PRIORITY + 1, nullptr);
    // xTaskCreate(generate_test_data_task, "generate", 1024, (void*) &connHandler, tskIDLE_PRIORITY + 1, nullptr);
    xTaskCreate(initialize_IPStack_task, "init", 1024, (void *) &connHandler, tskIDLE_PRIORITY + 2, nullptr);

    // xTaskCreate(test_task, "test", 4096, (void*) &connHandler, tskIDLE_PRIORITY + 1, nullptr);
    vTaskStartScheduler();

    for(;;){
        printf("shit is wrong\n");
    }

    return 0;
}
