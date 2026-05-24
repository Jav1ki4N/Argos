
#pragma once

/* ESP-IDF Components */
#include "freertos/idf_additions.h"

void UI_Task(void *arg);
extern QueueHandle_t ui2network_command_q; // Queue for sending commands from UI task to network task