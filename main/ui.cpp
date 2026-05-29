/* Application headers */
#include "freertos/idf_additions.h"
#include "main.hpp"
#include "network.hpp"
#include "input.hpp"
#include "ui.hpp"
#include "Argos/Argos_framework.hpp"

/* DDC headers */
#include "ddc.hpp"

QueueHandle_t ui2network_command_q = nullptr;

void UI_Task(void *arg) {
    //SSD1322 *framework = static_cast<SSD1322 *>(arg);

    /** Hardware Initialization
     *  @param spi_bus SPI bus of the display 
     *  @param display Display instance, provides u8g2 handle to be used in framework
     *  @param framework UI framework instance, render & manage states of the UI
    */
    SPI            spi_bus(SPI2_HOST);
    SSD1322        display(spi_bus, GPIO_NUM_5, GPIO_NUM_3, GPIO_NUM_4); // dc rst cs
    ArgosFramework framework(display);
    
    /** Queue Initialization
     *  @param enc_task_q Queue for receiving encoder events from input task
     *  @param network_task_command_q Queue for sending commands to network task
     *  @note  wait network task's notification before assigning queues handles
     */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    framework.setEncoderQueueHandle(enc_task_q);
    ui2network_command_q = framework.getNetworkTaskCommandQueue();
    xTaskNotifyGive(network_task_handle);
    
    for (;;) {
        framework.render();
        // /* Consume WIFI + Input messages before rendering */
        // UI_UpdateState(framework->get_UIAppState(), client_q, input_q);

        // /* Launch UI Render Service */
        // UI_Render(framework->get_U8g2(), framework->get_UIAppState());
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}