
#include "uart_instance.h"
#include "hardware/dma.h"
#include "dma_channel.h"

DMAChannel::DMAChannel(shared_uart uartptr, bool tx) : tx(tx) {
    channel = dma_claim_unused_channel(true);
    dma_channel_config c = dma_channel_get_default_config(channel);
    channel_config_set_transfer_data_size(&c, DMA_SIZE_8); // uart deals with 8 bit characters
    if (tx) {
        channel_config_set_read_increment(&c, true);
        channel_config_set_write_increment(&c, false);
        channel_config_set_dreq(&c, (uartptr->get_index()) ? DREQ_UART1_TX : DREQ_UART0_TX);
        dma_channel_configure(
            channel,
            &c,
            uartptr->get_dr_address(),
            nullptr,
            0,
            false
        );
    } else {
        channel_config_set_read_increment(&c, false); // read from same fifo so no increment
        channel_config_set_write_increment(&c, true);
        channel_config_set_dreq(&c, (uartptr->get_index()) ? DREQ_UART1_RX : DREQ_UART0_RX);
        dma_channel_configure(
            channel,
            &c,
            nullptr,
            uartptr->get_dr_address(),
            0,
            false
        );
        dma_channel_set_irq0_enabled(channel, true);
    }
}

DMAChannel::~DMAChannel() {
    //dma_channel_cleanup(channel);
    dma_channel_unclaim(channel);
}


void DMAChannel::start(uint8_t *buf, uint8_t len) {
    if (tx) {
        dma_channel_set_read_addr(channel, buf, false);
    } else {
        dma_channel_set_write_addr(channel, buf, false);
    }
    dma_channel_set_trans_count(channel, len, true);
}

void DMAChannel::abort() {
    dma_channel_abort(channel);
}

int DMAChannel::get_channel(void) {
    return channel;
}