#pragma once

/* ESP-IDF Components */
#include "freertos/idf_additions.h"
#include "ddc.hpp"

/* C/C++ Libraries */
#include <memory>
#include <string>
#include <variant>

#include "Argos/Argos_global.hpp"

class LFS;
class DNServer;
class HttpServer;
class mDNS;
class HttpClient;

extern QueueHandle_t network2ui_info_q;
extern QueueHandle_t network2ui_state_q;

struct ClientMsg
{
    char host_name[32]  = {};
    char os[32]         = {};
    char os_distro[64]  = {};

    int   cpu_core_freq = 0;
    int   cpu_cores     = 0;
    int   cpu_threads   = 0;
    float cpu_usage     = 0.0f;
    float cpu_temp      = 0.0f;

    int   mem_total     = 0;
    int   mem_used      = 0;
    float mem_usage     = 0.0f;

    int   disk_total    = 0;
    int   disk_used     = 0;
    float disk_usage    = 0.0f;
};

struct Profile
{
    char ssid[16]          = {};
    char password[32]      = {};
    char ntp_server[32]    = {};
    char profile_name[16]  = {};
};

/* ================================================================ */
/*                        NetworkTask                               */
/* ================================================================ */

class NetworkTask {
public:
    /* ---- Chain States (each owns its own data) ---- */

    struct ChainIdle {
        bool queue_sent = false;
    };

    struct OnSaveHandlerCTX { LFS* lfs; };

    struct ChainAddProfile {
        LFS*                        vault;
        std::unique_ptr<DNServer>   dns;
        std::unique_ptr<HttpServer> server;
        OnSaveHandlerCTX            save_ctx;
        SemaphoreHandle_t           semaphore = nullptr;
    };

    struct ChainLoadProfile {
        std::string profile_name;
    };

    struct ChainDeleteProfile {
        std::string profile_name;
    };

    struct ChainQueryTarget {
        std::string             ip;
        std::unique_ptr<mDNS>   mdns;
        uint8_t                 retry_count = 0;
        static constexpr uint8_t MAX_RETRY = 5;
    };

    struct ChainReceiveInfo {
        std::string                 ip;
        std::unique_ptr<HttpClient> client;
    };

    using Chain = std::variant<
        ChainIdle,
        ChainAddProfile,
        ChainLoadProfile,
        ChainDeleteProfile,
        ChainQueryTarget,
        ChainReceiveInfo
    >;

    /* ---- Public API ---- */
    void run();

private:
    /* Shared long-lived resources */
    LFS     _vault;
    WIFI    _wifi;
    PageMsg _ui_msg;

    Chain _chain = ChainIdle{};

    /* Command dispatch */
    Chain _handleCommand(const PageMsg& msg);

    /* Chain tick — each returns the next chain */
    Chain _tick(ChainIdle&);
    Chain _tick(ChainAddProfile&);
    Chain _tick(ChainLoadProfile&);
    Chain _tick(ChainDeleteProfile&);
    Chain _tick(ChainQueryTarget&);
    Chain _tick(ChainReceiveInfo&);
};

void network_task(void *arg);