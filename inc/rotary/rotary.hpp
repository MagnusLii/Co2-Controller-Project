#ifndef ROTARY_H
#define ROTARY_H

#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "queue.h"

#include "register_handler.h"


class Rotary {
    friend void rotary_irq_handler(uint gpio, uint32_t mask);
    public:
        Rotary(uint ROT_SW = 12, uint ROT_A = 10, uint ROT_B = 11, uint button = 8);
        // void subscribe_to_handler(ReadingType type, QueueHandle_t receiver);
        void add_subscriber(QueueHandle_t subscriber);
        void remove_subscriber(QueueHandle_t subscriber);
    private:
        void send_reading_from_isr();
        void irq_handler(uint gpio, uint32_t mask);
        bool debounce(void);
        void get_reading() {}
        const uint rot_sw;
        const uint rot_a;
        const uint rot_b;
        const uint button;
        bool is_co2 = false;
        uint16_t position = 0;
        uint32_t time;
        std::vector<QueueHandle_t> subscribers;
        Command command;
};

#endif