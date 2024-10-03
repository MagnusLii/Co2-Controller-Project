#include "TLSWrapper.h"
#include <string>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "register_handler.h"
#include "tls_common.h"
#include "debugPrints.h"
#include <cstdint>
#include <memory>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "mbedtls/ssl.h"
#include "mbedtls/ssl_internal.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/error.h"
#include "lwip/ip4_addr.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"
#include "connection_defines.h"
#include "read_runtime_ctr.cpp"


TLSWrapper::TLSWrapper(const char *ssid, const char *password, char* certificate, const int certlen, const uint32_t countryCode){
        this->certificate = certificate;
        this->certificateLength = certlen;

        TLSWRAPPERprintf("TLSWrapper::TLSWrapper: Initializing TLSWrapper\n");
        if (cyw43_arch_init_with_country(countryCode) != 0) {
            TLSWRAPPERprintf("TLSWrapper::TLSWrapper: failed init\n");
        }
        cyw43_arch_enable_sta_mode();

        TLSWRAPPERprintf("TLSWrapper::TLSWrapper: Connecting to wireless\n");
        if (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, CONNECTION_TIMEOUT_MS) != 0) {
            TLSWRAPPERprintf("TLSWrapper::TLSWrapper: failed to connect\n");
        } else {
            TLSWRAPPERprintf("TLSWrapper::TLSWrapper: connected\n");
        }
    }

void TLSWrapper::connect(const char* endpoint, const int port){
    tls_config = altcp_tls_create_config_client((uint8_t*)this->certificate, this->certificateLength); // wtf
    assert(tls_config);

    mbedtls_ssl_conf_authmode((mbedtls_ssl_config *)tls_config, MBEDTLS_SSL_VERIFY_OPTIONAL);

    tls_client = tls_client_init();
    if (!tls_client) {
        TLSWRAPPERprintf("TLSWrapper::connect: Failed to initialize TLS client state\n");
        this->connectionStatus = ConnectionStatus::ERROR;
        free(tls_client);
        altcp_tls_free_config(tls_config);
        return;
    }

    // Temp stuff
    const char* request = "GET /update?api_key=1WWH2NWXSM53URR5&field1=410&field2=45.7 HTTP/1.1\r\n"
                      "Host: api.thingspeak.com\r\n"
                      "\r\n";

    tls_client->http_request = request;
    tls_client->timeout = CONNECTION_TIMEOUT_MS;

    if (!tls_client_open(endpoint, tls_client)) {
        TLSWRAPPERprintf("TLSWrapper::connect: Failed to open connection\n");
        this->connectionStatus = ConnectionStatus::ERROR;
        free(tls_client);
        altcp_tls_free_config(tls_config);
        return;
    }

    this->connectionStatus = ConnectionStatus::CONNECTED;




    while (!tls_client->complete) {
        vTaskDelay(100);
        TLSWRAPPERprintf("TLSWrapper::connect: connection live!\n");
    }

    free(tls_client);
    altcp_tls_free_config(tls_config);
    return;
}