#include "ddc.hpp"
#include "network.hpp"
#include "devices/display/UI/Argos_u8g2.hpp"
#include "main.hpp"


void network_task(void *arg)
{
    WIFI Argos_network;
    Argos_network.init(WIFI::Mode::station,"N3V3RM1ND","KurtCobain"); // might take a while
    for(;;)
    {
        Argos_PageTurn(Argos_framework->get_U8g2(), 
        Direction::left, 
        Argos_framework->get_UIAppState());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}