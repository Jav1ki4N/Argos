
#pragma once

#include <string>
#include <esp_log.h>
#include "../../components/espressif__mdns/include/mdns.h"

class mDNS
{
    public:
    mDNS()  = default;
    ~mDNS(){
        stop();
    }

    void start() {ESP_ERROR_CHECK(mdns_init());}

    void stop() { mdns_free(); }

    //  Func   queryForIP
    /// @brief Query the IP address of a host using mDNS
    /// @param host_name The mDNS hostname to query (without the .local suffix)
    /// @param timeout   Timeout for the query in milliseconds
    std::string queryForIP(const char*    host_name,
                           const uint32_t timeout = 2000)
    {
        esp_ip4_addr_t addr;
        esp_err_t err = mdns_query_a(host_name, timeout, &addr); // Update addr with 
                                                                 // the resolved IP address
        if (err) {
            if (err == ESP_ERR_NOT_FOUND) {
                ESP_LOGW(TAG, "%s: Host was not found!", esp_err_to_name(err));
                return "";
            }
            ESP_LOGE(TAG, "Query Failed: %s", esp_err_to_name(err));
            return "";
        }
        ESP_LOGI(TAG, "Query A: %s.local resolved to: " IPSTR, 
                       host_name,      // fill %s
                       IP2STR(&addr)); // fill IPSTR (%d.%d.%d.%d)

        char ip_str[16];
        snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&addr)); // format addr into string
        return std::string(ip_str); // return as std::string
    }

    private:
    static constexpr const char* TAG = "mDNS";
    
};