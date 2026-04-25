#include "ddc.hpp"

void framework_task(void *arg)
{
    SPI spi_bus(SPI2_HOST); // SPI bus 
                            // MOSI -> 11 (data)
                            // SCLK  -> 12
    SSD1322 Hermes_Framework(spi_bus,
                             GPIO_NUM_4, // DC pin
                             GPIO_NUM_2, // RST pin
                             GPIO_NUM_5  // CS pin
                            );
    for(;;){}
}
