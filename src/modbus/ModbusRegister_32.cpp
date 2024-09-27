#include "ModbusRegister_32.h"

ModbusRegister32::ModbusRegister32(std::shared_ptr<ModbusClient> client_, int server_address,
                                   int register_address, int nr_of_registers,
                                   bool holding_register)
    : ModbusRegister(client_, server_address, register_address, holding_register),
      nr_of_regs(nr_of_registers) {}

uint32_t ModbusRegister32::read_32() {
    uint32_t value = 0;
    uint16_t value_arr[2] = {0, 0};

    client->set_destination_rtu_address(server);
    if (hr)
        client->read_holding_registers(reg_addr, nr_of_regs, value_arr);
    else
        client->read_input_registers(reg_addr, nr_of_regs, value_arr);

    return value;
}

void ModbusRegister32::write_32(uint32_t value) {
    uint16_t value_arr[2] = {0, 0};

    value_arr[0] = value;
    value_arr[1] = value >> 16;

    if (hr) {
        client->set_destination_rtu_address(server);
        client->write_multiple_registers(reg_addr, nr_of_regs, value_arr);
    }
}

float ModbusRegister32::read_float() {
    uint32_t value = read_32();
    union {
        float f;
        uint32_t i;
    } u;
    u.i = value;
    return u.f;
}
