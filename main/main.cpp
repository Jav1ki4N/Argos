/* Application headers */
#include "main.hpp"
#include "network.hpp"
#include "input.hpp"
#include "ui.hpp"

/* DDC headers */
#include "ddc.hpp"

/* ESP-IDF Components */
#include "freertos/idf_additions.h"
#include "hal/spi_types.h"
#include "soc/gpio_num.h"

SSD1322 *Argos_framework = nullptr;

TaskHandle_t network_task_handle;
TaskHandle_t input_task_handle;
TaskHandle_t ui_task_handle;

extern "C" void app_main(void)
{
    /* ---- UI Framework Init (must be in app_main) ---- */
    SPI     spi_bus(SPI2_HOST);
    SSD1322 framework(spi_bus, GPIO_NUM_5, GPIO_NUM_3, GPIO_NUM_4); // dc rst cs

    Argos_framework = &framework;

    /* Draw boot screen synchronously before any task runs */
    UI_DrawBootScreen(framework.get_U8g2());

    /* ---- Create Tasks ---- */
    xTaskCreate(network_task, "Network Task", 4096, nullptr,        2, &network_task_handle);
    xTaskCreate(Input_Task,   "Input Task",   2048, nullptr,        4, &input_task_handle);
    xTaskCreate(UI_Task,      "UI Task",      4096, Argos_framework, 3, &ui_task_handle);

    /* Signal network task that boot screen is done */
    xTaskNotifyGive(network_task_handle);

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}