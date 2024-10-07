
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
    xTaskCreate(Rotary::send_task, "ROTARY", 512, this, TaskPriority::ABSOLUTE, NULL);
}

void Rotary::irq_handler(uint gpio, uint32_t mask) {
    changed = true;
    if ((gpio == rot_sw) && (mask & GPIO_IRQ_EDGE_FALL)) {
        if (debounce()) {
            this->is_manual_local = !this->is_manual_local;
            // this->manual_changed = true;
        }
    } else if ((gpio == rot_a) && (mask & GPIO_IRQ_EDGE_FALL)) {
        if (gpio_get(rot_b)) {
            if (this->is_manual_local) {
                if (this->fan_speed_local < 1000) this->fan_speed_local += 10;
            } else {
                if (this->co2_target_local < 1500) this->co2_target_local +=10;
            }
        } else {
            if (this->is_manual_local) {
                if (this->fan_speed_local > 0) this->fan_speed_local -= 10;
            } else {
                if (this->co2_target_local > 0) this->co2_target_local -=10;
            }
        }
        send_reading_from_isr();
    } else if ((gpio == button) && (mask & GPIO_IRQ_EDGE_FALL)) {
        if (debounce()) {
            this->toggled = true;
        }
    }
}

void Rotary::send_task(void *pvParameters) {
    auto rot = static_cast<Rotary*>(pvParameters);

    while (true) {
        if (rot->changed) {
            rot->changed = false;
            if (rot->toggled) {
                rot->toggled = false;
                rot->command.type = WriteType::MODE_SET;
                rot->command.value.u16 = rot->is_manual_local ? 1 : 0;
                rot->send_reading();   
            }
            if (rot->is_manual_local) {
                rot->command.type = WriteType::FAN_SPEED;
                rot->command.value.u16 = rot->fan_speed_local;
            } else {
                rot->command.type = WriteType::CO2_TARGET;
                rot->command.value.f32 = rot->co2_target_local;
            }
            // if (rot->manual_changed) {
            //     rot->manual_changed = false;
            //     rot->command.type = WriteType::ROT_SW;
            // }
            rot->send_reading();
        }
        vTaskDelay(50);
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
}

void Rotary::send_reading() {
    for (const auto subscriber : subscribers) {
        xQueueSend(subscriber, &command, 0);
    }
}

void Rotary::set_initial_values(float co2_target, uint16_t fan_speed, bool is_manual) {
    this->co2_target_local = co2_target;
    this->fan_speed_local = fan_speed;
    this->is_manual_local = is_manual;
}



