#include "soc/gpio_num.h"
#include <cstdio>

/* Components */
#include "ESP-DDC.hpp"

extern "C" void app_main(void)
{
    Pin led(GPIO_NUM_7, GPIO_MODE_OUTPUT);
    led.set2(Pin::state::high);
    for(;;){}
}
