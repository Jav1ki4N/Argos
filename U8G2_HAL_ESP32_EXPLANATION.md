# `components/u8g2-hal-esp-idf/src/u8g2_esp32_hal.c` 详细说明

本文件是 `u8g2` 在 ESP32 平台上的硬件抽象层实现。它为 U8g2 库提供 SPI、I2C、GPIO 和延时回调，将 U8g2 的通用显示驱动请求映射到 ESP-IDF API。

---

## 文件顶层包含

```c
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "sdkconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "u8g2_esp32_hal.h"
```

- `<stdio.h>` 和 `<string.h>`：标准 C 库头文件，代码中本文件实际上未直接使用 `stdio.h` 的打印函数，而可能为未来扩展或日志占位。
- `esp_log.h`：ESP-IDF 日志模块，用于输出调试/错误信息。
- `sdkconfig.h`：用于读取 SDK 配置宏，常见于 ESP-IDF 项目。
- `freertos/FreeRTOS.h` 和 `freertos/task.h`：FreeRTOS API，用于 `vTaskDelay()` 延时。
- `u8g2_esp32_hal.h`：本 HAL 的头文件，声明了配置结构和回调函数接口。

---

## 全局静态变量

```c
static const char* TAG = "u8g2_hal";
static const unsigned int I2C_TIMEOUT_MS = 1000;

static spi_device_handle_t handle_spi;   // SPI handle.
static i2c_cmd_handle_t handle_i2c;      // I2C handle.
static u8g2_esp32_hal_t u8g2_esp32_hal;  // HAL state data.
```

### `TAG`
- 类型：`const char *`
- 作用：ESP-IDF 日志标签，用于 `ESP_LOGD`、`ESP_LOGI`、`ESP_LOGE` 等日志函数输出时标识模块。

### `I2C_TIMEOUT_MS`
- 类型：`const unsigned int`
- 作用：I2C 命令执行超时时间，单位毫秒。用于 `i2c_master_cmd_begin()` 的等待时间计算。

### `handle_spi`
- 类型：`spi_device_handle_t`
- 作用：保存 SPI 设备句柄。用于后续数据发送。
- 特性：静态全局变量，表示本 HAL 目前只维护一个全局 SPI 设备上下文。

### `handle_i2c`
- 类型：`i2c_cmd_handle_t`
- 作用：保存当前 I2C 命令链句柄。I2C 传输开始后创建，结束后提交并删除。

### `u8g2_esp32_hal`
- 类型：`u8g2_esp32_hal_t`
- 作用：保存由 `u8g2_esp32_hal_init()` 提供的硬件引脚配置和总线配置。
- 这个结构体包含 SPI 引脚、I2C 引脚、DC 引脚、RESET 引脚等。

---

## 自定义宏：`ESP_ERROR_CHECK`

```c
#undef ESP_ERROR_CHECK
#define ESP_ERROR_CHECK(x)                   \
  do {                                       \
    esp_err_t rc = (x);                      \
    if (rc != ESP_OK) {                      \
      ESP_LOGE("err", "esp_err_t = %d", rc); \
      assert(0 && #x);                       \
    }                                        \
  } while (0);
```

- 作用：封装 ESP-IDF API 返回值检查。
- 行为：如果函数返回不是 `ESP_OK`，则打印错误并触发 `assert(0)`。
- 注意：它使用日志标签 `"err"`，而不是顶部定义的 `TAG`。

---

## 函数说明

### `void u8g2_esp32_hal_init(u8g2_esp32_hal_t u8g2_esp32_hal_param)`

```c
void u8g2_esp32_hal_init(u8g2_esp32_hal_t u8g2_esp32_hal_param) {
  u8g2_esp32_hal = u8g2_esp32_hal_param;
}
```

- 用途：初始化 HAL 配置。
- 作用：将传入的配置结构复制到静态全局变量 `u8g2_esp32_hal`。
- 说明：这个函数不执行硬件初始化，只保存了引脚/总线参数。
- 桥接点：你的 C++ 代码应当在调用 U8g2 初始化前调用此函数，传入期望的 SPI/I2C/DC/RESET 引脚配置。

---

### `uint8_t u8g2_esp32_spi_byte_cb(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)`

这是 U8g2 SPI 总线回调函数。

#### 处理的消息类型

- `U8X8_MSG_BYTE_SET_DC`
  - 作用：设置数据/命令引脚电平。
  - 行为：如果 `dc` 引脚已定义，则调用 `gpio_set_level(u8g2_esp32_hal.dc, arg_int)`。

- `U8X8_MSG_BYTE_INIT`
  - 作用：初始化 SPI 总线和设备。
  - 行为：
    - 检查 SPI 引脚 `clk`、`mosi`、`cs` 是否已定义
    - 建立 `spi_bus_config_t bus_config`
    - 调用 `spi_bus_initialize(HOST, &bus_config, 1)`
    - 配置 `spi_device_interface_config_t dev_config`
    - 调用 `spi_bus_add_device(HOST, &dev_config, &handle_spi)`
  - 说明：这里使用固定主机 `HOST = SPI2_HOST`。

- `U8X8_MSG_BYTE_SEND`
  - 作用：通过 SPI 发送缓冲区数据。
  - 行为：
    - 构造 `spi_transaction_t trans_desc`
    - 设置 `trans_desc.length = 8 * arg_int`
    - 将 `arg_ptr` 作为发送缓冲区
    - 调用 `spi_device_transmit(handle_spi, &trans_desc)`
  - 说明：`arg_int` 表示字节数，ESP-IDF 接口要求传输位数。

#### 关键变量

- `spi_bus_config_t bus_config`
  - `sclk_io_num`: SPI 时钟引脚
  - `mosi_io_num`: MOSI 引脚
  - `miso_io_num`: 固定为 `GPIO_NUM_NC`
  - `quadwp_io_num`, `quadhd_io_num`: 固定为 `GPIO_NUM_NC`

- `spi_device_interface_config_t dev_config`
  - `address_bits`, `command_bits`, `dummy_bits`: 都设置为 0
  - `mode`: 0（SPI 模式 0）
  - `clock_speed_hz`: 10000
  - `spics_io_num`: 片选引脚
  - `queue_size`: 200
  - `pre_cb`, `post_cb`: NULL

#### 说明与限制

- 这段 SPI 代码不使用你的 C++ `SPI` 和 `SPIDevice` 类，而直接调用 ESP-IDF C API。
- `clock_speed_hz` 固定为 10000，可能偏低，适用于低速显示器。
- `handle_spi` 为静态全局变量，因此只支持一个 SPI 设备实例。

---

### `uint8_t u8g2_esp32_i2c_byte_cb(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)`

这是 U8g2 I2C 总线回调函数。

#### 处理的消息类型

- `U8X8_MSG_BYTE_SET_DC`
  - 与 SPI 回调相同：设置 DC 引脚。

- `U8X8_MSG_BYTE_INIT`
  - 初始化 I2C 总线。
  - 如果 `sda` 或 `scl` 引脚未定义，则直接跳过。
  - 调用 `i2c_param_config(I2C_MASTER_NUM, &conf)`
  - 调用 `i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0)`

- `U8X8_MSG_BYTE_SEND`
  - 逐字节写入 I2C 总线。
  - 对 `arg_int` 次字节调用 `i2c_master_write_byte(handle_i2c, *data_ptr, ACK_CHECK_EN)`。

- `U8X8_MSG_BYTE_START_TRANSFER`
  - 创建 I2C 命令链：`i2c_cmd_link_create()`
  - 发送起始条件：`i2c_master_start(handle_i2c)`
  - 写入设备地址：`i2c_master_write_byte(handle_i2c, i2c_address | I2C_MASTER_WRITE, ACK_CHECK_EN)`
  - `i2c_address` 由 `u8x8_GetI2CAddress(u8x8)` 返回。

- `U8X8_MSG_BYTE_END_TRANSFER`
  - 发送停止条件：`i2c_master_stop(handle_i2c)`
  - 执行命令链：`i2c_master_cmd_begin(I2C_MASTER_NUM, handle_i2c, pdMS_TO_TICKS(I2C_TIMEOUT_MS))`
  - 删除命令链：`i2c_cmd_link_delete(handle_i2c)`

#### 关键变量

- `i2c_config_t conf`
  - `mode`: `I2C_MODE_MASTER`
  - `sda_io_num`: SDA 引脚
  - `scl_io_num`: SCL 引脚
  - `sda_pullup_en`, `scl_pullup_en`: `GPIO_PULLUP_ENABLE`
  - `master.clk_speed`: `I2C_MASTER_FREQ_HZ`（50kHz）

- `uint8_t *data_ptr`
  - 指向待发送的数据缓冲区
- `i2c_cmd_handle_t handle_i2c`
  - 当前 I2C 命令链句柄

#### 说明与限制

- 该函数使用全局 `handle_i2c`，意味着单次传输结束前不能并行处理多个 I2C 命令链。
- 发送过程中采用逐字节写入，效率取决于 `arg_int` 大小。
- `I2C_MASTER_NUM` 是一个固定宏，不支持多 I2C 端口切换。

---

### `uint8_t u8g2_esp32_gpio_and_delay_cb(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr)`

这是 U8g2 的 GPIO 和延时回调函数。

#### 处理的消息类型

- `U8X8_MSG_GPIO_AND_DELAY_INIT`
  - 初始化 GPIO 输出引脚。
  - 构建 `bitmask`，包括：
    - `dc`
    - `reset`
    - `bus.spi.cs`
  - 仅当 bitmask 非零时，调用 `gpio_config(&gpioConfig)`。
  - `gpioConfig` 配置为输出、禁用上拉、使能下拉、禁用中断。

- `U8X8_MSG_GPIO_RESET`
  - 设置 RESET 引脚电平。

- `U8X8_MSG_GPIO_CS`
  - 设置片选引脚电平。

- `U8X8_MSG_GPIO_I2C_CLOCK`
  - 设置软件 I2C SCL 引脚电平。

- `U8X8_MSG_GPIO_I2C_DATA`
  - 设置软件 I2C SDA 引脚电平。

- `U8X8_MSG_DELAY_MILLI`
  - 调用 `vTaskDelay(arg_int / portTICK_PERIOD_MS)` 实现毫秒级延时。

#### 关键变量

- `uint64_t bitmask`
  - 用于在一次 GPIO 配置中包含多个引脚
- `gpio_config_t gpioConfig`
  - `pin_bit_mask`: 需要配置的引脚位掩码
  - `mode`: `GPIO_MODE_OUTPUT`
  - `pull_up_en`: `GPIO_PULLUP_DISABLE`
  - `pull_down_en`: `GPIO_PULLDOWN_ENABLE`
  - `intr_type`: `GPIO_INTR_DISABLE`

#### 说明

- 这个回调同时支持硬件 GPIO 和软件 I2C GPIO 控制。
- RESET、CS、DC 等控制引脚都通过同一个回调处理。
- 延时实现依赖 FreeRTOS 调度。

---

## 与你现有架构的桥接建议

### 1. 作为底层驱动层使用

这份文件本质上是“U8g2 平台适配器”。它应当留在 `components/u8g2-hal-esp-idf` 目录，作为第三方驱动组件。

### 2. C++ 入口桥接方式

在你的 `main/main.cpp` 中使用时，建议：

```cpp
extern "C" {
#include "u8g2_esp32_hal.h"
#include "u8g2.h"
}

extern "C" void app_main(void) {
  u8g2_esp32_hal_t hal = U8G2_ESP32_HAL_DEFAULT;
  hal.bus.spi.clk = GPIO_NUM_18;
  hal.bus.spi.mosi = GPIO_NUM_23;
  hal.bus.spi.cs = GPIO_NUM_5;
  hal.dc = GPIO_NUM_16;
  hal.reset = GPIO_NUM_17;

  u8g2_esp32_hal_init(hal);

  u8g2_t u8g2;
  u8g2_Setup_ssd1306_128x64_noname_f(
      &u8g2, U8G2_R0,
      u8g2_esp32_spi_byte_cb,
      u8g2_esp32_gpio_and_delay_cb);
  u8g2_InitDisplay(&u8g2);
  u8g2_ClearBuffer(&u8g2);
}
```

### 3. 你现有 `lib/i4N/DDC` 可以这样定位

- `lib/i4N/DDC`：业务层和显示逻辑
- `components/u8g2-hal-esp-idf`：底层硬件适配层
- `components/u8g2-master`：U8g2 绘图库
- `main`：对接 C++ 业务、构建显示驱动、执行应用生命周期

### 4. 如果想统一 C++ 风格

你可以在 `main` 中构建一个适配器类，把 `u8g2_esp32_hal_init()` 和回调函数包装起来：

- `init()`：保存配置
- `spi_cb()` / `i2c_cb()` / `gpio_and_delay_cb()`：静态转发函数

这样可以保持你现有业务层只依赖 C++ 接口。

---

## 结论

这份文件在你的项目中并不是业务层，而是“ESP32 平台专用 U8g2 HAL”。它的核心价值是：

- 实现 U8g2 SPI、I2C、GPIO、延时回调
- 提供 `u8g2_esp32_hal_init()` 来加载引脚配置
- 以全局静态状态保存 SPI/I2C 句柄和 HAL 参数

如果你想要更强的可扩展性，后续可以：

- 将 SPI/I2C 实现改为可复用的 C++ 类包装
- 重构全局句柄为实例成员
- 给 `clock_speed_hz` 和 I2C 端口增加配置项
