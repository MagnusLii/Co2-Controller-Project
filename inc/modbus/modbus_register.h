#pragma once

#include "pico/stdlib.h"
#include "modbus_controller.h"

#define PAYLOAD_MAX_LEN 32


class MODBUSRegister {
    public:
        MODBUSRegister(shared_modbus modbus, uint8_t device_address, uint16_t register_address, uint8_t number_of_registers);
        uint8_t *payload_address(void);
        uint8_t *rxbuf_address(void);
        uint8_t payload_length(void);
        uint16_t get16(void);
        uint32_t get32(void);
        float get_float(void);
        void wait(void);
        uint8_t rxbuf[PAYLOAD_MAX_LEN]; // this needs to be public for reasons
        uint8_t rx_len; // this needs to be public for reasons
    protected:
        shared_modbus modbus;
        uint8_t payload[PAYLOAD_MAX_LEN];
        uint8_t payload_len;
        uint8_t number_of_registers;
};

class ReadRegister : public MODBUSRegister {
    public:
        ReadRegister(shared_modbus modbus, uint8_t device_address, uint16_t register_address, uint8_t number_of_registers=1, bool holding=true);
        void start_transfer();
        bool is_ok();
};

class WriteRegister : public MODBUSRegister {
    public:
        WriteRegister(shared_modbus modbus, uint8_t device_address, uint16_t register_address, uint8_t number_of_registers);
        bool is_ok();
        // write one word of data. If number of registers greater than one doesn't write to rest of the registers.
        void start_transfer(uint16_t data);
        /* 
        write two words of data. If number of registers is 1 only write lower 16 bits.
        lower 16 bits go to the first register, higher 16 bits to the second.
        */
        void start_transfer(uint32_t data);
        void start_transfer(float data);
};
