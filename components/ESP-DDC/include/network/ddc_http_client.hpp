
#pragma once

/* ESP-IDF Components */
#include "esp_log.h"
#include "esp_http_client.h"

/* C/C++ Libraries */
#include <string>

/* one-time client */
/* - created & destroyed in func for one time purpose */
/* - return response body got from http server*/

/* ESP-LOG */
static const char *HTTP_TAG = "HTTP";

/* Persistent client */
class HttpClient
{
    public:
    HttpClient
    (
        const char *url,
        uint16_t timeout_ms = DEFAULT_TIMEOUT_MS,
        esp_http_client_method_t method = DEFAULT_METHOD
    )
    {
        cfg.url        = url;
        cfg.timeout_ms = timeout_ms;
        cfg.method     = method;
        client = esp_http_client_init(&cfg);
    }
    ~HttpClient()
    {
        if (client) 
        {
            esp_http_client_cleanup(client);
            client = nullptr;
        }
    }

     struct Msg
    {
        std::string body;
        int status_code = 0;
    };

    /* HTTP Client Get with no auto perform */
    Msg Get_ManualPerform()
    {
        Msg msg;
        esp_err_t err = esp_http_client_open(client, 0);
        if (err == ESP_OK)
        {
            msg.status_code = esp_http_client_get_status_code(client);
            
            // fetch headers
            int content_length = esp_http_client_fetch_headers(client);
            if (content_length < 0) {
                content_length =  1024; 
            }
                        
            int total_read_len = 0;
            int read_len = 0;
            char buffer[512];
            while ((read_len = esp_http_client_read(client, buffer, sizeof(buffer))) > 0) {
                msg.body.append(buffer, read_len);
                total_read_len += read_len;
            }
            msg.status_code = esp_http_client_get_status_code(client); // re-read status code just in case
            
            esp_http_client_close(client);
        }
        else
        {
            ESP_LOGE(HTTP_TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
        }

        return msg;
    }

    private:
    static constexpr uint16_t DEFAULT_TIMEOUT_MS = 5000;
    static constexpr esp_http_client_method_t DEFAULT_METHOD = HTTP_METHOD_GET;
    esp_http_client_handle_t client;
    esp_http_client_config_t cfg = {};
};