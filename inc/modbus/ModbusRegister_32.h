
#ifndef UART_IRQ_MODBUSREGISTER_32_H
#define UART_IRQ_MODBUSREGISTER_32_H

#include <memory>
#include "ModbusClient.h"
#include "ModbusRegister.h"

class ModbusRegister32 : public ModbusRegister {
public:
    ModbusRegister32(std::shared_ptr<ModbusClient> client_, uint8_t server_address, uint16_t register_address, uint8_t nr_of_registers, bool holding_register = true);
    uint32_t read_32();
    void write_32(uint32_t value);
    float read_float();
private:
    std::shared_ptr<ModbusClient> client;
    uint8_t server;
    uint16_t reg_addr;
    uint8_t nr_of_regs;
    bool hr;

};


#endif //UART_IRQ_MODBUSREGISTER_32_H
