
#include "rotary.hpp"



#define COUNTER_CLOCKWISE 2
#define CLOCKWISE 1

#define DELAY_MS 250

bool Rotary::debounce(void) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - time > DELAY_MS) {
        time = current_time;
        return true;
    }
    return false;
}

static Rotary *rotary;

void rotary_irq_handler(uint gpio, uint32_t mask) {
    rotary->irq_handler(gpio, mask);
}

Rotary::Rotary(uint ROT_SW, uint ROT_A, uint ROT_B)
: rot_sw(ROT_SW), rot_a(ROT_A), rot_b(ROT_B) {
    rotary = this;
    gpio_set_irq_enabled_with_callback(rot_sw, GPIO_IRQ_EDGE_FALL, true, rotary_irq_handler);
    gpio_set_irq_enabled(rot_a, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(rot_b, GPIO_IRQ_EDGE_FALL, true);
}


void Rotary::irq_handler(uint gpio, uint32_t mask) {
    if ((gpio == rot_sw) && (mask & GPIO_IRQ_EDGE_FALL)) {
        if (debounce()) {
            reading.type = ReadingType::ROT_SW;
            send_reading_from_isr();
        }
    } else if ((gpio == rot_a) && (mask & GPIO_IRQ_EDGE_FALL)) {
            if (gpio_get(rot_b)) {
                reading.type = ReadingType::CW;
                send_reading_from_isr();
            } else {
                reading.type = ReadingType::CCW;
                send_reading_from_isr();
            }
    }
}


