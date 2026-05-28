
#pragma once

/* Includes */
#include "network.hpp"

/* Public includes */
#include "ddc.hpp"
#include "../network.hpp"
#include "devices/ddc_encoder.hpp"
#include "Argos_icons.hpp"
#include <variant>


/* Public Structure def */

struct SystemState
{
    /** Messages
     *  @param wifi_msg    sent from wifi class   - wift connection status & wifi SSID
     *  @param system_info sent from network task - parsed system infos
     *  @param input_event sent from encoder task - encoder events
     */
          SystemInfoMsg system_info;
    NetworkTaskStateMsg network_msg;
    Encoder::EncoderMsg enc_msg;
    
    std::array<std::array<char,64>,3> profile_list;

    /** UI State
     *  @param focus_tab    shows which root page is focused in preview mode
     *  @param curr_time    stores current time to be displayed in the UI overlay
     *  @param isEnterStack tells if a page stack is entered
     */
    uint8_t focus_tab = 1;      
    char curr_time[9] = "00:00:00"; 
    bool isEnterStack = false;
    
    /** Update Flags
     *  @brief Update only when you need it to
     *  @param if2UpdateProfileList you won't want to read from filesys frequently.
     */
     bool if2UpdateProfileList = false;
};


enum class PageCommand : uint8_t
{
    // Reserved for page stack control
    PC_None,
    PC_Enter,
    PC_Exit,
    // To network task
    PC_LoadProfile,
    PC_AddProfile,
    // Reserved
    PC_DeleteProfile,
};

// Load & Delete profile <profile_name>
struct ProfilePayload { char profile_name[64]; };

using Payload = std::variant<
    std::monostate,
    ProfilePayload
>;

struct UIMsg {
    PageCommand command = PageCommand::PC_None;
        Payload payload = std::monostate{};
};

struct HWINFO // Hardware info
    {
        static constexpr uint8_t WIDTH  = 255;
        static constexpr uint8_t WEIGHT = 64;
    };

    struct STATIC // static element placement
    {
        struct NAV
        {
            static constexpr uint8_t NAV_BAR_WIDTH  = 255;
            static constexpr uint8_t NAV_BAR_HEIGHT = 13;
        };
   
        struct TITLE
        {
            static constexpr uint8_t GAP_FROM_LEFT      = 3;
            static constexpr uint8_t GAP_FROM_BUTTOM    = 3;
            static constexpr uint8_t GAP_FROM_FIRST_TAG = 13;
        };

        struct TABS
        {
            static constexpr uint8_t GAP_BETWEEN = 12;
            static constexpr uint8_t TIME_X = 204;
        };

        struct ICON
        {
            static constexpr uint8_t SIZE_S = 32; // small
            static constexpr uint8_t SIZE_M = 40; // medium
        };

        struct PAGE
        {
            static constexpr uint8_t TEXT_GAP_FROM_LEFT = 5;
            static constexpr uint8_t LINE1 = 29;
            static constexpr uint8_t LINE2 = 42;
            static constexpr uint8_t LINE3 = 55;
            static constexpr uint8_t LINE4 = 60;
        };
    };

    struct FONT
    {
        static constexpr uint8_t* BASE_FONT = (uint8_t*)u8g2_font_profont11_tr;
    };

    enum class PencilMode : uint8_t
    {
        Hollow = 0,
        Solid  = 1,
        Invert = 2
    };

/**
 * @brief Set the drawing mode for subsequent drawing operations.
 */
inline void setPencilMode(u8g2_t* u8g2, PencilMode mode) {
    uint8_t _mode = static_cast<uint8_t>(mode);
    u8g2_SetDrawColor(u8g2, _mode);
}

template<uint8_t FRAME_COUNT>
class Animation {
    public:
    Animation() = default;
    ~Animation() = default;


    bool update(uint32_t interval_ms) {
        if(isRunning) return false;
        uint32_t curr_tick = static_cast<uint32_t>(esp_timer_get_time() / 1000);
        if(last_tick == 0) {
            last_tick = curr_tick;
            return false;
        }
        if (curr_tick - last_tick >= interval_ms) {
            frame_index = (frame_index + 1) % FRAME_COUNT;
            last_tick = curr_tick;
            return true;
        }
        return false; 
    }

    bool update() {
        if (!isRunning) return false;
        uint32_t now = static_cast<uint32_t>(esp_timer_get_time() / 1000); 
        
        if (last_tick == 0) {
            last_tick = now;
            return false;
        }

        if (now - last_tick >= frame_durations[frame_index]) {
            frame_index = (frame_index + 1) % FRAME_COUNT;
            last_tick = now;
            return true;
        }
        return false;
    }

    void set_durations(std::initializer_list<uint32_t> list){
        uint8_t i = 0;
        for (uint32_t d : list)
        {
            if (i >= FRAME_COUNT) break;
            frame_durations[i++] = d;
        }
    }

    void reset() {
        frame_index = 0;
        last_tick = static_cast<uint32_t>(esp_timer_get_time() / 1000);
        isRunning   = true;
    }

    void stop()  { isRunning = false; }
    void start() { isRunning = true;  }
    uint8_t current() const { return frame_index; }

    private:
    uint8_t  frame_index = 0;
    uint32_t last_tick   = 0;
    bool     isRunning   = true;
    uint32_t frame_durations[FRAME_COUNT] = {};
};


