
#pragma once

/* Public includes */
#include "ddc.hpp"
#include "../network.hpp"
#include "devices/ddc_encoder.hpp"
#include <variant>

/* Public Structure def */

struct SystemState
{
    /** Messages
     *  @param wifi_msg    sent from wifi class   - wift connection status & wifi SSID
     *  @param system_info sent from network task - parsed system infos
     *  @param input_event sent from encoder task - encoder events
     */
          WIFI::WifiMsg wifi_msg;
              ClientMsg system_info;
    Encoder::EncoderMsg input_event;

    /** UI State
     *  @param focus_tab    shows which root page is focused in preview mode
     *  @param curr_time    stores current time to be displayed in the UI overlay
     *  @param isEnterStack tells if a page stack is entered
     */
    uint8_t focus_tab = 1;      
    char curr_time[9] = "00:00:00"; 
    bool isEnterStack = false; 
};

enum class PageCommand : uint8_t
{
    None,
    // Network Page
    // Profile Page
    LoadProfile,
    AddProfile,
    DeleteProfile,
    // Info Page
    AddtoGraph,
    // About Page
};

// Load & Delete profile <profile_name>
struct ProfilePayload { char profile_name[64]; };

using Payload = std::variant<
    std::monostate,
    ProfilePayload
>;

struct PageMsg {
    PageCommand command;
        Payload payload;
};





