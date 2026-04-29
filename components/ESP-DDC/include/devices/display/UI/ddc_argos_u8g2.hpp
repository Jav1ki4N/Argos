/*==============================*/
/* UI for Argos */
/* Display：SSD1322 256*64 */
/* ColorDepth: 1-bit Monochrome */
/* Graphic Lib: u8g2 */
/*===============================*/

#pragma once
#include "ddc_argos_icon.hpp"
#include "u8g2.h"
#include <cstdint>
#include <time.h>
#include "ddc_animation.hpp"
#include "../../../network/ddc_wifi.hpp"

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
                         WIFI_ICON_WIDTH = 40,
                         WIFI_ICON_HEIGHT = 35;

// Other constants
static constexpr const uint8_t* ARGOS_FONT  = (uint8_t*)u8g2_font_profont11_tr;
static constexpr const char*    ARGOS_TITLE = "Argos V1.0";
static constexpr const char*    PAGES[3]    = {"INFO", "NETWORK", "ABOUT"};
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

// UI Application state & Info

struct App_State
{
    int current_page_index = 1;
    WifiMsg::State wifi_state = WifiMsg::Connecting;
    char wifi_ssid[32] = {};
    char time_str[16] = "00:00:00";
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

inline void UI_DrawPageWifi(u8g2_t *u8g2, const App_State& state)
{
    u8g2_SetFont(u8g2, ARGOS_FONT);
    u8g2_SetDrawColor(u8g2, 1);
    static Animation<3> wifi_connecting_anime;

    if (state.wifi_state == WifiMsg::Failed)
    {
        UI_DrawWifiIcon(u8g2, icon_wifi_no_connect);
    }
    else if (state.wifi_state == WifiMsg::Connecting)
    {
        wifi_connecting_anime.update(WIFI_ANIMATION_INTERVAL);
        UI_DrawWifiIcon(u8g2, WIFI_CONNECTING_ICONS[wifi_connecting_anime.frame]);
    }
    else // WifiMsg::Connected
    {
        if (!wifi_connecting_anime.static_update(1500))
            UI_DrawWifiIcon(u8g2, icon_wifi_connected); // enter page
        /* page content */
        //u8g2_DrawStr(u8g2, 9, 24, state.wifi_ssid);
        u8g2_DrawBox(u8g2, 9, 23, 32, 32);
    }
}

inline void UI_DrawPage(u8g2_t *u8g2, const App_State& state)
{
    if(state.current_page_index == 1) // network page
    {
        UI_DrawPageWifi(u8g2, state);
    }
    else if(state.current_page_index == 2) // About page
    {

    }
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

/*****************************************************************************************/

// Drain the WIFI -> UI queue and update cached App_State.
// Call this once per frame before UI_Render.
inline void UI_UpdateState(App_State& state)
{
    QueueHandle_t q = WIFI::get_ui_queue();
    if (!q) return;
    WifiMsg msg;
    while (xQueueReceive(q, &msg, 0) == pdTRUE)
    {
        state.wifi_state = msg.state;
        strlcpy(state.wifi_ssid, msg.ssid, sizeof(state.wifi_ssid));
    }

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    if (timeinfo.tm_year > (2016 - 1900)) {
        strftime(state.time_str, sizeof(state.time_str), "%H:%M:%S", &timeinfo);
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
