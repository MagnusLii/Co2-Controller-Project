
#ifndef PRESSURE_REGISTER_H
#define PRESSURE_REGISTER_H

#include "PicoI2C.h"
#include <memory>

#define READ_DIFF_CODE 0xF1

class PressureRegister {
public:
    PressureRegister(shared_i2c i2c, uint8_t device_address);
    int16_t read_register();
private:
    shared_i2c i2c;
    uint8_t payload;
    uint8_t devaddr;

    const uint8_t payload_len = 3;
    const uint16_t scale_factor = 240;
    const uint16_t height_compensation = 95;
};

#endif //PRESSURE_REGISTER_H
