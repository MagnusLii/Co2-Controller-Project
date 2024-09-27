

#include "i2c_instance.h"
#include "pico/stdlib.h"

I2C_instance::I2C_instance(i2c_inst_t *instance, uint baudrate, uint sda, uint scl) : inst(instance) {
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);
    gpio_set_dir(sda, GPIO_OUT);
    gpio_set_dir(scl, GPIO_OUT);

    i2c_init(instance, baudrate);
}

i2c_inst_t *I2C_instance::get_instance(void) {
    return inst;
}