#include "connection_handler.h"
#include "connection_defines.c"
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <vector>
#include <string>
#include <cstring>
#include <iostream>

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


// A structure to store both the message to send and its length.
struct Message {
    std::vector<unsigned char> data;
    int length;
};


void send_data_task(void *param) {
    ConnectionHandler *connHandler = static_cast<ConnectionHandler *>(param);
    Message msgToSend;
    std::vector<unsigned char> receivedData;

    while (!connHandler->isIPStackInitialized()) {
        vTaskDelay(pdMS_TO_TICKS(ONE_SECOND));
    }

    for(;;) {
        if (xQueueReceive(sendQueue, &msgToSend, portMAX_DELAY) == pdPASS) {
            while (!connHandler->isConnected()) { // TODO: maybe this can be done in an independent task.
                connHandler->connect(THINGSPEAK_HOSTNAME, THINGSPEAK_PORT);
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
    ConnectionHandler *connHandler = static_cast<ConnectionHandler *>(param);
    std::vector<unsigned char> receivedData;

    while (!connHandler->isIPStackInitialized()) {
        vTaskDelay(pdMS_TO_TICKS(ONE_SECOND));
    }

    for(;;) {
        receivedData = connHandler->receive(MAX_MESSAGE_LENGHT, RECEIVE_TIMEOUT_MS); // TODO: this is blocking.

        if (!receivedData.empty()) {

            while (!connHandler->isConnected()) { // TODO: maybe this can be done in an independent task.
                connHandler->connect(THINGSPEAK_HOSTNAME, THINGSPEAK_PORT);
            }

            DEBUG_printf("Received %lu bytes\n", receivedData.size());
            DEBUG_printf("Received data: %s\n", receivedData.data());
        }

        vTaskDelay(pdMS_TO_TICKS(TIMEOUT_MS)); // Tested to not miss incoming data even with 10s timeout.
    }
}


void initialize_IPStack_task(void *param) {
    ConnectionHandler *connHandler = static_cast<ConnectionHandler *>(param);

    while (!connHandler->isIPStackInitialized()) {
        connHandler->initializeIPStack();
        vTaskDelay(pdMS_TO_TICKS(ONE_SECOND));
    }
    
    while (!connHandler->connect(THINGSPEAK_HOSTNAME, THINGSPEAK_PORT)) {
        vTaskDelay(pdMS_TO_TICKS(ONE_SECOND));
    }

    vTaskDelete(NULL);
}

// void generate_test_data_task(void *param) {
//     ConnectionHandler *connHandler = static_cast<ConnectionHandler *>(param);

//         for(;;) {
//             vTaskDelay(pdMS_TO_TICKS(10000));
//             connHandler->pushMessageToQueue("POST https://api.thingspeak.com/update", REQUEST_AND_URL,
//                                             "12", FIELD1,
//                                             nullptr);
//         }
// }
