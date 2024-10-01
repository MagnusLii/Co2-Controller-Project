#ifndef SCREEN_H
#define SCREEN_H

#include "ssd1306os.h"
#include "register_handler.h"

#include <map>

class Screen {
    public:
        Screen(std::shared_ptr<PicoI2C> i2c, uint16_t device_address = 0x3C, uint16_t width = 128, uint16_t height = 64);
        QueueHandle_t get_queue_handle(void);
    private:
        static void screen_task(void *pvParameters);
        void set_reading_text(void);
        void set_reading_value(Reading &reading);
        std::unique_ptr<ssd1306os> display;
        QueueHandle_t control_queue;
        mono_vlsb reading_blit_buf;
};

#endif