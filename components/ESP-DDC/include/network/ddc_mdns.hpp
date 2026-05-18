
#pragma once

#include <esp_log.h>
#include "../../components/espressif__mdns/include/mdns.h"

class mDNS
{
    public:
    mDNS()  = default;
    ~mDNS() = default;

    void start() {ESP_ERROR_CHECK(mdns_init());}

    void stop() { mdns_free(); }

    char* queryForIP(const char*    host_name,
                     const uint32_t timeout = 2000)
    {
        esp_ip4_addr_t addr;
        esp_err_t err = mdns_query_a(host_name, timeout, &addr);
        if (err)
        {
            if (err == ESP_ERR_NOT_FOUND) {
                ESP_LOGW(TAG, "%s: Host was not found!", esp_err_to_name(err));
                return nullptr;
            }
            ESP_LOGE(TAG, "Query Failed: %s", esp_err_to_name(err));
            return nullptr;
        }
        ESP_LOGI(TAG, "Query A: %s.local resolved to: " IPSTR, host_name, IP2STR(&addr));
        char ip_str[16];
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&addr));
        return strdup(ip_str);
    }

    private:
    static constexpr const char* TAG = "mDNS";
    
};