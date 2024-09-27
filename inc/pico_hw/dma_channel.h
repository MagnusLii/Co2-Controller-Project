#pragma once

#include "uart_instance.h"


class DMAChannel {
    public:
        DMAChannel(shared_uart uartptr, bool tx);
        ~DMAChannel();
        void start(uint8_t *buf, uint8_t len);
        void abort(void);
        int get_channel(void);
    private:
        bool tx;
        int channel;
};