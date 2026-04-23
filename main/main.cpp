#include "soc/gpio_num.h"
#include <cstdio>

/* My lib */
/* Better use absolute paths */
#include "../lib/i4N/DDC/ddc_io.hpp"

extern "C" void app_main(void)
{
    Pin led(GPIO_NUM_5, GPIO_MODE_OUTPUT);
    led.set2(Pin::state::high);
    for(;;){}

}
