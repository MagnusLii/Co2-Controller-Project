#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/cyw43_arch.h"
#include "connection_defines.h"
#include <string.h>
#include "tls_common.h"
#include "read_runtime_ctr.cpp"

extern bool run_tls_client_test(const uint8_t *cert, size_t cert_len, const char *server, const char *request, int timeout);

#define HTTPS_REQUEST "GET /update?api_key=1WWH2NWXSM53URR5&field1=410&field2=45.7 HTTP/1.1\r\n" \
                      "Host: api.thingspeak.com\r\n" \
                      "\r\n"

#define TLS_CLIENT_SERVER_IP "3.224.58.169"

#define TLS_CERTIFICATE NULL
#define TLS_CERTIFICATE_LEN 0

#define TLS_CLIENT_TIMEOUT_SECS 15

void tls_task(void *params) {
    // Initialize Wi-Fi
    if (cyw43_arch_init()) {
        printf("Failed to initialize Wi-Fi.\n");
        vTaskDelete(NULL);
    }
    cyw43_arch_enable_sta_mode();

    // Connect to Wi-Fi
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("Failed to connect to Wi-Fi.\n");
        vTaskDelete(NULL);
    }

    // Run the TLS client test
    bool success = run_tls_client_test(TLS_CERTIFICATE, TLS_CERTIFICATE_LEN, TLS_CLIENT_SERVER_IP, HTTPS_REQUEST, TLS_CLIENT_TIMEOUT_SECS);
    
    // Print the result
    if (success) {
        printf("TLS request successful.\n");
    } else {
        printf("TLS request failed.\n");
    }

    cyw43_arch_deinit();
    printf("Task finished.\n");

    vTaskDelete(NULL);
}

int main() {
    stdio_init_all();
    xTaskCreate(tls_task, "TLS Task", 4096, NULL, 1, NULL);
    vTaskStartScheduler();

    for(;;);
}
