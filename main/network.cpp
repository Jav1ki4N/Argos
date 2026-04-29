#include "ddc.hpp"
#include "network.hpp"
#include "devices/display/UI/ddc_argos_u8g2.hpp"
#include "freertos/idf_additions.h"
#include "main.hpp"
#include "network/ddc_http_client.hpp"
#include <cJSON.h>

QueueHandle_t client_q = nullptr;

void network_task(void *arg)
{
    /* Wait until main task signals boot screen is drawn */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    QueueHandle_t q = xQueueCreate(3, sizeof(WifiMsg));
    WIFI::set_ui_queue(q);

    WIFI Argos_network(WIFI::Mode::Station, "N3V3RM1ND", "KurtCobain");

    client_q = xQueueCreate(3, sizeof(ClientMsg));

    HttpClient Argos_client("http://10.132.210.84:8080/api/info");

    /* On booting & WiFi is up, sync time via SNTP */
    if (ddc_sntp_sync()) ESP_LOGI("SNTP", "Time sync successful");
    else                 ESP_LOGW("SNTP", "Time sync failed");


    for(;;)
    {
        // 1. 获取 HTTP 响应
        HttpClient::Msg msg = Argos_client.Get();

        if (msg.status_code == 200 && !msg.body.empty()) 
        {
            ESP_LOGI("JSON", "Raw Body: %s", msg.body.c_str());
            cJSON *root = cJSON_Parse(msg.body.c_str());

            if (root != nullptr) 
            {
                ClientMsg info;

                cJSON *host_name = cJSON_GetObjectItem(root, "host_name");
                if (cJSON_IsString(host_name)) {
                    strlcpy(info.host_name, host_name->valuestring, sizeof(info.host_name));
                }

                cJSON *os = cJSON_GetObjectItem(root, "os");
                if (cJSON_IsString(os)) {
                    strlcpy(info.os, os->valuestring, sizeof(info.os));
                }

                cJSON *os_distro = cJSON_GetObjectItem(root, "os_version");
                if (cJSON_IsString(os_distro)) {
                    strlcpy(info.os_distro, os_distro->valuestring, sizeof(info.os_distro));
                }

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

                cJSON *disk_total = cJSON_GetObjectItem(root, "disk_total_gb");
                if (cJSON_IsNumber(disk_total)) {
                    info.disk_total = (int)disk_total->valuedouble; // disk_total_gb 返回的是 float
                }

                cJSON *disk_used = cJSON_GetObjectItem(root, "disk_used_gb");
                if (cJSON_IsNumber(disk_used)) {
                    info.disk_used = (int)disk_used->valuedouble;   // 同理
                }

                if (info.disk_total > 0) {
                    info.disk_usage = (float)info.disk_used / info.disk_total * 100.0f;
                }

                xQueueSend(client_q, &info, 0);

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

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
