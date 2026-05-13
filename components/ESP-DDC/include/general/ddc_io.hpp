/*===================================================*/
/*      ____  ____  ____      ____  ____   ___       */
/*     (  __)/ ___)(  _ \ ___(    \(    \ / __)      */
/*      ) _) \___ \ ) __/(___)) D ( ) D (( (__       */
/*     (____)(____/(__)      (____/(____/ \___)      */
/*===================================================*/
/*           i4n@2026 | ddc_io | 2026-4-26            */
/*           GPIO pin abstraction for ESP-IDF        */
/*===================================================*/
/* Purpose: Wrap basic GPIO setup and control using   */
/*          ESP-IDF gpio APIs, with simple pin state  */
/*          helpers and automatic cleanup.            */
/*===================================================*/
#pragma once
/* Includes */
#include "soc/gpio_num.h"
#include <cassert>
#include <esp_err.h>
#include <driver/gpio.h>

/* Class */
class Pin
{
    public:
    /* Constructor */
    Pin(gpio_num_t pin_num, gpio_mode_t mode, gpio_pull_mode_t pull = GPIO_FLOATING)
    : _pin_num(pin_num)
    {
        esp_err_t ret = gpio_reset_pin(pin_num);
        assert(ret == ESP_OK);
        ret = gpio_set_direction(pin_num, mode);
        assert(ret == ESP_OK);
        ret = gpio_set_pull_mode(pin_num, pull);
        assert(ret == ESP_OK);
    }
    ~Pin()
    {
        gpio_reset_pin(_pin_num);
    }

    enum class State : uint8_t
    {
        Low  = 0,
        High = 1
    };

    esp_err_t set2(State s)
    {
        return gpio_set_level(_pin_num, static_cast<uint8_t>(s));
    }

    esp_err_t toggle()
    {
        return gpio_set_level(_pin_num, !gpio_get_level(_pin_num));
    }

    gpio_num_t get_pin_num() const
    {
        return _pin_num;
    }

    bool read() const
    {
        return gpio_get_level(_pin_num);
    }

     /* Destructor  */
     // Notice: If a destructor function is provided
     // the object will be destroyed when it goes out of scope
     // unless an infinite loop is used to prevent it from going out of scope

    protected:
    private:
    gpio_num_t _pin_num;

};