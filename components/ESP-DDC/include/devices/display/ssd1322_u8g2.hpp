#pragma once
/* includes */
#include "../../thirdparty/ddc_u8g2.hpp"
#include "driver/spi_master.h"
#include "hal/gpio_types.h"
#include "soc/gpio_num.h"
#include "u8g2.h"
#include "../display/UI/Argos_u8g2.hpp"
#include <stdint.h>

class SSD1322 : public SPIDevice
{
    public:
    SSD1322(SPI &spi,
            gpio_num_t dc,
            gpio_num_t rst,
            gpio_num_t cs)

    : SPIDevice(spi, &device_config, cs),         // chip select, managed by SPIDevice
      dc_pin   (dc, GPIO_MODE_OUTPUT),            // data/command
      rst_pin  (rst, GPIO_MODE_OUTPUT),           // reset
      hal      (cs_pin, dc_pin, rst_pin, _handle) // u8g2 HAL 
    {
        /* GUI Lib Init */
        u8g2_Setup_ssd1322_nhd_256x64_f(&u8g2, 
                                        U8G2_R0,
                                        spi_byte_cb_static, 
                                        gpio_and_delay_cb_static);
        u8g2.u8x8.user_ptr = this;
        u8g2_InitDisplay(&u8g2);
        u8g2_SetPowerSave(&u8g2, 0);

        /* Framework Init */
        ui_Init();
        
    }

    /* UI */
    /* All funcs passes no u8g2_t struct */
    /* And are ready-to-call */
    /* For low-level operations check Argos_u8g2.hpp */

    enum class Page : uint8_t
    {
        info = 0,
        network = 1,
        about = 2
    };

    void ui_Init()
    {
        // Shapes
        u8g2_ClearBuffer(&u8g2);
        u8g2_SendBuffer (&u8g2);
        ui_drawOutline  (&u8g2);
        ui_drawNavBar   (&u8g2);

        // Nav Tags
        ui_PencilMode(&u8g2,PencilMode::erase);
        ui_drawProjTitle(&u8g2);
        ui_drawTag(&u8g2, "INFO");
        ui_drawTag(&u8g2, "NETWORK");
        ui_drawTag(&u8g2, "ABOUT");

        // Booting Screen Animation
        ui_PlayBootingScreen(&u8g2);

        // Fresh
        u8g2_SendBuffer(&u8g2);
        ui_PencilMode(&u8g2, PencilMode::draw);
    }

    protected:

    private:
    static inline spi_device_interface_config_t device_config = 
    {
        .mode             = 0,
        .clock_speed_hz   = 10 * 1000 * 1000, // 10 MHz
        .spics_io_num     = -1,               // taken over by u8g2 
        .queue_size       = 7 
    };
    Pin dc_pin,
        rst_pin;

    u8g2_hal hal;
    u8g2_t   u8g2;

    /* static callback for u8g2 */
    static uint8_t spi_byte_cb_static(u8x8_t* u8x8,
                                      uint8_t msg,
                                      uint8_t arg_int,
                                      void* arg_ptr)
    {  
        auto *self = static_cast<SSD1322*>(u8x8->user_ptr);
        return self->hal.spi_byte_cb(u8x8, msg, arg_int, arg_ptr);
    }

    static uint8_t gpio_and_delay_cb_static(u8x8_t* u8x8,
                                            uint8_t msg,
                                            uint8_t arg_int,
                                            void* arg_ptr)
    {
        auto *self = static_cast<SSD1322*>(u8x8->user_ptr);
        return self->hal.gpio_and_delay_cb(u8x8, msg, arg_int, arg_ptr);
    }
};