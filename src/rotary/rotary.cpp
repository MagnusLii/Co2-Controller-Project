
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

Rotary::Rotary(uint ROT_SW, uint ROT_A, uint ROT_B)
: rot_sw(ROT_SW), rot_a(ROT_A), rot_b(ROT_B) {
    rotary = this;
}


void rotary_irq_handler(uint gpio, uint32_t mask) {
    rotary->irq_handler(gpio, mask);
}

void Rotary::irq_handler(uint gpio, uint32_t mask) {
    if ((gpio == rot_sw) && (mask & GPIO_IRQ_EDGE_FALL)) {
        if (debounce())
            ;
            // send(command rot_sw);
    } else if ((gpio == rot_a) && (mask & GPIO_IRQ_EDGE_FALL)) {
            if (gpio_get(rot_b))
                ;
                // send(command clockwise);
            else
                ;
                // send(command counterclockwise);
    }
}

