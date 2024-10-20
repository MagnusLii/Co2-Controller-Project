# Co2-Controller-Project
 Course project for ARM Processors and Embedded Operating Systems TX00EX86-3001


### Debug prints
Additional debug prints can be toggled from `debugPrints.h`.<br>

### Individual macros
Macros for wireless communication are defined in `inc/wireless-tasks/connection_defines.h`<br>
The file needs to be created manually and should contain the following macros:<br>

```cpp
#ifndef CONNECTION_DEFINES_H
#define CONNECTION_DEFINES_H

#include "pico/cyw43_arch.h"

#define API_KEY "XZY"
#define TALK_BACK_API_KEY "XZY"
#define TALKBACK_ID "XZY"
#define WIFI_SSID "XZY"
#define WIFI_PASSWORD "XZY"
#define THINGSPEAK_HOSTNAME "api.thingspeak.com"
#define DEFAULT_COUNTRY_CODE CYW43_COUNTRY_FINLAND
#define API_REQUEST_INTERVAL 3000
#define TLS_CERTIFICATE "-----BEGIN CERTIFICATE-----\n\
.....\
-----END CERTIFICATE-----\n"


#endif
```
**Note** you also need to include cyw43_arch.h if using cyw macros for country code.<br>