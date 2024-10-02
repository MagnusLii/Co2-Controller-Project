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


TLSWrapper::TLSWrapper(const char *ssid, const char *pw, const char* certificate, const int certlen, const uint32_t countryCode = CYW43_COUNTRY_FINLAND) {
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



err_t TLSWrapper::tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
    TLSWrapper* tlsWrapper = static_cast<TLSWrapper*>(arg);
    // Perform periodic checks, e.g., to handle timeouts or keep-alives
    return ERR_OK; // or ERR_ABRT to indicate error or disconnection
}

err_t TLSWrapper::tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TLSWrapper* tlsWrapper = static_cast<TLSWrapper*>(arg);
    // Handle the event when data has been successfully sent
    TLSWRAPPERprintf("Data sent successfully, length: %u\n", len);
    return ERR_OK;
}

err_t TLSWrapper::tcp_client_receive(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TLSWrapper* tlsWrapper = static_cast<TLSWrapper*>(arg);
    if (p == NULL) {
        // Connection closed by peer
        tcp_close(tpcb);
        return ERR_OK;
    }

    // Process received data from pbuf
    // Example: Convert pbuf to string and print
    std::string receivedData((char*)p->payload, p->len);
    TLSWRAPPERprintf("Received data: %s\n", receivedData.c_str());

    // Free the pbuf
    pbuf_free(p);

    return ERR_OK; // or an error code if needed
}

void TLSWrapper::tcp_client_err(void *arg, err_t err) {
    TLSWrapper* tlsWrapper = static_cast<TLSWrapper*>(arg);
    // Handle error, possibly log or clean up
    TLSWRAPPERprintf("TCP error occurred: %d\n", err);
}