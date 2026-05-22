
#pragma once

/* Includes */

/* Framework Global */
#include "Argos_global.hpp"
#include "ddc.hpp"

/* Graphics */
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
    virtual PageMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) = 0; // impl required

    protected:
    PageMsg makeEmptyMsg() const { PageMsg msg = {}; return msg; }
    private:
};

/* Network Page */
class NetworkPage : public ArgosPage
{
    public:
    NetworkPage () = default;
    ~NetworkPage() override = default;

    void draw(u8g2_t* u8g2, const SystemState& systate) override {
        u8g2_DrawStr(u8g2,50,30,"test_network");
    }

    PageMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) override {
        return makeEmptyMsg();
    }

    void onExit() override {
        // Implementation for exiting the network page
    }

    private:
    
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
        u8g2_DrawStr(u8g2,50,30,"test_profile");
    }

    PageMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) override {

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
                PageMsg msg = {};

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
            else return PageMsg{PageCommand::Exit, std::monostate{}};
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
        static constexpr uint8_t GAP_FROM_TOP     = 20;
        static constexpr uint8_t GAP_BETWEEN_EACH = 15;
        static constexpr uint8_t WIDTH            = 100;
        static constexpr uint8_t HEIGHT           = 10;
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
        u8g2_DrawStr(u8g2,50,30,"test_info");
    }

    PageMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) override {
        return makeEmptyMsg();
    }

    void onExit() override {
        // Implementation for exiting the info page
    }

    private:
};

class AboutPage : public ArgosPage
{
    public:
    AboutPage () = default;
    ~AboutPage() override = default;

    void draw(u8g2_t* u8g2, const SystemState& systate) override {
        u8g2_DrawStr(u8g2,50,30,"test_about");
    }

    PageMsg onEvent(Encoder::EncoderMsg msg, const SystemState& systate) override {
        return makeEmptyMsg();
    }

    void onExit() override {
        // Implementation for exiting the about page
    }

    private:
};