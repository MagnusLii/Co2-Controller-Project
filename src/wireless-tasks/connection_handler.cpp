#include "IPStack.h"
#include "connection_handler.h"
#include <cstring>
#include "connection_defines.h"
#include "api_tasks.h"
#include <cstdarg>

#define BUFFER_SIZE 1024
#define TIMEOUT_MS 5000

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


ConnectionHandler::ConnectionHandler() = default;


// void ConnectionHandler::initializeIPStack() {
//     bool initialized = false;
//     while (!initialized) {
//         ipStack = std::make_unique<IPStack>(WIFI_SSID, WIFI_PASSWORD);
//         if (!(initialized = ipStack->isInitialized())) {
//             DEBUG_printf("Deinit IPStack\n");
//             ipStack->disconnect();
//             cyw43_arch_deinit();
//         }
//     }
// }

void ConnectionHandler::initializeIPStack(){
    ipStack = std::make_unique<IPStack>(WIFI_SSID, WIFI_PASSWORD);

    // TODO: nothing seems to be working, cyw43_arch_init() creates two tasks, which aren't killed by cyw43_arch_deinit(), causing the system to lock up.
    // No idea how to reinit a failed IPStack.
    while (!ipStack->isInitialized()) {
        DEBUG_printf("ConnectionHandler::initializeIPStack(): IPStack not initialized\nReboot the system\n");
        cyw43_arch_deinit();
        vTaskDelay(1);
        ipStack.reset();
        ipStack = std::make_unique<IPStack>(WIFI_SSID, WIFI_PASSWORD);
    }
}


bool ConnectionHandler::connect(const std::string& hostname, int port) {
    if (ipStack->isConnected()) {
        return true;
    }

    int status = ipStack->connect(hostname.c_str(), port);
    DEBUG_printf("ConnectionHandler::connect(): Connection status: %d\n", status);
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
    DEBUG_printf("ConnectionHandler::isConnected: %d\n", ipStack->isConnected());
    return ipStack->isConnected();
}


bool ConnectionHandler::isIPStackInitialized() {
    DEBUG_printf("ConnectionHandler::isIPStackInitialized: %d\n", ipStack->isInitialized());
    return ipStack->isInitialized();
}


// Returns len of the message.
int ConnectionHandler::create_minimal_push_message(std::vector<unsigned char>& messageContainer, const float values[]) {
    messageContainer.clear();
    
    std::string message;
    message += "GET /update?api_key=";
    message += API_KEY;

    for (int i = 0; i <= FIELD8; i++) {
        message += FIELD_NAMES[i];
        char buffer[10];
        snprintf(buffer, sizeof(buffer), "%.2f", values[i]);
        message += buffer;
    }

    message += " HTTP/1.1\r\n";
    message += "Host: ";
    message += THINGSPEAK_HOSTNAME;
    message += "\r\n";
    message += "\r\n";

    messageContainer.insert(messageContainer.end(), message.begin(), message.end());
    return message.size();
}
