/* Hermes UI */
#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include "u8g2.h"

struct Frame
{
    uint8_t x=0, y=0;
    uint16_t width=256;
    uint8_t height = 64;
}frame;

struct Nav
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

inline void ui_drawOutline(u8g2_t *u8g2)
{
    u8g2_DrawFrame(u8g2,frame.x, frame.y, frame.width, frame.height);
}

inline void ui_drawNavBar(u8g2_t *u8g2)
{
    u8g2_DrawBox(u8g2, nav.x, nav.y, nav.width, nav.height);
}

inline void ui_color(u8g2_t *u8g2, bool isSolid)
{
    u8g2_SetDrawColor(u8g2, isSolid);
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



