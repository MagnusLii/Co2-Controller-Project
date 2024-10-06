/*
 * Copyright (c) 2023 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Include necessary libraries */
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
#include "connection_defines.h"

// MODIFIED to add response buffer for response storage
typedef struct TLS_CLIENT_T_ {
    struct altcp_pcb *pcb;
    bool complete;
    int error;
    const char *http_request;
    int timeout;
    char *response;
    bool response_stored;
    size_t response_len;
    bool initialized;
} TLS_CLIENT_T;

// Static variable for TLS configuration
static struct altcp_tls_config *tls_config = NULL;

/* Function to close a TLS client connection */
static err_t tls_client_close(void *arg) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;  // Retrieve client state
    err_t err = ERR_OK;

    state->complete = true;  // Mark the operation as complete
    if (state->pcb != NULL) {  // Check if a valid protocol control block exists
        altcp_arg(state->pcb, NULL);  // Clear the callback argument for this PCB
        altcp_poll(state->pcb, NULL, 0);  // Disable polling for this PCB
        altcp_recv(state->pcb, NULL);  // Clear the receive callback
        altcp_err(state->pcb, NULL);   // Clear the error callback
        err = altcp_close(state->pcb);  // Attempt to close the connection
        if (err != ERR_OK) {  // If close fails, force an abort
            printf("close failed %d, calling abort\n", err);
            altcp_abort(state->pcb);  // Abort the connection
            err = ERR_ABRT;  // Set the error code to abort
        }
        state->pcb = NULL;  // Clear the PCB reference
    }
    return err;  // Return the final status
}

static err_t tls_client_recv(void *arg, struct altcp_pcb *pcb, struct pbuf *p, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;
    if (!p) {
        printf("connection closed\n");
        return tls_client_close(state);
    }

    if (p->tot_len > 0) {
        /* For simplicity this examples creates a buffer on stack the size of the data pending here,
           and copies all the data to it in one go.
           Do be aware that the amount of data can potentially be a bit large (TLS record size can be 16 KB),
           so you may want to use a smaller fixed size buffer and copy the data to it using a loop, if memory is a concern */
        //char buf[p->tot_len + 1];
        char *buf= (char *) malloc(p->tot_len + 1);

        pbuf_copy_partial(p, buf, p->tot_len, 0);
        buf[p->tot_len] = 0;

        printf("***\nnew data received from server:\n***\n\n%s\n", buf);

        // copy response to state
        if (state->response == NULL) {
            state->response = (char *)malloc(p->tot_len + 1);
            if (state->response != NULL) {
            strcpy(state->response, buf);
            state->response_stored = true;
            state->response_len = p->tot_len;
            } else {
            printf("failed to allocate memory for response\n");
            }
        } else {

            size_t current_len = strlen(state->response);
            char *new_response = (char *)realloc(state->response, current_len + p->tot_len + 1);
            if (new_response != NULL) {
            state->response = new_response;
            strcat(state->response, buf);
            state->response_stored = true;
            state->response_len += p->tot_len;
            } else {
            printf("failed to reallocate memory for response\n");
            }
        }

        free(buf);

        altcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);

    return ERR_OK;
}

/* Callback when the TLS client successfully connects to the server */
static err_t tls_client_connected(void *arg, struct altcp_pcb *pcb, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;  // Retrieve the client state

    if (err != ERR_OK) {  // If the connection fails
        printf("connect failed %d\n", err);
        return tls_client_close(state);  // Close the connection
    }

    printf("connected to server, sending request\n");
    // Write the HTTP request to the server over the TLS connection
    err = altcp_write(state->pcb, state->http_request, strlen(state->http_request), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {  // Handle write errors
        printf("error writing data, err=%d", err);
        return tls_client_close(state);  // Close the connection
    }


    return ERR_OK;  // Return OK on success
}

/* Polling callback, executed when the client times out */
static err_t tls_client_poll(void *arg, struct altcp_pcb *pcb) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;  // Retrieve the client state
    printf("timed out\n");
    state->error = PICO_ERROR_TIMEOUT;  // Set timeout error
    return tls_client_close(arg);  // Close the connection
}

/* Error callback executed when an error occurs */
static void tls_client_err(void *arg, err_t err) {
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;  // Retrieve the client state
    printf("tls_client_err %d\n", err);
    tls_client_close(state);  // Close the connection
    state->error = PICO_ERROR_GENERIC;  // Set a generic error
}

/* Function to initiate a connection to a server given its IP address */
static void tls_client_connect_to_server_ip(const ip_addr_t *ipaddr, TLS_CLIENT_T *state) {
    err_t err;
    u16_t port = 443;  // Default to HTTPS port

    printf("connecting to server IP %s port %d\n", ipaddr_ntoa(ipaddr), port);
    // Start the TCP connection to the server
    err = altcp_connect(state->pcb, ipaddr, port, tls_client_connected);
    if (err != ERR_OK) {
        // Print error if the connection fails
        fprintf(stderr, "error initiating connect, err=%d\n", err);
        tls_client_close(state);  // Close the client
    }
}

/* Callback executed when DNS resolution completes */
static void tls_client_dns_found(const char* hostname, const ip_addr_t *ipaddr, void *arg) {
    if (ipaddr) {  // If DNS resolution was successful
        printf("DNS resolving complete\n");
        // Connect to the server using the resolved IP address
        tls_client_connect_to_server_ip(ipaddr, (TLS_CLIENT_T *) arg);
    } else {  // If DNS resolution failed
        printf("error resolving hostname %s\n", hostname);
        tls_client_close(arg);  // Close the client
    }
}

/* Function to initiate the DNS resolution and connection process */
static bool tls_client_open(const char *hostname, void *arg) {
    err_t err;
    ip_addr_t server_ip;  // Variable to hold the resolved IP address
    TLS_CLIENT_T *state = (TLS_CLIENT_T*)arg;

    // Create a new TLS-enabled PCB
    state->pcb = altcp_tls_new(tls_config, IPADDR_TYPE_ANY);
    if (!state->pcb) {
        printf("failed to create pcb\n");
        return false;
    }

    // Set various callbacks for the PCB
    altcp_arg(state->pcb, state);
    altcp_poll(state->pcb, tls_client_poll, state->timeout * 2);
    altcp_recv(state->pcb, tls_client_recv);
    altcp_err(state->pcb, tls_client_err);

    /* Set Server Name Indication (SNI) */
    mbedtls_ssl_set_hostname((mbedtls_ssl_context *)altcp_tls_context(state->pcb), hostname);

    printf("resolving %s\n", hostname);

    // Begin DNS resolution in a thread-safe manner
    cyw43_arch_lwip_begin();

    err = dns_gethostbyname(hostname, &server_ip, tls_client_dns_found, state);
    if (err == ERR_OK) {
        /* Host is in DNS cache */
        tls_client_connect_to_server_ip(&server_ip, state);
    } else if (err != ERR_INPROGRESS) {
        // Handle DNS resolution failure
        printf("error initiating DNS resolving, err=%d\n", err);
        tls_client_close(state->pcb);
    }

    cyw43_arch_lwip_end();  // End thread-safe block

    return err == ERR_OK || err == ERR_INPROGRESS;
}

/* Initialize the TLS client state */
static TLS_CLIENT_T* tls_client_init(void) {
    // Allocate memory for the client state structure
    TLS_CLIENT_T *state = (TLS_CLIENT_T *)calloc(1, sizeof(TLS_CLIENT_T));
    if (!state) {
        printf("failed to allocate state\n");
        return NULL;
    }

    return state;
}

/* Debug function to print TLS debug messages */
static void tlsdebug(void *ctx, int level, const char *file, int line, const char *message) {
    fputs(message, stdout);  // Output the debug message
}


/* Main function to run the TLS client test */
bool run_tls_client_test(const uint8_t *cert, size_t cert_len, const char *server, const char *request, int timeout) {
    // Create the TLS client configuration using the provided certificate
    tls_config = altcp_tls_create_config_client(cert, cert_len);
    assert(tls_config);

    // Initialize the TLS client state
    TLS_CLIENT_T *state = tls_client_init();
    if (!state) {
        return false;
    }

    // Set the HTTP request and timeout in the client state
    state->http_request = request;
    state->timeout = timeout;

    // Begin the process of connecting to the server
    if (!tls_client_open(server, state)) {
        return false;
    }

    // Main loop: wait for the connection to complete or time out
    while (!state->complete) {
#if PICO_CYW43_ARCH_POLL
        // Poll Wi-Fi and TCP/IP stack (only necessary for polling mode)
            cyw43_arch_poll();
            cyw43_arch_wait_for_work_until(make_timeout_time_ms(1000));
#else
        // In FreeRTOS mode, just delay
        vTaskDelay(1000);
#endif
    }

    // Store the error code and free resources
    int err = state->error;
    free(state);
    altcp_tls_free_config(tls_config);

    return err == 0;  // Return whether the connection was successful
}

void configure_tls(uint8_t *cert, size_t cert_len) {
    tls_config = altcp_tls_create_config_client(cert, cert_len);
}

bool send_tls_request(const char *server, const char *request, int timeout) {
    tls_config = altcp_tls_create_config_client(TLS_CERTIFICATE, sizeof(TLS_CERTIFICATE));
    assert(tls_config);

    mbedtls_ssl_conf_authmode((mbedtls_ssl_config *)tls_config, MBEDTLS_SSL_VERIFY_REQUIRED);
    
    TLS_CLIENT_T *state = tls_client_init();

    state->http_request = request;
    state->timeout = timeout;

    if (!tls_client_open(server, state)) {
        return false;
    }

    while (!state->complete) {
        vTaskDelay(1000);
    }

    int err = state->error;
    free(state);
    altcp_tls_free_config(tls_config);

    return err == 0;
}

bool send_tls_request_and_get_response(const char *server, const char *request, int timeout, char *response, size_t *response_len) {
    tls_config = altcp_tls_create_config_client(TLS_CERTIFICATE, sizeof(TLS_CERTIFICATE));
    assert(tls_config);

    mbedtls_ssl_conf_authmode((mbedtls_ssl_config *)tls_config, MBEDTLS_SSL_VERIFY_REQUIRED);
    
    TLS_CLIENT_T *state = tls_client_init();

    state->http_request = request;
    state->timeout = timeout;

    if (!tls_client_open(server, state)) {
        return false;
    }

    while (!state->complete) {
        vTaskDelay(1000);
    }

    int err = state->error;
    if (state->response_stored) {
        strcpy(response, state->response);
        *response_len = state->response_len;
    }

    free(state);
    altcp_tls_free_config(tls_config);

    return err == 0;
}