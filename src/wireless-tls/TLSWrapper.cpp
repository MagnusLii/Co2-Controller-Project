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
#include <iostream>
#include <array>

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

TLSWrapper::TLSWrapper(const std::string& ssid, const std::string& password, uint32_t countryCode)
    :
     ssid(ssid),
     password(password),
     countryCode(countryCode),
     mutex(){
        reading_queue = xQueueCreate(20, sizeof(Reading));
        sensor_data_queue = xQueueCreate(5, sizeof(Message));
        response_queue = xQueueCreate(5, sizeof(Message));

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



        // TODO: VERIFY STACK SIZES
        xTaskCreate(process_and_send_sensor_data_task, "Process sensor data task", 512, this, TaskPriority::LOW, nullptr);
        // xTaskCreate(send_field_update_request_task, "Update fields task", 10240, this, TaskPriority::LOW, nullptr); // 512 did not work.
        xTaskCreate(get_server_commands_task, "Request commands task", 11264, this, TaskPriority::LOW, nullptr); // 512 did not work.
        xTaskCreate(parse_server_commands_task, "Command parser task", 1024, this, TaskPriority::HIGH, nullptr);
        // xTaskCreate(reconnect_task, "Reconnect task", 512, this, TaskPriority::ABSOLUTE, &reconnect_task_handle);
}

TLSWrapper::~TLSWrapper() {
    // nothing.
}

void TLSWrapper::send_request(const std::string& endpoint, const std::string& request){
    TLSWRAPPERprintf("TLSWrapper::send_request: %s\n", request.c_str());
    mutex.lock();
    send_tls_request(endpoint.c_str(), request.c_str(), CONNECTION_TIMEOUT_MS);
    mutex.unlock();
}

void TLSWrapper::send_request_and_get_response(const std::string& endpoint, const std::string& request){
    Message receivedMsg;
    char response[MAX_BUFFER_SIZE];
    size_t response_len = 0;

    mutex.lock();
    TLSWRAPPERprintf("TLSWrapper::send_request_and_get_response: %s\n", request.c_str());
    send_tls_request_and_get_response(endpoint.c_str(), request.c_str(), CONNECTION_TIMEOUT_MS, response, &response_len);
    mutex.unlock();
    TLSWRAPPERprintf("TLSWrapper::response from server: %s\n %d", response, response_len);
    
    receivedMsg.data = std::string(response);
    xQueueSend(response_queue, &receivedMsg, 0);
}

void TLSWrapper::create_field_update_request(Message &messageContainer, const std::array<Reading, 8> &values){
    messageContainer.data = ""; // Clear message container
    
    messageContainer.data += "GET /update?api_key=";
    messageContainer.data += API_KEY;

    for (int i = 0; i < 5; i++) {
        messageContainer.data += FIELD_NAMES[i];
        char buffer[69];
        if (values[i].type == ReadingType::FAN_SPEED) {
            snprintf(buffer, sizeof(buffer), "%d", values[i].value.u16);
        } else {
            snprintf(buffer, sizeof(buffer), "%.2f", values[i].value.f32);
        }
        messageContainer.data += buffer;
    }

    messageContainer.data += " HTTP/1.1\r\n";
    messageContainer.data += "Host: ";
    messageContainer.data += THINGSPEAK_HOSTNAME;
    messageContainer.data += "\r\n";
    messageContainer.data += "Connection: close\r\n";
    messageContainer.data += "\r\n";

    return;
}

void TLSWrapper::create_command_request(Message &messageContainer){
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

void TLSWrapper::set_screen_write_handle(QueueHandle_t queue) {
    screen_writing_queue = queue;
}

QueueHandle_t TLSWrapper::get_read_handle(void) {
    return reading_queue;
}

void TLSWrapper::process_and_send_sensor_data_task_() {
    Message msg;
    Reading reading;

    std::array<Reading, 8> values;

    // Init zero values
    for (auto value : values){
        value = {ReadingType::UNSET, {0}};
    }

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(READING_SEND_INTERVAL));

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
            // TLSWRAPPERprintf("Writing to queue");
            xQueueSend(sensor_data_queue, &msg, 0);
        }
    }
}

// void TLSWrapper::send_field_update_request_task_(void *asd){
//     Message msg;
//     std::string hostname = THINGSPEAK_HOSTNAME;
//     for(;;){
//         vTaskDelay(1000);
//         if (xQueueReceive(sensor_data_queue, &msg, 0)){
//             // std::lock_guard<Fmutex> exclusive(mutex);
//             // mutex.lock();
//             send_request(hostname, msg.data);
//             // mutex.unlock();
//         }
//     }
// }

void TLSWrapper::get_server_commands_task_(void *param){
    Message commandMsg;
    Message msg;
    std::string hostname = THINGSPEAK_HOSTNAME;
    create_command_request(commandMsg);


    int counter = 0;
    for(;;){
        vTaskDelay(pdMS_TO_TICKS(1000));
        if (xQueueReceive(sensor_data_queue, &msg, 0)){
            send_request(hostname, msg.data);
        } else if (counter >= 5){
            send_request_and_get_response(hostname, commandMsg.data);
            counter = 0;
        }
        counter++;
    }
}

void TLSWrapper::parse_server_commands_task_(void *param){
    Message msg;
    Command modeCommand;
    Command valueCommand;
    std::size_t pos;
    std::size_t num_pos;

    for(;;){
        // vTaskDelay(5000);
        if (xQueueReceive(response_queue, &msg, portMAX_DELAY)){
            // Parse the response
            std::cout << msg.data << std::endl;
            msg.data = parse_command_from_http(msg.data);
            pos = msg.data.find(" ");
            num_pos = msg.data.find_first_of("0123456789", pos);

            if (msg.data.length() > 0) {
                if (msg.data.starts_with("fan")){
                    modeCommand.type = WriteType::MODE_SET;
                    modeCommand.value.u16 = 1;

                    valueCommand.type = WriteType::FAN_SPEED;
                    valueCommand.value.u16 = (uint16_t)std::stoi(msg.data.substr(num_pos));

                    xQueueSend(writing_queue, &modeCommand, 0);
                    xQueueSend(writing_queue, &valueCommand, 0);
                    xQueueSend(screen_writing_queue, &modeCommand, 0);
                    xQueueSend(screen_writing_queue, &valueCommand, 0);

                }
                else if (msg.data.starts_with("co2")){
                    modeCommand.type = WriteType::MODE_SET;
                    modeCommand.value.u16 = 0;

                    valueCommand.type = WriteType::CO2_TARGET;
                    valueCommand.value.f32 = std::stof(msg.data.substr(num_pos));

                    xQueueSend(writing_queue, &modeCommand, 0);
                    xQueueSend(writing_queue, &valueCommand, 0);
                    xQueueSend(screen_writing_queue, &modeCommand, 0);
                    xQueueSend(screen_writing_queue, &valueCommand, 0);
                }

            }
        }
    }
}

std::string TLSWrapper::parse_command_from_http(const std::string& http_response) {
    // Find the first '{' and the last '}'
    std::size_t start_pos = http_response.find('{');
    std::size_t end_pos = http_response.rfind('}');
    std::string substr;

    if (start_pos != std::string::npos && end_pos != std::string::npos && start_pos < end_pos) {
        substr = http_response.substr(start_pos, end_pos - start_pos + 1);
    } else {
        return "";
    }

    std::string key = "\"command_string\":\"";
    
    start_pos = substr.find(key);

    if (start_pos != std::string::npos) {
        start_pos += key.length();
        
        std::size_t end_pos = substr.find('"', start_pos);
        
        if (end_pos != std::string::npos) {
            return substr.substr(start_pos, end_pos - start_pos);
        }
    }

    return "";
}

// void TLSWrapper::reconnect_task_(void *param){
//     for (;;) {
//         xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
//         bool connected = false;
//         int retries = 0;
//         do {

//             TLSWRAPPERprintf("TLSWrapper::TLSWrapper: Connecting to wireless\n");
//             connected = cyw43_arch_wifi_connect_timeout_ms(ssid.c_str(), password.c_str(), CYW43_AUTH_WPA2_AES_PSK, CONNECTION_TIMEOUT_MS);
//             if ( connected != 0) {
//                 TLSWRAPPERprintf("TLSWrapper::TLSWrapper: failed to connect\n");
//             } else {
//                 TLSWRAPPERprintf("TLSWrapper::TLSWrapper: connected\n");
//             }
//             retries++;
//         } while (connected != 0 && retries < 5);

//     }
// }
