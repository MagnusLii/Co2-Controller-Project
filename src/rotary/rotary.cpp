
#include "rotary.hpp"



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

Rotary::Rotary(uint ROT_SW, uint ROT_A, uint ROT_B, uint button)
: rot_sw(ROT_SW), rot_a(ROT_A), rot_b(ROT_B), button(button) {
    rotary = this;
    gpio_init(rot_sw);
    gpio_init(button);
    gpio_pull_up(rot_sw);
    gpio_pull_up(button);
    gpio_set_irq_enabled_with_callback(rot_sw, GPIO_IRQ_EDGE_FALL, true, rotary_irq_handler);
    gpio_set_irq_enabled(rot_a, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(rot_b, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(button, GPIO_IRQ_EDGE_FALL, true);
}


void Rotary::irq_handler(uint gpio, uint32_t mask) {
    if ((gpio == rot_sw) && (mask & GPIO_IRQ_EDGE_FALL)) {
        if (debounce()) {
            command.type = WriteType::ROT_SW;
            send_reading_from_isr();
        }
    } else if ((gpio == rot_a) && (mask & GPIO_IRQ_EDGE_FALL)) {
        command.type = WriteType::TURN;
        if (gpio_get(rot_b)) {
            command.value.u16 = CLOCKWISE;
        } else {
            command.value.u16 = COUNTER_CLOCKWISE;
        }
        send_reading_from_isr();
    } else if ((gpio == button) && (mask & GPIO_IRQ_EDGE_FALL)) {
        if (debounce()) {
            command.type = WriteType::TOGGLE;
            send_reading_from_isr();
        }
    }
}


// Add a subscribers queue handle to the send list (vector)
void Rotary::add_subscriber(QueueHandle_t subscriber) { subscribers.push_back(subscriber); }

// Remove a subscribers queue handle from the send list
// -- probably not going to be used ever
void Rotary::remove_subscriber(QueueHandle_t subscriber) {
    subscribers.erase(std::remove(subscribers.begin(), subscribers.end(), subscriber), subscribers.end());
}

void Rotary::send_reading_from_isr() {
    for (const auto subscriber : subscribers) {
        xQueueSendFromISR(subscriber, &command, 0);
    }
    // xQueueSendFromISR(screen_queue, &command, 0);
    // xQueueSendFromISR(fan_queue, &command, 0);
}

void Rotary::set_screen_queue(QueueHandle_t queue) {
    screen_queue = queue;
}
void Rotary::set_fanctrl_queue(QueueHandle_t queue) {
    fan_queue = queue;
}



