/*===================================================*/
/*      ____  ____  ____      ____  ____   ___       */
/*     (  __)/ ___)(  _ \ ___(    \(    \ / __)      */
/*      ) _) \___ \ ) __/(___)) D ( ) D (( (__       */
/*     (____)(____/(__)      (____/(____/ \___)      */
/*===================================================*/
/*         i4n@2026 | ddc_wifi | 2026-4-26           */
/*         ESP-IDF Wi-Fi station helper class       */
/*===================================================*/
/* Purpose: Provide startup, NVS init, Wi-Fi station */
/*          connect/reconnect logic, and event       */
/*          handling for ESP-IDF applications.       */
/*===================================================*/
#pragma once

/* Includes */
/* CMakeLists.txt: REQUIRES esp_wifi FreeRTOS*/
#include "esp_netif_types.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "freertos/task.h"
#include "freertos/event_groups.h"

#include <string>



class WIFI
{
    public:

    enum class Mode : uint8_t
    {
        station,        // Connect to an existing Wi-Fi network
        softAP,         // Create a Wi-Fi network for other devices to connect to
        station_softAP, // Both station and softAP modes at the same time
        sniffer         // Monitor mode for sniffing Wi-Fi packets
    };

    WIFI() = default;

    void init(Mode mode, const std::string& ssid, const std::string& password, wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA2_PSK)
    {
        /* only for test */
        if (mode != Mode::station) {
            ESP_LOGE("WIFI", "Only station mode is implemented");
            return;
        }

        WIFI_SSID = ssid;
        WIFI_PASSWORD = password;

        /****************/
        //Initialize NVS
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);

        ESP_LOGI("WIFI", "ESP_WIFI_MODE_STA");
        init_core(mode, auth_mode);
    }

    private:
    
    std::string WIFI_SSID     = "Hermes";
    std::string WIFI_PASSWORD = "Clairvoyance";

    /* FreeRTOS Event Group  */
    enum class WifiEventBits : uint8_t
    {
        connected = 1 << 0, // Bit 0: Connected to Wi-Fi
        failed    = 1 << 1  // Bit 1: Failed to connect to Wi-Fi
    };
    static inline EventGroupHandle_t wifi_event_group;

    uint8_t wifi_retry_count = 0;
    static constexpr uint8_t MAX_WIFI_RETRY = 5;

    /* =================================== WIFI EVENT HANDLERS ======================================== */

    /* When an event is received, this function is called */
    static void static_event_handler(void*            arg,        // pointer to the instance of WIFI class
                                     esp_event_base_t event_base, // specifies the event base(type of event)
                                     int32_t          event_id,   // specifies the event ID
                                     void*            event_data) // pointer to the event data, additional info
    { 
        WIFI* instance = static_cast<WIFI*>(arg);
        instance->event_handler(event_base, event_id, event_data);
    }

    /* real event handler */
    void event_handler(esp_event_base_t event_base, 
                       int32_t          event_id, 
                       void*            event_data)
    {
        // WIFI CONNECT =========================================================
        // if Event is Wi-Fi event and event ID is "start", then connect to Wi-Fi
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
        {
            esp_wifi_connect();
        }
        // WIFI RECONNECT ===========================================================
        // if WIFI event's ID is "disconnected", retry to connect
        else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            if (wifi_retry_count < MAX_WIFI_RETRY) // before max allowed retry count 
            {
                esp_wifi_connect(); // retry to connect
                wifi_retry_count++; // update retry count
                ESP_LOGI("WIFI", "Retrying to connect to the AP");
            }
            // WIFI FAILED =======================================================================
            else // reach max retry count
            {
                xEventGroupSetBits(wifi_event_group, static_cast<uint8_t>(WifiEventBits::failed)); 
                ESP_LOGE("WIFI", "Failed to connect to the AP");
            }
        }
        // WIFI GOT IP ====================================================
        else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
        {
            ip_event_got_ip_t *event = static_cast<ip_event_got_ip_t*>(event_data);
            ESP_LOGI("WIFI", "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
            wifi_retry_count = 0;
            xEventGroupSetBits(wifi_event_group, static_cast<uint8_t>(WifiEventBits::connected));
        }
    }

    void init_core(Mode mode, wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA2_PSK)
    {
        // assign actual space for event group
        wifi_event_group = xEventGroupCreate();

        // Init TCP/IP stack
        ESP_ERROR_CHECK(esp_netif_init());

        // Create default event loop
        ESP_ERROR_CHECK(esp_event_loop_create_default());

        // Create default Wi-Fi interface, depending on the mode
        if(mode == Mode::station)esp_netif_create_default_wifi_sta();

        // Init Wi-Fi with default config pre-written by ESP-IDF
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        // The handle returned after handler is registered
        // is used to unregister the handler later if needed, ignore by default
        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;

        // Register static event handler as WIFI&IP event handler
        // so that event handler will  be called when a WIFI/IP event occurs
        ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &static_event_handler,
                                                            this,
                                                            &instance_any_id));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                            IP_EVENT_STA_GOT_IP,
                                                            &static_event_handler,
                                                            this,
                                                            &instance_got_ip));

        // Configure Wi-Fi connection settings specific to the mode
        // Notice that this config is not 'wifi_init_config_t' but 'wifi_config_t'

        wifi_config_t wifi_config = {};
        if (mode == Mode::station)
        {
            strlcpy((char*)wifi_config.sta.ssid, WIFI_SSID.c_str(), sizeof(wifi_config.sta.ssid));
            strlcpy((char*)wifi_config.sta.password, WIFI_PASSWORD.c_str(), sizeof(wifi_config.sta.password));
            wifi_config.sta.threshold.authmode = auth_mode; // Set minimum auth mode
            wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH; // Enable SAE H2E for WPA3 support

            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
            ESP_ERROR_CHECK(esp_wifi_start() );

            ESP_LOGI("WIFI_STA", "wifi_init_sta finished.");
        }

        EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
        static_cast<uint8_t>(WifiEventBits::connected) | static_cast<uint8_t>(WifiEventBits::failed),
        pdFALSE,
        pdFALSE,
        portMAX_DELAY);

        if (bits & static_cast<uint8_t>(WifiEventBits::connected)) {
            ESP_LOGI("WIFI_STA", "connected to ap SSID:%s password:%s",
                 WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
            } 
        else if (bits & static_cast<uint8_t>(WifiEventBits::failed)) {
            ESP_LOGI("WIFI_STA", "Failed to connect to SSID:%s, password:%s",
                 WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
        } 
        else ESP_LOGE("WIFI_STA", "UNEXPECTED EVENT");
    }
};