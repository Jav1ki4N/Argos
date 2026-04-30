
/* Application headers */
#include "network.hpp"

/* DDC headers */
#include "ddc.hpp"
#include "network/ddc_http_client.hpp"

/* ESP-IDF Components */
#include "freertos/idf_additions.h"

/* C/C++ Libraries */
#include <cJSON.h>

/* Global vars */
QueueHandle_t client_q = nullptr; // need to be accessed in UI Task
static const char* TARGET_URL = "http://10.132.210.84:8080/api/info";

void network_task(void *arg)
{
    /* Notify given by UI Task after boot screen is drawn */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    /* WIFI */
    QueueHandle_t q = xQueueCreate(3, sizeof(WifiMsg));
    WIFI::set_ui_queue(q);
    WIFI Argos_network(WIFI::Mode::Station, "N3V3RM1ND", "KurtCobain");

    /* Client */
    client_q = xQueueCreate(3, sizeof(ClientMsg));
    HttpClient Argos_client(TARGET_URL);

    /* SNTP Time Sync */
    if (ddc_sntp_sync()) ESP_LOGI("SNTP", "Time sync successful");
    else                 ESP_LOGW("SNTP", "Time sync failed");


    for(;;)
    {
        /* GET message from PC client */
        HttpClient::Msg msg = Argos_client.Get_ManualPerform();

        /* GET Success with a non-empty content */
        if (msg.status_code == 200 && !msg.body.empty()) 
        {
            ESP_LOGI("JSON", "Raw Body: %s", msg.body.c_str());

            /* Parse JSON */
            cJSON *root = cJSON_Parse(msg.body.c_str());

            if (root != nullptr) 
            {
                ClientMsg info;

                /* Host machine's name or label */
                cJSON *host_name = cJSON_GetObjectItem(root, "host_name");
                if (cJSON_IsString(host_name)) {
                    strlcpy(info.host_name, host_name->valuestring, sizeof(info.host_name));
                }

                /* Operating System */
                cJSON *os = cJSON_GetObjectItem(root, "os");
                if (cJSON_IsString(os)) {
                    strlcpy(info.os, os->valuestring, sizeof(info.os));
                }

                /* OS Version & Distro Info */
                cJSON *os_distro = cJSON_GetObjectItem(root, "os_version");
                if (cJSON_IsString(os_distro)) {
                    strlcpy(info.os_distro, os_distro->valuestring, sizeof(info.os_distro));
                }

                /* CPU */
                cJSON *cpu_usage = cJSON_GetObjectItem(root, "cpu_percent");
                if (cJSON_IsNumber(cpu_usage)) {
                    info.cpu_usage = (float)cpu_usage->valuedouble;
                }

                cJSON *cpu_cores = cJSON_GetObjectItem(root, "cpu_cores");
                if (cJSON_IsNumber(cpu_cores)) {
                    info.cpu_cores = cpu_cores->valueint;
                }
                
                cJSON *cpu_threads = cJSON_GetObjectItem(root, "cpu_threads");
                if (cJSON_IsNumber(cpu_threads)) {
                    info.cpu_threads = cpu_threads->valueint;
                }

                cJSON *cpu_core_freq = cJSON_GetObjectItem(root, "cpu_freq_mhz");
                if (cJSON_IsNumber(cpu_core_freq)) {
                    info.cpu_core_freq = cpu_core_freq->valueint;
                }

                cJSON *cpu_temp = cJSON_GetObjectItem(root, "cpu_temp");
                if (cJSON_IsNumber(cpu_temp)) {
                    info.cpu_temp = (float)cpu_temp->valuedouble;
                }

                /* Memory */
                cJSON *mem_total = cJSON_GetObjectItem(root, "mem_total_mb");
                if (cJSON_IsNumber(mem_total)) {
                    info.mem_total = mem_total->valueint;
                }

                cJSON *mem_used = cJSON_GetObjectItem(root, "mem_used_mb");
                if (cJSON_IsNumber(mem_used)) {
                    info.mem_used = mem_used->valueint;
                }

                cJSON *mem_usage = cJSON_GetObjectItem(root, "mem_percent");
                if (cJSON_IsNumber(mem_usage)) {
                    info.mem_usage = (float)mem_usage->valuedouble;
                }

                /* DISK */
                cJSON *disk_total = cJSON_GetObjectItem(root, "disk_total_gb");
                if (cJSON_IsNumber(disk_total)) {
                    info.disk_total = (int)disk_total->valuedouble; 
                }

                cJSON *disk_used = cJSON_GetObjectItem(root, "disk_used_gb");
                if (cJSON_IsNumber(disk_used)) {
                    info.disk_used = (int)disk_used->valuedouble;   
                }

                if (info.disk_total > 0) {
                    info.disk_usage = (float)info.disk_used / info.disk_total * 100.0f;
                }

                /* Send via queue created in task above */
                xQueueSend(client_q, &info, 0);
                
                /* delete root to prevent memory leak */
                cJSON_Delete(root);
            }
            else 
            {
                ESP_LOGE("JSON", "Parse Failed: [%s]", cJSON_GetErrorPtr());
            }
        }
        else 
        {
            ESP_LOGW("JSON", "HTTP Request Failed or Empty Body");
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS); // client send GET request every 1sec
    }
}
