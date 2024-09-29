#include "debug.h"

static QueueHandle_t debug_q = xQueueCreate(10, sizeof(debug_event_t));

void debug(const char *format, uint32_t d1, uint32_t d2, uint32_t d3) {
    debug_event_t e{};
    e.format = format;
    e.data[0] = d1;
    e.data[1] = d2;
    e.data[2] = d3;
    e.time = xTaskGetTickCount() * portTICK_PERIOD_MS;

    xQueueSend(debug_q, &e, portMAX_DELAY);
}

void debug_task(void *pvParameters) {
    char buffer[128];
    debug_event_t e{};

    while (1) {
        if (xQueueReceive(debug_q, &e, portMAX_DELAY)) {
            snprintf(buffer, 128, e.format, e.data[0], e.data[1], e.data[2]);
            std::cout << "[" << e.time << "] " << buffer << std::endl;
        }
    }
}
