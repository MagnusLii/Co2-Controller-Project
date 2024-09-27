#pragma once

#include <memory>
// #include "pico/stdlib.h"
#include "hardware/uart.h"

#define shared_uart std::shared_ptr<Uart_instance>

class Uart_instance {
    public:
        Uart_instance(uint uart_number, uint baudrate, uint TX_pin, uint RX_pin, uint stop_bits=1);
        io_rw_32 *get_dr_address(void);
        uint get_index(void);
        uint get_baud(void);
    private:
        uart_inst_t *uart;
        uint baudrate;
};

