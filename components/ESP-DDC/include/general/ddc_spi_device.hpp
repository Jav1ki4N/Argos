
/* Includes --------------------------------------------------------------- */
#include "ddc_io.hpp"
#include "ddc_spi.hpp"
#include "driver/spi_master.h"
#include "soc/gpio_num.h"

/* Class ------------------------------------------------------------------ */
/* This class itself does not function as a standalone entity.              */
/* The resources should only be accessed through a derived device class.    */
class SPIDevice
{
    public:
    /* Constructor --------------------------------------------------------- */
    SPIDevice(SPI &spi,
              spi_device_interface_config_t *dev_config,
              gpio_num_t cs_pin_alt = GPIO_NUM_NC      )
    : _spi(spi),                            // SPI bus host
      _dev_config(*dev_config),             // Device-specific config
      _handle(nullptr),                     // Device handle, initialized below
                                            
      cs_pin((cs_pin_alt != GPIO_NUM_NC)?cs_pin_alt:            // can be taken over by 
                                                                // derived class if needed
             static_cast<gpio_num_t>(_dev_config.spics_io_num), // else default to config
             GPIO_MODE_OUTPUT)  
    {
        esp_err_t ret = _spi.add_device(&_dev_config, &_handle);
        assert(ret == ESP_OK);
    }

    /* Destructor ---------------------------------------------------------- */
    // The object releases the SPI device when it goes out of scope.
    ~SPIDevice()
    {
        _spi.remove_device(_handle);
    }
    
    esp_err_t transmit(spi_transaction_t *trans_desc)
    {
        return spi_device_transmit(_handle,trans_desc);
    }

    SPI& get_bus()
    {
        return _spi;
    }

    protected:
    SPI& _spi;
    spi_device_interface_config_t _dev_config;
    spi_device_handle_t _handle;

    /* Pins */
    // rst and dc pins are not concerned by the SPIDevice itself,
    // as they are display-specific 
    Pin cs_pin;
    
    private:
    SPIDevice(const SPIDevice&) = delete;
    SPIDevice& operator=(const SPIDevice&) = delete;
};