#ifndef TEST_CONNECTION_H
#define TEST_CONNECTION_H

#include "uart_instance.h"
#include "register_handler.h"
#include "device_registry.h"
#include "connection_handler.h"

struct hw_setup_params {
    shared_uart uart;
    shared_modbus modbus;
    shared_i2c i2c;
    std::shared_ptr<DeviceRegistry> registry;
};

struct sub_setup_params {
    std::shared_ptr<DeviceRegistry> registry;
    ConnectionHandler* connection_handler;
};

#endif /* TEST_CONNECTION_H */