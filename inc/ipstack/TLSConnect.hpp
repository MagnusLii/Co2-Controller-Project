#ifndef TLSCONNECT_H
#define TLSCONNECT_H

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/altcp_tcp.h"
#include "lwip/altcp_tls.h"
#include "lwip/dns.h"

#include "FreeRTOS.h"
#include "task.h"

#include <string>
#include <iostream>
#include <sstream>

class TLSConnect {
    public:
        TLSConnect(const char *SSID, const char *PASSWD, const char *hostname, const char *apikey, const char *cert, size_t cert_len);
        std::string get_hostname(void);
    private:
        std::string hostname;
        std::string apikey;
        altcp_tls_config *tls_config;
        static void send_task(void *pvParameters);
};

#endif