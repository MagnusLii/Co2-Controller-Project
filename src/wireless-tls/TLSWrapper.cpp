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
        reading_queue = xQueueCreate(20, sizeof(Reading));
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

void TLSWrapper::empty_response_buffer(QueueHandle_t queue_where_to_store_msg) {  
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

void TLSWrapper::create_field_update_request(Message &messageContainer, const float values[]){
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

void TLSWrapper::create_command_request(Message &messageContainer, const char* command){
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

void TLSWrapper::set_write_handle(QueueHandle_t queue) {
    writing_queue = queue;
}

QueueHandle_t TLSWrapper::get_read_handle(void) {
    return reading_queue;
}
