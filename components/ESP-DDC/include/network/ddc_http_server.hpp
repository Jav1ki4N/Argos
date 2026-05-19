#pragma once

/* ESP-IDF Components */
#include "esp_http_server.h"
#include "esp_log.h"

/* DDC Headers */
#include "../thirdparty/ddc_littlefs.hpp"

/* C/C++ Libraries */
#include <cstring>
#include <string>
#include <vector>

class HttpServer
{
public:
    enum class Mode:uint8_t
    {
        Normal,
        CaptivePortal
    };

    enum class FileSys:uint8_t
    {
        Embedded,
        LittleFS
    };

    HttpServer(Mode mode = Mode::Normal,
               FileSys fs = FileSys::Embedded,
               httpd_config_t config = DEFAULT_CONFIG)
    : _config(config), _mode(mode), _fs(fs)
    {
        esp_log_level_set("httpd_uri", ESP_LOG_ERROR);
        esp_log_level_set("httpd_txrx", ESP_LOG_ERROR);
        esp_log_level_set("httpd_parse", ESP_LOG_ERROR);
        if(mode == Mode::CaptivePortal) _config = Make_CPConfig();
    }
    ~HttpServer() { stop(); }

    void start()
    {        
        /* Start Server */
        ESP_LOGI(TAG, "Starting server on port: '%d'", _config.server_port);
        if (httpd_start(&server, &_config) == ESP_OK)
        {
            root = {};
            root.uri = "/"; // this is not the path of file in file sys
                            // but the URL path to be requested by client
            root.method = HTTP_GET;
            root.handler = static_root_get_handler;
            root.user_ctx = this; // pass the instance pointer to the handler via user_ctx
            _instance = this;
            httpd_register_uri_handler(server, &root);
            httpd_register_err_handler(server, 
                                       HTTPD_404_NOT_FOUND, 
                                       static_http_404_error_handler);
            if(_mode == Mode::CaptivePortal)
            {
                generate_204 = {};
                generate_204.uri = "/generate_204";
                generate_204.method = HTTP_GET;
                generate_204.handler = static_root_get_handler; // serve the same captive portal HTML
                generate_204.user_ctx = this;
                httpd_register_uri_handler(server, &generate_204);
            }
            /* Register pending custom URI handlers */
            for (auto& h : _uri_handlers)
                httpd_register_uri_handler(server, &h);
        }
    };
    void stop()
    {
        if (server != nullptr) {
            httpd_stop(server);
            server = nullptr;
        }
    }

    void registerLittleFS(LFS* lfs)
    {
        _lfs = lfs;
    }

    void makeRootDir()
    {
        if(_fs == FileSys::LittleFS)
        {
            if(_lfs == nullptr)
            {
                ESP_LOGE(TAG, "LittleFS instance is null");
                return;
            }
            _lfs->mkdir("/web");
        }
    }

    //  Func    save_web
    /// @brief  Write a web asset to the web/ subdirectory (e.g. portal.html, style.css)
    /// @note   Convenience wrapper — prepends "web/" to the filename.
    ///         Usage:  lfs.save_web("portal.html", html_data, html_len);
    ///                 → writes to /lfs/web/portal.html
    ///         Works for any file type: .html, .css, .js, .png, etc.
    esp_err_t saveWeb(const char* filename, const void* data, size_t len)
    {
        std::string path = std::string("/web/") + filename;
        return _lfs->write(path.c_str(), data, len);
    }
    
    //  Func   registerURI
    /// @brief  Register a custom URI handler.
    /// @param  uri the URI path, e.g. /save
    /// @param  method the HTTP method
    /// @param  handler the handler function
    /// @param  user_ctx the user context to pass to the handler
    /// @note   May be called before or after start(). Pending handlers are
    ///         registered when start() is called.
    esp_err_t registerURI(const char* uri, 
                          httpd_method_t method,
                          esp_err_t (*handler)(httpd_req_t*), 
                          void* user_ctx = nullptr)
    {
        httpd_uri_t h = {};
        h.uri      = uri;
        h.method   = method;
        h.handler  = handler;
        h.user_ctx = user_ctx;
        _uri_handlers.push_back(h);

        if (server != nullptr)
            return httpd_register_uri_handler(server, &_uri_handlers.back());
        return ESP_OK;
    }

    static constexpr const char* ROOT_NAME = "root.html";

private:
    static constexpr const char *TAG = "HTTP_SERVER";
    static constexpr const httpd_config_t DEFAULT_CONFIG = HTTPD_DEFAULT_CONFIG();
  
    static constexpr const char* ROOT_PATH = "/web/root.html"; // /lfs/web/root.html in LittleFS

    httpd_config_t _config;
    Mode _mode;
    FileSys _fs;
    LFS* _lfs;
    httpd_handle_t server = nullptr;

    //  Var HTTP URIs
    /// @brief in class URI and custom URI
    /// @param root the root page, accessed by GET
    /// @param _uri_handlers vector of custom URI handlers registered by the user 
    std::vector<httpd_uri_t> _uri_handlers;
    httpd_uri_t root;
    httpd_uri_t generate_204; // for Android captive portal detection
    
    inline static HttpServer* _instance = nullptr;

    httpd_config_t Make_CPConfig()
    {
        httpd_config_t CPConfig = DEFAULT_CONFIG;
        CPConfig.max_open_sockets = 4;
        CPConfig.lru_purge_enable = true;
        return CPConfig;
    }
   
    //  Func   root_get_handler
    /// @brief If a GET request is made to the root URL this func will be called
    /// @note  This is the non-static version handler which is not directly called
    ///        because a C library can't tell the exact instance to call
    esp_err_t root_get_handler(httpd_req_t *req)
    {
        /* [1] Get HTML content from source */
        if(_fs == FileSys::Embedded) {
            // TODO: serve from embedded binary (extern root_start/root_end or similar)
        }
        else if(_fs == FileSys::LittleFS) {
            /* Check if LittleFS is initialized */
            if(_lfs == nullptr)
            {
                ESP_LOGE(TAG, "LittleFS instance is null");
                httpd_resp_send_err(req,
                                    HTTPD_500_INTERNAL_SERVER_ERROR,
                                    "LittleFS not initialized");
                return ESP_FAIL;
            }
            /* Open root HTML file as root */
            FILE* file = _lfs->read(ROOT_PATH);
            if(file == nullptr)
            {
                ESP_LOGE(TAG, "Failed to open root HTML file: %s", ROOT_PATH);
                httpd_resp_send_err(req,
                                    HTTPD_404_NOT_FOUND,
                                    "Root HTML not found");
                return ESP_FAIL;
            }
            /* Set response type */
            httpd_resp_set_type(req, "text/html");
            /* Measure file size */
            fseek(file, 0, SEEK_END);
            size_t file_size = ftell(file);
            fseek(file, 0, SEEK_SET);
            /* Read file content */
            std::string content;
            content.resize(file_size);
            fread(content.data(), 1, file_size, file);
            fclose(file);
            /* Send response */
            httpd_resp_send(req, content.c_str(), content.size());
            ESP_LOGI(TAG, "Served root HTML, %d bytes", file_size);
        }
        return ESP_OK;
    }

    //  Func   root_get_handler
    /// @brief static version of root_get_handler
    /// @note  This is the static version handler which is directly called by the C library
    static esp_err_t static_root_get_handler(httpd_req_t *req)
    {
        HttpServer* server_instance = reinterpret_cast<HttpServer*>(req->user_ctx);
        return server_instance->root_get_handler(req);
    }

    //  Func   http_404_error_handler
    /// @brief If GET request is made to a URL that doesn't exist, this func will be called
    /// @note  For captive portal mode, redirect to root URL to serve the captive portal
    esp_err_t http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
    {
        if(_mode == Mode::CaptivePortal)
        {
            // Set status
            httpd_resp_set_status(req, "303 See Other");
            // Redirect to the "/" root directory
            httpd_resp_set_hdr(req, "Location", "/");
            // iOS requires content in the response to detect a captive portal
            httpd_resp_send(req, "Redirect to the captive portal", HTTPD_RESP_USE_STRLEN);

            ESP_LOGI(TAG, "Redirecting to root");
            return ESP_OK;
        }
        return ESP_OK;
    }

    //  Func   static_http_404_error_handler
    /// @brief Static version of http_404_error_handler
    static esp_err_t static_http_404_error_handler(httpd_req_t *req, httpd_err_code_t err)
    {
        HttpServer* server_instance = reinterpret_cast<HttpServer*>(_instance);
        return server_instance->http_404_error_handler(req, err);
    }
};
