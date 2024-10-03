
#include "TLSConnect.hpp"

extern "C" {
extern bool tls_send(const char *request, const char *hostname);
}

TLSConnect::TLSConnect(const char *SSID, const char *PASSWD, const char *hostname, const char *apikey, const char *cert, size_t cert_len)
: hostname(hostname), apikey(apikey) {
    if (cyw43_arch_init()) 
        printf("failed to initialise\n");

    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(SSID, PASSWD, CYW43_AUTH_WPA2_AES_PSK, 30000))
        printf("failed to connect\n");

    tls_config = altcp_tls_create_config_client((uint8_t*)cert, cert_len);
    assert(tls_config);
    mbedtls_ssl_conf_authmode((mbedtls_ssl_config *)tls_config, MBEDTLS_SSL_VERIFY_REQUIRED);
    sleep_ms(10000);

    xTaskCreate(TLSConnect::send_task, "send", 500, this, tskIDLE_PRIORITY + 1, NULL);
}

std::string TLSConnect::get_hostname(void) {
    return hostname;
}

void TLSConnect::send_task(void *pvParameters) {
    auto connection = static_cast<TLSConnect*>(pvParameters);
    std::stringstream stream;
    stream << "GET /update?api_key=";
    stream << connection->apikey;
    stream << "&field1=0 HTTP/1.1\r\n";
    stream << "Host: ";
    stream << connection->get_hostname();
    stream << "Connection: close\r\n";
    stream << "\r\n";
    std::cout << stream.str() << std::endl;
    tls_send(stream.str().c_str(), connection->get_hostname().c_str());
    while (true) {
        vTaskDelay(500);
    }
}