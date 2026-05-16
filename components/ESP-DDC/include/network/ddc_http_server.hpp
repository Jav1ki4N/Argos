#pragma once

/* ESP-IDF Components */
#include "esp_http_server.h"
#include "esp_log.h"

/* C/C++ Libraries */
#include <cstring>
#include <string>

class HttpServer
{
public:
    HttpServer();
    ~HttpServer();

    void start();
    void stop();

private:
    static constexpr const char *TAG = "HTTP_SERVER";

    // [1] httpd_handle_t server_ = nullptr;
    // [2] static esp_err_t root_get_handler(httpd_req_t *req);
    // [3] static esp_err_t http_404_handler(httpd_req_t *req, httpd_err_code_t err);
    // [4] std::string load_portal_html();   // 从 LittleFS 读取
};