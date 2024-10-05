#ifndef TASK_DEFINES_H
#define TASK_DEFINES_H

#include "FreeRTOS.h"
#include "task.h"

enum TaskPriority {
    IDLE = (tskIDLE_PRIORITY),
    LOW = (tskIDLE_PRIORITY + 1),
    MEDIUM = (tskIDLE_PRIORITY + 2),
    HIGH = (tskIDLE_PRIORITY + 3),
    ABSOLUTE = (tskIDLE_PRIORITY + 4)
};

#endif
