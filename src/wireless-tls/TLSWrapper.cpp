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
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"
#include "connection_defines.h"
#include "debugPrints.h"
#include <algorithm>
#include "task_defines.h"

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
    //TLS_CLIENT_T* tls_client;
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


        bool connected = false;
        int retries = 0;
        do {

            TLSWRAPPERprintf("TLSWrapper::TLSWrapper: Connecting to wireless\n");
            connected = cyw43_arch_wifi_connect_timeout_ms(ssid.c_str(), password.c_str(), CYW43_AUTH_WPA2_AES_PSK, CONNECTION_TIMEOUT_MS);
            if ( connected != 0) {
                TLSWRAPPERprintf("TLSWrapper::TLSWrapper: failed to connect\n");
            } else {
                TLSWRAPPERprintf("TLSWrapper::TLSWrapper: connected\n");
            }
            retries++;
        } while (connected != 0 && retries < 5);
        reading_queue = xQueueCreate(20, sizeof(Reading));

        TLSWrapper_private_queue = xQueueCreate(5, sizeof(Message));

        xTaskCreate(api_call_creator, "Send task", 1024, this, TaskPriority::LOW, nullptr);
        xTaskCreate(testtask, "test", 256, this, TaskPriority::LOW, nullptr);
}

TLSWrapper::~TLSWrapper() {
    // nothing.
}

void TLSWrapper::send_request(const std::string& endpoint, const std::string& request){
    TLSWRAPPERprintf("TLSWrapper::send_request: %s\n", request.c_str());
    send_tls_request(endpoint.c_str(), request.c_str(), CONNECTION_TIMEOUT_MS);
}

void TLSWrapper::send_request_and_get_response(const std::string& endpoint, const std::string& request){

    char response[1024];
    size_t response_len = 0;

    TLSWRAPPERprintf("TLSWrapper::send_request: %s\n", request.c_str());
    send_tls_request_and_get_response(endpoint.c_str(), request.c_str(), CONNECTION_TIMEOUT_MS, response, &response_len);

    TLSWRAPPERprintf("TLSWrapper::response: %s\n %d", response, response_len);
}

void TLSWrapper::create_field_update_request(Message &messageContainer, const std::array<Reading, 8> &values){
    messageContainer.data = ""; // Clear message container
    
    messageContainer.data += "GET /update?api_key=";
    messageContainer.data += API_KEY;

    for (int i = 0; i < (int)ReadingType::end; i++) {
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

    return;
}

void TLSWrapper::set_write_handle(QueueHandle_t queue) {
    writing_queue = queue;
}

QueueHandle_t TLSWrapper::get_read_handle(void) {
    return reading_queue;
}

void TLSWrapper::api_call_creator_() {
    Message msg;
    Reading reading;

    std::array<Reading, 8> values;

    for (auto value : values){
        value = {ReadingType::UNSET, {0}};
    }

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(100));

        while (xQueueReceive(reading_queue, &reading, 0) == pdPASS) {
            switch (reading.type) {
                case ReadingType::CO2:
                    values[(int)ReadingType::CO2 - 1] = reading;
                    break;
                case ReadingType::CO2_TARGET:
                    values[(int)ReadingType::CO2_TARGET - 1] = reading;
                    break;
                case ReadingType::FAN_SPEED:
                    values[(int)ReadingType::FAN_SPEED - 1] = reading;
                    break;
                case ReadingType::REL_HUMIDITY:
                    values[(int)ReadingType::REL_HUMIDITY - 1] = reading;
                    break;
                case ReadingType::TEMPERATURE:
                    values[(int)ReadingType::TEMPERATURE - 1] = reading;
                    break;
                default:
                    break;
            }
        }

        create_field_update_request(msg, values);

        if (msg.data.length() > 0) {
            TLSWRAPPERprintf("Writing to queue");
            xQueueSend(TLSWrapper_private_queue, &msg, 0);
        }
    }
}

void TLSWrapper::testtask_(void *asd){
    Message msg;
    for(;;){
        vTaskDelay(1000);
        if (xQueueReceive(TLSWrapper_private_queue, &msg, 0)){
            send_request(THINGSPEAK_HOSTNAME, msg.data);
        }
    }
}