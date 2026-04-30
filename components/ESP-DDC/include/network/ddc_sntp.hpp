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
#include "esp_sntp.h"

/* C/C++ Libraries */
#include <time.h>
#include <sys/time.h>

static const char *SNTP_TAG = "SNTP";

/* Sync system time once, then deinit SNTP.     */
/* - Returns true on success, false on timeout. */
/* - Called only if Wi-Fi is connected.         */
inline bool ddc_sntp_sync(const char *server = "pool.ntp.org", // SNTP server
                          int max_retry_count = 15,            // max retry of SNTP synv request
                          int timeout_ms = 2000)               // timeout for each SNTP sync request
{
    /* Init SNTP service */
    /* - use default config */
    /* - start request   */
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(server);

    /* SNTP is one and only so no handle */
    /* - and there's no much to interact with */
    esp_netif_sntp_init(&config);

    int retry_count = 0; // current retry count
    while (esp_netif_sntp_sync_wait(timeout_ms / portTICK_PERIOD_MS) == ESP_ERR_TIMEOUT
           && ++retry_count < max_retry_count) {
        ESP_LOGI(SNTP_TAG, "Waiting for time sync... (%d/%d)", retry_count, max_retry_count);
    }

    /* Request done, deinit SNTP */
    /* - whether success or not  */
    esp_netif_sntp_deinit();

    /* Set local timezone */
    setenv("TZ", "CST-8", 1);
    tzset();

    if (retry_count < max_retry_count) {
        ESP_LOGI(SNTP_TAG, "Time synced successfully");
        return true;
    }

    ESP_LOGW(SNTP_TAG, "Time sync timed out");
    return false;
}
