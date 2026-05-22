
#pragma once

/* Includes */
#include "Argos_global.hpp"
#include <dirent.h>
#include <string_view>

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
    
    ArgosFramework(SSD1322 &display, LFS &filesys):
    _filesys(filesys),u8g2(display.get_U8g2())
    {}
    ~ArgosFramework() = default;

    void render() {
	    u8g2_ClearBuffer(u8g2);
	    drawStatic();
	    drawOverlay();

	    ArgosPage* active_root_page;
	    using enum Encoder::EncoderMsg;

	    /** Preview Mode
	     *  Enter stack or switch between root pages
	     */
	    if (system_state.isEnterStack == false) {

            /* Determine which root page to display */
            active_root_page = manager.root_pages[system_state.focus_tab];
		    
            /* Enter Stack */
		    if(system_state.input_event == ButtonPressed) {
                system_state.isEnterStack = true;                               // Enable page stack
                manager.active_stack = &manager.stacks[system_state.focus_tab]; // Set active stack
                manager.active_stack->curr_depth = 0;                           // preview mode curr_depth is 0
                manager.active_stack->push(active_root_page);                   // Push current root page to the top
            }

            /* Browse between root pages */
		    else system_state.focus_tab = (system_state.input_event == RotateRight) ?
		                                  ((system_state.focus_tab + 1) % 3):
		                                  ((system_state.focus_tab + 2) % 3);

		    /* Draw root page, this does not uses stack */
            active_root_page->draw(u8g2, system_state);
	    }
	    /** Stack Mode
	     *  In stack page browsing
	     *  focus_tab is locked
	     */
	    else {
            ArgosPage* active_page = manager.active_stack->top(); // Get current page from stack's top
            /* Handle message from encoder */
            if(system_state.input_event != EncoderMsg::None) {
                PageMsg msg_from_page = active_page->onEvent(system_state.input_event, system_state); // call top page's onEvent
                                                                                                      // and receive message
                /* Page intents to enter its subpage */
                if(msg_from_page.command == PageCommand::Enter) {
                    // e.g. <root_page,curr_depth = 1> --> order[1] = subpage1                                                 
                    manager.active_stack->push(manager.active_stack->order[manager.active_stack->curr_depth]);
                }
                /* Page intents to exit */
                else if (msg_from_page.command == PageCommand::Exit) {
                    if(manager.active_stack->curr_depth == 1)system_state.isEnterStack = false; // Exit from a root page is to exit the stack                         
                    active_page->onExit();                                                      // Call current page's onExit for cleanup
                    manager.active_stack->pop();                                                // Pop the current page from stack
                }                                                                               // do nothing if exit from root page
                /* Page intents to trigger an action */
                else commandDispatcher(msg_from_page); // too long to write here
            }
            active_page = manager.active_stack->top(); // Get current page again in case of page switch
            if(active_page)active_page->draw(u8g2, system_state);
	    }
	}

    public:
    /// @brief Page stack for each main page section
    /// @param MAX_DEPTH  defines how many pages (root + subpages) can be stored in the stack
    /// @param curr_depth stack[curr_depth-1] points to current page,i.e. number of pages in stack = curr_depth
    
    private:
    LFS &_filesys;
    u8g2_t* u8g2;
    SystemState system_state;

    struct PageStack
    {
        static constexpr uint8_t MAX_DEPTH = 3;
        uint8_t curr_depth = 0;
        std::array<ArgosPage*, MAX_DEPTH> stack = {nullptr};
        std::array<ArgosPage*, MAX_DEPTH> order = {nullptr};

        //  func   push
        /// @brief Push a new page to the stack, and update the current depth
        /// @note  this function does not check if current page is pushed in the correct order
        void push(ArgosPage* page){
            if(curr_depth < MAX_DEPTH){
                stack[curr_depth++] = page; // Push new page to the current depth
            }
        }

        //  func   pop
        /// @brief Pop the top page from the stack, and update the current depth
        void pop() {
            /* When in stack, curr_depth is always positive */
            if(curr_depth > 0 ){
                stack[curr_depth-1] = nullptr; // Clear pointer to popped page
                --curr_depth; // Move down the stack
            }
        };
        //  func   top
        /// @brief Get the top page from the stack, which is the current active page to be displayed
        ArgosPage* top() const { return (curr_depth > 0) ? stack[curr_depth - 1] : nullptr; }
    };
    
    struct PageManager
    {
        /* Pages */
        NetworkPage root_network_page;
        ProfilePage network_profile_page;
        InfoPage root_info_page;
        AboutPage root_about_page;

        ArgosPage* root_pages[3] = {&root_info_page, &root_network_page, &root_about_page};

        PageStack  stacks[3];
        PageStack* active_stack = nullptr;

        /* Initialize PageManager */
        /// @brief stack needs to know the order of pages to push
        ///        and pages are created in PageManager, so the order should be passed in the constructor of PageManager
        PageManager() {
            // order[0] is not reachable (and no need to be) as minimal curr_depth in stack is 1
            stacks[0].order = {&root_info_page,    nullptr, nullptr};               // Info page push order
            stacks[1].order = {&root_network_page, &network_profile_page, nullptr}; // Network page push order
            stacks[2].order = {&root_about_page,   nullptr, nullptr};               // About page push order
        }
    } manager;

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
            if(ext != ".txt")continue; // not a profile file, skip

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
     void commandDispatcher(const PageMsg& msg) {

     }
     
};