
#pragma once

#include "u8g2.h"
#include <cstdint>
#include "../../../../network/ddc_wifi.hpp"

struct ArgosAppContext
{
    /* Wi-Fi */
    WIFI::WifiMsg wifi_msg; // including connection state and ssid

    /* Http Server */
    
};