
#ifndef UART_IRQ_MODBUSREGISTER_32_H
#define UART_IRQ_MODBUSREGISTER_32_H

#include <memory>
#include "ModbusClient.h"
#include "ModbusRegister.h"

class ModbusRegister32 : public ModbusRegister {
public:
    ModbusRegister32(std::shared_ptr<ModbusClient> client_, int server_address, int register_address, int nr_of_registers, bool holding_register = true);
    uint32_t read_32();
    void write_32(uint32_t value);
    float read_float();
private:
    int nr_of_regs;
};


#endif //UART_IRQ_MODBUSREGISTER_32_H
