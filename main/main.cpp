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

TaskHandle_t network_task_handle;
TaskHandle_t input_task_handle;
TaskHandle_t ui_task_handle;

extern "C" void app_main(void)
{

    xTaskCreate(network_task, "Network Task", 4096, nullptr,        2, &network_task_handle);
    xTaskCreate(Input_Task,   "Input Task",   2048, nullptr,        4, &input_task_handle);
    xTaskCreate(UI_Task,      "UI Task",      4096, nullptr,        3, &ui_task_handle);

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}