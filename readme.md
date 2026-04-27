# ESP-DDC

![Static Badge](https://img.shields.io/badge/ESP--IDF-5.5.4-none?logo=espressif&color=%23E7352C) ![Static Badge](https://img.shields.io/badge/Clangd-20-none?logo=llvm&color=%23262D3A) ![Static Badge](https://img.shields.io/badge/C%2B%2B-none?logo=cplusplus&color=%2300599C)




## What is it

ESP-DDC is my C++ library for ESP32 development. It's currently work in progress.

## How it works

ESP-DDC is provided as ESP-IDF components and works exactly like many of them. It's currently designed to be header-only.

```bash
.
├── CMakeLists.txt
└── include
    ├── ddc.hpp
    ├── devices
    │   ├── display
    │   │   ├── ssd1322_u8g2.hpp
    │   │   └── UI
    │   │       ├── Argos_icon.hpp
    │   │       └── Argos_u8g2.hpp
    │   ├── gyro
    │   └── sensor
    ├── general
    │   ├── ddc_io.hpp
    │   ├── ddc_spi_device.hpp
    │   └── ddc_spi.hpp
    ├── network
    │   └── ddc_wifi.hpp
    └── thirdparty
        └── ddc_u8g2.hpp

10 directories, 10 files
```
To use, simply:
```cpp
#include "ddc.hpp"
```

## Application
Below is a example project structure using ESP-DDC.

```bash
.
├── CMakeLists.txt
├── main.cpp
├── main.hpp
├── network.cpp
└── network.hpp

1 directory, 5 files
```
`main.cpp` is where tasks and global pointers of objects are created. A header file `main.hpp` can be included by indivial task `.cpp` file to allow using object functions across tasks.

```cpp
/* Components */
#include "ddc.hpp"

SSD1322 *Argos_framework = nullptr;

TaskHandle_t ui_task_handle;
TaskHandle_t network_task_handle;


extern "C" void app_main(void)
{
    // Create Tasks
    xTaskCreate(network_task, "Network Task", 4096, nullptr, 5, &network_task_handle);
    
    // UI Framework Init
    // Init is done in at construction
    SPI spi_bus(SPI2_HOST);
    SSD1322 framework(spi_bus, GPIO_NUM_4, GPIO_NUM_2, GPIO_NUM_5);
    
    // Global pointer, declared in header
    Argos_framework = &framework;

    // Empty loop
    for(;;) vTaskDelay(portMAX_DELAY);
}
```

Objects are normally created separately in each's own task `.cpp` files, e.g. :

```cpp
void network_task(void *arg)
{
    WIFI Argos_network;
    for(;;){}
}
```

## Clangd

I have no idea why but clangd works poorly with ESP-IDF extension for VSCode.

A common issue is the `clang(pp_file_not_found)` error that occurs when you try to include some files of ESP-IDF in a newly created files, usually not in the same folder of `main.cpp`'s.

In my case the only way to supress this is to explicitly add `ADD：[]` in `.clangd` to include all possible ESP-IDF includes.

