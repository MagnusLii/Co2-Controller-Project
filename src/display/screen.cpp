
#include "screen.hpp"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define MARGIN 3
#define READING_GAP 5
#define DEGREE_WIDTH 3

#define CHAR_HEIGHT 8

#define COLON_X 4 * CHAR_HEIGHT 

#define C_Y MARGIN
#define C_X COLON_X - (CHAR_HEIGHT + DEGREE_WIDTH) + 2
#define CO2_Y CHAR_HEIGHT + READING_GAP + MARGIN
#define CO2_X COLON_X - (3 * CHAR_HEIGHT)
#define RH_Y 2 * (CHAR_HEIGHT + READING_GAP) + MARGIN
#define RH_X COLON_X - (2 * CHAR_HEIGHT)
#define HPA_Y 3 * (CHAR_HEIGHT + READING_GAP) + MARGIN
#define HPA_X COLON_X - (3 * CHAR_HEIGHT)

#define READING_X COLON_X + 3 * MARGIN

#define SPEED_FRAME_WIDTH 4 * CHAR_HEIGHT - 2
#define SPEED_FRAME_HEIGHT SCREEN_HEIGHT - (CHAR_HEIGHT + MARGIN)

Screen::Screen(std::shared_ptr<PicoI2C> i2c, uint16_t device_address, uint16_t width, uint16_t height)
: display(std::make_unique<ssd1306os>(i2c, device_address, width, height)),
  reading_blit_buf(4 * CHAR_HEIGHT, CHAR_HEIGHT) {
    // display = std::make_unique<ssd1306os>(i2c, device_address, width, height);
    control_queue = xQueueCreate(20, 8); //change size
    vQueueAddToRegistry(control_queue, "SCREEN_QUEUE");
    xTaskCreate(Screen::screen_task, "SCREEN", 512, this, tskIDLE_PRIORITY + 1, NULL);
}

QueueHandle_t Screen::get_queue_handle(void) {
    return control_queue;
}

void Screen::screen_task(void *pvParameters) {
    auto screen = static_cast<Screen *>(pvParameters);
    screen->set_static_shapes();
    screen->display->show();
    Reading reading;
    while (true) {
        xQueueReceive(screen->control_queue, &reading, portMAX_DELAY);
        screen->set_reading_value(reading);
        screen->display->show();
    }
}

void Screen::set_static_shapes(void) {
    display->rect(C_X - DEGREE_WIDTH, C_Y - 1, DEGREE_WIDTH, DEGREE_WIDTH, 1);
    display->text("C:", C_X, C_Y);
    display->text("CO2:", CO2_X, CO2_Y);
    display->text("RH:", RH_X, RH_Y);
    display->text("hPa:", HPA_X, HPA_Y);

    display->rect(SCREEN_WIDTH - SPEED_FRAME_WIDTH + 2, 0, SPEED_FRAME_WIDTH, SPEED_FRAME_HEIGHT, 1);
    display->text("100%", SCREEN_WIDTH - 4 * CHAR_HEIGHT, SCREEN_HEIGHT - CHAR_HEIGHT);
}

// enum class ReadingType { CO2, TEMPERATURE, REL_HUMIDITY, FAN_COUNTER, PRESSURE, UNSET };
void Screen::set_reading_value(Reading &reading) {
    char text[16];
    int height;
    if (reading.type == ReadingType::PRESSURE) {
        snprintf(text, 16, "%hd", reading.value.i16);
    } else {
        snprintf(text, 16, "%.1f", reading.value.f32);
    }
    
    switch (reading.type) {
    case ReadingType::CO2:
        height = CO2_Y;
        break;
    case ReadingType::TEMPERATURE:
        height = C_Y;
        break;
    case ReadingType::REL_HUMIDITY:
        height = RH_Y;
        break;
    case ReadingType::PRESSURE:
        height = HPA_Y;
        break;
    default:
        height = 200; // not on the screen
        break;
    }
    reading_blit_buf.fill(0);
    display->blit(reading_blit_buf, READING_X, height); // clear the area
    reading_blit_buf.text(text, 0, 0);
    display->blit(reading_blit_buf, READING_X, height); // blit the text
}

void Screen::set_manual_fan_speed(float percentage) {

}