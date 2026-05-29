
#pragma once

/* Includes */
/* Framework Global */
#include "Argos_global.hpp"

/* Graphics */
#include "network.hpp"
#include "Argos_icons.hpp"
#include "u8g2.h"
#include <stdint.h>

using EncoderMsg = Encoder::EncoderMsg;

class ArgosPage
{
    public:

    ArgosPage() = default;
    virtual ~ArgosPage() = default;

    /** Virtual Methods
     *  @note  Takes u8g2 pointer and system state from framework
     *         the framework interacts with page through system state, and
     *         in return the page send back commands to ask framework to do complex operations   
     *  @param draw() draw content of this page, system state is made const 
     *                cuz a read-only function should not be able to write
     *  @param onEvent() handle encoder input events, behaved differently in each page 
    */

    virtual void  draw(u8g2_t* u8g2, const SystemState& systate) = 0; // impl required
    virtual void  onEnter(){}
    virtual void  onExit() = 0; // impl required
    virtual UIMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) = 0; // impl required

    protected:
    bool inStack   = false; // for root pages
    UIMsg makeEmptyMsg() const { UIMsg msg = {}; return msg; }
    private:
};

/* Network Page */
class NetworkPage : public ArgosPage
{
    public:
    NetworkPage () = default;
    ~NetworkPage() override = default;

    void draw(u8g2_t* u8g2, const SystemState& systate) override {
        using enum NetworkTaskStateMsg::WiFiState;
        using enum NetworkTaskStateMsg::ChainStage;
        using enum NetworkTaskStateMsg::ChainError;
        setPencilMode(u8g2, PencilMode::Solid);

        const ICON* wicons[] = {&ICON_WIFI_1, &ICON_WIFI_2, &ICON_WIFI_3}; // animation frames
        const ICON* icon;                                                  // frame to render
        auto ws = systate.network_msg.wifi_state;
        if      (ws == WS_Offline)      icon = &ICON_WIFI_NO_CONNECT;      // static
        else if (ws == WS_STAConnected) icon = &ICON_WIFI_CONNECTED;
        else {
            wifi_animation.update(300);                                    // dynamic, using animation 
            icon = wicons[wifi_animation.current()];
        } 
        
        uint8_t icon_x = 3 * STATIC::PAGE::TEXT_GAP_FROM_LEFT;             // placement of wifi icon
        uint8_t icon_y = (((HWINFO::WEIGHT - STATIC::NAV::NAV_BAR_HEIGHT) - icon->height) >> 1)
                       + STATIC::NAV::NAV_BAR_HEIGHT;
        u8g2_DrawXBMP(u8g2, icon_x, icon_y, icon->width, icon->height, icon->data);
        u8g2_DrawStr(u8g2, TEXT_X, STATIC::PAGE::LINE3,"Setting: [PROFILES]"); // profiles button
        if(inStack) {                                                          // highlight
            setPencilMode(u8g2, PencilMode::Invert);
            u8g2_DrawBox(u8g2, 
            TEXT_X + u8g2_GetStrWidth(u8g2,"Settings: ")-4,  
            (STATIC::PAGE::LINE3 - 8), 
            u8g2_GetStrWidth(u8g2,"[PROFILES]") - 4,  
            u8g2_GetFontAscent(u8g2) + u8g2_GetFontDescent(u8g2) + 4);
        }
        wifi_state  = systate.network_msg.wifi_state;                          // make hint in line2
        error       = systate.network_msg.error;
        chain_stage = systate.network_msg.chain_stage;
        makeHint(u8g2);
    }

    UIMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) override {
        UIMsg page_msg = {};
        if      ( msg == EncoderMsg::ButtonPressed ) page_msg.command = PageCommand::PC_Enter;
        else if ( msg == EncoderMsg::ButtonHeld )    page_msg.command = PageCommand::PC_Exit;
        return page_msg;
    }
    void onEnter() override {
        wifi_animation.stop();
        inStack = true;
    }
    void onExit() override {
        inStack = false;
    }

    private:

    NetworkTaskStateMsg::WiFiState  wifi_state;  // displayed in line 1 
    NetworkTaskStateMsg::ChainStage chain_stage; // displayed in line 2
    NetworkTaskStateMsg::ChainError error;       // displayed in line 2 fallback to IDLE 

    Animation<3> wifi_animation;                 // wifi connecting animation

    static constexpr std::string_view makeWifiHint(NetworkTaskStateMsg::WiFiState s) {
        using enum NetworkTaskStateMsg::WiFiState;
        switch (s) {
            case WS_Offline:        return "Network: [OFFLINE]";
            case WS_AP:             return "Network: [SOFTAP]";
            case WS_STAConnecting:  return "Network: [STA CONNECTING]";
            case WS_STAConnected:   return "Network: [STA CONNECTED]";
            default:                return "";
        }
    }

    static constexpr std::string_view makeChainHint(NetworkTaskStateMsg::ChainStage s) {
        using enum NetworkTaskStateMsg::ChainStage;
        switch (s) {
            case CS_Idle:            return "Status:  [IDLE]";
            case CS_LoadingProfile:  return "Status:  [LOADING PROFILE]";
            case CS_TargetDiscovery: return "Status:  [TARGET DISCOVERY]";
            case CS_ReceivingInfo:   return "Status:  [RECEIVING INFO]";
            case CS_AddProfile:      return "Status:  [ADDING PROFILE]";
            case CS_CaptivePortal:   return "Status:  [CAPTIVE PORTAL]";
            case CS_SavingProfile:   return "Status:  [SAVING PROFILE]";
            default:                 return "";
        }
    }

    static constexpr std::string_view makeErrorHint(NetworkTaskStateMsg::ChainError e) {
        using enum NetworkTaskStateMsg::ChainError;
        switch (e) {
            case E_FailedToOpenProfile: return "Failed:  [OPEN PROFILE]";
            case E_FailedToReadProfile: return "Failed:  [READ PROFILE]";
            case E_FailedToConnectWiFi: return "Failed:  [CONNECT WIFI]";
            case E_TargetNotFound:      return "Failed:  [TARGET NOT FOUND]";
            case E_TargetLost:          return "Failed:  [TARGET LOST]";
            case E_FailedToParseInfo:   return "Failed:  [PARSE INFO]";
            case E_ProfileSlotFull:     return "Failed:  [PROFILE FULL]";
            default:                    return "";
        }
    }

    /**
     *@brief Make hints in line2 of network page
     */
    void makeHint(u8g2_t *u8g2){
        u8g2_DrawStr(u8g2, TEXT_X, STATIC::PAGE::LINE1, makeWifiHint(wifi_state).data());
        u8g2_DrawStr(u8g2, TEXT_X, STATIC::PAGE::LINE2, 
            (chain_stage == NetworkTaskStateMsg::ChainStage::CS_Idle &&
                   error != NetworkTaskStateMsg::ChainError::E_None    ) ? makeErrorHint(error).data() : 
                                                                           makeChainHint(chain_stage).data());
    }

    static constexpr uint8_t TEXT_X = 75;
};

class ProfilePage : public ArgosPage
{
    public:
    ProfilePage () = default;
    ~ProfilePage() override = default;

    void draw(u8g2_t* u8g2, const SystemState& systate) override {
        setPencilMode(u8g2, PencilMode::Solid);
        u8g2_SetFont(u8g2, FONT::BASE_FONT);

        const uint8_t ascent  = u8g2_GetAscent(u8g2);
        const uint8_t descent = -u8g2_GetDescent(u8g2);

        /* Graph frame placement */
        const uint8_t frame_y = STATIC::PAGE::LINE1 - ascent - 2;        
        const uint8_t frame_h = (STATIC::PAGE::LINE3 - STATIC::PAGE::LINE1) + ascent + descent + 2;

        /* Empty slot placement */
        const uint8_t         space_width = u8g2_GetStrWidth(u8g2, " "); 
        const uint8_t  left_bracket_width = u8g2_GetStrWidth(u8g2, "[");
        const uint8_t right_bracket_width = u8g2_GetStrWidth(u8g2, "]");
        const uint8_t inner_bracket_width = FRAME_X - GAP - BRACKET_X - left_bracket_width - right_bracket_width;
        const uint8_t n = inner_bracket_width / space_width;
        const std::string spaces(n, ' ');
        const std::string bracket = "[" + spaces + "]";               // an empty slot string like "[     ]"

        /* Slot Area */
        for (uint8_t i = 0; i < Slot::MAX_NUM; ++i) {                 // draw 3 slots
            uint8_t y = STATIC::PAGE::LINE1 + i * (STATIC::PAGE::LINE2 - STATIC::PAGE::LINE1);
            u8g2_DrawStr(u8g2, BRACKET_X, y, bracket.c_str());        // draw empty slot

            std::string_view name = systate.profile_list[i].data();   // draw profile name if exists
            if (!name.empty()) {
                uint8_t name_width = u8g2_GetStrWidth(u8g2, name.data());
                uint8_t name_x = BRACKET_X + left_bracket_width + (inner_bracket_width - name_width) / 2;
                u8g2_DrawStr(u8g2, name_x, y, name.data());
            }

            if (mode == MenuMode::Slot && slot.cursor == i) {                                          // highlight
                setPencilMode(u8g2, PencilMode::Invert);
                u8g2_DrawBox(u8g2, BRACKET_X + 2,
                                   y - ascent - 1,
                                   FRAME_X - GAP - BRACKET_X - 4,
                                   ascent + descent);
                setPencilMode(u8g2, PencilMode::Solid);
            }
        }

        /* Option Area */
        u8g2_DrawFrame(u8g2, FRAME_X, frame_y, FRAME_WIDTH, frame_h);
        uint8_t divider_y   = frame_y + frame_h / 2;
        u8g2_DrawHLine(u8g2, FRAME_X + 2, divider_y, FRAME_WIDTH - 4);   // divider

        /* Draw slot ordinal name */
        static constexpr std::array<const char*, 3> ordinal = {"1st Slot", "2nd Slot", "3rd Slot"};
        uint8_t ordinal_width = u8g2_GetStrWidth(u8g2, ordinal[slot.cursor]); // draw slot ordinal name
        uint8_t ordinal_x     = FRAME_X + (FRAME_WIDTH - ordinal_width) / 2;
        uint8_t ordinal_y     = frame_y + frame_h / 4 + ascent / 2; 
        u8g2_DrawStr(u8g2, ordinal_x, ordinal_y , ordinal[slot.cursor]);

        /* Draw option buttons */
        uint8_t frame_bottom_mid = frame_y + frame_h * 3 / 4;
        uint8_t btn_y   = frame_bottom_mid + ascent / 2;
        uint8_t box_y   = frame_bottom_mid - ascent / 2 - 2;
        uint8_t box_h   = ascent + descent;

        if (systate.profile_list[slot.cursor].data()[0] == '\0') {
            uint8_t add_width = u8g2_GetStrWidth(u8g2, "[ADD]");
            uint8_t add_x     = FRAME_X + (FRAME_WIDTH - add_width) / 2;
            u8g2_DrawStr(u8g2, add_x, btn_y, "[ADD]");
            if (mode == MenuMode::Option) {
                setPencilMode(u8g2, PencilMode::Invert);
                u8g2_DrawBox(u8g2, add_x + 2, box_y, add_width - 4, box_h);
                setPencilMode(u8g2, PencilMode::Solid);
            }
        }
        else {
            uint8_t inner_frame_half_width = (FRAME_WIDTH - 4) / 2; 
            static constexpr std::array<const char*, 2> labels = {"[LOAD]", "[DEL]"};
            for (uint8_t i = 0; i < 2; ++i) {
                uint8_t btn_width = u8g2_GetStrWidth(u8g2, labels[i]);
                uint8_t btn_x     = FRAME_X + 2 + inner_frame_half_width * i + 
                                    (inner_frame_half_width - btn_width) / 2;
                u8g2_DrawStr(u8g2, btn_x, btn_y, labels[i]);
                if ((mode == MenuMode::Option && option.cursor == i)) {
                    setPencilMode(u8g2, PencilMode::Invert);
                    u8g2_DrawBox(u8g2, btn_x + 2, box_y, btn_width - 4, box_h);
                    setPencilMode(u8g2, PencilMode::Solid);
                }
            }
        }
    }

    UIMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) override {

        /* Check if current slot is empty */ 
        slot.isEmpty = (systate.profile_list[slot.cursor].data()[0] == '\0');

        /** Max index cursor can reach in a menu */
        uint8_t           limit = (mode == MenuMode::Slot) ? Slot::MAX_NUM : Option::MAX_NUM;
        uint8_t& cursor_to_move = (mode == MenuMode::Slot) ? slot.cursor : option.cursor;

        /** move cursor to next or previous slot/option
         * -disabled if is in empty slot's opyion menu
         */
        if     ( msg == EncoderMsg::RotateRight && (!(slot.isEmpty && mode == MenuMode::Option))) 
               { cursor_to_move = ((cursor_to_move + 1) % limit); }
        else if( msg == EncoderMsg::RotateLeft  && (!(slot.isEmpty && mode == MenuMode::Option))) 
               { cursor_to_move = (cursor_to_move + limit - 1) % limit; }

        else if(msg == EncoderMsg::ButtonPressed) {
            /* In slots enter option menu */
            if(mode == MenuMode::Slot) {
                mode = MenuMode::Option;
                option.cursor=0; // reset option cursor               
            }
            /** In option menu load or delete this profile
             *  - Empty slot only has an "Add" option
             */
            else {
                ProfilePayload payload;
                UIMsg msg = {};

                if(slot.isEmpty)msg.command = PageCommand::PC_AddProfile;
                else {
                    strlcpy(payload.profile_name, 
                            systate.profile_list[slot.cursor].data(), 
                            sizeof(payload.profile_name));
                    msg.command = (option.cursor == 0) ? PageCommand::PC_LoadProfile : 
                                                         PageCommand::PC_DeleteProfile;
                    msg.payload = payload;
                }
                /* auto exit  */
                mode = MenuMode::Slot;
                return msg;
            }
        }
        /* Exit */
        else if(msg == EncoderMsg::ButtonHeld) {
            /* In option menu,exit with doing nothing */
            if(mode == MenuMode::Option)mode = MenuMode::Slot;

            /* Exit Profile Page */
            else return UIMsg{PageCommand::PC_Exit, std::monostate{}};
        } 
        return makeEmptyMsg();
    }

    void onExit() override {
        /* Reset all states */
        mode          = MenuMode::Slot;
        slot.isEmpty  = false;
        slot.cursor   = 0;
        option.cursor = 0;
    }

    private:
    enum class MenuMode : uint8_t {
        Slot,
        Option
    }mode = MenuMode::Slot;
    static constexpr uint8_t BRACKET_X   = 2 * STATIC::PAGE::TEXT_GAP_FROM_LEFT;
    static constexpr uint8_t FRAME_X     = HWINFO::WIDTH / 2;
    static constexpr uint8_t FRAME_WIDTH = HWINFO::WIDTH - FRAME_X - STATIC::PAGE::TEXT_GAP_FROM_LEFT;
    static constexpr uint8_t GAP         = 2 * STATIC::PAGE::TEXT_GAP_FROM_LEFT;

    struct Slot{
        bool isEmpty = false;
        uint8_t cursor = 0;
        static constexpr uint8_t MAX_NUM = 3;
    }slot;
    struct Option{
        uint8_t cursor = 0;
        static constexpr uint8_t MAX_NUM = 2;
    }option;
};

class InfoPage : public ArgosPage
{
    public:
    InfoPage () = default;
    ~InfoPage() override = default;

    void draw(u8g2_t* u8g2, const SystemState& systate) override {
        setPencilMode(u8g2, PencilMode::Solid);
        u8g2_SetFont(u8g2, FONT::BASE_FONT);

        const uint8_t ascent  = u8g2_GetAscent(u8g2);
        const uint8_t descent = -u8g2_GetDescent(u8g2);
        const uint8_t frame_y = STATIC::PAGE::LINE1 - ascent - 2;
        const uint8_t frame_h = (STATIC::PAGE::LINE3 - STATIC::PAGE::LINE1) + ascent + descent + 2;
        const uint8_t max_px  = FRAME_X - GAP - LABEL_X;
        
        buildItems(systate.system_info);

        for (uint8_t i = 0; i < MAX_VISIBLE && (scroll + i) < MAX_LINES; ++i) {
            uint8_t idx = scroll + i;
            uint8_t y   = STATIC::PAGE::LINE1 + i * (STATIC::PAGE::LINE2 - STATIC::PAGE::LINE1);
            u8g2_DrawStr(u8g2, LABEL_X, y, lines[idx].data());
            if (systate.isEnterStack && (idx == cursor)) {
                setPencilMode(u8g2, PencilMode::Invert);
                u8g2_DrawRBox(u8g2, LABEL_X - 1, y - ascent - 1, max_px, ascent + descent + 1, 1);
                setPencilMode(u8g2, PencilMode::Solid);
            }
        }

        u8g2_DrawFrame(u8g2, FRAME_X, frame_y, FRAME_WIDTH, frame_h);

        const uint8_t gx = FRAME_X + 4;
        const uint8_t gy = frame_y + 4;
        const uint8_t graph_width = FRAME_WIDTH - 8;
        const uint8_t gh = frame_h - 8;

        /* Temp graph: 5 dots evenly spaced across frame */
        uint8_t tx[5];
        for (uint8_t i = 0; i < 5; ++i) tx[i] = gx + i * graph_width / 4;

        float ct = systate.system_info.cpu_temp;
        if (ct > 0 && ct != temp_history[temp_idx]) {
            temp_idx = (temp_idx + 1) % 5;
            temp_history[temp_idx] = ct;
        }
        uint8_t py[5] = {};
        for (uint8_t i = 0; i < 5; ++i) {
            float val = temp_history[(temp_idx + 1 + i) % 5];
            if (val == 0) continue;
            if (val > 60) val = 60;
            if (val < 40) val = 40;
            uint8_t yi = gy + gh - (uint8_t)((val - 40.0f) * gh / 20.0f);
            u8g2_DrawVLine(u8g2, tx[i], yi - 2, 5);
            py[i] = yi;
        }
        for (uint8_t i = 1; i < 5; ++i) {
            if (py[i - 1] && py[i])
                u8g2_DrawLine(u8g2, tx[i - 1], py[i - 1], tx[i], py[i]);
        }
    }

    UIMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) override {
        buildItems(systate.system_info);

        if      (msg == EncoderMsg::RotateRight && cursor < MAX_LINES - 1) { cursor++; if (cursor >= scroll + MAX_VISIBLE) scroll++; }
        else if (msg == EncoderMsg::RotateLeft  && cursor > 0)             { cursor--; if (cursor <  scroll)               scroll--; }
        else if (msg == EncoderMsg::ButtonHeld) return UIMsg{PageCommand::PC_Exit, std::monostate{}};
        return makeEmptyMsg();
    }

    void onEnter() override { cursor = 0; scroll = 0; }
    void onExit () override {}

    private:
    static constexpr uint8_t MAX_VISIBLE = 3; // max system info lines visiable at once
    static constexpr uint8_t MAX_LINES   = 5; // max system info lines in total
    static constexpr uint8_t LABEL_X     = 2 * STATIC::PAGE::TEXT_GAP_FROM_LEFT;
    static constexpr uint8_t FRAME_X     = HWINFO::WIDTH * 2 / 3;
    static constexpr uint8_t FRAME_WIDTH = HWINFO::WIDTH - FRAME_X - STATIC::PAGE::TEXT_GAP_FROM_LEFT;
    static constexpr uint8_t GAP         = 2 * STATIC::PAGE::TEXT_GAP_FROM_LEFT;

    uint8_t cursor = 0;
    uint8_t scroll = 0;

    std::array<std::array<char, 64>, MAX_LINES> lines = {};
    void buildItems(const SystemInfoMsg& info) {
        uint8_t n = 0;
        if (info.os[0]) snprintf(lines[n++].data(), 64, "Host: %s (%s)", info.host_name, info.os);
        else            snprintf(lines[n++].data(), 64, "Host: %s",      info.host_name);
        snprintf(lines[n++].data(), 64, "CPU: %dC/%dT %dMHz %.1f%%",     info.cpu_cores,
                                                                         info.cpu_threads,
                                                                         info.cpu_core_freq,
                                                                 (double)info.cpu_usage);
        snprintf(lines[n++].data(), 64, "Temp: %.1fC",           (double)info.cpu_temp);
        snprintf(lines[n++].data(), 64, "MEM: %d/%d MB %.1f%%",          info.mem_used,
                                                                         info.mem_total,
                                                                 (double)info.mem_usage);
        snprintf(lines[n++].data(), 64, "Disk: %d/%d MB %.1f%%",         info.disk_used,
                                                                         info.disk_total,
                                                                 (double)info.disk_usage);
    }
    float   temp_history[5] = {};
    uint8_t temp_idx = 4;
};

/* STABLE VERSION DO NOT MODIFY */
class AboutPage : public ArgosPage
{
    public:
    AboutPage () = default;
    ~AboutPage() override = default;

    void draw(u8g2_t* u8g2, const SystemState& systate) override {
        setPencilMode(u8g2, PencilMode::Solid);
        u8g2_SetFont(u8g2, FONT::BASE_FONT);

        uint8_t icon_x = 3 * STATIC::PAGE::TEXT_GAP_FROM_LEFT;
        uint8_t icon_y = (((HWINFO::WEIGHT - STATIC::NAV::NAV_BAR_HEIGHT) - ICON_GITHUB.height) >> 1)
                       + STATIC::NAV::NAV_BAR_HEIGHT;
        u8g2_DrawXBMP(u8g2, icon_x, icon_y, ICON_GITHUB.width, ICON_GITHUB.height, ICON_GITHUB.data);

        uint8_t text_x = 7 * STATIC::PAGE::TEXT_GAP_FROM_LEFT + ICON_GITHUB.width;
        u8g2_DrawStr(u8g2, text_x, STATIC::PAGE::LINE1, "Argos V1.0");
        u8g2_DrawStr(u8g2, text_x, STATIC::PAGE::LINE2, "Powered by ESP32 & U8G2");
        u8g2_DrawStr(u8g2, text_x, STATIC::PAGE::LINE3, "Github.com/Jav1ki4N/Argos");
    }

    UIMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) override {
        UIMsg page_msg = {};
        if (msg == EncoderMsg::ButtonHeld)
            page_msg.command = PageCommand::PC_Exit;
        return page_msg;
    }

    void onExit() override {}

    private:
};