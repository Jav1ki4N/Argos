
#pragma once

#include "../general/ddc_io.hpp"
#include "freertos/FreeRTOS.h"
#include <esp_log.h>
#include <esp_timer.h>

#define ENCODER_DEBUG 1
#define AFTER_HELD_DEBOUNCE 1

class Encoder
{
    enum class AccumulationThreshold : int8_t
    {
        Low    = 1,  // {1, -1} — most sensitive (every half-step)
        Normal = 2,  // {2, -2} — one full detent
        High   = 4   // {4, -4} — two detents
    };

    public:
    Encoder(gpio_num_t pin_a, gpio_num_t pin_b, gpio_num_t pin_button,
            AccumulationThreshold accumulation_threshold = AccumulationThreshold::Normal)
    : _pin_a     (pin_a, GPIO_MODE_ENCODER, GPIO_PULL_ENCODER),
      _pin_b     (pin_b, GPIO_MODE_ENCODER, GPIO_PULL_ENCODER),
      _pin_button(pin_button, GPIO_MODE_ENCODER, GPIO_PULL_ENCODER),
      _accumulation_threshold(accumulation_threshold)
    {
        /* Update pulse graycode */
        /* -e.g. A=1, B=0 -> Graycode 10 */
        last_AB = ((_pin_a.read() ? 1 : 0) << 1) | (_pin_b.read() ? 1 : 0);
    }
    
    enum class BtnState : uint8_t
    {
        Idle,
        SPressed,
        SHeld,
    };

    enum class EncoderMsg : uint8_t
    {
        None,
        RotateLeft,
        RotateRight,
        ButtonPressed,
        ButtonHeld
    };

    void Botton_Detection()
    {
        uint32_t SCurrentTime = 0;
        if(btn_state == BtnState::Idle)
        {
            if(!_pin_button.read())btn_state = BtnState::SPressed; // LOW = pressed (pull-up + GND)
            SEnteredTime = static_cast<uint32_t>(esp_timer_get_time() / 1000);
        }
        else if(btn_state == BtnState::SPressed)
        {
            SCurrentTime = static_cast<uint32_t>(esp_timer_get_time() / 1000);
            if(SCurrentTime - SEnteredTime >= HOLD_THRESHOLD_MS){
                btn_state = BtnState::SHeld;
                SEnteredTime = 0;
            }
            if(_pin_button.read())
            {
                btn_state = BtnState::Idle; // released before hold threshold
                SEnteredTime = 0;
                msg = EncoderMsg::ButtonPressed; // message -> short press
                #if ENCODER_DEBUG
                    ESP_LOGI(TAG, "Button Pressed");
                #endif
            }
        }
        else if(btn_state == BtnState::SHeld)
        {
            if(_pin_button.read()) // HIGH = released (pull-up + GND)
            {
                btn_state = BtnState::Idle; // hold released
                msg = EncoderMsg::ButtonHeld;    // message -> long press
                #if ENCODER_DEBUG
                    ESP_LOGI(TAG, "Button Held");
                #endif
                #if AFTER_HELD_DEBOUNCE
                    vTaskDelay(AFTER_HELD_DEBOUNCE / portTICK_PERIOD_MS); // debounce after hold action
                #endif
            }
        }
    }

    void Rotation_Detection()
    {
        uint8_t curr_AB = ((_pin_a.read() ? 1 : 0) << 1) | (_pin_b.read() ? 1 : 0);
        
        if (curr_AB != last_AB) // state changed
        {
            uint8_t transition = (last_AB << 2) | curr_AB; // transition bitmask
            int8_t  step = TRANSITION_TABLE[transition];   // step of rotation based on transition
                                                           // -1 to CCW, 1 to CW, 0 to invalid/unchanged
            
            accumulation += step;                          // accumulate steps until it returns to static state (11)
            last_AB = curr_AB;

            /* If current state is static state (A,B)=(1,1) */
            /* check accumulation to determine rotation direction */
            if (curr_AB == STATIC_STATE) 
            {
                if (accumulation >= static_cast<int8_t>(_accumulation_threshold)) 
                {
                    msg = EncoderMsg::RotateRight;
                    #if ENCODER_DEBUG
                        ESP_LOGI(TAG, "Rotated Right (count: %d)", accumulation);
                    #endif
                } 
                else if (accumulation <= -static_cast<int8_t>(_accumulation_threshold)) 
                {
                    msg = EncoderMsg::RotateLeft;
                    #if ENCODER_DEBUG
                        ESP_LOGI(TAG, "Rotated Left (count: %d)", accumulation);
                    #endif
                }
                accumulation = 0; // reset accumulation
            }
        }
    }

    EncoderMsg GetMsg()
    {
        EncoderMsg temp = msg;
        msg = EncoderMsg::None; // reset message after reading
        return temp;
    }
    
    private:
    static constexpr const char* TAG = "Encoder";
    static constexpr gpio_mode_t GPIO_MODE_ENCODER = GPIO_MODE_INPUT;
    static constexpr gpio_pull_mode_t GPIO_PULL_ENCODER = GPIO_PULLUP_ONLY;
    static constexpr uint16_t HOLD_THRESHOLD_MS = 300;
    static constexpr uint16_t DEBOUNCE_DELAY_MS = 100;
    static constexpr uint8_t STATIC_STATE = 0b11;
    Pin _pin_a,
        _pin_b,
        _pin_button;

    uint32_t SEnteredTime = 0;
    uint8_t last_AB = STATIC_STATE; // default static state (A,B)=(1,1) -> Graycode 11

    static constexpr int8_t TRANSITION_TABLE[16]=
    {
        /* last state -> current state */
        /* 0  --> invalid or unchanged */
        /* 1  --> CW */
        /* -1 --> CCW */
        0, 1, -1, 0, // 0000, 0001, 0010, 0011
        -1, 0, 0, 1, // 0100, 0101, 0110, 0111
        1, 0, 0, -1, // 1000, 1001, 1010, 1011
        0, -1, 1, 0  // 1100, 1101, 1110, 1111
    };

    int8_t accumulation = 0;
    AccumulationThreshold _accumulation_threshold;

    BtnState btn_state = BtnState::Idle;
    EncoderMsg msg = EncoderMsg::None;
};