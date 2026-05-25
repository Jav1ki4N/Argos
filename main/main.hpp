
#pragma once

/* DDC headers */
#include "ddc.hpp"

/* ESP-IDF Components */
#include "freertos/idf_additions.h"

/* Task Handles */
extern TaskHandle_t network_task_handle;
extern TaskHandle_t input_task_handle;
extern TaskHandle_t ui_task_handle;
