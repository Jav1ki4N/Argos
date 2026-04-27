/*==============================*/
/* UI for Argos */
/* Display：SSD1322 256*64 */
/* ColorDepth: 1-bit Monochrome */
/* Graphic Lib: u8g2 */
/*===============================*/

#pragma once
#include "u8g2.h"
#include <cstdint>

// Element position
// All data is decreased by 1, cuz screen index starts from 0
static constexpr uint8_t SCREEN_WIDTH = 255, 
                         SCREEN_HEIGHT = 63,
                         NAV_BAR_WIDTH = 255,
                         NAV_BAR_HEIGHT = 13,
                         TITLE_GAP_FROM_LEFT = 3,
                         TITLE_GAP_FROM_BUTTOM =2,
                         FIRST_TAG_FROM_TITLE = 20,
                         GAP_BETWEEN_TAGS = 12;

// Other constants
static constexpr const uint8_t* ARGOS_FONT  = (uint8_t*)u8g2_font_profont11_tr;
static constexpr const char*    ARGOS_TITLE = "Argos V1.0";
static constexpr const char*    PAGES[3]    = {"INFO", "NETWORK", "ABOUT"};


enum class Direction : uint8_t
{
    left  = 0,
    right = 1,
    up    = 2,
    down  = 3
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
    int current_page_index = 1;      // basically current page     
};

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
        u8g2_DrawStr(u8g2, sep_x, y, "|");
        
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
}

/* Below functions started with the prefix 'Argos'             */
/* This indicates that they are designed to be called by user  */

inline void Argos_PageTurn(u8g2_t *u8g2, Direction dir, App_State& state)
{
    if(dir == Direction::left || dir == Direction::right)
    {
        // update current page info
        state.current_page_index += (dir == Direction::left) ? -1 : 1;
        if(state.current_page_index<0) state.current_page_index = 2;
        else if(state.current_page_index>2) state.current_page_index = 0;
    }
    // invaild direction
    else return;
}

/*****************************************************************************************/

// Main Loop UI Render Function
// Each Device instance has its own UI App state & u8g2 structure
// Passed to render function and called in main_loop

inline void Argos_UI_Render(u8g2_t *u8g2, const App_State& state)
{
    u8g2_ClearBuffer(u8g2);

    UI_DrawStatic(u8g2);

    UI_DrawTag(u8g2, state);

    u8g2_SendBuffer(u8g2);
}
