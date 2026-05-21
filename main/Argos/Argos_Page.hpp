
#pragma once

/* Includes */

/* Framework Global */
#include "Argos_global.hpp"
#include "ddc.hpp"

/* Graphics */
#include "u8g2.h"

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
    virtual PageMsg onEvent(Encoder::EncoderMsg msg) = 0; // impl required

    protected:
    uint8_t cursor = 0; // indicate which widget/item is focused
    uint8_t item_num;   // number of widgets or items in this page
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

class ProfilePage : public ArgosPage
{
    public:
    ProfilePage () = default;
    ~ProfilePage() override = default;

    void draw(u8g2_t* u8g2, const SystemState& systate) override {

    }

    PageMsg onEvent(Encoder::EncoderMsg msg) override {
        

        return PageMsg{PageCommand::None, std::monostate{}};
    }

    private:
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