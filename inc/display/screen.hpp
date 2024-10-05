#ifndef SCREEN_H
#define SCREEN_H

#include "ssd1306os.h"
#include "register_handler.h"
#include "task_defines.h"

#include <map>

class Screen {
    public:
        Screen(std::shared_ptr<PicoI2C> i2c, uint16_t device_address = 0x3C, uint16_t width = 128, uint16_t height = 64);
        QueueHandle_t get_control_queue_handle(void);
        QueueHandle_t get_reading_queue_handle(void);
    private:
        static void screen_task(void *pvParameters);
        void set_target_task(void *pvParameters);
        void set_static_shapes(void);
        void set_reading_value(Reading &reading);
        void set_bar(uint16_t percentage);
        std::unique_ptr<ssd1306os> display;
        QueueHandle_t control_queue;
        QueueHandle_t reading_queue;
        mono_vlsb reading_blit_buf;
        mono_vlsb bar_buf;
};

#endif