
#pragma once

#include "../general/ddc_io.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <driver/gpio.h>

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
        /* Initialize queue */
        queue = xQueueCreate(20, sizeof(Encoder::EncoderMsg));
        Attach_Interrupt();
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

    /* Botton Detection Polling */
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
                xQueueSend(queue, &msg, 0); 
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
                xQueueSend(queue, &msg, 0); 
                #if ENCODER_DEBUG
                    ESP_LOGI(TAG, "Button Held");
                #endif
            }
        }
    }

    /* Rotation Detection Polling */
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
                    xQueueSend(queue, &msg, 0); 
                    #if ENCODER_DEBUG
                        ESP_LOGI(TAG, "Rotated Right (count: %d)", accumulation);
                    #endif
                } 
                else if (accumulation <= -static_cast<int8_t>(_accumulation_threshold)) 
                {
                    msg = EncoderMsg::RotateLeft;
                    xQueueSend(queue, &msg, 0); 
                    #if ENCODER_DEBUG
                        ESP_LOGI(TAG, "Rotated Left (count: %d)", accumulation);
                    #endif
                }
                accumulation = 0; // reset accumulation
            }
        }
    }

    /* Rotation Detection Interrupt Service Routine */
    /* -Logic ISR */
    void IRAM_ATTR Rotation_Detection_ISR()
    {
        uint8_t curr_AB = ((gpio_get_level(_pin_a.get_pin_num()) ? 1 : 0) << 1) | 
                           (gpio_get_level(_pin_b.get_pin_num()) ? 1 : 0);
        
        if (curr_AB != last_AB) // state changed
        {
            uint8_t transition = (last_AB << 2) | curr_AB;
        
            int8_t step = 0;
            /* Avoiding accessing table in an ISR */
            /* - Flash cache miss */
            switch(transition) {
                case 1: case 7: case 8: case 14: step = 1; break;
                case 2: case 4: case 11: case 13: step = -1; break;
                default: step = 0; break;
            }
            //step = TRANSITION_TABLE[transition];
            
            accumulation += step;
            last_AB = curr_AB;

            if (curr_AB == STATIC_STATE) 
            {
                if (accumulation >= static_cast<int8_t>(_accumulation_threshold)) 
                {
                    EncoderMsg isr_msg = EncoderMsg::RotateRight;
                    if (queue) {
                        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                        xQueueSendFromISR(queue, &isr_msg, &xHigherPriorityTaskWoken);
                        if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
                    }
                } 
                else if (accumulation <= -static_cast<int8_t>(_accumulation_threshold)) 
                {
                    EncoderMsg isr_msg = EncoderMsg::RotateLeft;
                    if (queue) {
                        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                        xQueueSendFromISR(queue, &isr_msg, &xHigherPriorityTaskWoken);
                        if (xHigherPriorityTaskWoken) portYIELD_FROM_ISR();
                    }
                }
                accumulation = 0;
            }
        }
    }
    
    /* GPIO Interrupt Initialization */
    void Attach_Interrupt()                                           
    {
        gpio_config_t io_conf = {};
        io_conf.intr_type = GPIO_INTR_ANYEDGE;
        io_conf.pin_bit_mask = (1ULL << _pin_a.get_pin_num()) | (1ULL << _pin_b.get_pin_num());
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        gpio_config(&io_conf);

        gpio_install_isr_service(0);
        gpio_isr_handler_add(_pin_a.get_pin_num(), isr_handler, (void*)this);
        gpio_isr_handler_add(_pin_b.get_pin_num(), isr_handler, (void*)this);
    }

    QueueHandle_t GetQueue() // should be called by other task
    {
        return queue;
    }
    
    private:
    /* Log Tag */
    static constexpr const char* TAG = "Encoder";
    /* GPIO Config */
    static constexpr gpio_mode_t GPIO_MODE_ENCODER      = GPIO_MODE_INPUT;
    static constexpr gpio_pull_mode_t GPIO_PULL_ENCODER = GPIO_PULLUP_ONLY;
    /* Polling Threshold */
    static constexpr uint16_t HOLD_THRESHOLD_MS = 300;
    static constexpr uint16_t DEBOUNCE_DELAY_MS = 100;
    /* Encoder static state AB gray code */
    static constexpr uint8_t STATIC_STATE = 0b11;
    static constexpr int8_t TRANSITION_TABLE[16]= // step table based on AB transition
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
    /* Pins */
    Pin _pin_a,
        _pin_b,
        _pin_button;
    /* state vars */
    uint32_t SEnteredTime        = 0;                     // button pressed time counter
    volatile uint8_t last_AB     = STATIC_STATE;          // last state of AB graycode
    volatile int8_t accumulation = 0;                     // total accumulated steps on a certain direction
    AccumulationThreshold _accumulation_threshold; // determine sensitivity of rotation detection
    BtnState btn_state           = BtnState::Idle;        // button FSM state
    EncoderMsg msg               = EncoderMsg::None;      // message sent to other task if needed
    /* ISR */
    QueueHandle_t queue = nullptr;                                   
    static void IRAM_ATTR isr_handler(void* arg) { // real ISR called by GPIO interrupt
        Encoder* enc = static_cast<Encoder*>(arg); // get instance pointer
        enc->Rotation_Detection_ISR();
    }
};