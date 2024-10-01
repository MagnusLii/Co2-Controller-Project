
#include "rotary.hpp"



#define COUNTER_CLOCKWISE 2
#define CLOCKWISE 1

bool debounce(uint32_t *time) {
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    if (current_time - *time > 250) {
        *time = current_time;
        return true;
    }
    return false;
}

static Rotary *rotary;

Rotary::Rotary(uint ROT_SW, uint ROT_A, uint ROT_B)
: rot_sw(ROT_SW), rot_a(ROT_A), rot_b(ROT_B) {
    rotary = this;
}


void rotary_irq_handler(uint gpio, uint32_t mask) {
    rotary->irq_handler(gpio, mask);
}

void Rotary::irq_handler(uint gpio, uint32_t mask) {
    static uint32_t time = 0;
    static const uint counter_clockwise = COUNTER_CLOCKWISE;
    static const uint clockwise = CLOCKWISE;

    if ((gpio == rot_sw) && (mask & GPIO_IRQ_EDGE_FALL)) {
        if (debounce(&time))
            send();
    }
    switch (gpio) {
    case rot_sw:
        if ((mask & GPIO_IRQ_EDGE_FALL) && debounce(&time)) {
            xQueueSendFromISR(que, &gpio, NULL);
        }
        break;
    case ROT_A:
        if (mask & GPIO_IRQ_EDGE_FALL) {
            if (gpio_get(ROT_B)) {
                xQueueSendFromISR(que, &clockwise, NULL);
            } else {
                xQueueSendFromISR(que, &counter_clockwise, NULL);
            }
        }
    default:
        break;
    }
}

