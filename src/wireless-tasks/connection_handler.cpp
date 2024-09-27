#include "IPStack.h"
#include "connection_handler.h"
#include <cstring>
#include "connection_defines.c"

#define BUFFER_SIZE 1024
#define TIMEOUT_MS 5000

const char* FIELD_NAMES[] = {
    "field 1",
    "field 2",
    "field 3",
    "field 4",
    "field 5",
    "field 6",
    "field 7",
    "field 8"
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

    // Receive data
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

// This method now belongs to the class and is properly marked as const
bool ConnectionHandler::isIPStackInitialized() {
    return ipStack->isInitialized();
}
