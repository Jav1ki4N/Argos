#include "freertos/idf_additions.h"
#include "soc/gpio_num.h"
#include <cstdio>
#include "ui_framework.hpp"
#include "network.hpp"

/* Components */
#include "ddc.hpp"


TaskHandle_t ui_task_handle;
TaskHandle_t network_task_handle;

extern "C" void app_main(void)
{
    // Pin led(GPIO_NUM_7, GPIO_MODE_OUTPUT);
    // led.set2(high);
    xTaskCreate(framework_task, "UI Framework Task", 4096, nullptr, 5, &ui_task_handle);
    xTaskCreate(network_task, "Network Task", 4096, nullptr, 5, &network_task_handle);
    
    for(;;) vTaskDelay(portMAX_DELAY);
}
