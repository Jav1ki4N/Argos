
#pragma once

/* Custom */
#include "Argos_global.hpp"
#include "Argos_Page.hpp"

/* C/C++ */
#include <memory>
#include <time.h>
#include <dirent.h>
#include <string_view>

/* Third-Party */
#include "freertos/idf_additions.h"

class ArgosFramework
{
    public:
    
    explicit ArgosFramework(SSD1322 &display):
    u8g2(display.get_U8g2()) {
        ui2network_command_q = xQueueCreate(3, sizeof(UIMsg));
    }
    ~ArgosFramework() = default;

    /** Queue Handle Setters & getters
     *  @brief Set queue handles for receiving messages from other tasks
     *  @note  Passed queue handles must be global 
     */
    void setEncoderQueueHandle(QueueHandle_t encmsg_q) { encoder_msg_q = encmsg_q; }
    QueueHandle_t getNetworkTaskCommandQueue() { return ui2network_command_q; }

    void render() {
        updateSystemState(system_state);
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
		    if(system_state.enc_msg == ButtonPressed) {
                system_state.isEnterStack = true;                               // Enable page stack
                manager.active_stack = &manager.stacks[system_state.focus_tab]; // Set active stack
                manager.active_stack->curr_depth = 0;                           // preview mode curr_depth is 0
                manager.active_stack->push(active_root_page);                   // Push current root page to the top
                active_root_page->onEnter();
            }

            /* Browse between root pages */
		    else if(system_state.enc_msg == RotateLeft || 
                    system_state.enc_msg == RotateRight ) {
                system_state.focus_tab = (system_state.enc_msg == RotateRight) ?
                                         ((system_state.focus_tab + 1) % 3):
                                         ((system_state.focus_tab + 2) % 3);
            }

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
            if(system_state.enc_msg != EncoderMsg::None) {
                UIMsg msg_from_page = active_page->onEvent(system_state.enc_msg, system_state); // call top page's onEvent
                                                                                                      // and receive message
                /* Page intents to enter its subpage */
                if(msg_from_page.command == PageCommand::PC_Enter) {
                    manager.active_stack->push(manager.active_stack->order[manager.active_stack->curr_depth]);
                    manager.active_stack->top()->onEnter();
                }
                /* Page intents to exit */
                else if (msg_from_page.command == PageCommand::PC_Exit) {
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

        u8g2_SendBuffer(u8g2);
	}

    public:
    /// @brief Page stack for each main page section
    /// @param MAX_DEPTH  defines how many pages (root + subpages) can be stored in the stack
    /// @param curr_depth stack[curr_depth-1] points to current page,i.e. number of pages in stack = curr_depth
    
    private:
    static constexpr const char* TAG = "ArgosFramework";
    //LFS &_filesys;
    u8g2_t* u8g2;
    SystemState system_state;
    QueueHandle_t encoder_msg_q;
    QueueHandle_t wifi_msg_q;

    /* Queue sent to tasks */
    QueueHandle_t ui2network_command_q;

    

    /**
     * @brief Update system state before each frame render.
     * @note  2 message need to receive in total
     *        @param enc_msg user input from encoder
     *        @param network_msg network state from network task
     */
    void updateSystemState(SystemState &systate) {
        using enum Encoder::EncoderMsg;
        EncoderMsg enc_msg;
        if (xQueueReceive(encoder_msg_q, &enc_msg, 0) == pdTRUE) systate.enc_msg = enc_msg;
        else                                                     systate.enc_msg = EncoderMsg::None;

        NetworkTaskStateMsg network_msg;
        if (xQueueReceive(network2ui_state_q, &network_msg, 0) == pdTRUE)
            systate.network_msg = network_msg;

        SystemInfoMsg info_msg;
        if (xQueueReceive(network2ui_info_q, &info_msg, 0) == pdTRUE)
            systate.system_info = info_msg;

        /* Get time from RTC */
        time_t now = time(nullptr);
        if (now > 0) {
            tm* t = localtime(&now);
            snprintf(systate.curr_time, sizeof(systate.curr_time),
                     "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
        }

        /* Refresh profile list from LittleFS */
        for (auto& p : systate.profile_list) p[0] = '\0';
        auto dir = std::unique_ptr<DIR, decltype(&closedir)> {
            opendir("/lfs/profile"), // constructor
            closedir                 // deleter
        };
        if (dir) {
            uint8_t i = 0;
            while (dirent* entry = readdir(dir.get())) { // readdir expects a raw pointer *DIR
                                                         // traversing all files in profile directory
                if (!std::string_view{entry->d_name}.ends_with(".txt")) continue; // ignore non txt
                std::string name(entry->d_name);         // get file name
                name.resize(name.size() - 4);            // strip ".txt"
                strlcpy(systate.profile_list[i].data(), name.c_str(), 64); 
                if (++i >= 3) break;
            }
        }
    }

    static void deleteProfile(std::string_view name) {
        std::string path = "/lfs/profile/";
        path += name;
        path += ".txt";
        unlink(path.c_str());
    }

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
                --curr_depth;                  // Move down the stack
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
    void drawStatic() {
        setPencilMode(u8g2, PencilMode::Solid);
        u8g2_DrawFrame(u8g2, 0, 0, HWINFO::WIDTH, HWINFO::WEIGHT);
        u8g2_DrawBox(u8g2, 0, 0, STATIC::NAV::NAV_BAR_WIDTH, STATIC::NAV::NAV_BAR_HEIGHT);
        setPencilMode(u8g2, PencilMode::Hollow);
        u8g2_SetFont(u8g2, FONT::BASE_FONT);
        u8g2_DrawStr(u8g2,
                     STATIC::TITLE::GAP_FROM_LEFT,
                     STATIC::NAV::NAV_BAR_HEIGHT - STATIC::TITLE::GAP_FROM_BUTTOM,
                     TITLE);
    }

    /**
     * @brief Draw navigation tabs & selected effect
     */
    void drawTabs() {
        u8g2_SetFont(u8g2, FONT::BASE_FONT);
        uint8_t tab_y = STATIC::NAV::NAV_BAR_HEIGHT - STATIC::TITLE::GAP_FROM_BUTTOM;

        uint8_t tab_curr_x = STATIC::TITLE::GAP_FROM_LEFT
                           + STATIC::TITLE::GAP_FROM_FIRST_TAG
                           + u8g2_GetStrWidth(u8g2, TITLE);

        const uint8_t radius = 1;
        const uint8_t delimeter_width = u8g2_GetStrWidth(u8g2, "|");
        const uint8_t tab_height = u8g2_GetFontAscent(u8g2) - u8g2_GetFontDescent(u8g2);

        constexpr const char* text[3] = {"INFO", "NETWORK", "ABOUT"};

        for (uint8_t i = 0; i < std::size(text); i++) {
            uint8_t tab_width = u8g2_GetStrWidth(u8g2, text[i]);

            /* Delimeter */
            uint8_t delimeter_x = tab_curr_x + tab_width
                                + ((STATIC::TABS::GAP_BETWEEN - delimeter_width) >> 1);
            setPencilMode(u8g2, PencilMode::Hollow);
            if (i != 2) u8g2_DrawStr(u8g2, delimeter_x, tab_y, "|");

            /* Selected highlight */
            if (i == system_state.focus_tab) {
                u8g2_DrawRBox(u8g2, tab_curr_x - 2, tab_y - tab_height + 1,
                              tab_width + 4, tab_height, radius);
            }
            setPencilMode(u8g2, (i == system_state.focus_tab) ? PencilMode::Solid
                                                               : PencilMode::Hollow);
            u8g2_DrawStr(u8g2, tab_curr_x, tab_y, text[i]);

            tab_curr_x += tab_width + STATIC::TABS::GAP_BETWEEN;
        }
    }

    /**
     * @brief Draw the current time on the navigation bar
     */
    void drawTime() {
        setPencilMode(u8g2, PencilMode::Hollow);
        uint8_t w = u8g2_GetStrWidth(u8g2, system_state.curr_time);
        uint8_t x = HWINFO::WIDTH - w - STATIC::PAGE::TEXT_GAP_FROM_LEFT;
        uint8_t y = STATIC::NAV::NAV_BAR_HEIGHT - STATIC::TITLE::GAP_FROM_BUTTOM;
        u8g2_DrawStr(u8g2, x, y, system_state.curr_time);
    }

    /**
     * @brief Draw dynamic elements that are overlaid on all pages.
     */
    void drawOverlay() {
        drawTabs();
        drawTime();
    }

    /**
     * Command Handler
     * @brief Handler for commands sent from Pages 
     */
     void commandDispatcher(const UIMsg& msg) {
        using enum PageCommand;
        switch (msg.command) {
            case PC_LoadProfile:
            case PC_AddProfile: {
                xQueueSend(ui2network_command_q, &msg, 0);
                if (manager.active_stack && manager.active_stack->curr_depth > 0) {
                    manager.active_stack->top()->onExit();
                    manager.active_stack->pop();
                }
                break;
            }
            case PC_DeleteProfile: {
                if (auto* p = std::get_if<ProfilePayload>(&msg.payload))
                    deleteProfile(p->profile_name);
                break;
            }
            default: break;
        }
     }
     
};

