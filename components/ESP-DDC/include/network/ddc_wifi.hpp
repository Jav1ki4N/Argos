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
#include "freertos/queue.h"

#include <string>

/* Message passed from WIFI event handler to UI task via Queue */
/* - In evnet handler wifi status is updated and loaded into a */
/* - local WifiMsg struct, which will eventually be consumed.  */
struct WifiMsg
{
    /* state define */
    /* - each state represents a different wifi connection status */
    /* - and referred to a certain event bits of wifi event group */
    enum State : uint8_t
    {
        Connecting = 0,
        Connected  = 1,
        Failed     = 2
    };

    /* wifi connection state */
    State state;
    /* ssid of connected wifi */
    char ssid[32];
};



class WIFI
{
    public:

    enum class Mode : uint8_t
    {
        Station,        // client
        SoftAP,         // host
        StationSoftAP,  // do Both 
        Sniffer         // sniffing Wi-Fi packets
    };

    WIFI(Mode mode, 
         const std::string& ssid,                         // Route WIFI identfier, ESP32 will looking for this SSID to connect to
         const std::string& password,                     // Just password
         wifi_auth_mode_t auth_mode = WIFI_AUTH_WPA2_PSK) // Authentication mode, default is WPA2-PSK, can be set to WPA3 if supported by the AP
    {
        
        // Update Wi-Fi credentials (passed parameters -> private members)
        wifi_ssid     = ssid;
        wifi_password = password;

        // Initialize NVS (Non-Volatile Storage), 
        // Required
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);

        // Specific WIFI mode initialization
        // 2026-4-28: ONLY STA MODE IS IMPLEMENTED
        ESP_LOGI("WIFI", "ESP_WIFI_MODE_STA");
        init_core(mode, auth_mode);
    }

    /* FreeRTOS Event Group  */
    enum class WifiEventBits : uint8_t
    {
        connected  = 1 << 0,   // Bit 0: Connected to Wi-Fi
        failed     = 1 << 1,   // Bit 1: Failed to connect to Wi-Fi
        connecting = 1 << 2    // Bit 2: Currently trying to connect to Wi-Fi
    };

    static EventGroupHandle_t get_event_group()
    {
        return wifi_event_group;
    }

    const char* get_ssid() const
    {
        return wifi_ssid.c_str();
    }

    /* Queue-based IPC: event handler pushes WifiMsg here for UI task consumption */
    /* - Setter: Passed handle must be a return value from xQueueCreate */
    /* - Getter: Called within UI task to received the actual queue */
    /* - All set to static cuz of class-level design (only one instance) */
    static void set_ui_queue(QueueHandle_t q)
    {
        ui_queue = q; // ui_queue is now functionally points to a certain queue
    }

    static QueueHandle_t get_ui_queue()
    {
        return ui_queue;
    }

    private:
    
    std::string wifi_ssid     = "Hermes";
    std::string wifi_password = "Clairvoyance";

    /* FreeRTOS Handles */
    /* - wifi_event_group: handle of current wifi event group */
    /* - ui_queue: handle of queue where data sent to UI task */
    /* - all these handles will only work when assigned with an actual space of RAM */

    static inline EventGroupHandle_t wifi_event_group;
    static inline QueueHandle_t      ui_queue = nullptr;

    /* WIFI Reconnect Settings */
    uint8_t          wifi_retry_count = 0;        // connect retry counter
    static constexpr uint8_t MAX_WIFI_RETRY = 5;  // max allowed retry times
                                                  // once reached, connection is considered failed

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
            xEventGroupClearBits(wifi_event_group, static_cast<uint8_t>(WifiEventBits::connected) | static_cast<uint8_t>(WifiEventBits::failed));
            xEventGroupSetBits(wifi_event_group, static_cast<uint8_t>(WifiEventBits::connecting));
            esp_wifi_connect();

            if (ui_queue) { // check if ui_queue is set 
                WifiMsg msg{}; // local message struct, deleted after instant sent
                msg.state = WifiMsg::Connecting; // load state
                strlcpy(msg.ssid, wifi_ssid.c_str(), sizeof(msg.ssid)); // load ssid
                xQueueSend(ui_queue, &msg, 0); // send through the queue via ui_queue
            }
        }
        // WIFI RECONNECT ===========================================================
        // if WIFI event's ID is "disconnected", retry to connect
        else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            if (wifi_retry_count < MAX_WIFI_RETRY) // before max allowed retry count
            {
                xEventGroupClearBits(wifi_event_group, static_cast<uint8_t>(WifiEventBits::connected) | static_cast<uint8_t>(WifiEventBits::failed));
                xEventGroupSetBits(wifi_event_group, static_cast<uint8_t>(WifiEventBits::connecting));

                esp_wifi_connect(); // retry to connect
                wifi_retry_count++; // update retry count
                ESP_LOGI("WIFI", "Retrying to connect to the AP");

                if (ui_queue) {
                    WifiMsg msg{};
                    msg.state = WifiMsg::Connecting;
                    strlcpy(msg.ssid, wifi_ssid.c_str(), sizeof(msg.ssid));
                    xQueueSend(ui_queue, &msg, 0);
                }
            }
            // WIFI FAILED =======================================================================
            else // reach max retry count
            {
                xEventGroupSetBits(wifi_event_group, static_cast<uint8_t>(WifiEventBits::failed));
                xEventGroupClearBits(wifi_event_group, static_cast<uint8_t>(WifiEventBits::connecting));
                ESP_LOGE("WIFI", "Failed to connect to the AP");

                if (ui_queue) {
                    WifiMsg msg{};
                    msg.state = WifiMsg::Failed;
                    strlcpy(msg.ssid, wifi_ssid.c_str(), sizeof(msg.ssid));
                    xQueueSend(ui_queue, &msg, 0);
                }
            }
        }
        // WIFI GOT IP ====================================================
        else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
        {
            ip_event_got_ip_t *event = static_cast<ip_event_got_ip_t*>(event_data);
            ESP_LOGI("WIFI", "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
            wifi_retry_count = 0;
            xEventGroupSetBits(wifi_event_group, static_cast<uint8_t>(WifiEventBits::connected));
            xEventGroupClearBits(wifi_event_group, static_cast<uint8_t>(WifiEventBits::connecting));

            if (ui_queue) {
                WifiMsg msg{};
                msg.state = WifiMsg::Connected;
                strlcpy(msg.ssid, wifi_ssid.c_str(), sizeof(msg.ssid));
                xQueueSend(ui_queue, &msg, 0);
            }
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
        if(mode == Mode::Station)esp_netif_create_default_wifi_sta();

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
        if (mode == Mode::Station)
        {
            strlcpy((char*)wifi_config.sta.ssid, wifi_ssid.c_str(), sizeof(wifi_config.sta.ssid));
            strlcpy((char*)wifi_config.sta.password, wifi_password.c_str(), sizeof(wifi_config.sta.password));
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
                 wifi_ssid.c_str(), wifi_password.c_str());
            } 
        else if (bits & static_cast<uint8_t>(WifiEventBits::failed)) {
            ESP_LOGI("WIFI_STA", "Failed to connect to SSID:%s, password:%s",
                 wifi_ssid.c_str(), wifi_password.c_str());
        } 
        else ESP_LOGE("WIFI_STA", "UNEXPECTED EVENT");
    }
};