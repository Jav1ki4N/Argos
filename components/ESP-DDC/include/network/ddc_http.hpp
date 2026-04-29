/*===================================================*/
/*      ____  ____  ____      ____  ____   ___       */
/*     (  __)/ ___)(  _ \ ___(    \(    \ / __)      */
/*      ) _) \___ \ ) __/(___)) D ( ) D (( (__       */
/*     (____)(____/(__)      (____/(____/ \___)      */
/*===================================================*/
/*         i4n@2026 | ddc_http | 2026-4-29          */
/*         Minimal HTTP client wrapper              */
/*===================================================*/
/* Purpose: Send HTTP GET/POST and read response.   */
/* Usage:   ddc_http_get("http://...") or            */
/*          ddc_http_post("http://...", body, type)  */
/*===================================================*/
#pragma once

#include "esp_log.h"
#include "esp_http_client.h"
#include <string>

static const char *HTTP_TAG = "HTTP";

/* HTTP Client — Fire & Forget */
/* - minimal http get that launched by a local temp client      */
/* - any resource will be cleaned up when operation is complete */
/* - not suitable for long-running or frequent operations       */
inline std::string ddc_http_get_fnf(const char *url)
{
    std::string body;
    /* Http client settings, for GET operation */
    /* - no default config is provided */
    esp_http_client_config_t config = {};

    /* set url to request                              */
    /* - can be set via format: .host + .path + .query */
    /* - or simply .url                                */
    config.url = url;
    config.timeout_ms = 5000;

    /* Handle */
    /* - Client is made local that each client instance only do one request */
    /* - Fire & forget */
    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_err_t ret = esp_http_client_perform(client);

    if (ret == ESP_OK) {
        int status = esp_http_client_get_status_code   (client);
        int len    = esp_http_client_get_content_length(client);
        ESP_LOGI(HTTP_TAG, "GET %s -> %d, body=%d bytes", url, status, len);

        if (len > 0) {
            body.resize(len + 1);
            esp_http_client_read_response(client, &body[0], len);
            body[len] = '\0';
        }
    } else {
        ESP_LOGE(HTTP_TAG, "GET %s failed: %s", url, esp_err_to_name(ret));
    }

    /* Kill client */
    esp_http_client_cleanup(client);
    return body;
}

/* Minimal POST — returns response body as string, empty on failure */
inline std::string ddc_http_post(const char *url,
                                 const char *data,
                                 const char *content_type = "application/json")
{
    std::string body;

    esp_http_client_config_t config = {};
    config.url = url;
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 5000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    esp_http_client_set_header(client, "Content-Type", content_type);
    esp_http_client_set_post_field(client, data, strlen(data));

    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        int len = esp_http_client_get_content_length(client);
        ESP_LOGI(HTTP_TAG, "POST %s -> %d, body=%d bytes", url, status, len);

        if (len > 0) {
            body.resize(len + 1);
            esp_http_client_read_response(client, &body[0], len);
            body[len] = '\0';
        }
    } else {
        ESP_LOGE(HTTP_TAG, "POST %s failed: %s", url, esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    return body;
}

/*===================================================*/
/* Persistent HTTP GET — reuse one client handle     */
/* Usage:                                            */
/*   DDC_Http http("http://192.168.1.100:8080");     */
/*   std::string r = http.get("/api/info");           */
/*   // ... r = http.get("/api/info"); loop ...       */
/*===================================================*/
class DDC_Http
{
    esp_http_client_handle_t handle = nullptr;
    std::string base_url;

  public:
    DDC_Http(const char *host_url)
    {
        base_url = host_url;
        esp_http_client_config_t cfg = {};
        cfg.url = base_url.c_str();
        cfg.timeout_ms = 5000;
        handle = esp_http_client_init(&cfg);
    }

    ~DDC_Http()
    {
        if (handle) esp_http_client_cleanup(handle);
    }

    std::string get(const char *path)
    {
        std::string body;
        if (!handle) return body;

        std::string full_url = base_url + path;
        esp_http_client_set_url(handle, full_url.c_str());
        esp_http_client_set_method(handle, HTTP_METHOD_GET);

        esp_err_t err = esp_http_client_perform(handle);
        if (err == ESP_OK) {
            int len = esp_http_client_get_content_length(handle);
            ESP_LOGI(HTTP_TAG, "GET %s -> %d, body=%d bytes",
                     full_url.c_str(),
                     esp_http_client_get_status_code(handle), len);
            if (len > 0) {
                body.resize(len + 1);
                esp_http_client_read_response(handle, &body[0], len);
                body[len] = '\0';
            }
        } else {
            ESP_LOGE(HTTP_TAG, "GET %s failed: %s", full_url.c_str(), esp_err_to_name(err));
        }
        return body;
    }
};
