#ifndef TLSWRAPPER_H
#define TLSWRAPPER_H

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

#define SEND_QUEUE_SIZE 64
#define RECEIVE_QUEUE_SIZE 64
#define CONNECTION_TIMEOUT_MS 10000
#define MAX_BUFFER_SIZE 2048
#define POLL_TIME_S 10

struct Message {
    std::vector<unsigned char> data;
    int length;
};

class TLSWrapper {
public:
    // Enum for connection status
    enum class ConnectionStatus {
        CONNECTED,
        DISCONNECTED,
        ERROR
    };

    // Enum for order (API fields)
    enum class ApiFields {
        CO2_LEVEL,
        RELATIVE_HUMIDITY,
        TEMPERATURE,
        VENT_FAN_SPEED,
        CO2_SET_POINT,
        TIMESTAMP,
        DEVICE_STATUS,
        UNDEFINED
    };

    TLSWrapper(const char *ssid, const char *password, char* certificate, const int certlen, const uint32_t countryCode);

    void connect(const char* endpoint, const int port);


private:
    char* certificate;
    int certificateLength;
    ConnectionStatus connectionStatus = ConnectionStatus::DISCONNECTED;

    TLS_CLIENT_T* tls_client;

};











#endif //TLSWRAPPER_H