#include "soc/gpio_num.h"
#include <cstdio>

/* Components */
#include "ddc.hpp"

using enum Pin::state;

extern "C" void app_main(void)
{
    Pin led(GPIO_NUM_7, GPIO_MODE_OUTPUT);
    led.set2(high);

    SPI spi_bus(SPI2_HOST); // SPI bus 
                            // MOSI -> 11 (data)
                            // SCLK  -> 12
    SSD1322 display(spi_bus,
                    GPIO_NUM_4, // DC pin
                    GPIO_NUM_2, // RST pin
                    GPIO_NUM_5  // CS pin
                    );

    for(;;){
        //vTaskDelay(pdTICKS_TO_MS(200));
        //led.toggle();
    }
}
