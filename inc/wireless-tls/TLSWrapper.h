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
    enum class ConnectionStatus {
        CONNECTED,
        DISCONNECTED,
        ERROR
    };

    TLSWrapper(const char *ssid, const char *pw, const char* certificate, const int certlen, const uint32_t countryCode = CYW43_COUNTRY_FINLAND);

    ConnectionStatus connect(const std::string& hostname, int port);

    int send(const std::string& data);
    std::string receive();

private:
    ConnectionStatus connectionStatus;
    const char certificate[];
    const int certificateLength;

    // TCP connection details
    struct tcp_pcb* tcp_pcb;
    ip4_addr ipAddress;

    // TLS session data
    struct altcp_tls_config *tls_config;
    TLS_CLIENT_T *state_;

    const int connectionTimeout = CONNECTION_TIMEOUT_MS;
    const int maxBufferSize = MAX_BUFFER_SIZE;
    const int pollTime = POLL_TIME_S;

    static err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb);
    static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
    static err_t tcp_client_receive(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
    static void tcp_client_err(void *arg, err_t err);
};



#endif //TLSWRAPPER_H