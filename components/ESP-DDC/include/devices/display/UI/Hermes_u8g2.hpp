/* Hermes UI */
#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include "u8g2.h"
#include "Hermes_icon.hpp"

inline struct Frame
{
    uint8_t x=0, y=0;
    uint16_t width=256;
    uint8_t height = 64;
}frame;

inline struct Nav
{
    // nav bar pos
    uint8_t x=1, y=1;
    // nav bar size
    uint16_t width = 256;
    uint8_t  height = 12;

    // nav bar content
    const uint8_t *font = u8g2_font_profont11_tr;
    struct ProjTitle
    {
        uint8_t gap_from_nav = 2;
        const std::string text = "Hermes V1.0";
    }projTitle;

    uint8_t projTitle_x() const {return x + projTitle.gap_from_nav;}
    uint8_t projTitle_y() const {return y + height - 3;}
    uint8_t get_projTitle_length(u8g2_t *u8g2) const 
    {
        u8g2_SetFont(u8g2, font);
        return u8g2_GetStrWidth(u8g2, projTitle.text.c_str());
    }

    /* tags */
    uint8_t gap_tags = 10;
    struct Tag
    {
        uint8_t distance_from_projTitle = 20;
        std::vector<std::string> items = {
            "INFO", "NETWORK", "ABOUT"
        };
    } tag;

    uint8_t get_tagX(u8g2_t *u8g2, const std::string& tag_text) const
    {
        u8g2_SetFont(u8g2, font);
        uint8_t x = get_projTitle_length(u8g2) + projTitle_x() + gap_tags + tag.distance_from_projTitle;
        for (const auto& text : tag.items)
        {
            if (text == tag_text) break;
            x += u8g2_GetStrWidth(u8g2, text.c_str()) + gap_tags;
        }
        return x;
    }

    uint8_t get_tagY() const { return projTitle_y(); }

    uint8_t get_tagLength(u8g2_t *u8g2, const std::string& tag_text) const
    {
        u8g2_SetFont(u8g2, font);
        return u8g2_GetStrWidth(u8g2, tag_text.c_str());
    }
} nav;

inline struct Page
{
    /* page pos */
    uint8_t get_PageX() const { return frame.x + 1; }
    uint8_t get_PageY() const { return frame.y + nav.height + 1; }
    uint8_t get_PageWidth() const { return frame.width - 2; } // 254
    uint8_t get_PageHeight() const { return frame.height - nav.height - 2; } // 50

    /* pages */
    struct Info
    {

    }info;

    struct Network
    {
       uint8_t wifi_length = 40;
       uint8_t wifi_height = 35;
    }network;
    uint8_t get_wifiX() const {return (get_PageWidth() - network.wifi_length)>>1;}
    uint8_t get_wifiY() const {return 21;}


    struct About
    {

    }about;

}page;

inline void ui_drawOutline(u8g2_t *u8g2)
{
    u8g2_DrawFrame(u8g2,frame.x, frame.y, frame.width, frame.height);
}

inline void ui_drawNavBar(u8g2_t *u8g2)
{
    u8g2_DrawBox(u8g2, nav.x, nav.y, nav.width, nav.height);
}

inline void ui_color(u8g2_t *u8g2, uint8_t mode)
{
    u8g2_SetDrawColor(u8g2, mode);
}

inline void ui_drawProjTitle(u8g2_t *u8g2)
{
    u8g2_SetFont(u8g2, nav.font);
    u8g2_DrawStr(u8g2, nav.projTitle_x(), nav.projTitle_y(), nav.projTitle.text.c_str());
}

inline void ui_drawTag(u8g2_t *u8g2, const std::string& tag_text)
{
    uint8_t x = nav.get_tagX(u8g2, tag_text);
    uint8_t y = nav.get_tagY();
    uint8_t tag_len = nav.get_tagLength(u8g2, tag_text);

    u8g2_SetFont(u8g2, nav.font);
    u8g2_DrawStr(u8g2, x, y, tag_text.c_str());

    uint8_t sep_width = u8g2_GetStrWidth(u8g2, "|");
    uint8_t sep_x = x + tag_len + (nav.gap_tags - sep_width) / 2;
    u8g2_DrawStr(u8g2, sep_x, y, "|");
}

inline void ui_deselectTag(u8g2_t *u8g2, const std::string& tag_text)
{
    if (tag_text.empty()) return;
    uint8_t radius = 0;
    uint8_t side_gap=2;
    uint8_t x = nav.get_tagX(u8g2, tag_text) - side_gap;
    uint8_t y = nav.get_tagY() - 8;
    uint8_t w = nav.get_tagLength(u8g2, tag_text) + 2*side_gap;
    uint8_t h = 9;
    ui_color(u8g2, 2);
    u8g2_DrawRBox(u8g2, x, y, w, h, radius);
    ui_color(u8g2, 1);
}

inline void ui_SelectTag(u8g2_t *u8g2, const std::string& tag_text)
{
    static std::string last_selected = "";
    ui_deselectTag(u8g2, last_selected);
    last_selected = tag_text;
    uint8_t radius = 0;
    uint8_t side_gap=2;
    uint8_t x = nav.get_tagX(u8g2, tag_text) - side_gap;
    uint8_t y = nav.get_tagY() - 8;
    uint8_t w = nav.get_tagLength(u8g2, tag_text) + 2*side_gap;
    uint8_t h = 9;
    ui_color(u8g2, 2);
    u8g2_DrawRBox(u8g2, x, y, w, h, radius);
    ui_color(u8g2, 1);
}

inline void ui_fillPage(u8g2_t *u8g2)
{
    uint8_t x = page.get_PageX();
    uint8_t y = page.get_PageY();
    uint8_t w = page.get_PageWidth();
    uint8_t h = page.get_PageHeight();
    u8g2_DrawBox(u8g2, x, y, w, h);
}

inline void ui_drawIcon_Wifi(u8g2_t *u8g2,WifiIcon wifi_icon)
{
    ui_color(u8g2,1);
    u8g2_DrawXBMP(u8g2, page.get_wifiX(), 
                        page.get_wifiY(), 
                        page.network.wifi_length, 
                        page.network.wifi_height, wifi_icons[static_cast<uint8_t>(wifi_icon)]);
    ui_color(u8g2,0);
}



