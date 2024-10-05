#ifndef TLS_COMMON_H
#define TLS_COMMON_H

#include <string.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"
#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct TLS_CLIENT_T_ {
    struct altcp_pcb *pcb;
    bool complete;
    int error;
    const char *http_request;
    int timeout;
    char *response;
    bool response_stored;
    bool initialized = false;
} TLS_CLIENT_T;

err_t tls_client_close(void *arg);
err_t tls_client_connected(void *arg, struct altcp_pcb *pcb, err_t err);
err_t tls_client_poll(void *arg, struct altcp_pcb *pcb);
void tls_client_err(void *arg, err_t err);
err_t tls_client_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err);
void tls_client_connect_to_server_ip(const ip_addr_t *ipaddr, TLS_CLIENT_T *state);
void tls_client_dns_found(const char* hostname, const ip_addr_t *ipaddr, void *arg);
bool tls_client_open(const char *hostname, void *arg);
TLS_CLIENT_T* tls_client_init(void);
void tlsdebug(void *ctx, int level, const char *file, int line, const char *message);
bool run_tls_client_test(const uint8_t *cert, size_t cert_len, const char *server, const char *request, int timeout);
    bool send_tls_request(const char *server, const char *request, int timeout, TLS_CLIENT_T *state);
    void configure_tls(uint8_t *cert, size_t cert_len);

#ifdef __cplusplus
}
#endif

#endif // TLS_COMMON_H
