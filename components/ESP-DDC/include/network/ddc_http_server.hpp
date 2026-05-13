
#pragma once

#include <sys/param.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "lwip/inet.h"

#include "esp_http_server.h"

static const char *TAG = "HTTP_SERVER";

class HttpServer
{
    public:
    HttpServer();
    ~HttpServer();

    private:
};