
#pragma once 

#include "mqtt_client.h"
#include "esp_log.h"

#include <cstring>
#include <functional>
#include <string>

#define MQTT_CLIENT_LOG_ENABLE 1

class MQTTClient{
    public:
    MQTTClient(const char* uri,        // to broker URI, also the protocol
                                       // IP addr could be either xxx.xxx..
                                       // or mdns name as long as it can be interpreted

               const char* client_id   // ID of the client recognized by the broker

              )
        : uri_(uri), client_id_(client_id)
    {
        /* Configuration */
        assert(uri       != nullptr);
        assert(client_id != nullptr);
        config_                       = {};
        config_.broker.address.uri    = uri_.c_str();
        config_.credentials.client_id = client_id_.c_str();

        /* Client Creation */
        client_ = esp_mqtt_client_init(&config_);
        /* Event Registration */
        esp_mqtt_client_register_event(client_, MQTT_EVENT_ANY,static_event_handler, this);
    }
    ~MQTTClient() {
        if (client_) {
            esp_mqtt_client_stop(client_);
            esp_mqtt_client_destroy(client_);
        }
    }

    esp_err_t start() {
        return esp_mqtt_client_start(client_);
    }

    esp_err_t stop() {
        return esp_mqtt_client_stop(client_);
    }

    int publish(const char* topic, const char* payload,
            uint8_t qos = 0, bool retain = false) {
    return esp_mqtt_client_publish(client_, topic, payload,
                                   strlen(payload), qos, retain ? 1 : 0);
    }

    esp_err_t subscribe(const char* topic, uint8_t qos = 0) {
        return esp_mqtt_client_subscribe(client_, topic, qos);
    }

    /* Callback lambdas impl by user */
    std::function<void()> onConnect;
    std::function<void()> onDisconnect;
    std::function<void(const char* topic, int topic_len,
                       const char* data, int data_len)> onRecvData;

    private:
    static constexpr const char* TAG = "MQTTClient";
    std::string uri_;
    std::string client_id_;
    esp_mqtt_client_handle_t client_;
    esp_mqtt_client_config_t config_;

    /**
     * @brief MQTT log func for debugging only
     * @note  set MQTT_CLIENT_LOG_ENABLE to 0 to disable MQTT log
     */
    void mqttLog(const char* msg) {
        #if MQTT_CLIENT_LOG_ENABLE
            ESP_LOGI(TAG, "%s", msg);
        #endif
    }

    /**
     * @brief static event handler to be called when a MQTT event is triggered
     */
    static void static_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
    {
        MQTTClient* client_instance = reinterpret_cast<MQTTClient*>(arg);
        client_instance->event_handler(event_base, event_id, event_data);
    }

    /**
     * @brief event handler to be called when a MQTT event is triggered
     *        this is the logic implementation of the event handler, which is called by the static event handler
     */
    void event_handler(esp_event_base_t event_base, int32_t event_id, void* event_data)
    {
        ESP_LOGD(TAG, "Event dispatched base=%s, id=%" PRIi32 "", event_base, event_id);
        /* Do something according to the event type (id) */
        switch(static_cast<esp_mqtt_event_id_t>(event_id)){

            case MQTT_EVENT_CONNECTED:                 // connect to broker
                mqttLog("MQTT_EVENT_CONNECTED");
                if (onConnect) onConnect();            // do something e.g. subscribe to a topic 
                break;
            case MQTT_EVENT_DISCONNECTED:
                mqttLog("MQTT_EVENT_DISCONNECTED");
                if (onDisconnect) onDisconnect();
                break;
            case MQTT_EVENT_SUBSCRIBED:                // successfully subscribe to a topic
                mqttLog("MQTT_EVENT_SUBSCRIBED");
                break;
            case MQTT_EVENT_UNSUBSCRIBED:              // successfully unsubscribe from a topic
                mqttLog("MQTT_EVENT_UNSUBSCRIBED");
                break;
            case MQTT_EVENT_PUBLISHED:                 // successfully publish a message to a topic
                mqttLog("MQTT_EVENT_PUBLISHED");
                break;
            case MQTT_EVENT_DATA:
                mqttLog("MQTT_EVENT_DATA");
                if (onRecvData) {
                    auto data = static_cast<esp_mqtt_event_handle_t>(event_data);
                    onRecvData(data->topic, data->topic_len, data->data, data->data_len);
                }
                break;
            default:
                break;
        }

    }
};