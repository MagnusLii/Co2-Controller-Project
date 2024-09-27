#pragma once

#include "hardware/i2c.h"
#include <memory>

#define shared_i2c std::shared_ptr<I2C_instance>

class I2C_instance {
    public:
        I2C_instance(i2c_inst_t *instance, uint baudrate, uint sda, uint scl);
        i2c_inst_t *get_instance(void);
    private:
       i2c_inst_t *inst;
};