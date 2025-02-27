
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
#define SPEED_FRAME_X SCREEN_WIDTH - SPEED_FRAME_WIDTH + 2
#define SPEED_FRAME_Y 0

#define SPEED_COLUMN_WIDTH SPEED_FRAME_WIDTH - 4
#define SPEED_COLUMN_HEIGHT SPEED_FRAME_HEIGHT - 4
#define SPEED_COLUMN_X SPEED_FRAME_X + 2
#define SPEED_COLUMN_Y SPEED_FRAME_Y + 2

Screen::Screen(std::shared_ptr<PicoI2C> i2c, uint16_t device_address, uint16_t width, uint16_t height)
: display(std::make_unique<ssd1306os>(i2c, device_address, width, height)),
  reading_blit_buf(4 * CHAR_HEIGHT, CHAR_HEIGHT),
  bar_buf(SPEED_FRAME_WIDTH - 4, SPEED_FRAME_HEIGHT - 4) {
    // display = std::make_unique<ssd1306os>(i2c, device_address, width, height);
    reading_queue = xQueueCreate(20, sizeof(Reading));
    control_queue = xQueueCreate(50, sizeof(Command));
    set_static_shapes();
    xTaskCreate(Screen::screen_task, "SCREEN", 512, this, TaskPriority::ABSOLUTE, NULL);
    xTaskCreate(Screen::set_target_task, "SCREENTARGET", 512, this, TaskPriority::ABSOLUTE, NULL);
}

QueueHandle_t Screen::get_control_queue_handle(void) {
    return control_queue;
}
QueueHandle_t Screen::get_reading_queue_handle(void) {
    return reading_queue;
}

void Screen::screen_task(void *pvParameters) {
    auto screen = static_cast<Screen *>(pvParameters);
    Reading reading;
    while (true) {
        xQueueReceive(screen->reading_queue, &reading, portMAX_DELAY);
        if (reading.type == ReadingType::FAN_SPEED) {
            if (!screen->manual_highlighted)
                screen->set_fan_speed_percentage(reading.value.u16 / 10, screen->manual_highlighted);
        } else {
            screen->set_reading_value(reading);
        }
        screen->display->show();
    }
}

void Screen::set_target_task(void *pvParameters) {
    auto screen = static_cast<Screen *>(pvParameters);
    Command command;
    bool dehighlight = false;
    while (true) {
        while (xQueueReceive(screen->control_queue, &command, 100)) {
            switch (command.type) {
            case WriteType::MODE_SET:
                screen->set_mode((bool)command.value.u16);
                break;
            case WriteType::FAN_SPEED:
                screen->manual_highlighted = true;
                screen->last_fan_speed = command.value.u16;
                dehighlight = true;
                screen->set_fan_speed_percentage(command.value.u16 / 10, true);
                screen->set_co2_target(screen->last_co2, false);
                break;
            case WriteType::CO2_TARGET:
                screen->manual_highlighted = false;
                screen->last_co2 = command.value.f32;
                screen->set_co2_target(command.value.f32, true);
                if (dehighlight) {
                    dehighlight = false;
                    screen->set_fan_speed_percentage(screen->last_fan_speed / 10, false);
                }
                break;
            default:
                break;
            }
        }
        screen->display->show();
    }
}

void Screen::set_static_shapes(void) {
    display->rect(C_X - DEGREE_WIDTH, C_Y - 1, DEGREE_WIDTH, DEGREE_WIDTH, 1);
    display->text("C:", C_X, C_Y);
    display->text("CO2:", CO2_X, CO2_Y);
    display->text("RH:", RH_X, RH_Y);
    display->text("hPa:", HPA_X, HPA_Y);
    display->text("*:", 0, SCREEN_HEIGHT - CHAR_HEIGHT);
    display->text(":*", SCREEN_WIDTH - 2*CHAR_HEIGHT, SCREEN_HEIGHT - CHAR_HEIGHT);

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

void Screen::set_fan_speed_percentage(uint16_t percentage, bool highlight) {
    char text[16];
    snprintf(text, 16, "%hd%%", percentage);
    reading_blit_buf.fill(highlight ? 1 : 0);
    reading_blit_buf.text(text, 0, 0, highlight ? 0 : 1);
    display->blit(reading_blit_buf, 3 * CHAR_HEIGHT, SCREEN_HEIGHT - CHAR_HEIGHT);
}

void Screen::set_co2_target(float value, bool highlight) {
    char text[16];
    snprintf(text, 16, "%.0f", value);
    reading_blit_buf.fill(highlight ? 1 : 0);
    reading_blit_buf.text(text, 0, 0, highlight ? 0 : 1);
    display->blit(reading_blit_buf, SCREEN_WIDTH - 6 * CHAR_HEIGHT, SCREEN_HEIGHT - CHAR_HEIGHT);
}

void Screen::set_bar(uint16_t percentage) {
    bar_buf.fill(0);
    display->blit(bar_buf, SPEED_COLUMN_X, SPEED_COLUMN_Y);
    uint16_t new_height = percentage;
    bar_buf.rect(0, SPEED_COLUMN_HEIGHT - new_height, SPEED_COLUMN_WIDTH, new_height, 1, true);
    display->blit(bar_buf, SPEED_COLUMN_X, SPEED_COLUMN_Y);
}

void Screen::set_mode(bool is_manual) {
    reading_blit_buf.fill(0);
    display->blit(reading_blit_buf, SCREEN_WIDTH - 3 * CHAR_HEIGHT, 0);
    if (is_manual)
        reading_blit_buf.text("FAN", 0, 0);
    else
        reading_blit_buf.text("CO2", 0, 0);
    display->blit(reading_blit_buf, SCREEN_WIDTH - 3 * CHAR_HEIGHT, 0);
}

void Screen::set_initial_values(float co2_target, uint16_t fan_speed, bool is_manual) {
    set_co2_target(co2_target, !is_manual);
    set_fan_speed_percentage(fan_speed / 10, is_manual);
    set_mode(is_manual);
    manual_highlighted = is_manual;
    last_co2 = co2_target;
    last_fan_speed = fan_speed;

    display->show();
}