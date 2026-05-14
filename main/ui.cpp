/* Application headers */
#include "main.hpp"
#include "network.hpp"
#include "input.hpp"
#include "ui.hpp"

/* DDC headers */
#include "ddc.hpp"

void UI_Task(void *arg)
{
    SSD1322 *framework = static_cast<SSD1322 *>(arg);

    for (;;)
    {
        /* Consume WIFI + Input messages before rendering */
        UI_UpdateState(framework->get_UIAppState(), client_q, input_q);

        /* Launch UI Render Service */
        UI_Render(framework->get_U8g2(), framework->get_UIAppState());
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}