//
// Created by elmer on 4.10.2024.
//

#include "logger.hpp"

Logger::Logger(shared_i2c i2c_0) {
    this->i2c = i2c_0;
}

/*
void eeprom_write_address(uint16_t address) {
    eeprom_write_cycle_block();

    uint8_t out[2] = {address >> 8, address}; // shift by 8 not 4...
    i2c_write_blocking(i2c0, EEPROM_ADDRESS, out, 2, true);
}
*/
void Logger::write_address(uint16_t address) {
    uint8_t out[2] = {(uint8_t)(address >> 8), (uint8_t)address};
    i2c->write(0x50, out, 2);
}

/*
void eeprom_write_page(uint16_t address, uint8_t *src, size_t size) {
    uint8_t out[size + 2];
    out[0] = address >> 8; // Upper bits of the address
    out[1] = address; // Lower bits of the address
    for (int i = 0; i < size; i++) {
        out[i + 2] = src[i];
    }

    eeprom_write_cycle_block(); // Ensure EEPROM write cycle duration is within limits

    i2c_write_blocking(i2c0, EEPROM_ADDRESS, out, size + 2, false);

    write_init_time = get_absolute_time();
}
 */

void Logger::write_page(uint16_t address, uint8_t *src, size_t size) {
    uint8_t out[size + 2];
    out[0] = address >> 8; // Upper bits of the address
    out[1] = address; // Lower bits of the address

    for (int i = 0; i < size; i++) {
        out[i + 2] = src[i];
    }

    i2c->write(0x50, out, 2);
}

/*
void eeprom_read_page(uint16_t address, uint8_t *dst, size_t size) {
    eeprom_write_address(address);

i2c_read_blocking(i2c0, EEPROM_ADDRESS, dst, size, false);
}
 */

void Logger::read_page(uint16_t address, uint8_t *dst, size_t size) {
    write_address(address);

    i2c->read(0x50, dst, size);
}
