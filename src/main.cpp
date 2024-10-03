#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/cyw43_arch.h"
#include "connection_defines.h"
#include <string.h>
#include "tls_common.h"
#include "read_runtime_ctr.cpp"
#include "debugPrints.h"
#include "TLSWrapper.h"



void tls_task(void *pvParameters) {
    char *ssid = WIFI_SSID;
    char *password = WIFI_PASSWORD;
    char *certificate = TLS_CERTIFICATE;
    TLSWrapper tlsWrapper(ssid, password, certificate, sizeof(certificate), CYW43_COUNTRY_FINLAND);

    tlsWrapper.connect(THINGSPEAK_IP, THINGSPEAK_PORT);

    for (;;) {
        
        vTaskDelay(1000);
    }
}


int main() {
    stdio_init_all();
    xTaskCreate(tls_task, "TLS Task", 4096, NULL, 1, NULL);
    vTaskStartScheduler();

    for(;;);
}
