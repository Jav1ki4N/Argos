#pragma once

#include "freertos/idf_additions.h"

extern QueueHandle_t input_q;
void Input_Task(void *arg);