#ifndef TLSWRAPPER_H
#define TLSWRAPPER_H

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
#include "Fmutex.h"
#include <mutex>

#define SEND_QUEUE_SIZE 64
#define RECEIVE_QUEUE_SIZE 64
#define CONNECTION_TIMEOUT_MS 5000
#define MAX_BUFFER_SIZE 1024
#define POLL_TIME_S 10

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
    void set_screen_write_handle(QueueHandle_t queue);
    QueueHandle_t get_read_handle(void);

private:
    void send_request(const std::string& endpoint, const std::string& request);
    void send_request_and_get_response(const std::string& endpoint, const std::string& request);

    void create_field_update_request(Message &messageContainer, const std::array<Reading, 8> &values);
    void create_command_request(Message &messageContainer);

    void process_and_send_sensor_data_task_();
    static void process_and_send_sensor_data_task(void *param){
        auto *funcPointer = static_cast<TLSWrapper*>(param);
        funcPointer->process_and_send_sensor_data_task_();
    }

    void send_field_update_request_task_(void *param);
    static void send_field_update_request_task(void *param){
        auto *funcPointer = static_cast<TLSWrapper*>(param);
        funcPointer->send_field_update_request_task_(param);
    }

    void get_server_commands_task_(void *param);
    static void get_server_commands_task(void *param){
        auto *funcPointer = static_cast<TLSWrapper*>(param);
        funcPointer->get_server_commands_task_(param);
    }

    void parse_server_commands_task_(void *param);
    static void parse_server_commands_task(void *param){
        auto *funcPointer = static_cast<TLSWrapper*>(param);
        funcPointer->parse_server_commands_task_(param);
    }

    // void reconnect_task_(void *param);
    // static void reconnect_task(void *param){
    //     auto *funcPointer = static_cast<TLSWrapper*>(param);
    //     funcPointer->reconnect_task_(param);
    // }

    std::string parse_command_from_http(const std::string& http_response);

    const std::string ssid;
    const std::string password;
    const uint32_t countryCode;
    QueueHandle_t reading_queue;
    QueueHandle_t writing_queue;
    QueueHandle_t screen_writing_queue;
    QueueHandle_t sensor_data_queue;
    QueueHandle_t response_queue;
    TaskHandle_t reconnect_task_handle;
    Fmutex mutex;
};

#endif //TLSWRAPPER_H
