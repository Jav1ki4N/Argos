/*==============================*/
/* UI for Argos */
/* Display：SSD1322 256*64 */
/* ColorDepth: 1-bit Monochrome */
/* Graphic Lib: u8g2 */
/*===============================*/

#pragma once

/* Task requirement */
/* - for receiving queue handle */
#define NETWORK_TASK_ON 1
#if     NETWORK_TASK_ON
    #include "../../../../../../main/network.hpp"
#endif

/* DDC headers */
/* - same directory */
#include "ddc_argos_icon.hpp"
#include "ddc_animation.hpp"
/* - other directories */
#include "../../../network/ddc_wifi.hpp"

/* ESP-IDF Components */
#include "freertos/idf_additions.h"
#include "u8g2.h"

/* C/C++ Libraries */
#include <cstdint>
#include <time.h>

// Element placement
static constexpr uint8_t SCREEN_WIDTH = 255, 
                         SCREEN_HEIGHT = 64,
                         NAV_BAR_WIDTH = 255,
                         NAV_BAR_HEIGHT = 13,
                         TITLE_GAP_FROM_LEFT = 3,
                         TITLE_GAP_FROM_BUTTOM =2,
                         FIRST_TAG_FROM_TITLE = 13,
                         GAP_BETWEEN_TAGS = 12,
                         ICON_SIZE = 32,
                         ICON_SIZE_BIGGER = 40,
                         WIFI_ICON_WIDTH = 40,
                         WIFI_ICON_HEIGHT = 35,
                         LINE1 = 24,
                         LINE2 = 36,
                         LINE3 = 48,
                         LINE4 = 60,
                         TEXT_GAP_FROM_LEFT = 5,
                         ABOUT_TEXT_GAP_FROM_ICON = 10;


// Other constants
static constexpr const uint8_t* ARGOS_FONT  = (uint8_t*)u8g2_font_profont11_tr;
static constexpr const char*    ARGOS_TITLE = "Argos V1.0";
static constexpr const char*    PAGES[3]    = {"INFO", "NETWORK", "ABOUT"};
static constexpr const char*    ABOUT_TEXT[3] = {"Argos V1.0",
                                                 "Powered by ESP32 & U8G2",
                                                 "Github.com/Jav1ki4N/Argos"};

// Animation Frames & Intervals
static constexpr const uint8_t* BOOT_SCREEN_ICONS[5]     = {icon_argos,
                                                            icon_argos_alter,
                                                            icon_void,
                                                            icon_argos_alter,
                                                            icon_void};
static constexpr const uint8_t* WIFI_CONNECTING_ICONS[3] = {icon_wifi_1, icon_wifi_2, icon_wifi_3};
static constexpr       uint16_t WIFI_ANIMATION_INTERVAL = 400;


enum class Direction : uint8_t
{
    Left  = 0,
    Right = 1,
    Up    = 2,
    Down  = 3
};

enum class Page : uint8_t
{
    Info    = 0,
    Network = 1,
    About   = 2
};


struct App_State
{
    /* state control vars */
    int current_page_index = 2; // default to network page cuz I made an animation for it 

    /* WIFI Info */
    WifiMsg::State wifi_state = WifiMsg::Connecting; // in-class wifi state is defined as bits
    char wifi_ssid[32] = {};                         // this one is defined in WifiMsg

    /* Time Info */
    char time_str[16] = "00:00:00";

    /* System Info */
    char host_name[32]  = {};
    char os[32]         = {};
    char os_distro[64]  = {}; // not used

    int   cpu_core_freq = 0;
    int   cpu_cores     = 0;
    int   cpu_threads   = 0;
    float cpu_usage     = 0.0f;
    float cpu_temp      = 0.0f;

    int   mem_total     = 0;
    int   mem_used      = 0;
    float mem_usage     = 0.0f;

    int   disk_total    = 0;
    int   disk_used     = 0;
    float disk_usage    = 0.0f;
};

/**********************************************************************************************/

// Static outline & frame & elements of the UI

inline void UI_DrawStatic(u8g2_t *u8g2)
{
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawFrame(u8g2,0,0,SCREEN_WIDTH,SCREEN_HEIGHT);      // outline
    u8g2_DrawBox(u8g2,0,0,NAV_BAR_WIDTH,NAV_BAR_HEIGHT);      // navigation bar

    u8g2_SetFontMode(u8g2, 1);
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_SetFont(u8g2, ARGOS_FONT);
    u8g2_DrawStr(u8g2, TITLE_GAP_FROM_LEFT,                        // project title
                       NAV_BAR_HEIGHT - TITLE_GAP_FROM_BUTTOM - 1, // magic number but nvm
                       ARGOS_TITLE);
                       
    u8g2_SetFontMode(u8g2, 0);
    u8g2_SetDrawColor(u8g2, 1);
}

inline void UI_DrawBootScreen(u8g2_t *u8g2)
{
    UI_DrawStatic(u8g2);
    static Animation<5> boot_anime;
    boot_anime.set_durations({500,100,100,100,100});
    for(;;)
    {
        boot_anime.update();
        int frame = boot_anime.frame;
        u8g2_DrawXBMP(u8g2, 
                      (SCREEN_WIDTH - ICON_SIZE) >> 1, 
                      (((SCREEN_HEIGHT-NAV_BAR_HEIGHT)- ICON_SIZE) >> 1) + NAV_BAR_HEIGHT, 
                      ICON_SIZE, 
                      ICON_SIZE, 
                      BOOT_SCREEN_ICONS[frame]);
        u8g2_SendBuffer(u8g2);
        if(frame == 4)break;
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    /* For default page see struct App_State  */
}

// Update tags on the navigation bar
// handle page turn's tag selected effect by detecting current page index in App_State

inline void UI_DrawTag(u8g2_t *u8g2, const App_State& state)
{
    // set font to default
    u8g2_SetFont(u8g2, ARGOS_FONT);
    
    uint8_t y      = NAV_BAR_HEIGHT - TITLE_GAP_FROM_BUTTOM - 1; // see UI_DrawStatic

    uint8_t current_x = TITLE_GAP_FROM_LEFT  + 
                        FIRST_TAG_FROM_TITLE +
                        u8g2_GetStrWidth(u8g2, ARGOS_TITLE);

    uint8_t radius = 1;

    uint8_t sep_width = u8g2_GetStrWidth(u8g2, "|");
    
    for(int i=0; i<3; i++)
    {
        uint8_t width = u8g2_GetStrWidth(u8g2, PAGES[i]);
        uint8_t height = u8g2_GetFontAscent(u8g2) - u8g2_GetFontDescent(u8g2);

        // x and y is determined here
        
        // draw separator
        u8g2_SetDrawColor(u8g2,0);
        uint8_t sep_x = current_x + width + ((GAP_BETWEEN_TAGS - sep_width) >> 1);
        if(i!=2)u8g2_DrawStr(u8g2, sep_x, y, "|");
        
        if(i == state.current_page_index)
        {
            u8g2_SetDrawColor(u8g2,0);
            u8g2_DrawRBox(u8g2, 
                          current_x - 2, 
                          y - height + 1, 
                          width + 4,
                          height,
                          radius);
            u8g2_SetDrawColor(u8g2,1);
            u8g2_DrawStr(u8g2, current_x, y, PAGES[i]);
        }
        else
        {
            u8g2_SetDrawColor(u8g2,0);
            u8g2_DrawStr(u8g2, current_x, y, PAGES[i]);
        }
        current_x += width + GAP_BETWEEN_TAGS;
    }

    /* Display Time */
    u8g2_SetDrawColor(u8g2, 0);
    u8g2_DrawStr(u8g2, 204, y, state.time_str);
}

inline void UI_DrawWifiIcon(u8g2_t *u8g2, const uint8_t *icon)
{
    u8g2_DrawXBMP(u8g2, 
                  (SCREEN_WIDTH - WIFI_ICON_WIDTH) >> 1, 
                  (((SCREEN_HEIGHT-NAV_BAR_HEIGHT)- WIFI_ICON_HEIGHT) >> 1) + NAV_BAR_HEIGHT, 
                  WIFI_ICON_WIDTH,
                  WIFI_ICON_HEIGHT,
                  icon);
}

inline void UI_PageTurn(u8g2_t *u8g2, Direction dir, App_State& state)
{
    if(dir == Direction::Left || dir == Direction::Right)
    {
        // update current page info
        state.current_page_index += (dir == Direction::Left) ? -1 : 1;
        if(state.current_page_index<0) state.current_page_index = 2;
        else if(state.current_page_index>2) state.current_page_index = 0;
    }
    // invaild direction
    else return;
}

inline void UI_DrawPageAbout(u8g2_t *u8g2)
{
    u8g2_SetDrawColor(u8g2, 1);
    u8g2_DrawXBMP(u8g2, 
                  3*TEXT_GAP_FROM_LEFT, 
                  (((SCREEN_HEIGHT-NAV_BAR_HEIGHT)- ICON_SIZE_BIGGER) >> 1) + NAV_BAR_HEIGHT, 
                  ICON_SIZE_BIGGER, 
                  ICON_SIZE_BIGGER-1,
                  icon_github);

    u8g2_DrawStr(u8g2, 
                 7*TEXT_GAP_FROM_LEFT + ICON_SIZE_BIGGER, 
                 LINE1+6, 
                 ABOUT_TEXT[0]);
    
    u8g2_DrawStr(u8g2, 
                 7*TEXT_GAP_FROM_LEFT + ICON_SIZE_BIGGER, 
                 LINE2+6,
                 ABOUT_TEXT[1]);

    u8g2_DrawStr(u8g2, 
                 7*TEXT_GAP_FROM_LEFT + ICON_SIZE_BIGGER, 
                 LINE3+6,
                 ABOUT_TEXT[2]);
}

inline void UI_DrawPageInfo(u8g2_t *u8g2, const App_State& state)
{
    u8g2_SetFont(u8g2, ARGOS_FONT);
    u8g2_SetDrawColor(u8g2, 1);
    
    char buf[128];
    
    snprintf(buf, sizeof(buf), "HOST: %s (%s)", state.host_name, state.os);
    u8g2_DrawStr(u8g2, TEXT_GAP_FROM_LEFT, LINE1, buf);
    
    snprintf(buf, sizeof(buf), "CPU:  %dC/%dT %dMHz %.1f%%", state.cpu_cores, state.cpu_threads, state.cpu_core_freq, state.cpu_usage);
    u8g2_DrawStr(u8g2, TEXT_GAP_FROM_LEFT, LINE2, buf);
    
    snprintf(buf, sizeof(buf), "MEM:  %d/%d MB %.1f%%", state.mem_used, state.mem_total, state.mem_usage);
    u8g2_DrawStr(u8g2, TEXT_GAP_FROM_LEFT, LINE3, buf);
    
    snprintf(buf, sizeof(buf), "DISK: %d/%d GB %.1f%%", state.disk_used, state.disk_total, state.disk_usage);
    u8g2_DrawStr(u8g2, TEXT_GAP_FROM_LEFT, LINE4, buf);
}

inline void UI_DrawPageWifi(u8g2_t *u8g2, const App_State& state)
{
    static bool boot_enter = true;

    u8g2_SetFont(u8g2, ARGOS_FONT);
    u8g2_SetDrawColor(u8g2, 1);
    static Animation<3> wifi_connecting_anime;

    if (state.wifi_state == WifiMsg::Failed)
    {
        UI_DrawWifiIcon(u8g2, icon_wifi_no_connect);
        /* If failed, do not turn to Info cuz there's nothing */
    }
    else if (state.wifi_state == WifiMsg::Connecting)
    {
        wifi_connecting_anime.update(WIFI_ANIMATION_INTERVAL);
        UI_DrawWifiIcon(u8g2, WIFI_CONNECTING_ICONS[wifi_connecting_anime.frame]);
    }
    else // WifiMsg::Connected
    {
        if (!wifi_connecting_anime.static_update(1500))
        {
            UI_DrawWifiIcon(u8g2, icon_wifi_connected);
        }
        /* on boot page turn, execute once only */
        if(boot_enter)
        {
            static Animation<1> waiting;
            if (waiting.static_update(1000))
            {
                 UI_PageTurn(u8g2, Direction::Left, const_cast<App_State&>(state)); // to Info page
                 boot_enter = false;
            }
        }
        /* page content */
        //u8g2_DrawStr(u8g2, 9, 24, state.wifi_ssid);
        
    }
}

inline void UI_DrawPage(u8g2_t *u8g2, const App_State& state)
{
    /* page state is driven by state update func */
    if(state.current_page_index == 1) // Network page
    {
        UI_DrawPageWifi(u8g2, state);
    }
    else if(state.current_page_index == 2) // About page
    {
        UI_DrawPageAbout(u8g2);
    }
    else if(state.current_page_index == 0) // Info page
    {
        UI_DrawPageInfo(u8g2, state);
    }
}

/*****************************************************************************************/

// Drain the WIFI -> UI queue and update cached App_State.
// Call this once per frame before UI_Render.
inline void UI_UpdateState(App_State& state, QueueHandle_t client_q)
{
    QueueHandle_t q = WIFI::get_ui_queue();
    if (!q) return;
    WifiMsg msg;
    while (xQueueReceive(q, &msg, 0) == pdTRUE)
    {
        state.wifi_state = msg.state;
        strlcpy(state.wifi_ssid, msg.ssid, sizeof(state.wifi_ssid));
    }

    /* Get Time */
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    if (timeinfo.tm_year > (2016 - 1900)) {
        strftime(state.time_str, sizeof(state.time_str), "%H:%M:%S", &timeinfo);
    }

    /* Get System Info */
    if (client_q) 
    {
        ClientMsg cmsg;
        while (xQueueReceive(client_q, &cmsg, 0) == pdTRUE)
        {
            strlcpy(state.host_name, cmsg.host_name, sizeof(state.host_name));
            strlcpy(state.os, cmsg.os, sizeof(state.os));
            strlcpy(state.os_distro, cmsg.os_distro, sizeof(state.os_distro));

            state.cpu_core_freq = cmsg.cpu_core_freq;
            state.cpu_cores     = cmsg.cpu_cores;
            state.cpu_threads   = cmsg.cpu_threads;
            state.cpu_usage     = cmsg.cpu_usage;
            state.cpu_temp      = cmsg.cpu_temp;

            state.mem_total     = cmsg.mem_total;
            state.mem_used      = cmsg.mem_used;
            state.mem_usage     = cmsg.mem_usage;

            state.disk_total    = cmsg.disk_total;
            state.disk_used     = cmsg.disk_used;
            state.disk_usage    = cmsg.disk_usage;
        }
    }
}

// Main Loop UI Render Function
// Each Device instance has its own UI App state & u8g2 structure
// Passed to render function and called in main_loop

inline void UI_Render(u8g2_t *u8g2, const App_State& state)
{
    u8g2_ClearBuffer(u8g2);

    UI_DrawStatic(u8g2);

    UI_DrawTag(u8g2, state);

    UI_DrawPage(u8g2, state); // Draw current selected page contents

    u8g2_SendBuffer(u8g2);
}
