#ifndef TASK_DEFINES_H
#define TASK_DEFINES_H

#include "FreeRTOS.h"
#include "task.h"

enum TaskPriority {
    TASK_PRIORITY_IDLE = (tskIDLE_PRIORITY),
    TASK_PRIORITY_LOW = (tskIDLE_PRIORITY + 1),
    TASK_PRIORITY_MEDIUM = (tskIDLE_PRIORITY + 2),
    TASK_PRIORITY_HIGH = (tskIDLE_PRIORITY + 3),
    TASK_PRIORITY_ABSOLUTE = (tskIDLE_PRIORITY + 4)
};

#endif
