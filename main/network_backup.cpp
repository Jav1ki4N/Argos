
/* Self */
#include "network.hpp"

/* ESP-DDC */
#include "ddc.hpp"
#include "esp_log.h"
#include "main.hpp"
#include "ui.hpp"
#include "root.hpp"
#include "network/ddc_http_client.hpp"
#include "network/ddc_dns_server.hpp"
#include "Argos/Argos_global.hpp"

/* ESP-IDF Components */
#include "freertos/idf_additions.h"

/* Third-party */
#include <cJSON.h>
#include <sys/types.h>
#include <memory>

/** Queues
  * @param network2ui_info_q  Queue that sends system info from network task to UI task for display
  * @param network2ui_state_q Queue that sends current stage of network task chain to UI task for display
  */
QueueHandle_t network2ui_info_q  = nullptr;
QueueHandle_t network2ui_state_q = nullptr;

/** Network Task Chain
 *  @brief  active_chain is a set of current exexuting task chain, its state and error (from last execution or current)
 */
NetworkTaskChain active_chain    = {};

QueueHandle_t filesys_q          = nullptr;
QueueHandle_t wifi_msg_q         = nullptr;


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
    /* Saving Profile */
    active_chain.state = NetworkTaskChain::ChainState::SavingProfile;

    /* get user context  */
    auto* ctx = reinterpret_cast<OnSaveHandlerCTX*>(req->user_ctx);
    LFS*  lfs = ctx->lfs; // LittleFS instance

    /* Limit profiles to 3 */
    {
        std::string profile_dir = std::string(lfs->base()) + "/profile";
        auto dir = std::unique_ptr<DIR, decltype(&closedir)>{
            opendir(profile_dir.c_str()), closedir
        };
        if (dir) {
            int count = 0;
            struct dirent* entry;
            while ((entry = readdir(dir.get()))) {
                if (!std::string_view{entry->d_name}.ends_with(".txt")) continue;
                count++;
            }
            if (count >= 3) {
                httpd_resp_send(req, "Maximum profiles reached", HTTPD_RESP_USE_STRLEN);
                /* Error: Profile slot is full */
                active_chain.error = NetworkTaskChain::ChainError::ProfileSlotFull;
                xQueueSend(network2ui_state_q, &active_chain, 0);
                xSemaphoreGive(isProfileLoaded); // Unblock network task to exit captive portal
                return ESP_OK;
            }
        }
    }

    /* buffer to receive raw body of POST request */
    char buf[256];
    int ret = httpd_req_recv(req, buf, req->content_len);
    if (ret <= 0) return ESP_FAIL;
    buf[ret] = '\0';

    /* Fill up profile struct direct from POST body */
    Profile profile = {};
    httpd_query_key_value(buf, "ssid",         profile.ssid,         sizeof(profile.ssid));
    httpd_query_key_value(buf, "password",     profile.password,     sizeof(profile.password));
    httpd_query_key_value(buf, "ntp_server",   profile.ntp_server,   sizeof(profile.ntp_server));
    httpd_query_key_value(buf, "profile_name", profile.profile_name, sizeof(profile.profile_name));

    ESP_LOGI(TAG, "Captive portal received SSID: %s", profile.ssid);

    /* Save profile under /profile with format <profile_name>.txt */
    std::string profile_path = "/profile/" + std::string(profile.profile_name) + ".txt";
    lfs->write(profile_path.c_str(), &profile, sizeof(profile));
    httpd_resp_send(req, "Config received", HTTPD_RESP_USE_STRLEN);
    xQueueSend(network2ui_state_q, &active_chain, 0);
    xSemaphoreGive(isProfileLoaded);
    return ESP_OK;
}

/** 
 * Task   network_task
 * @brief Main network task for handling WiFi, DNS, and HTTP operations
 * @note  This task will run after the UI task is initialized
*/

void network_task(void *arg)
{

    /************************************************************/
    /*      Long-lasting objects - Lifetime = TaskTime          */
    /************************************************************/

    LFS vault;   
    vault.mkdir("/profile");
    ESP_LOGI(TAG,"LittleFS mounted at: %s", vault.base().data()); 
        
    WIFI wifi;
    wifi.setMsgQueue(wifi_msg_q);
    ESP_LOGI(TAG,"WiFi module is initialized but no mode is set yet");
    
    std::string ip;
    PageMsg ui_msg;

    /** Chain Objects
     *  @brief These objects are used in task chains, but their lifetime should be long-latsing
     *         across different iterations of chain execution and do only once construction
     *  @note  All of these objects resets when leaving their chain
     */
    std::unique_ptr<HttpClient> http_client;
    std::unique_ptr<mDNS>       mdns;
    std::unique_ptr<DNServer>   dns;
    std::unique_ptr<HttpServer> server;
    OnSaveHandlerCTX save_ctx = { .lfs = &vault };

    /************************************************************/
    /*                       FSM Loop                           */
    /************************************************************/

    using enum PageCommand;
    using enum NetworkTaskChain::TaskChain;
    using enum NetworkTaskChain::ChainState;
    
    /** Queue Initialization
     *  @param network2ui_state_q Queue for sending current stage of network task chain
     *  @param network2ui_info_q Queue for sending parsed info of target device from network task to UI task
     *  @note  None of these queues' creation should be done inside a loop
     *         or it can cause memory leak
     */
    network2ui_state_q = xQueueCreate(3, sizeof(NetworkTaskChain));
    network2ui_info_q  = xQueueCreate(3, sizeof(ClientMsg));

    for(;;){

        /** Pt.1   Read commands from UI
         *  @brief Read commands sent from UI task to determine which task chain to execute
         *         by updating active_chain.name
         *  @note  This process will not be blocked by any chain excution
         *         so the UI task can interrupt current chain by sending a new command
         */
        
        if (xQueueReceive(ui2network_command_q, &ui_msg, 0) == pdTRUE) { 
            switch(ui_msg.command) {
                case LoadProfile:   {active_chain.name = StartLoadProfile; break;}
                case AddProfile:    {active_chain.name = StartAddProfile; break;}
                case DeleteProfile: {active_chain.name = StartDeleteProfile; break;}
                /* Other not related commands */
                default: break;
            }
        }

        /** Pt.2   Execute task chain according to active_chain.name
         *  @brief A chain is a series of steps to achieve a specific goal of network operation
         *  @note  Every chain ends with entering a new chain, whether Idle or another chain
         *         the chain operation will eventually ends up with Idle or ReceivingSysInfo stage
         *         All chain is interruptible by UI task sending a new command to start a different chain
         */
        
        /* Clean up resources when leaving their owning chain */
        if (active_chain.name != ReceivingSysInfo) http_client.reset();
        if (active_chain.name != StartQueryTarget) mdns.reset();
        if (active_chain.name != StartAddProfile) {
            dns.reset();
            server.reset();
        }

         /** Idle Chain
         *  @brief An idle chain does do nothing
         *  @note  Queue will only be sent at entry from other chain calling resetToIdle()
         *         which resets chain state and enable idle_queue_sent flag to allow sending idle state to UI task.
         *         The first entry will also trigger the queue to send
         */
        if(active_chain.name == Idle) {
           active_chain.state = NoEvent;
           if(!active_chain.idle_queue_sent){
             xQueueSend(network2ui_state_q, &active_chain, 0);
             active_chain.idle_queue_sent = true;
           }
        }
        /** Start Add Profile task chain 
         *  @brief Set wifi to SoftAP, launch Captive Poratl and save profiles
         */
        else if(active_chain.name == StartAddProfile) {
            /* Set wifi to softAP */
            if(wifi.getCurrentMode() != WIFI::Mode::SoftAP)
                wifi.start(WIFI::Mode::SoftAP, "Argos", "clairvoyance");

            /* Lazy-init captive portal services */
            if (!dns) {
                dns = std::make_unique<DNServer>();
                dns->startRedirection();
            }
            if (!server) {
                server = std::make_unique<HttpServer>(
                    HttpServer::Mode::CaptivePortal,
                    HttpServer::FileSys::LittleFS);
                server->registerLittleFS(&vault);
                server->makeRootDir();
                server->saveWeb(HttpServer::ROOT_NAME,
                               CAPTIVE_PORTAL_HTML.c_str(),
                               CAPTIVE_PORTAL_HTML.length());
                server->registerURI("/save", HTTP_POST, http_on_save_handler, &save_ctx);
                server->start();
            }

            /* Captive Portal Launched */
            active_chain.state = CaptivePortalLaunched;
            xQueueSend(network2ui_state_q, &active_chain, 0);
            if (!isProfileLoaded)
                isProfileLoaded = xSemaphoreCreateBinary();

            /* wait for on_save_handler to be called    */
            /* and call xSemaphoreGive(isProfileLoaded) */

            xSemaphoreTake(isProfileLoaded, portMAX_DELAY);

            /* Chain completed */
            active_chain.resetToIdle();
        }
        /** Start Load Profile task chain
         *  @brief Load profile from LittleFS according to profile name, connect to WiFi in STA mode, start mDNS and query target device IP
         */
        else if (active_chain.name == StartLoadProfile) {
            /* Get profile name from UI message payload */
            auto* p = std::get_if<ProfilePayload>(&ui_msg.payload);
            if (!p) {
                active_chain.resetToIdle();
                continue;
            }
            /* Calculate profile path */
            std::string profile_to_load_path = "/profile/" + std::string(p->profile_name) + ".txt";
            /* Loading Profile */
            active_chain.state = LoadingProfile;
            xQueueSend(network2ui_state_q, &active_chain, 0);

            Profile profile = {};
            auto f = std::unique_ptr<FILE, decltype(&fclose)>{
                vault.read(profile_to_load_path.c_str()), fclose
            };
            if (!f) {
                active_chain.resetToIdle();
                continue;
            }
            fread(&profile, sizeof(profile), 1, f.get());

            /* WiFi switch to STA mode and try to connect */
            wifi.start(WIFI::Mode::Station, profile.ssid, profile.password);
            active_chain.state = WaitingWiFi;
            xQueueSend(network2ui_state_q, &active_chain, 0);
            active_chain.name = StartQueryTarget;
        }
        else if (active_chain.name == StartQueryTarget) {
            static uint8_t retry_count = 0;
            static constexpr uint8_t MAX_RETRY = 5;

            /* Wait for WiFi connection before starting mDNS */
            if (active_chain.state == WaitingWiFi) {
                auto conn = wifi.getCurrentConnection();
                if (conn.state == WIFI::WifiMsg::State::Connected) {
                    /* WiFi ready: init mDNS and begin querying */
                    active_chain.state = TargetingDevice;
                    xQueueSend(network2ui_state_q, &active_chain, 0);
                    retry_count = 0;
                    mdns = std::make_unique<mDNS>();
                    mdns->start();
                }
                else if (conn.state == WIFI::WifiMsg::State::Failed) {
                    active_chain.error = NetworkTaskChain::ChainError::WiFiConnectFailed;
                    xQueueSend(network2ui_state_q, &active_chain, 0);
                    active_chain.resetToIdle();
                }
                /* else: still connecting, wait */
                continue;
            }

            /* Guard: UI interruption may have destroyed mdns */
            if (!mdns) {
                active_chain.state = WaitingWiFi;
                continue;
            }

            ip = mdns->queryForIP(TARGET_HOSTNAME);

            if (!ip.empty()) {
                active_chain.name = ReceivingSysInfo;
            }
            else if (++retry_count >= MAX_RETRY) {
                active_chain.error = NetworkTaskChain::ChainError::TargetNotFound;
                xQueueSend(network2ui_state_q, &active_chain, 0);
                active_chain.resetToIdle();
    
            }
            /* else: stay in StartQueryTarget, retry next tick */
        }
        /** Chain Stage: Receiving System Info
         * 
         *  @note This stage won't exit unless is being interrupted by UI 
         */
        else if (active_chain.name == ReceivingSysInfo) {
            if (!http_client)
                http_client = std::make_unique<HttpClient>(ip.c_str());

            HttpClient::Msg msg = http_client->Get_ManualPerform();

            /* Check if the request is valid and start parsing */
            if(msg.status_code == 200 && !msg.body.empty()) {
                auto root_handle = std::unique_ptr<cJSON, decltype(&cJSON_Delete)>{
                    cJSON_Parse(msg.body.c_str()), cJSON_Delete
                };
                cJSON *root = root_handle.get();
                if(root) {
                    ClientMsg info;
                    /* Host name */
                    cJSON *host_name = cJSON_GetObjectItem(root, "host_name");
                    if (cJSON_IsString(host_name)) {
                        strlcpy(info.host_name, host_name->valuestring, sizeof(info.host_name));
                    }
                    /* Operating System */
                    cJSON *os = cJSON_GetObjectItem(root, "os");
                    if (cJSON_IsString(os)) {
                        strlcpy(info.os, os->valuestring, sizeof(info.os));
                    }
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
                    /* Disk */
                    cJSON *disk_total = cJSON_GetObjectItem(root, "disk_total_mb");
                    if (cJSON_IsNumber(disk_total)) {
                        info.disk_total = disk_total->valueint;
                    }
                    cJSON *disk_used = cJSON_GetObjectItem(root, "disk_used_mb");
                    if (cJSON_IsNumber(disk_used)) {
                        info.disk_used = disk_used->valueint;
                    }
                    cJSON *disk_usage = cJSON_GetObjectItem(root, "disk_percent");
                    if (cJSON_IsNumber(disk_usage)) {
                        info.disk_usage = (float)disk_usage->valuedouble;
                    }
                    /* Send parsed info to UI */
                    active_chain.state = NetworkTaskChain::ChainState::Succeed;
                    xQueueSend(network2ui_info_q, &info, 0);
                }
            }
            else {
               active_chain.error = NetworkTaskChain::ChainError::TargetLost;
               xQueueSend(network2ui_state_q, &active_chain, 0);
               active_chain.resetToIdle();
   
            }
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

//     /* SNTP Time Sync */
//     const char *ntp = profile.ntp_server[0] ? profile.ntp_server : "pool.ntp.org";
//     ESP_LOGI(TAG,"Launching SNTP Time Sync with server: %s", ntp);
//     SNTP sntp(ntp);
//     sntp.setTimezone("CST-8");
//     sntp.sync();