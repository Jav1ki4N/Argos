# ESP-DDC

![Static Badge](https://img.shields.io/badge/ESP--IDF-5.5.4-none?logo=espressif&color=%23E7352C) ![Static Badge](https://img.shields.io/badge/Clangd-20-none?logo=llvm&color=%23262D3A) ![Static Badge](https://img.shields.io/badge/C%2B%2B-none?logo=cplusplus&color=%2300599C)




## What is it

ESP-DDC is my C++ library for ESP32 development. It's currently work in progress.

## How it works

ESP-DDC is provided as ESP-IDF components and works exactly like many of them, is currently designed to be header-only.

```bash
.
├── CMakeLists.txt
└── include
    ├── ddc.hpp
    ├── devices
    │   ├── display
    │   │   └── ssd1322.hpp
    │   ├── gyro
    │   └── sensor
    ├── general
    │   ├── ddc_io.hpp
    │   ├── ddc_spi_device.hpp
    │   └── ddc_spi.hpp
    └── thirdparty
        └── ddc_u8g2.hpp

8 directories, 7 files
```
To use, simply:
```cpp
#include "ddc.hpp"
```

## Clangd

I have no idea why but clangd works poorly with ESP-IDF extension for VSCode.

A common issue is the `clang(pp_file_not_found)` error that occurs when you try to include some files of ESP-IDF in a newly created files, usually not in the same folder of `main.cpp`'s.

In my case the only way to supress this is to explicitly add `ADD：[]` in `.clangd` to include all possible ESP-IDF includes.

