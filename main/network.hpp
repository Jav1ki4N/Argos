
# pragma once
#include "freertos/idf_additions.h"

/** Network Globals
 *  @brief Tyepdef and structs needed to be accessed by both Network Tasks and Other Tasks
 */

extern QueueHandle_t network2ui_state_q;
extern QueueHandle_t network2ui_info_q;

void network_task(void *arg);

struct Profile
{
    char ssid[16]          = {};
    char password[32]      = {};
    char ntp_server[32]    = {};
    char profile_name[16]  = {};
};

/** System Info Message
 *  @brief System info parsed from JSON received from target device
 *  @note  Sent from Network Task to UI Task 
 */
 struct SystemInfoMsg
 {
     char host_name[16]  = {};
     char        os[16]  = {};
     char os_distro[64]  = {};

     int   cpu_core_freq = 0;
     int   cpu_cores     = 0;
     int   cpu_threads   = 0;
     float cpu_usage     = 0.0f;
     float cpu_temp      = 0.0f;

     int   mem_total     = 0;
     int   mem_used      = 0;
     float mem_usage     = 0.0f;

     float disk_total_gb = 0.0f;
     float disk_used_gb  = 0.0f;
     float disk_usage    = 0.0f;
 };

 /** Network Task State
  *  @brief State of current network task stage
  *  @note  This should correspond to the network task chain defs in network task class
  */
struct NetworkTaskStateMsg{
    enum class ChainError : uint8_t{
        E_None,
        E_FailedToOpenProfile,
        E_FailedToReadProfile,
        E_FailedToConnectWiFi,
        E_TargetNotFound,
        E_TargetLost,
        E_FailedToParseInfo,
        E_ProfileSlotFull
    }error;

    enum class ChainStage : uint8_t{
        CS_Idle,
        CS_LoadingProfile,
        CS_TargetDiscovery,
        CS_ReceivingInfo,
        /* Add profile stages */
        CS_AddProfile,
        CS_CaptivePortal,
        CS_SavingProfile // set in handler
    }chain_stage;

    enum class WiFiState : uint8_t{
        WS_Offline,
        WS_STAConnecting,
        WS_STAConnected,
        WS_AP
    }wifi_state;

    char wifi_ssid[32] = {};
};