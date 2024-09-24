#include "pico/stdlib.h"
#include <iostream>
#include <sstream>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "hardware/gpio.h"
#include "PicoOsUart.h"
#include "ssd1306.h"
#include "hardware/timer.h"

int main() {
    stdio_init_all();
    printf("Boot\n");

    for(;;) {
        printf("It builds!\n");
        sleep_ms(1000);
    }
    return 0;
}
