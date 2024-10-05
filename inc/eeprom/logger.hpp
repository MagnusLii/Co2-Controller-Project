//
// Created by elmer on 4.10.2024.
//

#ifndef KEIJOS_RETIREMENT_PLAN_PRODUCT_LOGGER_HPP
#define KEIJOS_RETIREMENT_PLAN_PRODUCT_LOGGER_HPP

#include "PicoI2C.h"
#include <memory>

class Logger {
  public:
    Logger(shared_i2c i2c_0);
    void write_address(uint16_t address);
    void write_page(uint16_t address, uint8_t *src, size_t size);
    void read_page(uint16_t address, uint8_t *dst, size_t size);
 private:
    std::shared_ptr<PicoI2C> i2c;
};

#endif // KEIJOS_RETIREMENT_PLAN_PRODUCT_LOGGER_HPP
