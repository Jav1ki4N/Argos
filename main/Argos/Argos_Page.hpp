
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

    virtual void    draw(u8g2_t* u8g2, const SystemState& systate) = 0; // impl required
    //virtual void    onEnter(){};
    virtual void    onExit() = 0; // impl required
    virtual UIMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) = 0; // impl required

    protected:
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
        setPencilMode(u8g2, PencilMode::Solid);
        
        
    }

    UIMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) override {
        UIMsg page_msg = {};
        /* Click or leave */
        if      ( msg == EncoderMsg::ButtonPressed ) page_msg.command = PageCommand::Enter; // to profile page
        else if ( msg == EncoderMsg::ButtonHeld )    page_msg.command = PageCommand::Exit;  // exit stack
        return page_msg;
    }

    void onExit() override {}

    private:

    //uint8_t cursor = 0; no need, only one clickable item in this page

    /* Placement */
    static constexpr uint8_t TEXT_X = 75; // assigns with 'INFO' tab
    // static constexpr uint8_t WIFI_LOGO_X = ;
    // static constexpr uint8_t WIFI_LOGO_Y = ;
    
    /* Wifi Status in LINE1 */
    std::string_view wifi_offline = "OFFLINE",
                     wifi_sta     = "STA |",       // + ssid
                     wifi_ap      = "AP  | Argos"; // fixed ssid 

    /* Prompts on LINE2 */
    /* Pain in the arse to get these messages from network services */
    std::string_view prompt_idle          = "No profile loaded",         
                     prompt_cp            = "Captive Portal Launched",   
                     prompt_save          = "Saving Profile...",         
                     prompt_load          = "Loading Profile...",        
                     prompt_connect       = "Connecting to WiFi...",     
                     prompt_search        = "Searching for target...",   
                     prompt_done          = "Target device connected";   
    std::string_view prompt_e_exceed      = "E: Profile slot is full",   
                     prompt_e_wifi        = "E: Failed to connect WiFi", 
                     prompt_e_target      = "E: Failed at targeting";    
    
    /* Buttons on LINE3 */
    std::string_view btn_profile_text = "Profile Configuration"; 


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

        const uint8_t available_height = HWINFO::WEIGHT - STATIC::NAV::NAV_BAR_HEIGHT;
        const uint8_t used_height = (Slot::MAX_NUM * Slot::HEIGHT) + ((Slot::MAX_NUM - 1) * Slot::GAP_BETWEEN_EACH);
        const uint8_t top_gap = (available_height - used_height) / 2;
        const uint8_t start_y = STATIC::NAV::NAV_BAR_HEIGHT + top_gap;

        for(uint8_t i = 0; i < Slot::MAX_NUM; ++i) {
            const uint8_t frame_y = start_y + i * (Slot::HEIGHT + Slot::GAP_BETWEEN_EACH);
            u8g2_DrawFrame(u8g2, Slot::GAP_FROM_LEFT, frame_y, Slot::WIDTH, Slot::HEIGHT);
        }
    }

    UIMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) override {

        /* Check if current slot is empty */ 
        slot.isEmpty = (systate.profile_list[slot.cursor].data()[0] == '\0');

        /** Max index cursor can reach in a menu */
        uint8_t           limit = (mode == MenuMode::Option)? Slot::MAX_NUM : Option::MAX_NUM;
        uint8_t& cursor_to_move = (mode == MenuMode::Option)? slot.cursor : option.cursor;

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

                if(slot.isEmpty)msg.command = PageCommand::AddProfile;
                else {
                    strlcpy(payload.profile_name, 
                            systate.profile_list[slot.cursor].data(), 
                            sizeof(payload.profile_name));
                    msg.command = (option.cursor == 0) ? PageCommand::LoadProfile : 
                                                         PageCommand::DeleteProfile;
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
            else return UIMsg{PageCommand::Exit, std::monostate{}};
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
    struct Slot{
        bool isEmpty = false;
        uint8_t cursor = 0;
        static constexpr uint8_t MAX_NUM = 3;
        static constexpr uint8_t GAP_FROM_LEFT    = 10;
        static constexpr uint8_t GAP_BETWEEN_EACH = 3;
        static constexpr uint8_t WIDTH            = 110;
        static constexpr uint8_t HEIGHT           = 11;
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
        u8g2_DrawStr(u8g2,50,30,"test_info");
    }

    UIMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) override {
        UIMsg page_msg = {};
        if (msg == EncoderMsg::ButtonHeld)
            page_msg.command = PageCommand::Exit;
        return page_msg;
    }

    void onExit() override {}

    private:
};

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
            page_msg.command = PageCommand::Exit;
        return page_msg;
    }

    void onExit() override {}

    private:
};