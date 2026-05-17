
/* Application headers */
#include "network.hpp"

/* DDC headers */
#include "ddc.hpp"
#include "root.hpp"
#include "network/ddc_http_client.hpp"
#include "network/ddc_dns_server.hpp"

/* ESP-IDF Components */
#include "freertos/idf_additions.h"

/* Third-party */
#include <cJSON.h>

/* Global vars */
QueueHandle_t client_q = nullptr; // need to be accessed in UI Task
static const char* TARGET_URL = "http://10.57.166.84:8080/api/info";

void network_task(void *arg)
{
    //  Obj    LittleFS File System
    /// @brief Initialize LittleFS as to store configs & htmls
    /// @note  base() returns "/lfs"
    LFS vault; 
    ESP_LOGI("LittleFS","LittleFS mounted at: %s", vault.base());

    /* Create directories */
    // do nothing if already exists
    vault.mkdir("/config");
    vault.mkdir("/config/wifi");
    vault.mkdir("/config/http");
    vault.mkdir("/web");

    /* Clean up stale files from previous runs (if any) */
    vault.remove("/test.txt");

    // ============================================================
    // LittleFS Write + Read Test
    // ============================================================
    // {
    //     const char *test_msg = "hello world";
    //     ESP_LOGI("LittleFS", "[LFS TEST] Writing: %s", test_msg);
    //     vault.write("/test.txt", test_msg, strlen(test_msg));

    //     FILE *f = vault.read("/test.txt");
    //     if (f) {
    //         char buf[64] = {};
    //         fgets(buf, sizeof(buf), f);
    //         fclose(f);
    //         ESP_LOGI("LittleFS", "[LFS TEST] Read back: %s", buf);
    //     } else {
    //         ESP_LOGE("LittleFS", "[LFS TEST] Read failed");
    //     }
    // }

    /* ==================== NETWORK TASK START ================== */
    /* network task will only run after UI task's booting is done */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    //  Obj    WiFi
    /// @brief Set up the ESP32 as a WiFi Access Point for the Captive Portal
    /// @note  Initialized as SoftAP and will switch to STA after configuration
    WIFI Argos_network(WIFI::Mode::SoftAP);

    //  Obj    DNS Server for Configuration Captive Portal
    /// @brief Redirect any DNS query to ESP32's AP IP
    /// @note  Must be initialized after WIFI is set to SoftAP
    DNServer dns;
    dns.startRedirection();

    //  Obj    HTTP Server for Configuration Captive Portal
    /// @brief Create an HTTP to serve the captive portal page 
    HttpServer Argos_server(HttpServer::Mode::CaptivePortal, 
                            HttpServer::FileSys::LittleFS, 
                            &vault);
                            
    // Save captive portal HTML to LittleFS
    Argos_server.saveWeb(HttpServer::ROOT_NAME,        // Captive Portal HTML as root
                         CAPTIVE_PORTAL_HTML.c_str(),  // Captive Portal content
                         CAPTIVE_PORTAL_HTML.length());
    // Start HTTP server to serve captive portal                     
    Argos_server.start();   

    // //  Obj    HTTP Client
    // /// @brief HTTP client to fetch system info from PC
    // HttpClient Argos_client(TARGET_URL);
    // client_q = xQueueCreate(3, sizeof(ClientMsg));

    // /* SNTP Time Sync */
    // if (ddc_sntp_sync()) ESP_LOGI("SNTP", "Time sync successful");
    // else                 ESP_LOGW("SNTP", "Time sync failed");


    for(;;)
    {
        // /* GET message from PC client */
        // HttpClient::Msg msg = Argos_client.Get_ManualPerform();

        // /* GET Success with a non-empty content */
        // if (msg.status_code == 200 && !msg.body.empty()) 
        // {
        //     ESP_LOGI("JSON", "Raw Body: %s", msg.body.c_str());

        //     /* Parse JSON */
        //     cJSON *root = cJSON_Parse(msg.body.c_str());

        //     if (root != nullptr) 
        //     {
        //         ClientMsg info;

        //         /* Host machine's name or label */
        //         cJSON *host_name = cJSON_GetObjectItem(root, "host_name");
        //         if (cJSON_IsString(host_name)) {
        //             strlcpy(info.host_name, host_name->valuestring, sizeof(info.host_name));
        //         }

        //         /* Operating System */
        //         cJSON *os = cJSON_GetObjectItem(root, "os");
        //         if (cJSON_IsString(os)) {
        //             strlcpy(info.os, os->valuestring, sizeof(info.os));
        //         }

        //         /* OS Version & Distro Info */
        //         cJSON *os_distro = cJSON_GetObjectItem(root, "os_version");
        //         if (cJSON_IsString(os_distro)) {
        //             strlcpy(info.os_distro, os_distro->valuestring, sizeof(info.os_distro));
        //         }

        //         /* CPU */
        //         cJSON *cpu_usage = cJSON_GetObjectItem(root, "cpu_percent");
        //         if (cJSON_IsNumber(cpu_usage)) {
        //             info.cpu_usage = (float)cpu_usage->valuedouble;
        //         }

        //         cJSON *cpu_cores = cJSON_GetObjectItem(root, "cpu_cores");
        //         if (cJSON_IsNumber(cpu_cores)) {
        //             info.cpu_cores = cpu_cores->valueint;
        //         }
                
        //         cJSON *cpu_threads = cJSON_GetObjectItem(root, "cpu_threads");
        //         if (cJSON_IsNumber(cpu_threads)) {
        //             info.cpu_threads = cpu_threads->valueint;
        //         }

        //         cJSON *cpu_core_freq = cJSON_GetObjectItem(root, "cpu_freq_mhz");
        //         if (cJSON_IsNumber(cpu_core_freq)) {
        //             info.cpu_core_freq = cpu_core_freq->valueint;
        //         }

        //         cJSON *cpu_temp = cJSON_GetObjectItem(root, "cpu_temp");
        //         if (cJSON_IsNumber(cpu_temp)) {
        //             info.cpu_temp = (float)cpu_temp->valuedouble;
        //         }

        //         /* Memory */
        //         cJSON *mem_total = cJSON_GetObjectItem(root, "mem_total_mb");
        //         if (cJSON_IsNumber(mem_total)) {
        //             info.mem_total = mem_total->valueint;
        //         }

        //         cJSON *mem_used = cJSON_GetObjectItem(root, "mem_used_mb");
        //         if (cJSON_IsNumber(mem_used)) {
        //             info.mem_used = mem_used->valueint;
        //         }

        //         cJSON *mem_usage = cJSON_GetObjectItem(root, "mem_percent");
        //         if (cJSON_IsNumber(mem_usage)) {
        //             info.mem_usage = (float)mem_usage->valuedouble;
        //         }

        //         /* DISK */
        //         cJSON *disk_total = cJSON_GetObjectItem(root, "disk_total_gb");
        //         if (cJSON_IsNumber(disk_total)) {
        //             info.disk_total = (int)disk_total->valuedouble; 
        //         }

        //         cJSON *disk_used = cJSON_GetObjectItem(root, "disk_used_gb");
        //         if (cJSON_IsNumber(disk_used)) {
        //             info.disk_used = (int)disk_used->valuedouble;   
        //         }

        //         if (info.disk_total > 0) {
        //             info.disk_usage = (float)info.disk_used / info.disk_total * 100.0f;
        //         }

        //         /* Send via queue created in task above */
        //         xQueueSend(client_q, &info, 0);
                
        //         /* delete root to prevent memory leak */
        //         cJSON_Delete(root);
        //     }
        //     else 
        //     {
        //         ESP_LOGE("JSON", "Parse Failed: [%s]", cJSON_GetErrorPtr());
        //     }
        // }
        // else 
        // {
        //     ESP_LOGW("JSON", "HTTP Request Failed or Empty Body");
        // }

        vTaskDelay(1000 / portTICK_PERIOD_MS); // client send GET request every 1sec
    }
}
