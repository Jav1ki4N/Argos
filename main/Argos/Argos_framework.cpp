
#include "Argos_framework.hpp"
#include "Argos_Page.hpp"
#include "devices/ddc_encoder.hpp"

/* Constructor */
ArgosFramework(SSD1322 &display):
u8g2(display.get_U8g2())
{}

/// @note Being a private struct it's definition however should be 
///       done in .cpp and before any public method uses it while
///       in header public method don't even care where you make the definition


/* Public */


/* Private Methods Implementation */

void setPencilMode(PencilMode mode)
{
    u8g2_SetDrawColor(u8g2, static_cast<uint8_t>(mode));
}

void drawStatic()
{
    setPencilMode(PencilMode::Solid);
    /* Outline */
    u8g2_DrawFrame(u8g2, 0, 0, HWINFO::WIDTH,
                                HWINFO::WEIGHT);
    /* Navigation Bar background*/     
    u8g2_DrawBox(u8g2, 0, 0, STATIC::NAV::NAV_BAR_WIDTH,
                              STATIC::NAV::NAV_BAR_HEIGHT);
    /* Title */
    setPencilMode(PencilMode::Hollow);
    u8g2_SetFont(u8g2, FONT::BASE_FONT);
    u8g2_DrawStr(u8g2, STATIC::TITLE::GAP_FROM_LEFT, 
                       STATIC::NAV::NAV_BAR_HEIGHT - STATIC::TITLE::GAP_FROM_BUTTOM, 
                       TITLE);
}

void drawTabs()
{
    u8g2_SetFont(u8g2, FONT::BASE_FONT);
    uint8_t focus_tab = system_state.focus_tab;
    uint8_t tab_y = STATIC::NAV::NAV_BAR_HEIGHT - STATIC::TITLE::GAP_FROM_BUTTOM;

    uint8_t tab_curr_x = STATIC::TITLE::GAP_FROM_LEFT      + 
                         STATIC::TITLE::GAP_FROM_FIRST_TAG + 
                         u8g2_GetStrWidth(u8g2, TITLE);

    uint8_t radius = 1;
    uint8_t delimeter_width = u8g2_GetStrWidth(u8g2, "|");

    const constexpr char* text[3] = {"INFO", "NETWORK", "ABOUT"};

    uint8_t tab_height = u8g2_GetFontAscent(u8g2) - u8g2_GetFontDescent(u8g2);

    for (uint8_t tab_cnt = 0; tab_cnt < std::size(text); tab_cnt++)
    {
        uint8_t tab_width  = u8g2_GetStrWidth(u8g2, text[tab_cnt]);
        
        /* Draw Delimeter */
        uint8_t delimeter_x = tab_curr_x + tab_width + 
                              ((STATIC::TABS::GAP_BETWEEN - delimeter_width)>>1);
        setPencilMode(PencilMode::Hollow);
        if (tab_cnt != 2) u8g2_DrawStr(u8g2, delimeter_x, tab_y, "|");
        
        /* Draw Tab */
        if (tab_cnt == focus_tab)
        {
            u8g2_DrawRBox(u8g2, 
                          tab_curr_x - 2, 
                          tab_y - tab_height + 1, 
                          tab_width + 4,
                          tab_height,
                          radius);
        }
        setPencilMode((tab_cnt == focus_tab) ? PencilMode::Solid : PencilMode::Hollow);
        u8g2_DrawStr(u8g2, tab_curr_x, tab_y, text[tab_cnt]);
  
        tab_curr_x += tab_width + STATIC::TABS::GAP_BETWEEN;
    }
}

void drawTime()
{
    setPencilMode(PencilMode::Hollow);
    u8g2_SetFont(u8g2, FONT::BASE_FONT);
    uint8_t time_y = STATIC::NAV::NAV_BAR_HEIGHT - STATIC::TITLE::GAP_FROM_BUTTOM;
    u8g2_DrawStr(u8g2, STATIC::TABS::TIME_X, time_y, system_state.curr_time);
}

void drawOverlay()
{
    drawTabs();
    drawTime();
}

