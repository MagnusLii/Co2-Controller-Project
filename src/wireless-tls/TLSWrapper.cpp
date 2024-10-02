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
#include "lwip/ip4_addr.h"
#include "lwip/altcp_tls.h"


TLSWrapper::ConnectionStatus TLSWrapper::connect(const std::string& hostname, int port) {
    // Initialize TLS configuration
    tls_config = altcp_tls_create_config_client(certificate, certificateLength);
    if (!tls_config) {
        TLSWRAPPERprintf("TLSWrapper::connect: Failed to create TLS config\n");
        return ConnectionStatus::ERROR;
    }

    // Initialize TLS client state
    state_ = tls_client_init();
    if (!state_) {
        TLSWRAPPERprintf("TLSWrapper::connect: Failed to initialize TLS client state\n");
        return ConnectionStatus::ERROR;
    }

    // Convert hostname to IP address
    if (!ip4addr_aton(hostname.c_str(), &ipAddress)) {
        TLSWRAPPERprintf("TLSWrapper::connect: Failed to convert hostname to IP address\n");
        return ConnectionStatus::ERROR;
    }

    // Create new TCP PCB (protocol control block)
    tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(ipAddress));
    if (!tcp_pcb) {
        TLSWRAPPERprintf("TLSWrapper::connect: Failed to create TCP control block\n");
        return ConnectionStatus::ERROR;
    }

    // Assign connection handlers for TCP connection
    tcp_arg(tcp_pcb, this);
    tcp_poll(tcp_pcb, TLSWrapper::tcp_client_poll, pollTime);
    tcp_sent(tcp_pcb, TLSWrapper::tcp_client_sent);
    tcp_recv(tcp_pcb, TLSWrapper::tcp_client_receive);
    tcp_err(tcp_pcb, TLSWrapper::tcp_client_err);

    // Start the connection
    TLSWRAPPERprintf("TLSWrapper::connect: Connecting to %s:%d\n", hostname.c_str(), port);
    cyw43_arch_lwip_begin();
    if (tcp_connect(tcp_pcb, &ipAddress, port, TLSWrapper::tcp_client_connected) != ERR_OK) {
        cyw43_arch_lwip_end();
        TLSWRAPPERprintf("TLSWrapper::connect: Failed to connect to the server\n");
        return ConnectionStatus::ERROR;
    }
    cyw43_arch_lwip_end();

    // Perform SSL handshake
    if (!tls_client_open(hostname.c_str(), state_)) {
        TLSWRAPPERprintf("TLSWrapper::connect: Failed to establish TLS connection\n");
        return ConnectionStatus::ERROR;
    }

    TLSWRAPPERprintf("TLSWrapper::connect: Connected successfully to %s\n", hostname.c_str());
    connectionStatus = ConnectionStatus::CONNECTED;

    return connectionStatus;
}

int TLSWrapper::send(const std::string& data) {
    if (connectionStatus != ConnectionStatus::CONNECTED) {
        TLSWRAPPERprintf("TLSWrapper::send: Not connected\n");
        return -1;
    }

    cyw43_arch_lwip_begin();
    int result = tcp_write(tcp_pcb, data.c_str(), data.size(), TCP_WRITE_FLAG_COPY);
    tcp_output(tcp_pcb);
    cyw43_arch_lwip_end();

    if (result != ERR_OK) {
        TLSWRAPPERprintf("TLSWrapper::send: Failed to send data\n");
        return -1;
    }

    TLSWRAPPERprintf("TLSWrapper::send: Data sent successfully\n");
    return data.size();
}

std::string TLSWrapper::receive() {
    if (connectionStatus != ConnectionStatus::CONNECTED) {
        TLSWRAPPERprintf("TLSWrapper::receive: Not connected\n");
        return "";
    }

    cyw43_arch_lwip_begin();
    struct pbuf* buffer = tcp_recv(tcp_pcb, nullptr, nullptr);
    cyw43_arch_lwip_end();

    if (!buffer) {
        TLSWRAPPERprintf("TLSWrapper::receive: Failed to receive data\n");
        return "";
    }

    std::string data((char*)buffer->payload, buffer->len);
    pbuf_free(buffer);
    TLSWRAPPERprintf("TLSWrapper::receive: Data received successfully\n");

    return data;
}