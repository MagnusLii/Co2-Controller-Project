
#include "pressure_register.h"

#include <iostream>
#include <ostream>

PressureRegister::PressureRegister(shared_i2c i2c, const uint8_t device_address)
    : i2c(i2c) {
    this->payload = READ_DIFF_CODE;
    this->devaddr = device_address;

    read_register();
}

int16_t PressureRegister::read_register() {
    i2c->write(devaddr, &payload, 1);

    uint8_t data[payload_len];
    i2c->read(devaddr, data, payload_len);
    vTaskDelay(pdMS_TO_TICKS(10));

    int16_t value = (data[0] << 8) | data[1];

    value = (value * scale_factor) / (height_compensation * 100);
    return value;
}
