
#pragma once

/* DDC headers */
#include "ddc.hpp"

/* ESP-IDF Components */
#include "freertos/idf_additions.h"

/* Global pointers of objs */
extern SSD1322 *Argos_framework;

/* Task Handles */
extern TaskHandle_t network_task_handle;
extern TaskHandle_t input_task_handle;
extern TaskHandle_t ui_task_handle;
