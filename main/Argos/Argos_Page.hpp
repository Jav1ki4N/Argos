
#pragma once

/* Includes */

/* Framework Global */
#include "Argos_global.hpp"
#include "ddc.hpp"

/* Graphics */
#include "network.hpp"
#include "network/ddc_wifi.hpp"
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
    bool inStack = false; // for root pages
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

        const ICON& icon = (systate.network_msg.wifi_state == WS_Offline)
                         ? ICON_WIFI_NO_CONNECT
                         : ICON_WIFI_CONNECTED;

        uint8_t icon_x = 3 * STATIC::PAGE::TEXT_GAP_FROM_LEFT;
        uint8_t icon_y = (((HWINFO::WEIGHT - STATIC::NAV::NAV_BAR_HEIGHT) - icon.height) >> 1)
                       + STATIC::NAV::NAV_BAR_HEIGHT;
        u8g2_DrawXBMP(u8g2, icon_x, icon_y, icon.width, icon.height, icon.data);

        {
            u8g2_DrawStr(u8g2, TEXT_X, STATIC::PAGE::LINE3,     "Setting: [PROFILES]");
            if(inStack) {
                setPencilMode(u8g2, PencilMode::Invert);
                u8g2_DrawBox(u8g2, 
                TEXT_X + u8g2_GetStrWidth(u8g2,"Settings: ")-4,  
                (STATIC::PAGE::LINE3 - 8), 
                u8g2_GetStrWidth(u8g2,"[PROFILES]") - 4,  
                u8g2_GetFontAscent(u8g2) + u8g2_GetFontDescent(u8g2) + 4);
            }
            wifi_state  = systate.network_msg.wifi_state;
            error       = systate.network_msg.error;
            chain_stage = systate.network_msg.chain_stage;
            
            makeHint(u8g2);
        }
    }

    UIMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) override {
        UIMsg page_msg = {};
        if      ( msg == EncoderMsg::ButtonPressed ) page_msg.command = PageCommand::PC_Enter;
        else if ( msg == EncoderMsg::ButtonHeld )    page_msg.command = PageCommand::PC_Exit;
        return page_msg;
    }
    void onEnter() override {
        inStack = true;
    }
    void onExit() override {
        inStack = false;
    }

    private:

    NetworkTaskStateMsg::WiFiState  wifi_state;
    NetworkTaskStateMsg::ChainError error;
    NetworkTaskStateMsg::ChainStage chain_stage;

    static constexpr std::string_view wifi_hint(NetworkTaskStateMsg::WiFiState s) {
        using enum NetworkTaskStateMsg::WiFiState;
        switch (s) {
            case WS_Offline:        return "Network: [OFFLINE]";
            case WS_AP:             return "Network: [SOFTAP]";
            case WS_STAConnecting:  return "Network: [STA CONNECTING]";
            case WS_STAConnected:   return "Network: [STA CONNECTED]";
            default:                return "";
        }
    }

    static constexpr std::string_view chain_hint(NetworkTaskStateMsg::ChainStage s) {
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

    static constexpr std::string_view error_hint(NetworkTaskStateMsg::ChainError e) {
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

    void makeHint(u8g2_t *u8g2){
        u8g2_DrawStr(u8g2, TEXT_X, STATIC::PAGE::LINE1, wifi_hint(wifi_state).data());
        u8g2_DrawStr(u8g2, TEXT_X, STATIC::PAGE::LINE2, 
        (chain_stage == NetworkTaskStateMsg::ChainStage::CS_Idle &&
         error != NetworkTaskStateMsg::ChainError::E_None) 
        ? error_hint(error).data() : chain_hint(chain_stage).data());
    }

    static constexpr uint8_t TEXT_X = 75;
};

/**
 * @brief Profile Configuration Page
 *        4 profiles slots are listed column-wise, when cursor is on a profile
 *        it's displayed as inverted. When clicked, enter the right side of the page
 *        where 2 options "Load" and "Delete" are available to choose from.
 *        At the bottom of the profile list an "Add" is provided
 */

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
        const uint8_t frame_y = STATIC::PAGE::LINE1 - ascent - 2;
        const uint8_t frame_h = (STATIC::PAGE::LINE3 - STATIC::PAGE::LINE1) + ascent + descent + 2;

        const uint8_t space_w = u8g2_GetStrWidth(u8g2, " ");
        const uint8_t l_w = u8g2_GetStrWidth(u8g2, "[");
        const uint8_t r_w = u8g2_GetStrWidth(u8g2, "]");
        const uint8_t inner_w = FRAME_X - GAP - BRACKET_X - l_w - r_w;
        const uint8_t n = inner_w / space_w;
        const std::string spaces(n, ' ');
        const std::string bracket = "[" + spaces + "]";

        for (uint8_t i = 0; i < Slot::MAX_NUM; ++i) {
            uint8_t y = STATIC::PAGE::LINE1 + i * (STATIC::PAGE::LINE2 - STATIC::PAGE::LINE1);
            bool is_cursor = (mode == MenuMode::Slot && slot.cursor == i);

            if (is_cursor) {
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_DrawBox(u8g2, BRACKET_X + 2, y - ascent - 1, FRAME_X - GAP - BRACKET_X - 4, ascent + descent);
                u8g2_SetDrawColor(u8g2, 0);
            }

            u8g2_DrawStr(u8g2, BRACKET_X, y, bracket.c_str());

            std::string_view name = systate.profile_list[i].data();
            if (name[0]) {
                uint8_t name_w = u8g2_GetStrWidth(u8g2, name.data());
                uint8_t name_x = BRACKET_X + l_w + (inner_w - name_w) / 2;
                u8g2_DrawStr(u8g2, name_x, y, name.data());
            }

            if (is_cursor) u8g2_SetDrawColor(u8g2, 1);
        }

        u8g2_DrawFrame(u8g2, FRAME_X, frame_y, FRAME_W, frame_h);

        bool is_option_mode = (mode == MenuMode::Option);
        uint8_t mid_y   = frame_y + frame_h / 2;
        uint8_t top_mid = frame_y + frame_h / 4;
        uint8_t bot_mid = frame_y + frame_h * 3 / 4;

        u8g2_DrawHLine(u8g2, FRAME_X + 2, mid_y, FRAME_W - 4);

        static constexpr const char* ordinal[] = {"1st Slot", "2nd Slot", "3rd Slot"};
        uint8_t w = u8g2_GetStrWidth(u8g2, ordinal[slot.cursor]);
        u8g2_DrawStr(u8g2, FRAME_X + (FRAME_W - w) / 2, top_mid + ascent / 2, ordinal[slot.cursor]);

        auto drawBtn = [&](const char* text, uint8_t opt_idx) {
            uint8_t btn_w = u8g2_GetStrWidth(u8g2, text);
            uint8_t third_w = (FRAME_W - 4) / 2;
            uint8_t bx = FRAME_X + 2 + third_w * opt_idx + (third_w - btn_w) / 2;
            uint8_t by = bot_mid + ascent / 2;
            bool highlight = (is_option_mode && option.cursor == opt_idx);
            if (highlight) {
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_DrawBox(u8g2, bx + 4, bot_mid - ascent / 2 - 2, btn_w - 8, ascent + descent);
                u8g2_SetDrawColor(u8g2, 0);
            }
            u8g2_DrawStr(u8g2, bx, by, text);
            if (highlight) u8g2_SetDrawColor(u8g2, 1);
        };

        bool empty = (systate.profile_list[slot.cursor].data()[0] == '\0');
        if (empty) {
            uint8_t btn_w = u8g2_GetStrWidth(u8g2, "[ADD]");
            uint8_t bx = FRAME_X + (FRAME_W - btn_w) / 2;
            uint8_t by = bot_mid + ascent / 2;
            bool highlight = is_option_mode;
            if (highlight) {
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_DrawBox(u8g2, bx + 4, bot_mid - ascent / 2 - 2, btn_w - 8, ascent + descent);
                u8g2_SetDrawColor(u8g2, 0);
            }
            u8g2_DrawStr(u8g2, bx, by, "[ADD]");
            if (highlight) u8g2_SetDrawColor(u8g2, 1);
        } else {
            drawBtn("[LOAD]", 0);
            drawBtn("[DEL]", 1);
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
        mode = MenuMode::Slot;
        slot.isEmpty = false;
        slot.cursor = 0;
        option.cursor = 0;
    }

    private:
    enum class MenuMode : uint8_t {
        Slot,
        Option
    }mode = MenuMode::Slot;
    static constexpr uint8_t BRACKET_X = 2 * STATIC::PAGE::TEXT_GAP_FROM_LEFT;
    static constexpr uint8_t FRAME_X   = HWINFO::WIDTH / 2;
    static constexpr uint8_t FRAME_W   = HWINFO::WIDTH - FRAME_X - STATIC::PAGE::TEXT_GAP_FROM_LEFT;
    static constexpr uint8_t GAP       = 2 * STATIC::PAGE::TEXT_GAP_FROM_LEFT;

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
        const bool in_stack   = systate.isEnterStack;

        uint8_t count = buildItems(systate.system_info, max_px, u8g2);

        for (uint8_t i = 0; i < MAX_VISIBLE && (scroll + i) < count; ++i) {
            uint8_t idx = scroll + i;
            uint8_t y   = STATIC::PAGE::LINE1 + i * (STATIC::PAGE::LINE2 - STATIC::PAGE::LINE1);
            bool is_cursor = in_stack && (idx == cursor);

            if (is_cursor) {
                u8g2_SetDrawColor(u8g2, 1);
                u8g2_DrawRBox(u8g2, LABEL_X - 1, y - ascent - 1, max_px, ascent + descent + 1, 1);
                u8g2_SetDrawColor(u8g2, 0);
            }

            u8g2_DrawStr(u8g2, LABEL_X, y, lines[idx]);
            if (is_cursor) u8g2_SetDrawColor(u8g2, 1);
        }

        u8g2_DrawFrame(u8g2, FRAME_X, frame_y, FRAME_W, frame_h);
    }

    UIMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) override {
        buildItems(systate.system_info, 0, nullptr);
        uint8_t count = line_count;

        if      (msg == EncoderMsg::RotateRight && cursor < count - 1) { cursor++; if (cursor >= scroll + MAX_VISIBLE) scroll++; }
        else if (msg == EncoderMsg::RotateLeft  && cursor > 0)          { cursor--; if (cursor <  scroll)               scroll--; }
        else if (msg == EncoderMsg::ButtonHeld) return UIMsg{PageCommand::PC_Exit, std::monostate{}};
        return makeEmptyMsg();
    }

    void onEnter() override { cursor = 0; scroll = 0; }
    void onExit() override {}

    private:
    static constexpr uint8_t MAX_VISIBLE = 3;
    static constexpr uint8_t MAX_LINES   = 8;
    static constexpr uint8_t LABEL_X     = 2 * STATIC::PAGE::TEXT_GAP_FROM_LEFT;
    static constexpr uint8_t FRAME_X     = HWINFO::WIDTH * 2 / 3;
    static constexpr uint8_t FRAME_W     = HWINFO::WIDTH - FRAME_X - STATIC::PAGE::TEXT_GAP_FROM_LEFT;
    static constexpr uint8_t GAP         = 2 * STATIC::PAGE::TEXT_GAP_FROM_LEFT;

    uint8_t cursor = 0;
    uint8_t scroll = 0;
    uint8_t line_count = 0;
    char lines[MAX_LINES][64] = {};

    uint8_t buildItems(const SystemInfoMsg& info, uint8_t max_px, u8g2_t* u8g2) {
        uint8_t n = 0;
        auto add = [&](const char* label, const char* fmt, ...) __attribute__((format(printf, 3, 4))) {
            int pos = snprintf(lines[n], sizeof(lines[n]), "%s ", label);
            va_list args;
            va_start(args, fmt);
            vsnprintf(lines[n] + pos, sizeof(lines[n]) - pos, fmt, args);
            va_end(args);
            if (u8g2 && u8g2_GetStrWidth(u8g2, lines[n]) > max_px) {
                while (lines[n][0] && u8g2_GetStrWidth(u8g2, lines[n]) + u8g2_GetStrWidth(u8g2, "..") > max_px)
                    lines[n][strlen(lines[n]) - 1] = '\0';
                strcat(lines[n], "..");
            }
            n++;
        };

        if (info.os[0]) add("Host:", "%s (%s)", info.host_name, info.os);
        else            add("Host:", "%s", info.host_name);
        add("CPU:", "%dC/%dT %dMHz %.1f%%", info.cpu_cores, info.cpu_threads, info.cpu_core_freq, (double)info.cpu_usage);
        add("Temp:", "%.1fC", (double)info.cpu_temp);
        add("MEM:", "%d/%d MB %.1f%%", info.mem_used, info.mem_total, (double)info.mem_usage);
        add("Disk:", "%d/%d MB %.1f%%", info.disk_used, info.disk_total, (double)info.disk_usage);
        line_count = n;
        return n;
    }
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