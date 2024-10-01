
#include "screen.hpp"

#define MARGIN 3
#define READING_GAP 5
#define DEGREE_WIDTH 3

#define CHAR_HEIGHT 8

#define C_HEIGHT MARGIN
#define CO2_HEIGHT CHAR_HEIGHT + READING_GAP + MARGIN
#define RH_HEIGHT 2 * (CHAR_HEIGHT + READING_GAP) + MARGIN
#define HPA_HEIGHT 3 * (CHAR_HEIGHT + READING_GAP) + MARGIN

#define READING_X 4 * CHAR_HEIGHT + 2 * MARGIN

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
    screen->set_reading_text();
    screen->display->show();
    Reading reading;
    while (true) {
        xQueueReceive(screen->control_queue, &reading, portMAX_DELAY);
        screen->set_reading_value(reading);
        screen->display->show();
    }
}

void Screen::set_reading_text(void) {
    display->rect(MARGIN, MARGIN, DEGREE_WIDTH, DEGREE_WIDTH, 1);
    display->text("C:", MARGIN + DEGREE_WIDTH, C_HEIGHT);
    display->text("CO2:", MARGIN, CO2_HEIGHT);
    display->text("RH:", MARGIN, RH_HEIGHT);
    display->text("hPa:", MARGIN, HPA_HEIGHT);
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
        height = CO2_HEIGHT;
        break;
    case ReadingType::TEMPERATURE:
        height = C_HEIGHT;
        break;
    case ReadingType::REL_HUMIDITY:
        height = RH_HEIGHT;
        break;
    case ReadingType::PRESSURE:
        height = HPA_HEIGHT;
    default:
        height = 200; // not on the screen
        break;
    }
    reading_blit_buf.fill(0);
    display->blit(reading_blit_buf, READING_X, height); // clear the area
    reading_blit_buf.text(text, 0, 0);
    display->blit(reading_blit_buf, READING_X, height); // blit the text
}