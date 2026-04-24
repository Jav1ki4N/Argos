/*===================================================*/
/*      ____  ____  ____      ____  ____   ___       */
/*     (  __)/ ___)(  _ \ ___(    \(    \ / __)      */
/*      ) _) \___ \ ) __/(___)) D ( ) D (( (__       */
/*     (____)(____/(__)      (____/(____/ \___)      */
/*===================================================*/
/*          i4n@2026 | ddc_u8g2 | 2026-4-24          */
/*        ESP-IDF Abstraction Layer for u8g2         */
/*===================================================*/
/* Ref: - https://github.com/olikraus/u8g2           */
/*      - https://github.com/mkfrey/u8g2-hal-esp-idf */
/*===================================================*/


/* Includes */
#include <cstring>
#include "driver/spi_master.h"
#include "esp_log.h"
#include "soc/gpio_num.h"
#include "u8g2.h"
#include "../general/ddc_spi_device.hpp"
#include "u8x8.h"

using enum Pin::state;

/* class */
/* This class should serve as a class member of a specific display class   */
/* Two static functions: bus & gpio_and_delay should be provided explictly */
/* Init is not provided as it varies on the specific display               */

class u8g2_hal
{
    public:
    /* SPI */
    u8g2_hal(Pin &cs,  // provided by Device class instead of device_interface_config
                       // so that cs pin is managed only by u8g2
             Pin &dc, 
             Pin &rst,
             spi_device_handle_t handle)
    : config{cs, dc, rst}, _handle(handle)
    {}

    /* I2C */
    /* todo */
    
    /* Callback funcs                                  */
    /* Made public so static callbacks can access them */

    uint8_t spi_byte_cb(u8x8_t* u8x8,
                        uint8_t msg,
                        uint8_t arg_int,
                        void* arg_ptr)
    {
        ESP_LOGD("u8g2", "spi_byte_cb: Received a msg: %d, arg_int: %d, arg_ptr: %p",
                                                       msg,         arg_int,     arg_ptr);
        switch(msg)
        {
            case U8X8_MSG_BYTE_SET_DC:
                if(config.dc.get_pin_num() != GPIO_NUM_NC)config.dc.set2(arg_int?high:low);
                break;
            case U8X8_MSG_BYTE_INIT:
                // bus init already done in ddc_spi.hpp constructor, so do nothing here
                // device added in ddc_spi_device,hpp constructor, so do nothing here
                break;

            case U8X8_MSG_BYTE_SEND: {
                spi_transaction_t trans_desc;
                std::memset(&trans_desc, 0, sizeof(trans_desc));
                trans_desc.addr      = 0;
                trans_desc.cmd       = 0;
                trans_desc.flags     = 0;
                trans_desc.length    = 8 * arg_int;  // Number of bits NOT number of bytes.
                trans_desc.rxlength  = 0;
                trans_desc.tx_buffer = arg_ptr;
                trans_desc.rx_buffer = NULL;
                esp_err_t ret = spi_device_transmit(_handle, &trans_desc);
                assert(ret == ESP_OK);
                break;
            }
        }
        return 0;
    }

    uint8_t gpio_and_delay_cb(u8x8_t* u8x8,
                              uint8_t msg,
                              uint8_t arg_int,
                              void*   arg_ptr)
    {
        ESP_LOGD("u8g2","gpio_and_delay_cb: Received a msg: %d, arg_int: %d, arg_ptr: %p",
                                                            msg,         arg_int,     arg_ptr);
        switch(msg)
        {
            case U8X8_MSG_GPIO_AND_DELAY_INIT:
                // done in advance 
                break;
            case U8X8_MSG_GPIO_RESET:
                if(config.rst.get_pin_num() != GPIO_NUM_NC)config.rst.set2(arg_int?high:low);
                break;
            case U8X8_MSG_GPIO_CS:
                if(config.cs.get_pin_num() != GPIO_NUM_NC)config.cs.set2(arg_int?high:low);
                break;
            case U8X8_MSG_DELAY_MILLI:
                if (arg_int >= portTICK_PERIOD_MS) {
                    vTaskDelay(arg_int / portTICK_PERIOD_MS);
                } 
                else vTaskDelay(1); 
                break;
        }
        return 0;
    }
    
    protected:


    private:
    struct HAL_Config
    {
        Pin &cs;
        Pin &dc;
        Pin &rst;
    }config;

    spi_device_handle_t _handle;
};