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
#define CONNECTION_TIMEOUT_MS 15000
#define MAX_BUFFER_SIZE 2048 // obsolete
#define POLL_TIME_S 10

struct Message {
    std::string data;
    int length;
};

class TLSWrapper {
public:
    enum class ConnectionStatus {
        CONNECTED,
        DISCONNECTED,
        ERROR
    };

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

    TLSWrapper(const std::string& cert, const std::string& ssid, const std::string& password, uint32_t countryCode);
    ~TLSWrapper();
    
    void send_request(const std::string& endpoint, const std::string& request);
    
    void emptyresponseBuffer(QueueHandle_t queue_where_to_store_msg);

    void createFieldUpdateRequest(Message &messageContainer, const float values[]);
    void createCommandRequest(Message &messageContainer, const char* command);

private:
    const std::string certificate;
    const std::string ssid;
    const std::string password;
    const uint32_t countryCode;
    ConnectionStatus connectionStatus = ConnectionStatus::DISCONNECTED;
    TLS_CLIENT_T* tls_client;
};

#endif //TLSWRAPPER_H