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

// Structure representing the TLS client state
typedef struct TLS_CLIENT_T_ {
    struct altcp_pcb *pcb;       // Pointer to the ALT-TCP (Protocol Control Block)
    bool complete;               // Flag indicating if the operation is complete
    int error;                   // Holds error code, if any
    const char *http_request;    // HTTP request to be sent to the server
    int timeout;                 // Timeout duration in seconds
} TLS_CLIENT_T;

/**
 * @brief Close a TLS client connection.
 *
 * @param arg Pointer to the client state.
 * @return err_t Status of the close operation.
 */
static err_t tls_client_close(void *arg);

/**
 * @brief Callback when the TLS client successfully connects to the server.
 *
 * @param arg Pointer to the client state.
 * @param pcb Pointer to the protocol control block.
 * @param err Error code of the connection attempt.
 * @return err_t Status of the connection.
 */
static err_t tls_client_connected(void *arg, struct altcp_pcb *pcb, err_t err);

/**
 * @brief Polling callback, executed when the client times out.
 *
 * @param arg Pointer to the client state.
 * @param pcb Pointer to the protocol control block.
 * @return err_t Status of the polling.
 */
static err_t tls_client_poll(void *arg, struct altcp_pcb *pcb);

/**
 * @brief Error callback executed when an error occurs.
 *
 * @param arg Pointer to the client state.
 * @param err Error code of the failure.
 */
static void tls_client_err(void *arg, err_t err);

/**
 * @brief Receive callback triggered when data is received from the server.
 *
 * @param arg Pointer to the client state.
 * @param pcb Pointer to the protocol control block.
 * @param p Packet buffer containing received data.
 * @param err Error code of the receive operation.
 * @return err_t Status of the receive operation.
 */
static err_t tls_client_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err);

/**
 * @brief Function to initiate a connection to a server given its IP address.
 *
 * @param ipaddr Pointer to the server's IP address.
 * @param state Pointer to the client state.
 */
static void tls_client_connect_to_server_ip(const ip_addr_t *ipaddr, TLS_CLIENT_T *state);

/**
 * @brief Callback executed when DNS resolution completes.
 *
 * @param hostname Server hostname.
 * @param ipaddr Resolved IP address.
 * @param arg Pointer to the client state.
 */
static void tls_client_dns_found(const char* hostname, const ip_addr_t *ipaddr, void *arg);

/**
 * @brief Function to initiate the DNS resolution and connection process.
 *
 * @param hostname Server hostname.
 * @param arg Pointer to the client state.
 * @return true if the DNS resolution is successful, false otherwise.
 */
static bool tls_client_open(const char *hostname, void *arg);

/**
 * @brief Initialize the TLS client state.
 *
 * @return Pointer to the initialized client state.
 */
static TLS_CLIENT_T* tls_client_init(void);

/**
 * @brief Debug function to print TLS debug messages.
 *
 * @param ctx Context of the debug function.
 * @param level Debug level.
 * @param file Source file where the debug message originates.
 * @param line Line number in the source file.
 * @param message Debug message.
 */
static void tlsdebug(void *ctx, int level, const char *file, int line, const char *message);

/**
 * @brief Main function to run the TLS client test.
 *
 * @param cert Pointer to the certificate data.
 * @param cert_len Length of the certificate data.
 * @param server Server hostname or IP address.
 * @param request HTTP request to be sent.
 * @param timeout Timeout duration in seconds.
 * @return true if the test succeeds, false otherwise.
 */
bool run_tls_client_test(const uint8_t *cert, size_t cert_len, const char *server, const char *request, int timeout);

#endif // TLS_COMMON_H
