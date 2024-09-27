#include "IPStack.h"
#include "connection_handler.h"
#include <cstring>
#include "connection_defines.c"
#include "api_tasks.h"

#define BUFFER_SIZE 1024
#define TIMEOUT_MS 5000

const char* FIELD_NAMES[] = {
    "field1",
    "field2",
    "field3",
    "field4",
    "field5",
    "field6",
    "field7",
    "field8"
};


ConnectionHandler::ConnectionHandler() = default;


void ConnectionHandler::initializeIPStack() {
    ipStack = std::make_unique<IPStack>(WIFI_SSID, WIFI_PASSWORD);
    if (!ipStack->isInitialized()) {
        ipStack.reset();
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


bool ConnectionHandler::disconnect() {
    if (ipStack->isConnected()) {
        int result = ipStack->disconnect();
        if (result == ERR_OK) {
            return ipStack->isConnected();
        } else {
            return ipStack->isConnected();
        }
    }
}


bool ConnectionHandler::isConnected() {
    return ipStack->isConnected();
}


bool ConnectionHandler::isIPStackInitialized() {
    return ipStack->isInitialized();
}


// Not tested.
void ConnnectionHandler::pushMessageToQueue(const char* field, ...) {
    Message msg;

    std::string messageStart = "POST https://api.thingspeak.com/update\r\n"
                                "api_key=" API_KEY "\r\n";
                                

    va_list args;
    va_start(args, field);

    while (field != nullptr) {
        std::string fieldName = FIELD_NAMES[field];
        std::string fieldValue = va_arg(args, const char*);
        messageStart += fieldName + "=" + fieldValue + "\r\n";
        field = va_arg(args, const char*);
    }

    messageStart += "talkback_key=" TALK_BACK_API_KEY;

    va_end(args);

    msg.data = std::vector<unsigned char>(messageStart.begin(), messageStart.end());
    msg.length = messageStart.lenght();

    xQueueSend(sendQueue, &msg, QUEUE_TIMEOUT);
}
