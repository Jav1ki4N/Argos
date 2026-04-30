# ESP-DDC

![Static Badge](https://img.shields.io/badge/ESP--IDF-5.5.4-none?logo=espressif&color=%23E7352C) ![Static Badge](https://img.shields.io/badge/Clangd-20-none?logo=llvm&color=%23262D3A) ![Static Badge](https://img.shields.io/badge/C%2B%2B-none?logo=cplusplus&color=%2300599C)




## What is it

ESP-DDC is my C++ library for ESP32 development. It's currently work in progress.

## Verification

- ESP32S3 - Verified
- ESP32C3
- ESP32P4

## How it works

ESP-DDC is provided as ESP-IDF components and works exactly like many of them. It's currently designed to be header-only.

```bash
.
├── assets
│   └── icon
├── CMakeLists.txt
└── include
    ├── ddc.hpp
    ├── devices
    │   ├── display
    │   │   ├── ddc_ssd1322_u8g2.hpp
    │   │   └── UI
    │   │       ├── ddc_animation.hpp
    │   │       ├── ddc_argos_icon.hpp
    │   │       ├── ddc_argos_u8g2.hpp
    │   │       └── readme.md
    │   ├── gyro
    │   └── sensor
    ├── general
    │   ├── ddc_io.hpp
    │   ├── ddc_spi_device.hpp
    │   └── ddc_spi.hpp
    ├── network
    │   ├── ddc_http_client.hpp
    │   ├── ddc_http.hpp
    │   ├── ddc_sntp.hpp
    │   └── ddc_wifi.hpp
    ├── thirdparty
    │   └── ddc_u8g2.hpp
    └── utility
        └── ddc_json.hpp

13 directories, 16 files
```
To use, simply:
```cpp
#include "ddc.hpp"
```

## Application
Below is an example project structure using ESP-DDC.

```bash
.
├── CMakeLists.txt
├── input.cpp
├── main.cpp
├── main.hpp
├── network.cpp
└── network.hpp

1 directory, 6 files
```
Object are created in tasks, with a pair of `.cpp/.hpp` created separately. Queue & Notify are used to achieve communications across tasks.

Global pointers will normally not be introduced unless for necessary needs, e.g:

```cpp
QueueHandle_t q = nullptr;
```



## Clangd

Clangd works poorly with ESP-IDF extension in VSCode.

A common issue is the `clang(pp_file_not_found)` error that occurs when you try to include some files of ESP-IDF in a newly created files, usually not under `/main`.

In my case the only way to supress this is to explicitly add `ADD：[]` in `.clangd` to include all possible ESP-IDF includes.

As for includes of custom files, directly include `xxx.hpp` if the file is placed under the same directory, unless use absolute paths.

