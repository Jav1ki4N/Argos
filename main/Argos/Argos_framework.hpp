
#pragma once

/* Includes */

/** 
  * Pages
  * Can't include page headers directly as page header includes this file 
  */
class ArgosPage;
class NetworkPage;

/* DDC */
#include "ddc.hpp"

/* C/C++ */
#include <time.h>

/* Tasks */
#include "../network.hpp"

#include "Argos_global.hpp"
#include "Argos_Page.hpp"

class ArgosFramework
{
    public:
    
    friend class ArgosPage;
    /* Network & Sub pages */
    friend class NetworkPage;
    friend class ProfilePage;
    /* Info & Sub Pages */
    friend class InfoPage;
    /* About & Sub Pages */
    friend class AboutPage;

    ArgosFramework(SSD1322 &display); 
    ~ArgosFramework() = default;

    void render() {
	    u8g2_ClearBuffer(u8g2);
	    drawStatic();
	    drawOverlay();

	    ArgosPage* active_page;
	    using enum Encoder::EncoderMsg;

	    /** Preview Mode
	    *  Enter stack or switch between root pages
	    */
	    if (system_state.isEnterStack == false) {

            switch (system_state.focus_tab) {
                case 0: active_page = &manager.root_info_page;    break;
                case 1: active_page = &manager.root_network_page; break;
                case 2: active_page = &manager.root_about_page;   break;
            }
		    /** Handle Input Events
		     *  Button Pressed: Enter page stack & lock focus_tab
		     *  Rotate:         Browse between root pages
             */
		    if(system_state.input_event == ButtonPressed) {
                system_state.isEnterStack = true;
                manager.active_stack = &manager.stacks[system_state.focus_tab];
                manager.active_stack->push(active_page);
                manager.active_stack->curr_depth = 0;
            }
		    else system_state.focus_tab = (system_state.input_event == RotateRight) ?
		                                  ((system_state.focus_tab + 1) % 3):
		                                  ((system_state.focus_tab + 2) % 3);

		    /* If no input event to handle, draw current root page content*/
            active_page->draw(u8g2, system_state);
	    }
	    /** Stack Mode
	    *  In stack page browsing
	    *  focus_tab is locked
	    */
	    else {
            if(system_state.input_event != Encoder::EncoderMsg::None) {
               //manager.active_stack->top()->onEvent();
            }
            manager.active_stack->top()->draw(u8g2, system_state);
	    }
	}

    public:
    /// @brief Page stack for each main page section
    /// @param MAX_DEPTH  defines how many pages (root + subpages) can be stored in the stack
    /// @param curr_depth stack[curr_depth] shows how many pages are currently in the stack
    ///                   and also tells which page is on the top of the stack
    /// @note  class 'ArgosPage' is forward declared cuz it relies on ArgosFramework::SystemState
    ///        and in return relied on by struct ArgosFramework::PageManager, which can caused 
    ///        circular dependency or recursive inclusion.
    ///
    ///        When forward declared, in PageStack defination ArgosPage* can be used as pointer
    ///        while in PageManager instances of ArgosPage are required, so PageManager must be 
    ///        defined in the .cpp where you can just #include "Argos_Page.hpp" to access full
    ///        definition.
    ///
    ///        And also, PageManager has PageStack instance, so PageStack must be made public
    ///        so PageManager can access the full definition, too.
    ///
    ///        Fucking C++


    private:
    u8g2_t* u8g2;
    SystemState system_state;

    struct PageStack
    {
        static constexpr uint8_t MAX_DEPTH = 3;
        uint8_t curr_depth = 0;
        ArgosPage* stack[MAX_DEPTH] = {nullptr};
        void push(ArgosPage* page);
        void pop();
        ArgosPage* top() const;
    };
    
    struct PageManager
    {
        PageStack stacks[3];                       
        PageStack* active_stack = nullptr;         
                                                                
        PageStack& network() { return stacks[0]; }
        PageStack& info()    { return stacks[1]; }
        PageStack& about()   { return stacks[2]; }

        NetworkPage root_network_page;
        ProfilePage network_profile_page;

        InfoPage root_info_page;
        AboutPage root_about_page;
    }manager;

    /****/
    
    static constexpr const char* TITLE = "Argos V1.0";
    
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
    void setPencilMode(PencilMode mode);

    /**
     * @brief Draw static UI elements that do not change across pages
     *        The outer frame, navigation bar background, and title are drawn here.
     */
    void drawStatic();

    /**
     * @brief Draw navigation tabs & selected effect
     */
    void drawTabs();

    /**
     * @brief Draw the current time on the navigation bar
     */
    void drawTime();

    /**
     * @brief Draw dyanmic elements that are overlaid on all pages, such as navigation tabs and time.
     */
    void drawOverlay();

    /****/


};