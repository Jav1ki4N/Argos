
#pragma once

/* Includes */
#include "Argos_global.hpp"
#include <dirent.h>
#include <string_view>
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

    ArgosFramework(SSD1322 &display, LFS &filesys):
    _filesys(filesys),u8g2(display.get_U8g2())
    {}
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
    LFS &_filesys;
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

    /**
     * Initialization
     * @brief Initialize system state 
     */

     void getProfileList()
     {
        std::string_view profile_dir = "/lfs/profile"; // where profile stored
        uint8_t count = 0;

        DIR* dir = opendir(profile_dir.data()); // POSIX API to read directory
        if(!dir)return; //failed to open
       
        while(dirent* entry = readdir(dir)) {
            if(count >= system_state.profile_list.size()) break;
            std::string_view name = entry->d_name; // file name

            auto pos = name.find_last_of('.');
            if(pos == std::string_view::npos)continue;
            std::string_view ext = name.substr(pos); // file extension
            if(ext != ".profile")continue; // not a profile file, skip

            std::string_view base = name.substr(0, pos); // raw filename without extension
            auto& dst = system_state.profile_list[count];

            size_t len = std::min(base.size(), dst.size() - 1);
            std::memcpy(dst.data(), base.data(), len);
            dst[len] = '\0';
            ++count;
        }
        closedir(dir);
        system_state.if2UpdateProfileList = false; // reset flag after update
     }

    /**
     * Command Handler
     * @brief Handler for commands sent from Pages 
     */

     void 
};