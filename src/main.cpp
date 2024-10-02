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

#define TLS_CERTIFICATE "-----BEGIN CERTIFICATE-----\n\
MIIDXTCCAkWgAwIBAgIUPp2A/waMZh3cPqpQ9xqhu5d3lA0wDQYJKoZIhvcNAQEL\n\
BQAwPjEXMBUGA1UEAwwOMTguMTk4LjE4OC4xNTExCzAJBgNVBAYTAlVTMRYwFAYD\n\
VQQHDA1TYW4gRnJhbnNpc2NvMB4XDTI0MDYxMjExMjEwM1oXDTI1MDYwMzExMjEw\n\
M1owPjEXMBUGA1UEAwwOMTguMTk4LjE4OC4xNTExCzAJBgNVBAYTAlVTMRYwFAYD\n\
VQQHDA1TYW4gRnJhbnNpc2NvMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKC\n\
AQEAuWejVUsk/cYJHp+vOYkBzWdSvHlYWbkdWf2HnHy8qYLMJ/sQyYcL9XEv85dq\n\
HrOCuS1vp7UC0YxnfFQ2tmQ9PNqaEUOOvIwJUOK5jutK+H16gFTbOHM4EdcY1WkJ\n\
43jffHSiq7RRiAUhTwh+2ISCMAxPlXcOiEPoUrFauOKTRMvBFcfgqFHbOdCA9X5z\n\
ol0JzdeV9MMYtSWhMi+F+DJBMrNDxQhymJFyt6p9ft0v8m5B5mTKGuhppMCUSHNP\n\
ij3WQkTnByOynUAQ3WG/LaSNg1ItqPVf9/RHKWWViRAwB4DEfOoeKkM2EFHqxHLw\n\
bjybmleFnxQguzX8+oEe9NKGTQIDAQABo1MwUTAdBgNVHQ4EFgQUx8JPYn//MjiT\n\
4o38VAS4advRrtQwHwYDVR0jBBgwFoAUx8JPYn//MjiT4o38VAS4advRrtQwDwYD\n\
VR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOCAQEADqaV5F+HhRqL35O5sycZ\n\
E4Gn8KewK/BDWkmQzqP5+w1PZ9bUGiSY49ftT2EGiLoOawnztkGBWX1Sv520T5qE\n\
wvB/rDzxOU/v9OIUTqCX7X68zVoN7A7r1iP6o66mnfgu9xDSk0ROZ73bYtaWL/Qq\n\
SJWBN1pPY2ekFxYNwBg8C1DTJ3H51H6R7kN0wze7lMN1tglrvLl1e60a8rm+QNwX\n\
FzQGTenLecgMGeXVsIGhnivQTvF2HN+EcXHs8O8LzHpX7fpt/KcsBx+kYmltkdJW\n\
QaFXAdvGJkhKEwJVn3qETVlTdtSKpc/1KdXq/01HuX7cPfXVMGJVXuJAk6Yxgx8z\n\
Ew==\n\
-----END CERTIFICATE-----\n"
#define TLS_CERTIFICATE_LEN 1270

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

    char ssid[] = WIFI_SSID;
    char pwd[] = WIFI_PASSWORD;

    const uint8_t cert_joe[] = TLS_CERTIFICATE;
    const uint8_t dummy_cert[]={0};

    bool success = run_tls_client_test(cert_joe, sizeof(cert_joe), TLS_CLIENT_SERVER_IP, HTTPS_REQUEST, TLS_CLIENT_TIMEOUT_SECS);

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
