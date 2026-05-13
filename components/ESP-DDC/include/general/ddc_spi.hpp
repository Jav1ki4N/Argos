/*===================================================*/
/*      ____  ____  ____      ____  ____   ___       */
/*     (  __)/ ___)(  _ \ ___(    \(    \ / __)      */
/*      ) _) \___ \ ) __/(___)) D ( ) D (( (__       */
/*     (____)(____/(__)      (____/(____/ \___)      */
/*===================================================*/
/*          i4n@2026 | ddc_spi | 2026-4-26           */
/*          ESP-IDF SPI bus wrapper class            */
/*===================================================*/
/* Purpose: Initialize and manage an SPI bus instance */
/*          for ESP-IDF, including default config    */
/*          and device attachment / detachment.      */
/*===================================================*/
#pragma once

/* ESP-IDF Components */
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "hal/spi_types.h"
#include "esp_err.h"

/* Class */
class SPI
{
    public:
    /* Constructor */
    SPI(spi_host_device_t host, spi_bus_config_t *config = nullptr)
    : _host(host), _config(config ? *config : DEFAULT_BUS_CONFIG)
    {
        esp_err_t ret = spi_bus_initialize(_host, &_config, SPI_DMA_CH_AUTO);
        assert(ret == ESP_OK);
    }
    ~SPI()
    {
        spi_bus_free(_host);
    }

    esp_err_t add_device(spi_device_interface_config_t *dev_config, spi_device_handle_t *handle)
    {
        return spi_bus_add_device(_host, dev_config, handle);
    }

    esp_err_t remove_device(spi_device_handle_t handle)
    {
        return spi_bus_remove_device(handle);
    }

    spi_bus_config_t get_config() const
    {
        return _config;
    }

    /* Destructor  */
    // Notice: If a destructor function is provided
    // the object will be destroyed when it goes out of scope
    // unless an infinite loop is used to prevent it from going out of scope
    protected:
    private:
    spi_host_device_t _host;

    spi_bus_config_t _config;
   
    static inline const spi_bus_config_t DEFAULT_BUS_CONFIG = []
    {
        spi_bus_config_t config = {};
        config.mosi_io_num     = 6;
        config.miso_io_num     = 8;
        config.sclk_io_num     = 7;
        config.quadwp_io_num   = -1;
        config.quadhd_io_num   = -1;
        config.max_transfer_sz = 4096;
        return config;
    }();
};