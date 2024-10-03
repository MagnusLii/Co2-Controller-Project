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
#include "lwip/pbuf.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"


TLSWrapper::TLSWrapper(const char *ssid, const char *password, const uint32_t countryCode,
        const char* certificate, const int certlen, const uint32_t countryCode = CYW43_COUNTRY_FINLAND){
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

void TLSWrapper::connect(const char* endpoint, const int port, const char* certificate, const size_t certificatelen){
    tls_config = altcp_tls_create_config_client(certificate, certificatelen);
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

    tls_client->http_request = nullptr;
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
        TLSWWRAPPERprintf("TLSWrapper::connect: connection live!\n");
    }

    free(tls_client);
    altcp_tls_free_config(tls_config);
    return;
}

















// TLSWrapper::ConnectionStatus TLSWrapper::connect(const std::string& hostname, int port) {
//     TLS_CLIENT_T* state = new TLS_CLIENT_T();
//     state->http_request = nullptr;  // Initialize with nullptr or you can define a default request
//     state->timeout = CONNECTION_TIMEOUT_MS;

//     // Initialize the TLS configuration (client certificate, etc.)
//     tls_config = altcp_tls_create_config_client(certificate, certificateLength);
//     if (!tls_config) {
//         TLSWRAPPERprintf("TLSWrapper::connect: Failed to create TLS config\n");
//         return ConnectionStatus::ERROR;
//     }

//     // Create a new PCB (Protocol Control Block)
//     state->pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY);
//     if (!state->pcb) {
//         TLSWRAPPERprintf("TLSWrapper::connect: Failed to create PCB\n");
//         altcp_tls_free_config(tls_config);
//         delete state;
//         return ConnectionStatus::ERROR;
//     }

//     // Set arguments, error handler, and receive handler
//     altcp_arg(state->pcb, state);
//     altcp_poll(state->pcb, tls_client_poll, POLL_TIME_S);
//     altcp_recv(state->pcb, tls_client_recv);
//     altcp_err(state->pcb, tls_client_err);

//     // Set the SNI (Server Name Indication) for SSL
//     mbedtls_ssl_set_hostname(altcp_tls_context(state->pcb), hostname.c_str());

//     // Resolve hostname to IP address
//     cyw43_arch_lwip_begin();
//     err_t dns_err = dns_gethostbyname(hostname.c_str(), &server_ip, tls_client_dns_found, state);
//     cyw43_arch_lwip_end();

//     if (dns_err == ERR_OK) {
//         // Host is already resolved in DNS cache
//         tls_client_connect_to_server_ip(&server_ip, state);
//     } else if (dns_err != ERR_INPROGRESS) {
//         TLSWRAPPERprintf("TLSWrapper::connect: DNS resolving failed, err=%d\n", dns_err);
//         tls_client_close(state);
//         return ConnectionStatus::ERROR;
//     }

//     // Wait for connection to complete (with task delay)
//     while (!state->complete) {
//         vTaskDelay(1000);  // Delay to avoid busy waiting
//     }

//     int connection_err = state->error;
//     delete state;

//     if (connection_err == 0) {
//         return ConnectionStatus::CONNECTED;
//     } else {
//         TLSWRAPPERprintf("TLSWrapper::connect: Connection failed\n");
//         return ConnectionStatus::ERROR;
//     }
// }