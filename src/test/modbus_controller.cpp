#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/timer.h"
#include "hardware/dma.h"
#include "modbus_controller.h"
#include "modbus_register.h"



static ModbusCtrl *point;
static int rx_chan;
void dma_irq_handler(void) {
    if (dma_channel_get_irq0_status(rx_chan)) {
        dma_channel_acknowledge_irq0(rx_chan);
        point->end();
    }
}

void alarm_hand(uint alarm_num) {
    point->end(true);
}

ModbusCtrl::ModbusCtrl(shared_uart uart_pointer) :
    tx_channel(uart_pointer, true),
    rx_channel(uart_pointer, false),
    uart_ptr(uart_pointer),
    uart_baud(uart_pointer->get_baud()),
    busy(false),
    ctrl_time(0) {
        irq_set_exclusive_handler(DMA_IRQ_0, dma_irq_handler);
        irq_set_enabled(DMA_IRQ_0, true);
        point = this;
        rx_chan = rx_channel.get_channel();
        alarm = hardware_alarm_claim_unused(true);
        hardware_alarm_set_target(alarm, nil_time);
        hardware_alarm_set_callback(alarm, alarm_hand);
    }


static inline uint64_t get_char_delay(uint baud) {
    // 1/baud = time it takes to send one bit
    // bit time * 8 = time it takes to send one character (8 bits)
    // byte time * 1000000 = byte time in microseconds
    // byte time us * 4 = time it takes to send 4 characters in microseconds
    return (8*1000000*4)/baud;
}

void ModbusCtrl::start(MODBUSRegister *who, uint8_t rxlen, uint32_t timeout_ms) {
    // if busy and not enough delay for next transfer wait
    uint64_t delay = get_char_delay(uart_baud);
    while(busy || ((time_us_64() - ctrl_time) < delay)) tight_loop_contents();
    busy = true;
    current_user = who;
    rx_channel.start(who->rxbuf_address(), rxlen);
    tx_channel.start(who->payload_address(), who->payload_length());
    if (timeout_ms) {
        hardware_alarm_set_target(alarm, delayed_by_ms(get_absolute_time(), timeout_ms));
    }
}

void ModbusCtrl::end(bool timeout) {
    if (timeout) {
        rx_channel.abort();
        tx_channel.abort();
    }
    ctrl_time = time_us_64();
    busy = false;
    uint8_t len = (uint32_t)(dma_hw->ch[rx_chan].al1_write_addr) - (uint32_t)current_user->rxbuf;
    current_user->rx_len = len;
}

bool ModbusCtrl::isbusy(void) {
    return busy;
}