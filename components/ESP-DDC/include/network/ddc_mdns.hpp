
#pragma once

#include <cstring>
#include <string>
#include <esp_log.h>
#include <cstdio>
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

    struct ServiceInfo {
        std::string ip;
        uint16_t port = 0;
    };

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

    //  Func   queryForService
    /// @brief Discover a service via mDNS PTR query, filter by instance name, return IP and port
    /// @param instance Instance name to look for (e.g. "argos")
    /// @param service  Service type (e.g. "_http")
    /// @param proto    Protocol (e.g. "_tcp")
    /// @param timeout  Timeout in milliseconds
    ServiceInfo queryForService(const char* instance,
                                const char* service = "_http",
                                const char* proto   = "_tcp",
                                const uint32_t timeout = 2000)
    {
        mdns_result_t* results = nullptr;
        esp_err_t err = mdns_query_ptr(service, proto, timeout, 1, &results);
        if (err || !results) {
            if (err == ESP_ERR_NOT_FOUND)
                ESP_LOGW(TAG, "Service %s.%s not found", service, proto);
            else
                ESP_LOGE(TAG, "Query Failed: %s", esp_err_to_name(err));
            return {};
        }

        for (auto* r = results; r; r = r->next) {
            if (!r->addr) continue;
            if (instance && r->instance_name && strcmp(instance, r->instance_name) != 0) continue;

            for (auto* a = r->addr; a; a = a->next) {
                if (a->addr.type != ESP_IPADDR_TYPE_V4) continue;

                auto& ip4 = a->addr.u_addr.ip4;
                ServiceInfo info;
                char ip_str[16];
                snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&ip4));
                info.ip   = ip_str;
                info.port = r->port;

                ESP_LOGI(TAG, "Found %s (%s.%s) at " IPSTR ":%d",
                         r->instance_name, service, proto, IP2STR(&ip4), r->port);

                mdns_query_results_free(results);
                return info;
            }
        }

        ESP_LOGW(TAG, "Instance '%s' not found in %s.%s results", instance, service, proto);
        mdns_query_results_free(results);
        return {};
    }

    private:
    static constexpr const char* TAG = "mDNS";
    
};