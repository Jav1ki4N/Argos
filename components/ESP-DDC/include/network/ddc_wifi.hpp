
#pragma once

/* ESP-IDF Components */
/* CMakeLists.txt: REQUIRES esp\_wifi FreeRTOS */
#include "esp_netif_types.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/* C/C++ Libraries */
#include <string>


class WIFI
{
    public:

    enum class Mode : uint8_t
    {
        Idle,           // Only on construction
        Station,        // client
        SoftAP,         // host
        StationSoftAP,  // do Both 
        Sniffer         // sniffing Wi-Fi packets
    };

    WIFI() {
        /* Init nvs flash */
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ESP_ERROR_CHECK(nvs_flash_init());
        } else if (ret != ESP_OK) {
            ESP_ERROR_CHECK(ret);
        }

        ESP_ERROR_CHECK(esp_event_loop_create_default());

        /* Initialize TCP/IP stack */
        ESP_ERROR_CHECK(esp_netif_init());

        /* Init wifi with default config */
        /* THIS IS NOT wifi_config_t, which is used for mode-specific configuration */
        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_ERROR_CHECK(esp_wifi_init(&cfg));

        /* Create event handlers, only used if wish to unregister */
        esp_event_handler_instance_t instance_any_id;
        esp_event_handler_instance_t instance_got_ip;

        /* Register static event handler as WIFI&IP event handler */
        /* This has NOTHING to do with FreeRTOS event group ! */
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
    }

    void start( Mode               mode,
                const std::string& ssid,
                const std::string& password,
                wifi_auth_mode_t   auth_mode = WIFI_AUTH_WPA2_PSK ) {
        
        if(wifi_msg.mode != Mode::Idle) {
            /* Stop current running mode */
            esp_wifi_stop();
            /* clean up running state */
            wifi_retry_count = 0;
        }

        /* Fill up new ssid and password */
        wifi_ssid     = ssid;
        wifi_password = password; 
        startMode(mode,auth_mode);
        wifi_msg.mode = mode; 
    }

    /* WIFI Message */
    /* - If any tasks needs to receive WIFI status updates */
    /* Currently only STA will use this */
    struct WifiMsg
    {
        /* WIFI Connection state */
        enum class State : uint8_t
        {
            Connecting = 0,
            Connected  = 1,
            Failed     = 2,
            Offline    = 3
        }state = State::Offline;

        /* WiFi work mode */
        Mode mode = Mode::Idle;

        /* WiFi ssid */
        char ssid[32]; // must not use std::string cuz FreeRTOS queue does not support it
        
    };

    WifiMsg::State getCurrentState() const { return wifi_msg.state; }
    Mode           getCurrentMode()  const { return wifi_msg.mode;  }
  
    private:
    WifiMsg wifi_msg = {};
    std::string wifi_ssid;
    std::string wifi_password;
    static constexpr uint8_t MAX_STA_CONNECT = 4;
    static constexpr const char* TAG = "WiFi";
    QueueHandle_t msg_queue = nullptr;

    /* WIFI Reconnect Settings */
    uint8_t          wifi_retry_count = 0;        // connect retry counter
    static constexpr uint8_t MAX_WIFI_RETRY = 5;  // max allowed retry times
                                                  // once reached, connection is considered failed

    /* =================================== WIFI EVENT HANDLERS ======================================== */

    //  func   static_event_handler
    /// @brief Handler to called when a WIFi event is triggered (because of C++ and C)
    static void static_event_handler(void*            arg,        // pointer to the instance of WIFI class
                                     esp_event_base_t event_base, // specifies the event base(type of event)
                                     int32_t          event_id,   // specifies the event ID
                                     void*            event_data) // pointer to the event data, additional info
    { 
        WIFI* instance = static_cast<WIFI*>(arg);
        instance->event_handler(event_base, event_id, event_data);
    }

    // func event_handler
    /// @brief Real event handler for processing WIFI events
    void event_handler(esp_event_base_t event_base, 
                       int32_t          event_id, 
                       void*            event_data) {
        /* Event: STA mode - WIFI start to connect */
        if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
            esp_wifi_connect();
            wifi_msg.state = WifiMsg::State::Connecting;
            strlcpy(wifi_msg.ssid, wifi_ssid.c_str(), sizeof(wifi_msg.ssid));
            wifi_msg.mode = Mode::Station;
            if (msg_queue) xQueueSend(msg_queue, &wifi_msg, 0);
        }
        /* Event: STA mode - WIFI reconnection & WIFI failed */
        else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            if (wifi_retry_count < MAX_WIFI_RETRY) // before max allowed retry count
            {
                esp_wifi_connect(); // retry to connect
                wifi_retry_count++; // update retry count
                ESP_LOGI(TAG, "Retrying to connect to the AP");
            }
            else // reach max retry count
            {
                ESP_LOGE(TAG, "Failed to connect to the AP");
                wifi_msg.state = WifiMsg::State::Failed;
                strlcpy(wifi_msg.ssid, wifi_ssid.c_str(), sizeof(wifi_msg.ssid));
                wifi_msg.mode = Mode::Station;
                if (msg_queue) xQueueSend(msg_queue, &wifi_msg, 0);
            }
        }
        // Event: STA mode - WIFI got IP (connected successfully)
        else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
        {
            //ip_event_got_ip_t *event = static_cast<ip_event_got_ip_t*>(event_data);
            //ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
            wifi_retry_count = 0;
            wifi_msg.state = WifiMsg::State::Connected;
            strlcpy(wifi_msg.ssid, wifi_ssid.c_str(), sizeof(wifi_msg.ssid));
            wifi_msg.mode = Mode::Station;
            if (msg_queue) xQueueSend(msg_queue, &wifi_msg, 0);
        }
        // Event: AP mode - a STA connected 
        else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
        {
            wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
            //ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
                     //MAC2STR(event->mac), event->aid);
        }
        // Event: AP mode - a STA disconnected
        else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
        {
            wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
            //ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d, reason=%d",
                     //MAC2STR(event->mac), event->aid, event->reason);
        }
    }

    void startMode(Mode mode, wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA2_PSK)
    {
        /* create default netif objects if is not created */
        if (mode == Mode::Station && esp_netif_get_handle_from_ifkey("WIFI_STA_DEF") == nullptr)
            esp_netif_create_default_wifi_sta();
        else if (mode == Mode::SoftAP && esp_netif_get_handle_from_ifkey("WIFI_AP_DEF") == nullptr)
            esp_netif_create_default_wifi_ap();

        /* wifi_config (for mode-specific configuration) */
        wifi_config_t wifi_config = {};

        /* Mode Specific Configuration */
        /* Mode: STA */
        if (mode == Mode::Station)
        {
            /* SSID & Password */
            strlcpy((char*)wifi_config.sta.ssid, 
                           wifi_ssid.c_str(), 
                           sizeof(wifi_config.sta.ssid));
            strlcpy((char*)wifi_config.sta.password, 
                           wifi_password.c_str(), 
                           sizeof(wifi_config.sta.password));

            /* auth mode */
            wifi_config.sta.threshold.authmode = auth_mode;  // Set minimum auth mode
            wifi_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH; // Enable SAE H2E for WPA3 support

            /* Set and start */
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
            ESP_ERROR_CHECK(esp_wifi_start() );

            ESP_LOGI(TAG, "WiFi is set up as Station with SSID:%s, password:%s",
                     wifi_ssid.c_str(), wifi_password.c_str());
        }

        /* Mode: SoftAP */
        else if (mode == Mode::SoftAP)
        {
            strlcpy((char*)wifi_config.ap.ssid, wifi_ssid.c_str(), sizeof(wifi_config.ap.ssid));
            wifi_config.ap.ssid_len = wifi_ssid.length();
            strlcpy((char*)wifi_config.ap.password, wifi_password.c_str(), sizeof(wifi_config.ap.password));
            wifi_config.ap.channel = 1;
            wifi_config.ap.max_connection = MAX_STA_CONNECT;
            wifi_config.ap.authmode = auth_mode;
            wifi_config.ap.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

            if (wifi_password.empty()) {
                wifi_config.ap.authmode = WIFI_AUTH_OPEN;
            }

            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP) );
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config) );
            ESP_ERROR_CHECK(esp_wifi_start() );

            ESP_LOGI(TAG, "WiFi is set up as SoftAP with SSID:%s, password:%s",
                     wifi_ssid.c_str(), wifi_password.c_str());
        }
    }
};