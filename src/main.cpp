#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "pico/cyw43_arch.h"
#include "connection_defines.h"
#include <string.h>
#include "read_runtime_ctr.cpp"
#include "debugPrints.h"
#include "TLSWrapper.h"


// Example tls test, verified to work.
void tls_test(void) {
    std::string request = "GET /update?api_key=1WWH2NWXSM53URR5&field1=410&field2=45.7 HTTP/1.1\r\n"
                      "Host: api.thingspeak.com\r\n"
                      "Connection: close\r\n" \
                      "\r\n";

    char ssid[] = WIFI_SSID;
    char pwd[] = WIFI_PASSWORD;
    printf("SSID: %s\nPWD: %s\n", ssid, pwd);//WIFI_SSID, WIFI_PASSWORD);
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return;
    }
    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect\n");
        return;
    }
    const uint8_t cert_joe[] = TLS_CERTIFICATE;

    bool pass = run_tls_client_test(cert_joe, sizeof(cert_joe), THINGSPEAK_IP, request.c_str(), 30000);
    //bool pass = run_tls_client_test(NULL, 0, TLS_CLIENT_SERVER, TLS_CLIENT_HTTP_REQUEST, TLS_CLIENT_TIMEOUT_SECS);
    if (pass) {
        printf("Test passed\n");
    } else {
        printf("Test failed\n");
    }
    /* sleep a bit to let usb stdio write out any buffer to host */
    sleep_ms(100);

    cyw43_arch_deinit();
    printf("All done\n");
    return;
}

void tls_task(void *pvParameters) {

    std::string cert = TLS_CERTIFICATE;
    std::string ssid = WIFI_SSID;
    std::string password = WIFI_PASSWORD;
    uint32_t countryCode = DEFAULT_COUNTRY_CODE;

    // std::string request = "POST /talkbacks/" TALKBACK_ID "/commands/execute.json HTTP/1.1\r\n"
    //                   "Host: api.thingspeak.com\r\n"
    //                   "Content-Length: 24\r\n"
    //                   "Content-Type: application/x-www-form-urlencoded\r\n"
    //                   "\r\n"
    //                   "api_key=" TALK_BACK_API_KEY;

    std::string request = "GET /update?api_key=" API_KEY "&field1=410&field2=45.7 HTTP/1.1\r\n"
                      "Host: api.thingspeak.com\r\n"
                      "Connection: close\r\n"
                      "\r\n";

    TLSWrapper tlsWrapper(cert, ssid, password, countryCode);

    tlsWrapper.send_request(THINGSPEAK_IP, request);

    vTaskDelete(NULL);
}

void tls_task2(void* param){
    tls_test();
    vTaskDelete(NULL);
}

int main() {
    stdio_init_all();
    // xTaskCreate(tls_task, "TLS Task", 4096, NULL, 1, NULL);
    xTaskCreate(tls_task2, "TLS Task 2", 4096, NULL, 1, NULL);
    vTaskStartScheduler();

    for(;;);
}
