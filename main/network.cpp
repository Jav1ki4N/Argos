#include "ddc.hpp"
#include "network.hpp"
#include "main.hpp"


void network_task(void *arg)
{
    WIFI Argos_network;
    Argos_network.init(WIFI::Mode::station,"N3V3RM1ND","KurtCobain");
    for(;;){}
}