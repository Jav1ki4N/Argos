/*===================================================*/
/*      ____  ____  ____      ____  ____   ___       */
/*     (  __)/ ___)(  _ \ ___(    \(    \ / __)      */
/*      ) _) \___ \ ) __/(___)) D ( ) D (( (__       */
/*     (____)(____/(__)      (____/(____/ \___)      */
/*===================================================*/
/*         i4n@2026 | ddc_sntp | 2026-4-29          */
/*         Minimal SNTP time sync — fire & forget   */
/*===================================================*/
/* Purpose: Sync system time once after Wi-Fi is up,*/
/*          then release all SNTP resources.         */
/* Usage:   Call ddc_sntp_sync() after WIFI ctor.   */
/*===================================================*/
#pragma once

/* ESP-IDF Components */
#include "esp_log.h"
#include "esp_netif_sntp.h"

/* C/C++ Libraries */
#include <string>
#include <time.h>
#include <sys/time.h>

class SNTP 
{
    public:

    struct SNTPSyncConfig {
        uint8_t  max_retry_count = 15;      // max retry of SNTP synv request
        uint16_t tiemout_ms      = 2000;    // timeout for each SNTP
    };
    
    SNTP(const char* server):
    _server(server)
    {
        /* Use default config for SNTP */
        esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(server);
        /* Init with config - no handler needed */
        esp_err_t err = esp_netif_sntp_init(&config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize SNTP");
        }
    }
    ~SNTP() = default;

    void setSyncConfig(const SNTPSyncConfig& config){ _config = config;}

    void sync(){
        uint8_t retry_count = 0;
        while (esp_netif_sntp_sync_wait(_config.tiemout_ms / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT
               && ++retry_count < _config.max_retry_count) {
                ESP_LOGI(TAG, "Waiting for time sync... (%d/%d)", retry_count, _config.max_retry_count);
        }
        esp_netif_sntp_deinit();
        if (retry_count < _config.max_retry_count) {
            setTimezone("CST-8");
            ESP_LOGI(TAG, "Time sync successful");
        } else {
            ESP_LOGW(TAG, "Time sync failed after %d attempts", _config.max_retry_count);
        }
    }

    void setTimezone(const char* tz) {
        setenv("TZ", tz, 1);
        tzset();
    }

    private:
    static constexpr const char* TAG = "SNTP";
    const char*              _server = nullptr;
    SNTPSyncConfig _config;
    
};
