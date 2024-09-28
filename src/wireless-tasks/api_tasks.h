#ifndef COMMUNICATION_TASKS_H
#define COMMUNICATION_TASKS_H

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
#define MAX_MESSAGE_LENGTH 1024
#define TIMEOUT_MS 5000
#define RECEIVE_TIMEOUT_MS 100
#define QUEUE_TIMEOUT 100

#define DEBUG_printf printf

extern QueueHandle_t sendQueue;
extern QueueHandle_t receiveQueue;

struct Message {
    std::vector<unsigned char> data;
    int length;
};

void send_data_task(void *param);
void receive_data_task(void *param);
void fully_initialize_connhandler_task(void *param);
void reconnect_task(void *param);
// void generate_test_data_task(void *param);

#endif
