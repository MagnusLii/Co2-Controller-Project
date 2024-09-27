#include "IPStack.h"
#include "connection_handler.h"
#include <cstring>
#include "connection_defines.c"
#include "api_tasks.h"
#include <cstdarg>

#define BUFFER_SIZE 1024
#define TIMEOUT_MS 5000

// const char* FIELD_NAMES[] = {
//     "",
//     "&field1=",
//     "&field2=",
//     "&field3=",
//     "&field4=",
//     "&field5=",
//     "&field6=",
//     "&field7=",
//     "&field8=",
//     "&datetime="
// };


ConnectionHandler::ConnectionHandler() = default;


void ConnectionHandler::initializeIPStack() {
    bool initialized = false;
    while (!initialized) {
        ipStack = std::make_unique<IPStack>(WIFI_SSID, WIFI_PASSWORD);
        
        if (!(initialized = ipStack->isInitialized())) {
            ipStack->disconnect();
            cyw43_arch_deinit();
        }
    }
}


bool ConnectionHandler::connect(const std::string& hostname, int port) {
    if (ipStack->isConnected()) {
        return true;
    }

    int status = ipStack->connect(hostname.c_str(), port);
    printf("Connection status: %d\n", status);
    return status;
}


int ConnectionHandler::send(const std::vector<unsigned char>& data) {
    if (!ipStack->isConnected()) {
        return -1;  // Not connected
    }

    return ipStack->write(const_cast<unsigned char*>(data.data()), data.size(), TIMEOUT_MS);
}


std::vector<unsigned char> ConnectionHandler::receive(int length, int timeout_ms) {
    std::vector<unsigned char> buffer(length);

    if (!ipStack->isConnected()) {
        return {};
    }

    int bytesRead = ipStack->read(buffer.data(), length, timeout_ms);
    if (bytesRead > 0) {
        buffer.resize(bytesRead);
    } else {
        buffer.clear();  // Clear buffer if no data was read
    }

    return buffer;
}

// Returns status of IPStack after disconnect attempt.
bool ConnectionHandler::disconnect() {
    if (ipStack->isConnected()) {
        int result = ipStack->disconnect();
        if (result == ERR_OK) {
            return ipStack->isConnected();
        } else {
            return ipStack->isConnected();
        }
    }
    return ipStack->isConnected();
}


bool ConnectionHandler::isConnected() {
    return ipStack->isConnected();
}


bool ConnectionHandler::isIPStackInitialized() {
    return ipStack->isInitialized();
}


// bool ConnectionHandler::updateFieldsMessage(const std::map<std::string, int> &fields) {
//     std::string messageString;

//     for (const auto& [key, value] : fields) {
//         messageString += FIELD_NAMES[key] + std::to_string(value);
//     }


// }


// // First argument is field value, second is field number from enum ApiFields, final argument must be nullptr to terminate the list.
// // All fields except POST_AND_URL are optional.
// // If you don't follow the rules this will crash and you will get no supper!
// void ConnectionHandler::pushMessageToQueue(const void* field, ...) {

//     Message msg;

//     std::string messageString;                  

//     va_list args;
//     va_start(args, field);

//     while (field != nullptr) {

//     }

//     va_end(args);

//     msg.data = std::vector<unsigned char>(messageString.begin(), messageString.end());
//     msg.length = messageString.length();

//     DEBUG_printf("ConnectionHandler::pushMessageToQueue:\n\n----------------------------------\n%s\n", messageString.c_str());
//     DEBUG_printf("ConnectionHandler::pushMessageToQueue: %d\n---------------------------------\n\n", msg.length);

//     if (xQueueSend(sendQueue, &msg, QUEUE_TIMEOUT) != pdPASS) {
//         DEBUG_printf("ConnectionHandler::pushMessageToQueue: Failed to send message to queue\n");
//     }
// }
