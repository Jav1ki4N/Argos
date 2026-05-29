/* Application headers */
#include "main.hpp"

/* DDC headers */
#include "ddc.hpp"
#include "soc/gpio_num.h"

QueueHandle_t enc_task_q = nullptr;

void Input_Task(void *arg) {
    // A, B, Button
    Encoder encoder(GPIO_NUM_1, GPIO_NUM_2, GPIO_NUM_10);
    enc_task_q = encoder.GetQueue(); // queue send is done inside the class
                                  // but ui task still needs a exposed handler to receive messages
    for(;;) {
        encoder.Botton_Detection();
        // encoder.Rotation_Detection(); 
        vTaskDelay(pdMS_TO_TICKS(10)); 
    }
}; 