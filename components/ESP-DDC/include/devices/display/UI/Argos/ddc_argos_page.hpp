
#pragma once

#include "u8g2.h"

class ArgosPage
{
    public:
        ArgosPage() = default;
        virtual ~ArgosPage() = default;

        virtual void draw(u8g2_t* u8g2) = 0; // Pure virtual function for drawing the page
    protected:
    private:
};