#include "TLSWrapper.h"
#include <string>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "register_handler.h"
//#include "tls_common.h"
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
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"
#include "connection_defines.h"
#include "debugPrints.h"

const char* FIELD_NAMES[] = {
    "&field1=",
    "&field2=",
    "&field3=",
    "&field4=",
    "&field5=",
    "&field6=",
    "&field7=",
    "&field8="
};
    TLS_CLIENT_T* tls_client;
TLSWrapper::TLSWrapper(const std::string& ssid, const std::string& password, uint32_t countryCode)
    :
     ssid(ssid),
     password(password),
     connectionStatus(ConnectionStatus::DISCONNECTED),
     countryCode(countryCode){

        TLSWRAPPERprintf("TLSWrapper::TLSWrapper: Initializing TLSWrapper\n");
        if (cyw43_arch_init_with_country(countryCode) != 0) {
            TLSWRAPPERprintf("TLSWrapper::TLSWrapper: failed init\n");
        }
        cyw43_arch_enable_sta_mode();

        TLSWRAPPERprintf("TLSWrapper::TLSWrapper: Connecting to wireless\n");
        if (cyw43_arch_wifi_connect_timeout_ms(ssid.c_str(), password.c_str(), CYW43_AUTH_WPA2_AES_PSK, CONNECTION_TIMEOUT_MS) != 0) {
            TLSWRAPPERprintf("TLSWrapper::TLSWrapper: failed to connect\n");
        } else {
            TLSWRAPPERprintf("TLSWrapper::TLSWrapper: connected\n");
        }
}

TLSWrapper::~TLSWrapper() {
    if (tls_client) {
        // Free the response if it was allocated
        if (tls_client->response) {
            free(tls_client->response);
            tls_client->response = nullptr;
        }

        // Free the tls_client structure itself
        free(tls_client);
        tls_client = nullptr;
    }
}

void TLSWrapper::send_request(const std::string& endpoint, const std::string& request){
    TLSWRAPPERprintf("TLSWrapper::send_request: %s\n", request.c_str());
    send_tls_request(endpoint.c_str(), request.c_str(), CONNECTION_TIMEOUT_MS, tls_client); // custom made.
    // run_tls_client_test(cert, sizeof(cert), endpoint.c_str(), request.c_str(), CONNECTION_TIMEOUT_MS); // Verified to work
}

void TLSWrapper::emptyresponseBuffer(QueueHandle_t queue_where_to_store_msg) {  
    Message msg;

    // Check if the response is not NULL
    if (tls_client->response != NULL) {
        msg.data = strdup(tls_client->response);
        msg.length = strlen(tls_client->response);
        if (msg.data.length() != 0) {
            TLSWRAPPERprintf("Failed to allocate memory for msg.data\n");
            return;
        }
        tls_client->response_stored = false;
        xQueueSend(queue_where_to_store_msg, &msg, 0);
    } else {
        msg.data = strdup(""); // TODO: Remove?
    }
}

void TLSWrapper::createFieldUpdateRequest(Message &messageContainer, const float values[]){
    messageContainer.data = ""; // Clear message container
    
    messageContainer.data += "GET /update?api_key=";
    messageContainer.data += API_KEY;

    for (int i = 0; i < (int)TLSWrapper::ApiFields::UNDEFINED; i++) {
        messageContainer.data += FIELD_NAMES[i];
        char buffer[69];
        snprintf(buffer, sizeof(buffer), "%.2f", values[i]);
        messageContainer.data += buffer;
    }

    messageContainer.data += " HTTP/1.1\r\n";
    messageContainer.data += "Host: ";
    messageContainer.data += THINGSPEAK_HOSTNAME;
    messageContainer.data += "Connection: close\r\n";
    messageContainer.data += "\r\n";

    messageContainer.length = messageContainer.data.length();

    return;
}

void TLSWrapper::createCommandRequest(Message &messageContainer, const char* command){
    messageContainer.data = ""; // Clear message container

    messageContainer.data += "POST /talkbacks/" TALKBACK_ID "/commands/execute.json HTTP/1.1\r\n";
    messageContainer.data += "Host: " THINGSPEAK_HOSTNAME "\r\n";
    messageContainer.data += "Content-Length: 24\r\n";
    messageContainer.data += "Content-Type: application/x-www-form-urlencoded\r\n";
    messageContainer.data += "\r\n";
    messageContainer.data += "api_key=" TALK_BACK_API_KEY;

    messageContainer.length = messageContainer.data.length();

    return;
}






// void TLSWrapper::send_request(const std::string& endpoint, const std::string& request) {

//     // Initialize TLS config
//     altcp_tls_config* tls_config = altcp_tls_create_config_client((uint8_t*)this->certificate.c_str(), this->certificateLength);
//     assert(tls_config);

//     mbedtls_ssl_conf_authmode((mbedtls_ssl_config *)tls_config, MBEDTLS_SSL_VERIFY_REQUIRED);

//     // Initialize the client state
//     tls_client = tls_client_init();
//     if (!tls_client) {
//         TLSWRAPPERprintf("TLSWrapper::connect: Failed to initialize client state\n");
//         this->connectionStatus = ConnectionStatus::ERROR;
//         free(tls_client);
//         altcp_tls_free_config(tls_config);
//         return;
//     }

//     tls_client->http_request = request.c_str();
//     tls_client->timeout = CONNECTION_TIMEOUT_MS;

//     // Open the TLS connection
//     if (!tls_client_open(endpoint.c_str(), tls_client)) {
//         TLSWRAPPERprintf("TLSWrapper::connect: Failed to open connection\n");
//         this->connectionStatus = ConnectionStatus::ERROR;
//         free(tls_client);
//         altcp_tls_free_config(tls_config);
//         return;
//     }

//     this->connectionStatus = ConnectionStatus::CONNECTED;

//     // Wait for completion
//     while (!tls_client->complete) {
//         vTaskDelay(100);
//     }

//     TLSWRAPPERprintf("Response: %s\n", tls_client->response);

//     free(tls_client);
//     altcp_tls_free_config(tls_config);
// }

// bool TLSWrapper::test_mbed(const char *server, const char *request, const uint8_t *cert, size_t cert_len, int timeout) {
//     int ret;
//     mbedtls_net_context server_fd;
//     mbedtls_ssl_context ssl;
//     mbedtls_ssl_config conf;
//     mbedtls_entropy_context entropy;
//     mbedtls_ctr_drbg_context ctr_drbg;
//     mbedtls_x509_crt ca_cert;

//     const char *pers = "ssl_client";

//     mbedtls_net_init(&server_fd);
//     mbedtls_ssl_init(&ssl);
//     mbedtls_ssl_config_init(&conf);
//     mbedtls_entropy_init(&entropy);
//     mbedtls_ctr_drbg_init(&ctr_drbg);
//     mbedtls_x509_crt_init(&ca_cert);

//     if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers))) != 0) {
//         TLSWRAPPERprintf("Failed to seed RNG: -0x%x\n", -ret);
//         return false;
//     }

//     if ((ret = mbedtls_x509_crt_parse(&ca_cert, cert, cert_len)) < 0) {
//         TLSWRAPPERprintf("Failed to parse CA certificate: -0x%x\n", -ret);
//         return false;
//     }

//     if ((ret = mbedtls_net_connect(&server_fd, server, "443", MBEDTLS_NET_PROTO_TCP)) != 0) {
//         printf("Failed to connect to server: -0x%x\n", -ret);
//         TLSWRAPPERprintf false;
//     }

//     if ((ret = mbedtls_ssl_config_defaults(&conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT)) != 0) {
//         TLSWRAPPERprintf("Failed to setup SSL config: -0x%x\n", -ret);
//         return false;
//     }

//     mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
//     mbedtls_ssl_conf_ca_chain(&conf, &ca_cert, NULL);
//     mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

//     if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0) {
//         TLSWRAPPERprintf("Failed to setup SSL context: -0x%x\n", -ret);
//         return false;
//     }

//     if ((ret = mbedtls_ssl_set_hostname(&ssl, server)) != 0) {
//         TLSWRAPPERprintf("Failed to set hostname: -0x%x\n", -ret);
//         return false;
//     }

//     mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

//     while ((ret = mbedtls_ssl_handshake(&ssl)) != 0) {
//         if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) {
//             TLSWRAPPERprintf("Failed SSL handshake: -0x%x\n", -ret);
//             return false;
//         }
//     }

//     if ((ret = mbedtls_ssl_write(&ssl, (const unsigned char *)request, strlen(request))) <= 0) {
//         TLSWRAPPERprintf("Failed to send request: -0x%x\n", -ret);
//         return false;
//     }

//     unsigned char buffer[4096];
//     memset(buffer, 0, sizeof(buffer));

//     do {
//         ret = mbedtls_ssl_read(&ssl, buffer, sizeof(buffer) - 1);
//         if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
//             continue;
//         } else if (ret < 0) {
//             TLSWRAPPERprintf("Failed to read response: -0x%x\n", -ret);
//             return false;
//         } else if (ret == 0) {
//             TLSWRAPPERprintf("Connection closed\n");
//             break;
//         }

//         TLSWRAPPERprintf("Response: %s\n", buffer);
//     } while (ret > 0);

//     mbedtls_ssl_close_notify(&ssl);
//     mbedtls_net_free(&server_fd);
//     mbedtls_ssl_free(&ssl);
//     mbedtls_ssl_config_free(&conf);
//     mbedtls_ctr_drbg_free(&ctr_drbg);
//     mbedtls_entropy_free(&entropy);
//     mbedtls_x509_crt_free(&ca_cert);

//     return true;
// }
