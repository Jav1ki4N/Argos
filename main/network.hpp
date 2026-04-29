#pragma once
#include "freertos/idf_additions.h"

extern QueueHandle_t client_q;

struct ClientMsg
{
    char host_name[32]  = {};
    char os[32]         = {};
    char os_distro[64]  = {};

    int   cpu_core_freq = 0;
    int   cpu_cores     = 0;
    int   cpu_threads   = 0;
    float cpu_usage     = 0.0f;
    float cpu_temp      = 0.0f;

    int   mem_total     = 0;
    int   mem_used      = 0;
    float mem_usage     = 0.0f;

    int   disk_total    = 0;
    int   disk_used     = 0;
    float disk_usage    = 0.0f;
};

void network_task(void *arg);