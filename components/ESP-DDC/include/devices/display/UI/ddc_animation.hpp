#pragma once
#include <cstdint>
#include <esp_timer.h>
#include <initializer_list>

template<uint8_t FRAME_COUNT>
struct Animation
{
    uint8_t  frame     = 0;
    uint32_t last_tick = 0;
    bool     running   = true;
    uint32_t durations[FRAME_COUNT] = {};  

    bool static_update(uint32_t interval_ms)
    {
        static uint32_t begin = static_cast<uint32_t>(esp_timer_get_time() / 1000);
        uint32_t now = static_cast<uint32_t>(esp_timer_get_time() / 1000);
        if (now - begin >= interval_ms)
        {
            return true;
        }
        return false;
    }

    // default using static frame duration, pass interval_ms to tick()
    bool update(uint32_t interval_ms)
    {
        if (!running) return false;
        uint32_t now = static_cast<uint32_t>(esp_timer_get_time() / 1000); // 转换为毫秒
        
        if (last_tick == 0) {
            last_tick = now;
            return false;
        }
        
        if (now - last_tick >= interval_ms)
        {
            frame     = (frame + 1) % FRAME_COUNT;
            last_tick = now;
            return true;
        }
        return false;
    }

    // if a duration table is provided via set_durations()
    // dynamic frame durations
    bool update()
    {
        if (!running) return false;
        uint32_t now = static_cast<uint32_t>(esp_timer_get_time() / 1000); // 转换为毫秒
        
        if (last_tick == 0) {
            last_tick = now;
            return false;
        }

        if (now - last_tick >= durations[frame]) 
        {
            frame     = (frame + 1) % FRAME_COUNT;
            last_tick = now;
            return true;
        }
        return false;
    }

    // Set durations for each frame using an initializer list
    // Passes [,,, ...] format
    void set_durations(std::initializer_list<uint32_t> list)
    {
        uint8_t i = 0;
        for (uint32_t d : list)
        {
            if (i >= FRAME_COUNT) break;
            durations[i++] = d;
        }
    }

    void reset()
    {
        frame     = 0;
        last_tick = static_cast<uint32_t>(esp_timer_get_time() / 1000);
        running   = true;
    }

    void stop()  { running = false; }
    void start() { running = true;  }
};