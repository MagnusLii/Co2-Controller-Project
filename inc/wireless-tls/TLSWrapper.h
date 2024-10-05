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
#define READING_SEND_INTERVAL 10000

struct Message {
    std::string data;
};

class TLSWrapper {
public:
    enum class ConnectionStatus {
        CONNECTED,
        DISCONNECTED,
        ERROR
    };
    TLSWrapper(const std::string& ssid, const std::string& password, uint32_t countryCode);
    ~TLSWrapper();
    void set_write_handle(QueueHandle_t queue);
    QueueHandle_t get_read_handle(void);

private:
    void send_request(const std::string& endpoint, const std::string& request);
    void send_request_and_get_response(const std::string& endpoint, const std::string& request);

    void create_field_update_request(Message &messageContainer, const std::array<Reading, 8> &values);
    void create_command_request(Message &messageContainer, const char* command);

    void api_call_creator_();


    static void api_call_creator(void *param){
        auto *wtfIsThis = static_cast<TLSWrapper*>(param);
        wtfIsThis->api_call_creator_();
    }

    void testtask_(void *asd);

    static void testtask(void *param){
        auto *IhateThis = static_cast<TLSWrapper*>(param);
        IhateThis->testtask_(param);
    }

    const std::string ssid;
    const std::string password;
    ConnectionStatus connectionStatus = ConnectionStatus::DISCONNECTED;
    const uint32_t countryCode;
    QueueHandle_t reading_queue;
    QueueHandle_t writing_queue;
    QueueHandle_t TLSWrapper_private_queue;
};

#endif //TLSWRAPPER_H