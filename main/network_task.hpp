
#pragma once 

/* Inlcudes */

/* Global */
#include "cJSON.h"
#include "freertos/idf_additions.h"
#include "ddc.hpp"
#include "network/ddc_http_client.hpp"
#include "network/ddc_mdns.hpp"
#include "network.hpp"
#include "Argos/Argos_global.hpp"
#include "web/root_html.hpp"

#include <atomic>
#include <variant>
#include <memory>

class NetworkTask {
    public:

    NetworkTask() = default;
    ~NetworkTask() = default;

    /** Network Task Chains
     *  @brief Each chain represents a sequence of operations for a specific network-related task
     */
    struct ChainIdle{
        bool send_queue = true; // first entry will trigger queue to send
    };

    /** Load Profile Chain
     *  @brief Chain for loading and processing profile data
     */
    struct ChainLoadProfile {
        std::string profile_to_load_name;
        std::string ntp_server;
        bool waiting_wifi = false;
    };

    struct ChainAddProfile {
       std::unique_ptr<DNServer> dns;
       std::unique_ptr<HttpServer> server;
       // bool isSaved = false; --> moved to class member so that handler can set it
    };

    struct ChainTargetDiscovery {
        static constexpr uint8_t MAX_RETRY = 5;
        uint8_t retry_count = 0;
        std::unique_ptr<mDNS> mdns;
        std::unique_ptr<SNTP> sntp;
        std::string ntp_server;
    };

    struct ChainReceiveInfo {
        std::string ip;
        std::unique_ptr<HttpClient> http_client;
    };

    using Chain = std::variant<std::monostate,
                               ChainIdle,
                               ChainLoadProfile,
                               ChainAddProfile,
                               ChainTargetDiscovery,
                               ChainReceiveInfo>;

    void init()
    {
        network2ui_state_q = xQueueCreate(3, sizeof(NetworkTaskStateMsg));
        network2ui_info_q  = xQueueCreate(3, sizeof(SystemInfoMsg));
        vault.mkdir("/profile");

    }

    QueueHandle_t getStateQueue() { return network2ui_state_q; }
    QueueHandle_t getInfoQueue()  { return network2ui_info_q;  }

    void dispatcher(QueueHandle_t ui2network_command_q) {
        UIMsg ui_msg = {};
        if (xQueueReceive(ui2network_command_q, &ui_msg, 0) != pdTRUE) return;
        switch (ui_msg.command) {

            case PageCommand::PC_LoadProfile:  {                                                 // start LoadProfile chain
                auto* payload = std::get_if<ProfilePayload>(&ui_msg.payload);
                if(payload) active_chain = ChainLoadProfile{payload->profile_name};
                break;
            }
            case PageCommand::PC_AddProfile: { active_chain = ChainAddProfile{}; isSaved = false; break; }
            default:break;
        }
    }

    void tick(QueueHandle_t ui2network_command_q) {
        dispatcher(ui2network_command_q);
        std::visit([this](auto& c) { execute(c); }, active_chain);
        applyChain();
    }

    private:
    /* Messages to send and their queue handles */
    SystemInfoMsg       sys_info_msg = {};
    QueueHandle_t network2ui_info_q  = nullptr;

    NetworkTaskStateMsg    state_msg = {};
    QueueHandle_t network2ui_state_q = nullptr;
    
    std::string_view TARGET_HOSTNAME = "argos-target";
    std::string_view AP_SSID = "Argos";
    std::string_view AP_PASSWORD = "clairvoyance";
    static constexpr uint8_t MAX_PROFILE = 3;

    std::atomic<bool> isSaved = false;

    /* Chain Helper Functions */
    void queueSendNetworkState() {
        xQueueSend(network2ui_state_q, &state_msg, 0);
    }

    void queueSendSystemInfo() {
        xQueueSend(network2ui_info_q, &sys_info_msg, 0);
    }

    void setError(NetworkTaskStateMsg::ChainError error) {
        state_msg.error = error;
        state_msg.chain_stage = NetworkTaskStateMsg::ChainStage::CS_Idle;
        state_msg.wifi_state  = NetworkTaskStateMsg::WiFiState::WS_Offline;
        state_msg.wifi_ssid[0] = '\0';
        pending_chain = ChainIdle{};
    }

    void applyChain() {
        if (!std::holds_alternative<std::monostate>(pending_chain)) {
            active_chain = std::move(pending_chain);
            pending_chain = {};
        }
    }

    static void jsonGetStr(cJSON* root, const char* key, char* dest, size_t n)
    { auto* item = cJSON_GetObjectItem(root, key); if (cJSON_IsString(item)) strlcpy(dest, item->valuestring, n); }

    static void jsonGetInt(cJSON* root, const char* key, int& dest)
    { auto* item = cJSON_GetObjectItem(root, key); if (cJSON_IsNumber(item)) dest = item->valueint; }

    static void jsonGetFloat(cJSON* root, const char* key, float& dest)
    { auto* item = cJSON_GetObjectItem(root, key); if (cJSON_IsNumber(item)) dest = (float)item->valuedouble; }

    /** HTTP Handler for Saving Profile
     * @brief This handler is called when the captive portal receives a POST request to "/save"
     * @note  Static version is the onr registered to the HTTP server, which then calls the non-static version with the correct context
     */
    static esp_err_t static_http_on_save_handler(httpd_req_t *req) {
        NetworkTask* self = reinterpret_cast<NetworkTask*>(req->user_ctx);
        return self->http_on_save_handler(req);
    }

    esp_err_t http_on_save_handler(httpd_req_t *req) {
        using enum NetworkTaskStateMsg::ChainStage;
        using enum NetworkTaskStateMsg::ChainError;
        using enum NetworkTaskStateMsg::WiFiState;
        state_msg.chain_stage = CS_SavingProfile;
        queueSendNetworkState();

        std::string profile_dir = std::string(vault.base()) + "/profile";              // Check existing profiles in LittelFS
        auto dir = std::unique_ptr<DIR, decltype(&closedir)>{ 
            opendir(profile_dir.c_str()), closedir
        };
        if (dir) {
            uint8_t profile_count = 0;
            while(dirent* entry = readdir(dir.get())) {
                if (!std::string_view{entry->d_name}.ends_with(".txt"))continue;          // count only .txt files as profiles                                    
                profile_count++;
            }
            if(profile_count >= MAX_PROFILE) {
                httpd_resp_send(req, "Maximum profiles reached", HTTPD_RESP_USE_STRLEN);
                state_msg.error = E_ProfileSlotFull;
                isSaved = true;                                                              // notify main loop, which will transition to Idle
                return ESP_OK;
            }
        }
        char buf[256];
        int ret = httpd_req_recv(req, buf, req->content_len);                            // receive POST body
        if (ret <= 0) return ESP_FAIL;
        buf[ret] = '\0';

        Profile profile = {};
        httpd_query_key_value(buf, "ssid",         profile.ssid,         sizeof(profile.ssid));
        httpd_query_key_value(buf, "password",     profile.password,     sizeof(profile.password));
        httpd_query_key_value(buf, "ntp_server",   profile.ntp_server,   sizeof(profile.ntp_server));
        httpd_query_key_value(buf, "profile_name", profile.profile_name, sizeof(profile.profile_name));

        std::string profile_path = "/profile/" + std::string(profile.profile_name) + ".txt"; // Save profile to LittleFS
        vault.write(profile_path.c_str(), &profile, sizeof(profile));
        httpd_resp_send(req, "Config received", HTTPD_RESP_USE_STRLEN);

        isSaved = true;
        return ESP_OK;
    }

    /* Task chain instance & long-lasting objects */
    WIFI wifi;
    LFS vault;

    Chain active_chain  = ChainIdle{};
    Chain pending_chain = {};

    /* Chain Execution Functions */

    void execute(std::monostate&) {}

    /** Chain: Idle
     *  @brief Execute the idle chain
     *  @param chain The idle chain to execute
     */
    void execute(ChainIdle& chain) {
        if(chain.send_queue) {
            queueSendNetworkState();
            chain.send_queue = false;
        }
    }
    
    /** Chain: Load Profile
     *  @brief Load profile from LittleFS, connect WiFi in STA mode, then transition to TargetDiscovery
     *  @param chain The load profile chain to execute
     */
    void execute(ChainLoadProfile& chain) {
        using enum NetworkTaskStateMsg::ChainStage;
        using enum NetworkTaskStateMsg::ChainError;
        using enum NetworkTaskStateMsg::WiFiState;
        if(!chain.waiting_wifi){
            state_msg.chain_stage = CS_LoadingProfile;
            state_msg.error       = E_None;
            state_msg.wifi_state  = WS_Offline;
            state_msg.wifi_ssid[0] = '\0';
            queueSendNetworkState();
            std::string path = "/profile/" + chain.profile_to_load_name + ".txt"; // get path
            auto file = std::unique_ptr<FILE, decltype(&fclose)>{                 // read path
                vault.read(path.c_str()), fclose                                  // get file
            };
            if(!file) {                                                           // check file
                setError(E_FailedToOpenProfile);
                queueSendNetworkState();
                return;  
            }                               
            Profile loaded_profile = {};
            if(!fread(&loaded_profile, sizeof(loaded_profile), 1, file.get())) {  // load profile
                setError(E_FailedToReadProfile);
                queueSendNetworkState();
                return;
            }

            chain.ntp_server = loaded_profile.ntp_server;
            wifi.start(WIFI::Mode::Station, loaded_profile.ssid,                  // Connect to WiFi
                                            loaded_profile.password);
            strlcpy(state_msg.wifi_ssid, loaded_profile.ssid, sizeof(state_msg.wifi_ssid));
            chain.waiting_wifi = true;                                            // set waiting wifi flag
        }
        else {                                                                    // waiting for wifi connection
            using enum WIFI::WifiMsg::State;
            WIFI::WifiMsg::State wifi_state = wifi.getCurrentState();
            if(wifi_state == Connected || wifi_state == Failed) {                 // conncetion result
                state_msg.wifi_state = (wifi_state == Connected) ? WS_STAConnected : 
                                                                   WS_Offline;
                
                if(wifi_state == Failed)setError(E_FailedToConnectWiFi);
                else pending_chain = ChainTargetDiscovery{.ntp_server = chain.ntp_server};
                queueSendNetworkState();
            }
            else{                                                                 // still connecting
                state_msg.wifi_state = WS_STAConnecting;
                queueSendNetworkState();
            }
        }
    }

    /** Chain: Target Discovery
     *  @brief Query target IP via mDNS, transition to ReceiveInfo on success or Idle on max retry
     *  @param chain The target discovery chain to execute
     */
    void execute (ChainTargetDiscovery& chain) {
        using enum NetworkTaskStateMsg::ChainStage;
        using enum NetworkTaskStateMsg::ChainError;
        using enum NetworkTaskStateMsg::WiFiState;
        if(!chain.sntp) {
            chain.sntp = std::make_unique<SNTP>("pool.ntp.org");                 // Create SNTP instance if not exist
            chain.sntp->sync();                                                     // Sync time (blocking)
        }
        if(!chain.mdns) {                                                         // Create mDNS instance if not exist
            chain.mdns = std::make_unique<mDNS>();
            chain.mdns->start();
            state_msg.error       = E_None;
            state_msg.chain_stage = CS_TargetDiscovery;
            queueSendNetworkState();
        }
        std::string ip = chain.mdns->queryForIP(TARGET_HOSTNAME.data());

        if(!ip.empty()) {                                                        // Target found
            pending_chain = ChainReceiveInfo{std::move(ip), nullptr};
            queueSendNetworkState();
            return;
        }
        else if(++chain.retry_count >= chain.MAX_RETRY) {                        // Retry limit reached
            setError(E_TargetNotFound);
            queueSendNetworkState();
            return;
        }
    }

    /** Chain: Receive Info
     *  @brief Poll target device via HTTP for system info, parse JSON, send to UI. On failure fall back to Idle
     *  @param chain The receive info chain to execute
     */
    void execute(ChainReceiveInfo& chain){
        using enum NetworkTaskStateMsg::ChainStage;
        using enum NetworkTaskStateMsg::ChainError;
        using enum NetworkTaskStateMsg::WiFiState;
        if(!chain.http_client) {
            chain.http_client = std::make_unique<HttpClient>(chain.ip.c_str());  // Create HTTP client instance if not exist
            state_msg.chain_stage = CS_ReceivingInfo;
            state_msg.error       = E_None;
            queueSendNetworkState();
        }

        HttpClient::Msg msg = chain.http_client->Get_ManualPerform();            // Perform HTTP GET request
        if (msg.status_code == 200 && !msg.body.empty()) {
            auto root_handler = std::unique_ptr<cJSON, decltype(&cJSON_Delete)>{ // Parse JSON response
                cJSON_Parse(msg.body.c_str()), cJSON_Delete
            };
            cJSON* root = root_handler.get();
            if(root) {
                jsonGetStr  (root, "host_name",     sys_info_msg.host_name,     sizeof(sys_info_msg.host_name));
                jsonGetStr  (root, "os",            sys_info_msg.os,            sizeof(sys_info_msg.os));
                jsonGetStr  (root, "os_version",    sys_info_msg.os_distro,     sizeof(sys_info_msg.os_distro));
                jsonGetFloat(root, "cpu_percent",   sys_info_msg.cpu_usage);
                jsonGetInt  (root, "cpu_cores",     sys_info_msg.cpu_cores);
                jsonGetInt  (root, "cpu_threads",   sys_info_msg.cpu_threads);
                jsonGetInt  (root, "cpu_freq_mhz",  sys_info_msg.cpu_core_freq);
                jsonGetFloat(root, "cpu_temp",      sys_info_msg.cpu_temp);
                jsonGetInt  (root, "mem_total_mb",  sys_info_msg.mem_total);
                jsonGetInt  (root, "mem_used_mb",   sys_info_msg.mem_used);
                jsonGetFloat(root, "mem_percent",   sys_info_msg.mem_usage);
                jsonGetInt  (root, "disk_total_mb", sys_info_msg.disk_total);
                jsonGetInt  (root, "disk_used_mb",  sys_info_msg.disk_used);
                jsonGetFloat(root, "disk_percent",  sys_info_msg.disk_usage);
                queueSendSystemInfo();
            }
            else {
                setError(E_FailedToParseInfo);                                              // JSON parsing failed
                queueSendNetworkState();
            }
        }
        else {
            setError(E_TargetLost);                                              // HTTP request failed, target lost
            queueSendNetworkState();
        }
    }

    void execute (ChainAddProfile& chain) {
        using enum NetworkTaskStateMsg::ChainStage;
        using enum NetworkTaskStateMsg::ChainError;
        using enum NetworkTaskStateMsg::WiFiState;
        state_msg.wifi_state  = WS_AP;
        strlcpy(state_msg.wifi_ssid, AP_SSID.data(), sizeof(state_msg.wifi_ssid));
        state_msg.chain_stage = CS_AddProfile;
        state_msg.error       = E_None;
        if (wifi.getCurrentMode() != WIFI::Mode::SoftAP) {
            wifi.start(WIFI::Mode::SoftAP, AP_SSID.data(), AP_PASSWORD.data());
            queueSendNetworkState();
            return;                                                                    // wait next tick for SoftAP ready
        }
        if (!chain.dns) {
            chain.dns = std::make_unique<DNServer>();
            chain.dns->startRedirection();
            return;                                                                    // wait next tick for init done
        }
        if (!chain.server) {
            chain.server = std::make_unique<HttpServer>(
                HttpServer::Mode::CaptivePortal,
                HttpServer::FileSys::LittleFS);
            chain.server->registerLittleFS(&vault);
            chain.server->makeRootDir();
            chain.server->saveWeb(HttpServer::ROOT_NAME,
                                   CAPTIVE_PORTAL_HTML.c_str(),
                                   CAPTIVE_PORTAL_HTML.length());
            chain.server->registerURI("/save", HTTP_POST,
                                      static_http_on_save_handler, this);
            chain.server->start();
            state_msg.chain_stage = CS_CaptivePortal;
            queueSendNetworkState();
            return;                                                                    // wait next tick for handler
        }
        if (isSaved) {
            esp_wifi_stop();
            state_msg.chain_stage = CS_Idle;
            state_msg.error       = E_None;
            state_msg.wifi_state  = WS_Offline;
            state_msg.wifi_ssid[0] = '\0';
            pending_chain = ChainIdle{};
            queueSendNetworkState();
        }
    }
};