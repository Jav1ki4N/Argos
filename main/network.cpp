
/* Self */
#include "network.hpp"

/* ESP-DDC */
#include "ddc.hpp"
#include "esp_log.h"
#include "root.hpp"
#include "network/ddc_http_client.hpp"
#include "network/ddc_dns_server.hpp"

/* ESP-IDF Components */
#include "freertos/idf_additions.h"

/* Third-party */
#include <cJSON.h>

/**
 * Global Variables & Typedefs
 * - client_q         Queue for sending parsed client info from network task to UI task 
 * - TARGET_HOSTNAME  mDNS hostname of the target PC, must be modified along with the PC client app
 * - isProfileLoaded  Determine if WIFI can switch to STA mode and start mDNS query for target IP
 * - OnSaveHandlerCTX Extra context used in the handler for POST request to "/save"
 *                    Here is the LittleFS instance for saving profiles
 * - Log Tags
*/

QueueHandle_t filesys_q = nullptr;
QueueHandle_t client_q = nullptr; 
static const char* TARGET_HOSTNAME = "argos-target"; 
static SemaphoreHandle_t isProfileLoaded = nullptr;
struct OnSaveHandlerCTX{ LFS* lfs;};
static const char* TAG = "Argos";

#define LOG_ON true

/**
 * Func   http_on_save_handler
 * @brief In captive portal page when user clicks "Save" a POST request to "/save" 
 *        uri will be sent and thus triggering this handler
 * @note  Parse received data from captive portal and save the profile to LittleFS
*/

esp_err_t http_on_save_handler(httpd_req_t *req)
{
    /* get user context  */
    auto *ctx = reinterpret_cast<OnSaveHandlerCTX*>(req->user_ctx);
    LFS* lfs = ctx->lfs; // LittleFS instance

    char buf[256];
    int ret = httpd_req_recv(req, buf, req->content_len);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    /* Key value pairs */
    char ssid[64] = {0};
    char password[64] = {0};
    char profile_name[64] = {0};
    char ntp_server[64] = {0};

    /* From body buffer parse key value pairs */
    httpd_query_key_value(buf, "ssid",         ssid,         sizeof(ssid));
    httpd_query_key_value(buf, "password",     password,     sizeof(password));
    httpd_query_key_value(buf, "profile_name", profile_name, sizeof(profile_name));
    httpd_query_key_value(buf, "ntp_server",   ntp_server,   sizeof(ntp_server));

    ESP_LOGI(TAG, "Captive portal received SSID: %s", ssid);

    /* Fill up profile struct */
    Profile profile = {};
    strlcpy(profile.ssid,         ssid,         sizeof(profile.ssid));
    strlcpy(profile.password,     password,     sizeof(profile.password));
    strlcpy(profile.profile_name, profile_name, sizeof(profile.profile_name));
    strlcpy(profile.ntp_server,   ntp_server,   sizeof(profile.ntp_server));
    
    /* Save profile in LittleFS under /profile/<name> 
     * E.g. For profile_name = "test", the profile will be saved under "/profile/test.profile"
     */
    
    std::string profile_path = "/profile/" + std::string(profile.profile_name) + ".profile";
    lfs->write(profile_path.c_str(), &profile, sizeof(profile));
    httpd_resp_send(req, "Config received", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

/** 
 * Task   network_task
 * @brief Main network task for handling WiFi, DNS, and HTTP operations
 * @note  This task will run after the UI task is initialized
*/

void network_task(void *arg)
{
    //  Obj    LittleFS File System
    /// @brief Initialize LittleFS as to store configs & htmls
    /// @note  In this case the root path is set to "/lfs"
    LFS vault;
    ESP_LOGI(TAG,"LittleFS mounted at: %s", vault.base());
    vault.mkdir("/web");     // make directory
    vault.mkdir("/profile"); // skip if exists

    /**
     * Wait for UI task to signal
     * - to add a new profile
     * - to add a new profile, if not profile exists
     * - to start with existing profile (if any)
     */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    //  Obj    WiFi SoftAP 
    /// @brief Set up the ESP32 as a WiFi Access Point for the Captive Portal
    /// @note  Initialized as SoftAP and will switch to STA after configuration
    WIFI wifi(WIFI::Mode::SoftAP,
             "Argos",         //ssid
             "Clairvoyance"); //pwd

    //  Obj    DNS Server for Configuration Captive Portal
    /// @brief Redirect any DNS query to ESP32's AP IP
    /// @note  Must be initialized after WIFI is set to SoftAP
    DNServer dns;
    dns.startRedirection();

    //  Obj    HTTP Server for Configuration Captive Portal
    /// @brief Create an HTTP to serve the captive portal page 
    HttpServer server(HttpServer::Mode::CaptivePortal,
                      HttpServer::FileSys::LittleFS);

    // register littleFS instance
    server.registerLittleFS(&vault);
    
    // Create root directory for webs
    server.makeRootDir();

    // Save captive portal HTML to LittleFS under /lfs/web
    server.saveWeb(HttpServer::ROOT_NAME,        // Captive Portal HTML as root
                   CAPTIVE_PORTAL_HTML.c_str(),  // Captive Portal content
                   CAPTIVE_PORTAL_HTML.length());

    // Register URI & handler for receiving profile from captive portal
    // When user clicks "Save" a POST request with profile data will be sent to uri "/save"
    // thus triggering the registered handler
    OnSaveHandlerCTX save_ctx = { .lfs = &vault };
    server.registerURI("/save", HTTP_POST, http_on_save_handler, &save_ctx);

    // Start HTTP server to serve captive portal                     
    server.start();   
    
    ESP_LOGI(TAG, "Waiting for profile to be loaded...");
    isProfileLoaded = xSemaphoreCreateBinary();

    /* At this point the user should enter captive portal */
    /* Fill up all fields and click "Save" */
    /* Hnadler will be called and profile will be stored */

    xSemaphoreTake(isProfileLoaded, portMAX_DELAY);

    /* Read active profile name */
    char active_name[32] = {};
    FILE* af = vault.read("/profile/.active");
    if (af) {
        size_t n = fread(active_name, 1, sizeof(active_name) - 1, af);
        active_name[n] = '\0';
        fclose(af);
    }
    std::string profile_path = std::string("/profile/") + active_name + ".profile";

    ESP_LOGI(TAG, "Loading profile: %s", active_name);

    Profile profile = {};
    FILE* f = vault.read(profile_path.c_str());
    if (f) {
        fread(&profile, sizeof(profile), 1, f);
        fclose(f);
        ESP_LOGI(TAG, "SSID: %s, NTP: %s", profile.ssid, profile.ntp_server);
    } else {
        ESP_LOGW(TAG, "Profile '%s' not found in LittleFS", active_name);
    }

    // Switch ESP32 to STA and connect to the target WIFI
    // Default using profile under /profile
    // Stop captive portal services before switching WiFi mode
    dns.Stop();
    server.stop();

    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", profile.ssid);
    wifi.restart(WIFI::Mode::Station,
                          profile.ssid,
                          profile.password);

    //  obj    mDNS Service
    /// @brief Query target IP using mDNS
    mDNS mdns;
    mdns.start();
    std::string ip = mdns.queryForIP(TARGET_HOSTNAME);
    if (ip.empty()) {
        ESP_LOGE(TAG, "Failed to resolve target IP via mDNS");
        vTaskDelete(nullptr); // Terminate task if mDNS query fails
        return;
    }
    std::string target_url = "http://" + ip + ":8080/api/info";
    ESP_LOGI(TAG, "Target URL: %s", target_url.c_str());
    mdns.stop();

    //  Obj    HTTP Client
    /// @brief HTTP client to fetch system info from PC
    HttpClient Argos_client(target_url.c_str());
    client_q = xQueueCreate(3, sizeof(ClientMsg));

    /* SNTP Time Sync */
    const char *ntp = profile.ntp_server[0] ? profile.ntp_server : "pool.ntp.org";
    ESP_LOGI(TAG,"Launching SNTP Time Sync with server: %s", ntp);
    SNTP sntp(ntp);
    sntp.setTimezone("CST-8");
    sntp.sync();


    for(;;)
    {
        /* GET message from PC client */
        HttpClient::Msg msg = Argos_client.Get_ManualPerform();

        /* GET Success with a non-empty content */
        if (msg.status_code == 200 && !msg.body.empty()) 
        {
            ESP_LOGI(TAG, "Raw Body: %s", msg.body.c_str());

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
                ESP_LOGE(TAG, "Parse Failed: [%s]", cJSON_GetErrorPtr());
            }
        }
        else 
        {
            ESP_LOGW(TAG, "HTTP Request Failed or Empty Body");
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS); // client send GET request every 1sec
    }
}


