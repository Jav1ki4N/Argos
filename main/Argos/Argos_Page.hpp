
#pragma once

/* Includes */

/* Framework Global */
#include "Argos_global.hpp"
#include "ddc.hpp"

/* Graphics */
#include "u8g2.h"

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
    virtual PageMsg onEvent(Encoder::EncoderMsg msg, SystemState& systate) = 0; // impl required

    protected:
    private:
};

/* Network Page */
class NetworkPage : public ArgosPage
{
    public:
    NetworkPage () = default;
    ~NetworkPage() override = default;

    void draw(u8g2_t* u8g2, const SystemState& systate) override {
        
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

    }

    PageMsg onEvent(Encoder::EncoderMsg msg, SystemState& systate) override {

        /* Check if current slot is empty */
        bool isEmptySlot = systate.profile_list[cursor_slot].data()[0] == '\0';

       /** Max index cursor can reach
        *  - In slots:   MAX_SLOT_NUM   (max profile slot displayed is 3)
        *  - In options: MAX_OPTION_NUM (max options      displayed is 2)
        */
        uint8_t           limit = (mode == MenuMode::Option)? MAX_OPTION_NUM : MAX_SLOT_NUM;
        uint8_t& cursor_to_move = (mode == MenuMode::Option)? cursor_option : cursor_slot;
        /** move cursor to next or previous slot/option
            - slot menu & non-empty slot's option menu, cursor is moveable
            - disabled in empty slot's option menu cuz only one option is available
         */
        if     ( msg == EncoderMsg::RotateRight && (!(isEmptySlot && mode == MenuMode::Option))) { cursor_to_move = ((cursor_to_move + 1) % limit); }
        else if( msg == EncoderMsg::RotateLeft  && (!(isEmptySlot && mode == MenuMode::Option))) { cursor_to_move = (cursor_to_move + limit - 1) % limit; }

        else if(msg == EncoderMsg::ButtonPressed) {
            /* In slots */
            if(mode == MenuMode::Slot) {
                /* Enter option menu */
                mode = MenuMode::Option;
                cursor_option=0; // reset option cursor               
            }
            /** In option menu
             *  - Load or Delete this profile
             *  - Add profile to an empty slot
             *  either way will do a instant exit cuz system state is changed 
             */
            else {
                ProfilePayload payload;
                PageMsg msg = {};

                /* Empty slot option: Add profile */
                if(isEmptySlot)msg.command = PageCommand::AddProfile;

                /* Regular slot option: Load or Delete */
                else {
                    strlcpy(payload.profile_name, 
                            systate.profile_list[cursor_slot].data(), 
                            sizeof(payload.profile_name));
                    msg.command = (cursor_option == 0) ? PageCommand::LoadProfile : PageCommand::DeleteProfile;
                    msg.payload = payload;
                }
                /* exit option menu */
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
        return PageMsg{PageCommand::None, std::monostate{}};
    }

    private:
    enum class MenuMode : uint8_t {
        Slot,
        Option
    }mode = MenuMode::Slot;
    uint8_t cursor_slot = 0;
    uint8_t cursor_option = 0;
    static constexpr uint8_t MAX_SLOT_NUM = 3;   // 3 profile slots at the same time
    static constexpr uint8_t MAX_OPTION_NUM = 2; // 2 options displayed at the same time                                          
};

class InfoPage : public ArgosPage
{
    public:
    InfoPage () = default;
    ~InfoPage() override = default;

    void draw(u8g2_t* u8g2, const SystemState& systate) override;

    private:
};

class AboutPage : public ArgosPage
{
    public:
    AboutPage () = default;
    ~AboutPage() override = default;

    void draw(u8g2_t* u8g2, const SystemState& systate) override;

    private:
};