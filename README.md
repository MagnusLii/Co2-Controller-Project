# Co2-Controller-Project
 Course project for ARM Processors and Embedded Operating Systems TX00EX86-3001


### Individual macros
Macros for wireless communication are defined in `inc/wireless-tasks/connection_defines.h`<br>
The file needs to be created manually and should contain the following macros:<br>
```cpp
#ifndef CONNECTION_DEFINES_H
#define CONNECTION_DEFINES_H

#include "pico/cyw43_arch.h"

#define API_KEY "XXX"
#define TALK_BACK_API_KEY "XXX"
#define TALKBACK_ID "XXX"
#define WIFI_SSID "XXX"
#define WIFI_PASSWORD "XXX"
#define THINGSPEAK_IP "3.224.58.169"
#define THINGSPEAK_HOSTNAME "api.thingspeak.com"
#define THINGSPEAK_PORT 80
#define DEFAULT_COUNTRY_CODE CYW43_COUNTRY_XXX
#define TLS_CERTIFICATE "XXX..."

#endif
```
