#ifndef HARDWARECONST_H
#define HARDWARECONST_H

#define UART_NR 1 // 0 or 1
#define UART_BAUD 9600
#define UART_TX_PIN 4 // 0 if UART_NR = 0, 4 if UART_NR = 1
#define UART_RX_PIN 5 // 1 if UART_NR = 0, 5 if UART_NR = 1
#define UART_BITS 2 // Stopbits: 1 or 2

#define I2C_0 0
#define I2C_1 1
#define I2C_DEVADDR_PRESSURE 0x40

#define MB_DEVADDR_CO2 240
#define MB_DEVADDR_TEMP 241
#define MB_DEVADDR_REL_HUM 241
#define MB_DEVADDR_FAN 1

#define MB_REGADDR_CO2 0x0
#define MB_REGADDR_TEMP 0x2
#define MB_REGADDR_REL_HUM 0x0
#define MB_REGADDR_FAN_COUNTER 4
#define MB_REGADDR_FAN_SPEED 0

#endif //HARDWARECONST_H
