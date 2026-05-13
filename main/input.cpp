/* Application headers */
#include "main.hpp"

/* DDC headers */
#include "ddc.hpp"
#include "soc/gpio_num.h"

QueueHandle_t input_q = nullptr;

void Input_Task(void *arg)
{
    // A, B, Button
    Encoder encoder(GPIO_NUM_1, GPIO_NUM_2, GPIO_NUM_10);

    input_q = xQueueCreate(5, sizeof(Encoder::EncoderMsg));

    for(;;)
    {
        encoder.Botton_Detection();
        encoder.Rotation_Detection();

        Encoder::EncoderMsg msg = encoder.GetMsg();
        xQueueSend(input_q, &msg, 0);

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}; 