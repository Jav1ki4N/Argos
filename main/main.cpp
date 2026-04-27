#include "devices/display/UI/Argos_u8g2.hpp"
#include "freertos/idf_additions.h"
#include "hal/spi_types.h"
#include "soc/gpio_num.h"
#include <cstdio>
#include "network.hpp"
#include "main.hpp"

/* Components */
#include "ddc.hpp"

SSD1322 *Argos_framework = nullptr;

TaskHandle_t ui_task_handle;
TaskHandle_t network_task_handle;


extern "C" void app_main(void)
{
    // Create Tasks
    xTaskCreate(network_task, "Network Task", 4096, nullptr, 5, &network_task_handle);
    
    // UI Framework Init
    // Init is done in at construction
    SPI spi_bus(SPI2_HOST);
    SSD1322 framework(spi_bus, GPIO_NUM_4, GPIO_NUM_2, GPIO_NUM_5);
    
    // Global pointer, declared in header
    Argos_framework = &framework;

    // Empty loop
    for(;;)
    {
        Argos_UI_Render(framework.get_U8g2(),framework.get_UIAppState());
    }
}
