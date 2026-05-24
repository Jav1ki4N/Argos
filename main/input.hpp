#pragma once

#include "freertos/idf_additions.h"

extern QueueHandle_t enc_task_q;
void Input_Task(void *arg);