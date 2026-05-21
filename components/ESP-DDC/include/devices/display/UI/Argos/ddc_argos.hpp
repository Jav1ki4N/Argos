
/**  
 * UI of argos
 * Based on u8g2 lib & SSD1322 display 
 */

#pragma once

/**  
 * Hardware Support
 * Graphic Library is included along with the display class 
 */

#include "u8g2.h"
# define USE_SSD1322  true
# define USE_SSD1363Z false //todo hardware upgrade

#if USE_SSD1322
    #include "../../../../devices/display/ddc_ssd1322_u8g2.hpp"
#endif

#include <variant>
#include "ddc_argos_icons.hpp"
#include "../../../../devices/ddc_encoder.hpp"

class ArgosFramework 
{
    public:
    ArgosFramework(SSD1322 &display)
    :u8g2(display.get_U8g2())
    {}
    ~ArgosFramework() = default;

    void start()
    {
        u8g2_ClearBuffer(u8g2);
        drawStatic();
    }

    private:
    u8g2_t* u8g2;
    static constexpr const char* TITLE = "Argos V1.0";
    struct SystemState 
    {
        uint8_t focus_tab = 0;
    }global_state;
    
    struct HWINFO // Hardware info
    {
        static constexpr const uint8_t WIDTH  = 255;
        static constexpr const uint8_t WEIGHT = 64;
    };

    struct STATIC // static element placement
    {
        struct NAV
        {
            static constexpr const uint8_t NAV_BAR_WIDTH  = 255;
            static constexpr const uint8_t NAV_BAR_HEIGHT = 13;
        };
   
        struct TITLE
        {
            static constexpr const uint8_t GAP_FROM_LEFT      = 3;
            static constexpr const uint8_t GAP_FROM_BUTTOM    = 3;
            static constexpr const uint8_t GAP_FROM_FIRST_TAG = 13;
        };

        struct TABS
        {
            static constexpr const uint8_t GAP_BETWEEN = 12;
        };

        struct ICON
        {
            static constexpr const uint8_t SIZE_S = 32; // small
            static constexpr const uint8_t SIZE_M = 40; // medium
        };

        struct PAGE
        {
            static constexpr const uint8_t LINE[4] = {29,42,55,60};
        };
    };

    struct FONT
    {
        static constexpr const uint8_t* BASE_FONT = (uint8_t*)u8g2_font_profont11_tr;
    };

    enum class PencilMode : uint8_t
    {
        Hollow = 0,
        Solid  = 1,
        Invert = 2
    };



    /*********************************************************************************************************/

    //  Func   setPencilMode
    /// @brief Set the drawing mode for subsequent drawing operations.
    void setPencilMode(PencilMode mode){ u8g2_SetDrawColor(u8g2, static_cast<uint8_t>(mode)); }

    //  Func  drawStatic
    /// @brief Draw static UI elements that do not change across pages
    ///        The outer frame, navigation bar background, and title are drawn here.
    void drawStatic(){
        setPencilMode(PencilMode::Solid);
        /* Outline */
        u8g2_DrawFrame(u8g2,0,0,HWINFO::WIDTH,
                                 HWINFO::WEIGHT);
        /* Navigation Bar background*/     
        u8g2_DrawBox(u8g2,0,0,STATIC::NAV::NAV_BAR_WIDTH,
                               STATIC::NAV::NAV_BAR_HEIGHT);
        /* Title */
        setPencilMode(PencilMode::Hollow);
        u8g2_SetFont(u8g2, FONT::BASE_FONT);
        u8g2_DrawStr(u8g2, STATIC::TITLE::GAP_FROM_LEFT, 
                           STATIC::NAV::NAV_BAR_HEIGHT - STATIC::TITLE::GAP_FROM_BUTTOM, 
                           TITLE);
    }

    //  Func   drawTabs
    /// @brief draw navigation tabs & selected effect
    void drawTabs()
    {
        u8g2_SetFont(u8g2, FONT::BASE_FONT);
        uint8_t focus_tab = global_state.focus_tab;
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

    //  Func   drawTime
    /// @brief Draw the current time on the navigation bar
    void drawTime()
    {
        //todo
    }

    //  Func  drawOverlay
    /// @brief Draw dyanmic elements that are overlaid on all pages, such as navigation tabs and time.
    void drawOverlay()
    {
        drawTabs();
        drawTime();
    }

    /*********************************************************************************************************/


    struct AddProfilePayload
    {};

    struct LoadProfilePayload
    {};

    using EventPayload = std::variant<
          AddProfilePayload, 
          LoadProfilePayload
    >;
};