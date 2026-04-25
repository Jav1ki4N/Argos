#include "ddc.hpp"
#include "network.hpp"


void network_task(void *arg)
{
    WIFI Hermers_network;
    Hermers_network.init(WIFI::Mode::station,"N3V3RM1ND","KurtCobain");
    for(;;){}
}