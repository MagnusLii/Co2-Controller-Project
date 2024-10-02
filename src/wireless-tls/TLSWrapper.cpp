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

TLSWrapper::TLSWrapper(const char *ssid, const char *password, const uint32_t countryCode = CYW43_COUNTRY_FINLAND,
    ConnectionProtocol ConnectionProtocol = ConnectionProtocol::HTTP,
    QueueHandle_t sendQueueHandle = nullptr,
    QueueHandle_t receiveQueueHandle = nullptr)
    : connectionProtocol(ConnectionProtocol),
      sendQueue(sendQueueHandle ? sendQueueHandle : xQueueCreate(SEND_QUEUE_SIZE, sizeof(Message))),
      receiveQueue(receiveQueueHandle ? receiveQueueHandle : xQueueCreate(RECEIVE_QUEUE_SIZE, sizeof(Message))),
      connectionStatus(ConnectionStatus::DISCONNECTED) {
        
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

ConnectionStatus TLSWrapper::connect(const std::string& hostname, int port) {
    switch (connectionProtocol)
    {
    case ConnectionProtocol::HTTPS:
        // Initialize mbedTLS structures
        mbedtls_ssl_init(&ssl);
        mbedtls_ssl_config_init(&conf);
        mbedtls_ctr_drbg_init(&ctr_drbg);
        mbedtls_entropy_init(&entropy);

        // Seed the random number generator
        if (mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, nullptr, 0) != 0) {
            TLSWRAPPERprintf("TLSWrapper::connect: failed to seed RNG\n");
            return ConnectionStatus::ERROR_RNG;
        }

        // Load default SSL configuration
        if (mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
            TLSWRAPPERprintf("TLSWrapper::connect: failed to load SSL config\n");
            return ConnectionStatus::ERROR_SSL_CONFIG;
        }

        mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

        // Set hostname for SNI (Server Name Indication)
        if (mbedtls_ssl_set_hostname(&ssl, hostname.c_str()) != 0) {
            TLSWRAPPERprintf("TLSWrapper::connect: failed to set hostname\n");
            return ConnectionStatus::ERROR_SSL_HOSTNAME;
        }

        // Setup SSL context
        if (mbedtls_ssl_setup(&ssl, &conf) != 0) {
            TLSWRAPPERprintf("TLSWrapper::connect: failed to setup SSL\n");
            return ConnectionStatus::ERROR_SSL_SETUP;
        }

        // Create a new TCP connection
        if (!ip4addr_aton(hostname.c_str(), &ipAddress)) {
            TLSWRAPPERprintf("TLSWrapper::connect: failed IP address conversion\n");
            return ConnectionStatus::ERROR_ADDRESS_CONVERSION;
        }

        tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(ipAddress));
        if (!tcp_pcb) {
            TLSWRAPPERprintf("TLSWrapper::connect: failed to create PCB\n");
            return ConnectionStatus::ERROR_PCB_CREATION;
        }

        tcp_arg(tcp_pcb, this);
        tcp_poll(tcp_pcb, TLSWrapper::tcp_client_poll, POLL_TIME_S);
        tcp_sent(tcp_pcb, TLSWrapper::tcp_client_sent);
        tcp_receive(tcp_pcb, TLSWrapper::tcp_client_receive);
        tcp_err(tcp_pcb, TLSWrapper::tcp_client_err);

        cyw43_arch_lwip_begin(); // Critical section
        if (tcp_connect(tcp_pcb, &ipAddress, port, TLSWrapper::tcp_client_connected) != ERR_OK) {
            TLSWRAPPERprintf("TLSWrapper::connect: failed to connect\n");
            tcp_close(tcp_pcb); // Cleanup
            tcp_pcb = nullptr;
            cyw43_arch_lwip_end();
            return ConnectionStatus::ERROR_OUT_OF_MEMORY;
        }

        // Perform SSL handshake
        int ret;
        while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
            TLSWRAPPERprintf("TLSWrapper::connect: SSL handshake failed\n");
            return ConnectionStatus::ERROR_SSL_HANDSHAKE;
            }
        }

        cyw43_arch_lwip_end(); // Stop critical section
        break;
    
    default:
        // Reconnect guard.
        if (tcp_pcb != nullptr) {
            TLSWRAPPERprintf("TLSWrapper::connect: already connected or connecting\n");
            return ConnectionStatus::ALREADY_CONNECTED;
        }

        // TODO: add hostname validation?
        if (!ip4addr_aton(hostname.c_str(), &ipAddress)) {
            TLSWRAPPERprintf("TLSWrapper::connect: failed ipAddr conversion\n");
            return ConnectionStatus::ERROR_ADDRESS_CONVERSION;
        }

        tcp_pcb = tcp_new_ip_type(IP_GET_TYPE(ipAddress));
        if (!tcp_pcb) {
            TLSWRAPPERprintf("TLSWrapper::connect: failed to create pcb\n");
            return ConnectionStatus::ERROR_PCB_CREATION;
        }

        tcp_arg(tcp_pcb, this);
        tcp_poll(tcp_pcb, TLSWrapper::tcp_client_poll, POLL_TIME_S);
        tcp_sent(tcp_pcb, TLSWrapper::tcp_client_sent);
        tcp_receive(tcp_pcb, TLSWrapper::tcp_client_receive);
        tcp_err(tcp_pcb, TLSWrapper::tcp_client_err);


        cyw43_arch_lwip_begin(); // Critical section
        if (tcp_connect(tcp_pcb, &ipAddress, port, TLSWrapper::tcp_client_connected) != ERR_OK) {
            TLSWRAPPERprintf("TLSWrapper::connect: failed to connect\n");
            tcp_close(tcp_pcb); // cleanup
            tcp_pcb = nullptr;
            cyw43_arch_lwip_end();
            return ConnectionStatus::ERROR_OUT_OF_MEMORY;
        }
        break;
    }

    cyw43_arch_lwip_end(); // Stop critical section
    return ConnectionStatus::CONNECTED;
}











// --------------------lwip functions---------------------------------------------------------------------------------
err_t IPStack::tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    //auto state = static_cast<IPStack *>(arg);
    DEBUG_printf("tcp_client_sent %u\n", len);

    return ERR_OK;
}

err_t IPStack::tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err) {
    auto state = static_cast<IPStack *>(arg);
    if (err != ERR_OK) {
        printf("connect failed %d\n", err);
    }
    state->connected = true;

    return ERR_OK;
}

err_t IPStack::tcp_client_poll(void *arg, struct tcp_pcb *tpcb) {
    //auto state = static_cast<IPStack *>(arg);
    DEBUG_printf("tcp_client_poll\n");
    return ERR_OK;
}

void IPStack::tcp_client_err(void *arg, err_t err) {
    //auto state = static_cast<IPStack *>(arg);
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err %d\n", err);
        //state->tcp_result(err);
    }
}

err_t IPStack::tcp_client_receive(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    auto state = static_cast<IPStack *>(arg);
    if (!p) {
        // connection has been closed - do we need to react to this somehow?
        return ERR_OK;
    }
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
#if 0
        DEBUG_printf("recv %d err %d\n", p->tot_len, err);
        for (struct pbuf *q = p; q != NULL; q = q->next) {
            DUMP_BYTES(q->payload, q->len);
        }
#endif
        // Receive the buffer
        uint16_t available = BUF_SIZE - state->count;
        uint16_t bytes_to_copy = available > p->tot_len ? p->tot_len : available;
        uint16_t wr_end = state->wr + bytes_to_copy;
        uint16_t first_copy = 0;

        // check if bytes are to be dropped
        if (bytes_to_copy < p->tot_len) {
            state->dropped += p->tot_len - bytes_to_copy;
        }

        if (wr_end > BUF_SIZE) {
            // need to copy in two parts
            first_copy = BUF_SIZE - state->wr; // calculate the size of first part to copy
            if (first_copy) { //
                bytes_to_copy -= pbuf_copy_partial(p, state->buffer + state->wr, first_copy, 0);
                state->wr = 0; // start next copy from beginning
            }
            state->count += first_copy; // increment count by copied bytes
            //printf("tot:%d, fc: %d, bc: %d, wr:%d\n", p->tot_len, first_copy, bytes_to_copy, state->wr);
        }
        state->wr += pbuf_copy_partial(p, state->buffer + state->wr, bytes_to_copy, first_copy);
        state->wr %= BUF_SIZE; // wrap over
        state->count += bytes_to_copy; // increment count by the rest of the copied bytes

        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p); // can we omit this call instead of dropping bytes to save the buffer for copying the rest later?

    return ERR_OK;
}
