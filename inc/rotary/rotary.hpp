#ifndef ROTARY_H
#define ROTARY_H

#include "pico/stdlib.h"

#include "FreeRTOS.h"
#include "queue.h"

#include "register_handler.h"
#include <vector>
#include "task_defines.h"

#define COUNTER_CLOCKWISE 2
#define CLOCKWISE 1


class Rotary {
    friend void rotary_irq_handler(uint gpio, uint32_t mask);
    public:
        Rotary(uint ROT_SW = 12, uint ROT_A = 10, uint ROT_B = 11, uint button = 8);
        // void subscribe_to_handler(ReadingType type, QueueHandle_t receiver);
        void add_subscriber(QueueHandle_t subscriber);
        void remove_subscriber(QueueHandle_t subscriber);
        void set_initial_values(float co2_target, uint16_t fan_speed, bool is_manual);
    private:
        static void send_task(void *pvParameters);
        void send_reading_from_isr();
        void send_reading();
        void irq_handler(uint gpio, uint32_t mask);
        bool debounce(void);
        void get_reading() {}
        const uint rot_sw;
        const uint rot_a;
        const uint rot_b;
        const uint button;
        uint32_t time;
        float co2_target_local;
        uint16_t fan_speed_local;
        bool is_manual_local;
        bool toggled = false;
        bool changed = false;
        bool manual_changed = false;
        std::vector<QueueHandle_t> subscribers;
        QueueHandle_t screen_queue;
        QueueHandle_t fan_queue;
        Command command;
};

#endif