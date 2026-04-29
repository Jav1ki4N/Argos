#include "ddc.hpp"
#include "network.hpp"
#include "devices/display/UI/ddc_argos_u8g2.hpp"
#include "main.hpp"

void network_task(void *arg)
{
    /* Wait until main task signals boot screen is drawn */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    QueueHandle_t q = xQueueCreate(3, sizeof(WifiMsg));
    WIFI::set_ui_queue(q);

    WIFI Argos_network(WIFI::Mode::Station, "N3V3RM1ND", "KurtCobain");

    /* On booting & WiFi is up, sync time via SNTP */
    if (ddc_sntp_sync()) ESP_LOGI("SNTP", "Time sync successful");
    else                 ESP_LOGW("SNTP", "Time sync failed");


    for(;;)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
