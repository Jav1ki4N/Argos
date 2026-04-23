
/* Includes */
#include "hal/spi_types.h"
#include <driver/spi_master.h>
#include <esp_err.h>
/* If include files can't be found by clangd     */
/* make sure this file is included by a cpp file */
/* that is in SRC of main/CMakeLists.txt         */

/* Class */
class SPI
{
    public:
    /* Constructor */
    SPI(spi_host_device_t host, spi_bus_config_t *config = nullptr)
    : _host(host)
    {
        if(config == nullptr)
        {
            config = const_cast<spi_bus_config_t*>(&default_bus_config);
        }
        spi_bus_initialize(host, config, SPI_DMA_CH_AUTO);
    }

    esp_err_t add_device(spi_device_interface_config_t *dev_config, spi_device_handle_t *handle)
    {
        return spi_bus_add_device(_host, dev_config, handle);
    }

    esp_err_t remove_device(spi_device_handle_t handle)
    {
        return spi_bus_remove_device(handle);
    }

    /* Destructor  */
    // Notice: If a destructor function is provided
    // the object will be destroyed when it goes out of scope
    // unless an infinite loop is used to prevent it from going out of scope
    protected:
    private:
    [[maybe_unused]]spi_host_device_t _host;

    static inline const spi_bus_config_t default_bus_config = []
    {
        spi_bus_config_t config = {};
        config.mosi_io_num     = 11;
        config.miso_io_num     = 13;
        config.sclk_io_num     = 12;
        config.quadwp_io_num   = -1;
        config.quadhd_io_num   = -1;
        config.max_transfer_sz = 4096;
        return config;
    }();
};