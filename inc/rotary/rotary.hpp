#ifndef ROTARY_H
#define ROTARY_H

#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "queue.h"


class Rotary {
    friend void rotary_irq_handler(uint gpio, uint32_t mask);
    public:
        Rotary(uint ROT_SW = 12, uint ROT_A = 10, uint ROT_B = 11);
        // void subscribe_to_handler(ReadingType type, QueueHandle_t receiver);

    private:
        void irq_handler(uint gpio, uint32_t mask);
        bool debounce(void);
        const uint rot_sw;
        const uint rot_a;
        const uint rot_b;
        uint32_t time;
};

#endif