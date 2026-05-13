/* Application headers */
#include "main.hpp"
#include "network.hpp"
#include "input.hpp"

/* DDC headers */
#include "ddc.hpp"

/* ESP-IDF Components */
#include "freertos/idf_additions.h"
#include "hal/spi_types.h"
#include "soc/gpio_num.h"

/* C/C++ Libraries */
#include <cstdio>

SSD1322 *Argos_framework = nullptr;

TaskHandle_t network_task_handle;
TaskHandle_t input_task_handle;

extern "C" void app_main(void)
{
    // Create Tasks
    xTaskCreate(network_task, "Network Task", 4096, nullptr, 5, &network_task_handle);
    xTaskCreate(Input_Task, "Input Task", 2048, nullptr, 5, &input_task_handle);
    
    // UI Framework Init
    // Init is done in at construction
    SPI spi_bus(SPI2_HOST);
    SSD1322 framework(spi_bus, GPIO_NUM_5, GPIO_NUM_3, GPIO_NUM_4); // dc rst cs
    
    // Global pointer, declared in header
    Argos_framework = &framework;

    UI_DrawBootScreen(framework.get_U8g2());

    /* Signal network task that boot screen is done */
    xTaskNotifyGive(network_task_handle);

    // Empty loop
    for(;;)
    {
        /* Consume WIFI -> UI messages before rendering */
        UI_UpdateState(framework.get_UIAppState(),client_q,input_q);

        /* Launch UI Render Service */
        UI_Render(framework.get_U8g2(), framework.get_UIAppState());
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}
