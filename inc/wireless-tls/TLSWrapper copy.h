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
#include "connection_defines.h"

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
        ERROR,
        ERROR_OUT_OF_MEMORY,
        ERROR_ADDRESS_CONVERSION
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

    // Enum for connection type
    enum class ConnectionProtocol {
        HTTP,
        HTTPS
    };

    TLSWrapper(const char *ssid, const char *pw, const uint32_t countryCode,
        ConnectionProtocol ConnectionProtocol = ConnectionProtocol::HTTP,
        QueueHandle_t sendQueueHandle = nullptr,
        QueueHandle_t receiveQueueHandle = nullptr)
        : connectionProtocol(ConnectionProtocol),
          sendQueue(sendQueueHandle ? sendQueueHandle : xQueueCreate(SEND_QUEUE_SIZE, sizeof(Message))),
          receiveQueue(receiveQueueHandle ? receiveQueueHandle : xQueueCreate(RECEIVE_QUEUE_SIZE, sizeof(Message))),
          connectionStatus(ConnectionStatus::DISCONNECTED) {
            certificateLength = sizeof(TLS_CERTIFICATE);
          }

    // Destructor to destroy queues if they were created internally
    ~TLSWrapper() {
        if (sendQueue != nullptr && sendQueueHandle == nullptr) {
            vQueueDelete(sendQueue);
        }
        if (receiveQueue != nullptr && receiveQueueHandle == nullptr) {
            vQueueDelete(receiveQueue);
        }
    }

    // Connection status
    void setConnectionStatus(ConnectionStatus status);
    ConnectionStatus getConnectionStatus();
    bool isConnected();
    ConnectionStatus disconnect();

    // Connection initialization
    ConnectionStatus connect(const std::string& hostname, int port);
    int connect(const char *hostname, int port);

    // Sending data
    int send(const Message& message, int timeout_ms);
    // int send(const std::string& message, int timeout_ms);
    // int send(const char *message, int length, int timeout_ms);

    // Receiving data
    Message receive(int maxCharsToRead, int timeout_ms);
    // std::string receiveString(int maxCharsToRead, int timeout_ms);
    // char *receiveCharArray(int maxCharsToRead, int timeout_ms);

    // Message creation
    Message createMinimalPushMessage(const Reading* readings, int numReadings);


    static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
    static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb);
    static err_t tcp_client_receive(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
    static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
    static void tcp_client_err(void *arg, err_t err);

private:
    // general
    ConnectionStatus connectionStatus;
    QueueHandle_t sendQueue;             // Default queue where constructed messages are sent
    QueueHandle_t receiveQueue;          // Default queue where received messages are sent
    ConnectionProtocol connectionProtocol;

    //HTTP
    struct tcp_pcb *tcp_pcb;
    ip_addr_t ipAddress;
    uint8_t buffer[MAX_BUFFER_SIZE];
    uint16_t count;
    uint32_t dropped;
    uint16_t wr; // write index
    uint16_t rd; // read index


    // HTTPS
    //const char* server_;
    const uint8_t certificate = TLS_CERTIFICATE;
    size_t certificateLength;
    const char* request;
    int timeout;
    TLS_CLIENT_T* state;
    struct altcp_tls_config* tls_config;
};


#endif //TLSWRAPPER_H