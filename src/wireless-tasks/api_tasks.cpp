#include "connection_handler.h"
#include "connection_defines.h"
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <vector>
#include <string>
#include <cstring>
#include <iostream>
#include "api_tasks.h"
#include "task_defines.h"
#include "connection_defines.h"

#define SEND_QUEUE_SIZE 64
#define RECEIVE_QUEUE_SIZE 64
#define MAX_MESSAGE_LENGHT 1024
#define TIMEOUT_MS 5000
#define ONE_SECOND 1000
#define RECEIVE_TIMEOUT_MS 100
#define QUEUE_TIMEOUT 100

#define DEBUG_printf printf

// Queue handles
QueueHandle_t sendQueue;
QueueHandle_t receiveQueue;

// Taskhandles for notification
TaskHandle_t reconnectHandle = NULL;


void send_data_task(void *param) {
    auto *connHandler = static_cast<ConnectionHandler *>(param);
    Message msgToSend;
    std::vector<unsigned char> receivedData;

    while (connHandler->isIPStackInitialized() != true) {
        DEBUG_printf("send_data_task(): while loop: isIPStackinitialized: %d", connHandler->isIPStackInitialized());
        vTaskDelay(pdMS_TO_TICKS(5 * ONE_SECOND));
    }

    for(;;) {

        if (xQueueReceive(sendQueue, &msgToSend, portMAX_DELAY) == pdPASS) {

            if (connHandler->isConnected() != true) {
                xTaskNotifyGive(reconnectHandle);
                while (connHandler->isConnected() != true) // prevent multiple notifications.
                {
                    vTaskDelay(pdMS_TO_TICKS(5 * ONE_SECOND));
                }
            }

            int result = connHandler->send(msgToSend.data);

            if (result >= 0) {
                DEBUG_printf("Message sent: %d bytes\n", result);
            } else {
                DEBUG_printf("Failed to send message\n");
            }

        }
    }
}


// This task should be high prio.
void receive_data_task(void *param) {
    auto *connHandler = static_cast<ConnectionHandler *>(param);
    std::vector<unsigned char> receivedData;

    while (connHandler->isIPStackInitialized() != true) {
        vTaskDelay(pdMS_TO_TICKS(ONE_SECOND));
    }

    for(;;) {

        if (connHandler->isConnected() != true) {
            xTaskNotifyGive(reconnectHandle);
            while (!connHandler->isConnected()) // prevent multiple notifications.
            {
                vTaskDelay(pdMS_TO_TICKS(5 * ONE_SECOND));
            }
        }

        receivedData = connHandler->receive(MAX_MESSAGE_LENGHT, RECEIVE_TIMEOUT_MS); // TODO: this is blocking.

        if (!receivedData.empty()) {
            DEBUG_printf("Received %lu bytes\n", receivedData.size());
            DEBUG_printf("Received data: %s\n", receivedData.data());
        }

        vTaskDelay(pdMS_TO_TICKS(TIMEOUT_MS)); // Tested to not miss incoming data even with 10s timeout.
    }
}


// void test(void *param) {
//     auto *connHandler = static_cast<ConnectionHandler *>(param);
//         while (connHandler->isIPStackInitialized() != true) {
//         vTaskDelay(pdMS_TO_TICKS(ONE_SECOND));
//     }
// }


// This task must be called, and should preferably be one of the first tasks to be called on startup.
void fully_initialize_connhandler_task(void *param) {
    auto *connHandler = static_cast<ConnectionHandler *>(param);

    // This may not be working as intended, doesn't seem to reinit upon failure.
    while (!connHandler->isIPStackInitialized()) {
        connHandler->initializeIPStack();
        vTaskDelay(pdMS_TO_TICKS(5 * ONE_SECOND));
    }
    
    while (!connHandler->connect(THINGSPEAK_IP, THINGSPEAK_PORT)) {
        vTaskDelay(pdMS_TO_TICKS(5 * ONE_SECOND));
    }

    // Create dependent tasks
    xTaskCreate(reconnect_task, "Reconnect task", 2048, connHandler, TASK_PRIORITY_LOW, &reconnectHandle);
    xTaskCreate(send_data_task, "send", 2048, (void *) connHandler, TASK_PRIORITY_LOW, nullptr);
    xTaskCreate(receive_data_task, "receive", 2048, (void *) connHandler, TASK_PRIORITY_LOW, nullptr);

    vTaskDelete(NULL);
}


void reconnect_task(void *param) {
    auto *connHandler = static_cast<ConnectionHandler *>(param);

    for (;;){
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        while (!connHandler->isConnected()) {
            connHandler->connect(THINGSPEAK_IP, THINGSPEAK_PORT);
            // TODO: Maybe add a IPStack reinitialization here.
        }
    }
}
