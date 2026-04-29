
#pragma once

#include <string>
#include <esp_log.h>
#include <esp_http_client.h>

/* one-time client */
/* - created & destroyed in func for one time purpose */
/* - return response body got from http server*/

 static const char *HTTP_TAG = "HTTP";

inline std::string HttpClient_FNF_Get(const char *url)
{
    std::string body; 

    /* Temp http client */
    esp_http_client_config_t cfg = {}; // does not concern any other configs
                                       // other than:
    cfg.url        = url;
    cfg.timeout_ms = 5000;
    
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    esp_err_t err = esp_http_client_open(client, 0);

    if(err == ESP_OK)
    {
        esp_http_client_fetch_headers(client);
        
        int read_len = 0;
        char buffer[512];
        while ((read_len = esp_http_client_read(client, buffer, sizeof(buffer))) > 0) {
            body.append(buffer, read_len);
        }
    }
    else
    {
        ESP_LOGE(HTTP_TAG, "Error occurred while opening HTTP connection: %s", esp_err_to_name(err));
    }

    /* Kill client */
    esp_http_client_cleanup(client);
    return body;
}

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

    Msg Get()
    {
        Msg msg;
        esp_err_t err = esp_http_client_open(client, 0);
        if (err == ESP_OK)
        {
            msg.status_code = esp_http_client_get_status_code(client);
            
            // fetch headers
            int content_length = esp_http_client_fetch_headers(client);
            if (content_length < 0) {
                // Chunked encoding or unknown length
                content_length =  1024; // start with a small buffer, but we don't know the full length
            }
            
            // Wait, actually the easiest way without event handler is just esp_http_client_read in a loop.
            // Wait, if it's chunked, esp_http_client_read still works.
            
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