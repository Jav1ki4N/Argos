#include "ddc.hpp"
#include "network.hpp"
#include "devices/display/UI/ddc_argos_u8g2.hpp"
#include "main.hpp"

void network_task(void *arg)
{
    QueueHandle_t q = xQueueCreate(3, sizeof(WifiMsg));
    WIFI::set_ui_queue(q);

    WIFI Argos_network(WIFI::Mode::Station, "N3V3RM1ND", "KurtCobain");

    for(;;)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
