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
#include "connection_defines.h"
#include "api_tasks.h"
#include "task_defines.h"


void test_task(void *param) {
    ConnectionHandler *connHandler = static_cast<ConnectionHandler *>(param);
    while (connHandler->isConnected() != true && connHandler->isIPStackInitialized() != true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    Message msg;

    float values[8] = {2.0, 3.0, 4.0, 1.0, 1.0, 2.0, 5.0, 2.0};

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        msg.length = connHandler->create_minimal_push_message(msg.data, values);
        DEBUG_printf("Message length: %d\n", msg.length);
        
        DEBUG_printf("\n---------------------------------------------------------------\n\n");        
        for (int i = 0; i < msg.length; i++) {
            printf("%c", msg.data[i]);
        }

        DEBUG_printf("\n---------------------------------------------------------------\n\n");

        if (msg.length > 0) {
            xQueueSend(sendQueue, &msg, QUEUE_TIMEOUT);
        }
    }


    xQueueSend(sendQueue, &msg, QUEUE_TIMEOUT);
    vTaskDelete(NULL);
}


int main() {
    stdio_init_all();
    
    sendQueue = xQueueCreate(SEND_QUEUE_SIZE, sizeof(Message));
    receiveQueue = xQueueCreate(RECEIVE_QUEUE_SIZE, sizeof(Message));

    ConnectionHandler connHandler = ConnectionHandler();

    xTaskCreate(fully_initialize_connhandler_task, "init IPStack", 1024, (void *) &connHandler, TASK_PRIORITY_ABSOLUTE, nullptr);

    // xTaskCreate(test_task, "test", 1024, (void*) &connHandler, TASK_PRIORITY_LOW, nullptr);
    vTaskStartScheduler();

    for(;;){
        printf("shit is wrong\n");
    }
    return 0;
}