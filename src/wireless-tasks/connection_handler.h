#ifndef CONNECTION_HANDLER_H
#define CONNECTION_HANDLER_H

#include <string>
#include "IPStack.h"
#include <vector>

enum ApiFields {
    FIELD1,
    FIELD2,
    FIELD3,
    FIELD4,
    FIELD5,
    FIELD6,
    FIELD7,
    FIELD8
};

extern const char* FIELD_NAMES[];

class ConnectionHandler {
public:
    explicit ConnectionHandler();
    void initializeIPStack();
    bool connect(const std::string& hostname, int port);
    int send(const std::vector<unsigned char>& data);
    std::vector<unsigned char> receive(int length, int timeout_ms);
    bool disconnect();
    bool isConnected();
    bool isIPStackInitialized();

private:
    std::unique_ptr<IPStack> ipStack;
};

#endif
