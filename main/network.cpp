
#include "freertos/idf_additions.h"
#include "ui.hpp"
#include "main.hpp"
#include "network.hpp"
#include "network_task.hpp"

QueueHandle_t network2ui_state_q = nullptr; // declared extern in network.hpp
QueueHandle_t network2ui_info_q  = nullptr;

void network_task (void* arg) {
    static NetworkTask task;
    task.init();

    network2ui_info_q = task.getInfoQueue();
    network2ui_state_q = task.getStateQueue();

    xTaskNotifyGive(ui_task_handle);
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    for(;;) {
        task.tick(ui2network_command_q); // declared extern in ui.hpp
                                         // set in ui.cpp
                                         // create and send by framework's commandDispatcher
                                         
        vTaskDelay(pdMS_TO_TICKS(1000)); // polling rate : per sec
    }
}