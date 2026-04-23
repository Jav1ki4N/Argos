
/* Includes */
#include "ddc_spi.hpp"
#include "driver/spi_master.h"
/* Class */
class SPIDevice
{
    public:
    /* Constructor */
    SPIDevice(SPI &spi, spi_device_interface_config_t *dev_config)
    : _spi(spi), _dev_config(*dev_config), _handle(nullptr)
    {
        _spi.add_device(&_dev_config, &_handle);
    }
    /* Destructor  */
    // Notice: If a destructor function is provided
    // the object will be destroyed when it goes out of scope
    // unless an infinite loop is used to prevent it from going out of scope
    ~SPIDevice()
    {
        _spi.remove_device(_handle);
    }
    
    esp_err_t transmit(spi_transaction_t *trans_desc)
    {
        return spi_device_transmit(_handle,trans_desc);
    }

    protected:
    SPI& _spi;
    spi_device_interface_config_t _dev_config;
    spi_device_handle_t _handle;

    
    private:
    SPIDevice(const SPIDevice&) = delete;
    SPIDevice& operator=(const SPIDevice&) = delete;
};