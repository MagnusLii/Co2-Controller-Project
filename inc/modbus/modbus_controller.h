#pragma once

#include <memory>
#include "pico/time.h"
#include "uart_instance.h"
#include "dma_channel.h"


#define shared_modbus std::shared_ptr<ModbusCtrl>

class MODBUSRegister; // forward declaration need to prevent circular dependencies

class ModbusCtrl {
    public:
        ModbusCtrl(shared_uart uart_pointer);
        void start(MODBUSRegister *who, uint8_t rxlen, uint32_t timeout_ms=500);
        void end(bool timeout=false);
        bool isbusy(void);
    private:
        DMAChannel tx_channel;
        DMAChannel rx_channel;
        MODBUSRegister *current_user;
        shared_uart uart_ptr;
        uint uart_baud;
        volatile bool busy;
        uint64_t ctrl_time;
        int alarm;
};