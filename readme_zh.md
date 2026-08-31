# Argos

[English](./readme.md) | **中文**

![ESP-IDF](https://img.shields.io/badge/ESP--IDF-5.5.4-none?logo=espressif&color=%23E7352C)

---

## 关于

**Argos**（*Ἄργος*）是一台基于 ESP32‑C3 的系统监视器，在 `256×64` OLED 屏幕上实时显示主机指标：

> **项目状态：** Argos 当前是可运行的功能原型，并非可用于生产环境的 V1.0。设备与主机代理之间的核心遥测链路已经实现，但可复现固件构建、自动化测试、配置安全加固和下一版 PCB 仍在进行中。

| 分类         | 指标                               |
| ------------ | ---------------------------------- |
| **主机名**   | 系统主机名                         |
| **CPU**      | 频率、温度、线程数、核心数         |
| **内存**     | 总量、已用、使用率                 |
| **磁盘**     | 总量、已用、使用率                 |
| **操作系统** | 类型、发行版、版本                 |
| **时钟**     | 经 SNTP 同步的本地时间              |

### 配置文件

可通过强制门户将命名配置保存到 Argos 并按需加载。每个配置包含 Wi‑Fi 凭据和 NTP 服务器设置，也可直接在设备上删除；当前原型最多支持三个配置槽位。

---

<div align="center">
  <img src="./assets/gallery/argos_info.png" width="600" alt="Argos 设备">
  <img src="./assets/gallery/example.gif" width="600" alt="Argos 演示">
  <p><em>从本地加载配置流程演示</em></p>
</div>

---

## 工作原理

Argos 由两部分组成：

1. **目标代理**（`deploy/`）—— 基于 Go 的系统指标采集程序（CPU、内存、磁盘、操作系统、温度），在 `1883` 端口运行 MQTT Broker，通过 mDNS 广播 `_mqtt._tcp` 服务，并每两秒向 `argos/info` 发布一次 JSON 遥测数据。

2. **ESP32 设备** —— 连接到同一 Wi‑Fi 网络，通过 mDNS 发现 `argos` MQTT 服务，订阅 `argos/info`，解析收到的 JSON 消息并渲染到 OLED 屏幕。

```mermaid
graph TD
  A["目标设备<br/>MQTT 代理"]
  B["Argos: SoftAP<br/>& 强制门户"]
  C["浏览器"]
  D["Argos: STA<br/>& MQTT 客户端"]
  E["Argos: UI"]
  F["Argos: 编码器"]

  F -->|"输入事件"| E
  E -->|"添加配置"| B
  B -->|"提供门户"| C
  C -->|"POST /save"| B
  B -->|"保存后回到空闲"| E
  E -->|"加载配置"| D
  D -->|"mDNS 发现并订阅"| A
  A -->|"argos/info JSON"| D
  D -->|"解析数据"| E
```

---

## 组件依赖

### 固件 (ESP32)

**[`ESP-DDC`](./components/ESP-DDC)** 是项目自有的 ESP-IDF 抽象组件并纳入 git 管理。下列第三方组件被刻意加入 gitignore，固件构建前必须确保它们存在。因此，当前仓库尚不能仅凭干净检出直接完成固件构建。

| 组件 | 预期目录 | 用途 |
|---|---|---|
| [u8g2](https://github.com/olikraus/u8g2) | `components/u8g2` | SSD1322 OLED 图形库 |
| [esp_littlefs](https://github.com/joltwallet/esp_littlefs) | `components/esp_littlefs` | LittleFS 集成 |
| [espressif/mdns](https://components.espressif.com/components/espressif/mdns) | `components/espressif__mdns` | mDNS 服务发现 |

目前没有受支持的一键依赖初始化命令：组件清单、本地目录布局和 ESP-DDC 头文件路径仍需统一。请选用兼容 ESP-IDF 5.5.4 的版本，并在构建前核对头文件布局。

**ESP‑IDF 框架** —— 由 SDK 提供：

`driver` · `esp_wifi` · `nvs_flash` · `esp_http_client` · `esp_http_server` · `esp_timer` · `esp_netif` · `mdns` · `mqtt` · `cJSON` · `esp_event` · `freertos` · `lwip`

> 锁文件当前固定为 **ESP-IDF 5.5.4**、目标为 **ESP32-C3**。组件清单仍保留了较宽泛的旧版本约束；当前原型建议使用 5.5.4。

### 目标代理

跨平台 Go 代理；详见[部署](#1-目标代理)。

| 包 | 版本 |
|---|---|
| [cobra](https://github.com/spf13/cobra) | v1.10.2 |
| [gopsutil](https://github.com/shirou/gopsutil) | v4.26.4 |
| [zeroconf](https://github.com/grandcat/zeroconf) | v1.0.0 |
| [mochi-mqtt](https://github.com/mochi-mqtt/server) | v2.7.9 |

[`deploy/`](deploy/) 目录下提供了 Linux (amd64/arm64) 和 Windows (amd64) 的预编译二进制文件。

---

## 部署

### 1. 目标代理

[`deploy/`](deploy/) 目录下提供了预编译二进制文件，选择对应平台：

| 二进制文件 | 平台 | 状态 |
|---|---|---|
| `deploy/linux/amd64/argos-linux-amd64` | Linux x86‑64 | 已验证（**Ubuntu** 24.04 / 22.04） |
| `deploy/linux/arm64/argos-linux-arm64` | Linux ARM64（如 **树莓派**） | 未验证 |
| `deploy/windows/argos-windows-amd64.exe` | Windows x86‑64 | 未验证 |

```bash
# Linux
chmod +x deploy/linux/amd64/argos-linux-amd64
./deploy/linux/amd64/argos-linux-amd64

# Windows
deploy\windows\argos-windows-amd64.exe
```

更好的做法是将二进制文件重命名为 `argos`，可以省去很多麻烦，然后：

```bash
sudo mv argos /usr/local/bin/
```

### 从源码构建：

进入 `go.mod` 所在根目录：

```bash
cd deploy
go mod download
```

```bash
# 示例平台：linux amd64
GOOS=linux GOARCH=amd64 go build -ldflags="-s -w" -o argos .
```

### 用法

二进制文件通过 CLI 在目标机器上启动 Argos 服务。

```bash
# 查看当前版本
cd linux/amd64
./argos-linux-amd64 --version
                       ___                         
                      /   |  _________ _____  _____
                     / /| | / ___/ __ `/ __ \/ ___/
                    / ___ |/ /  / /_/ / /_/ (__  ) 
                   /_/  |_/_/   \__, /\____/____/  
                               /____/              

===========================================================================
     2026 @ i4N  https://github.com/Jav1ki4N/Argos | Version: Prototype
```

```bash
# 启动 MQTT Broker 和遥测发布器；添加 -v/--verbose 可打印每次采集的 JSON
./argos-linux-amd64 start
2026/06/01 20:38:36 [Argos]: MQTT broker running on 192.168.1.10:1883
2026/06/01 20:38:36 [Argos]: mDNS broadcasting: workstation.local → 192.168.1.10:1883
2026/06/01 20:38:36 [Argos]: Press Ctrl+C to stop
2026/06/01 20:38:36 [Argos]: Publishing to 'argos/info' every 2s
```

```bash
# 使用 -h、--help 或 help 了解详细用法
./argos-linux-amd64 --help

argos [-v | --version] [-h | --help | help] <command>

===========================================================================

flags:
	-v, --version Show the current version of Argos
	-h, --help    Show this help message

commands:
	start[-v | --verbose] Start the Argos server to monitor and control your
	ESP32 devices. Use -v or --verbose for detailed logging.

===========================================================================
```

**验证遥测数据**（需要安装 Mosquitto 等 MQTT 客户端）：

```bash
mosquitto_sub -h 127.0.0.1 -p 1883 -t argos/info -v
```

### 2. 强制门户

在设备的配置菜单中选择 **ADD** 后，ESP32 才会启动 SoftAP。使用另一台设备连接后，DNS 服务器会将查询重定向到 ESP32 flash 中的**强制门户**，然后在页面中填写配置信息：

| 字段           | 描述                         |
| -------------- | ---------------------------- |
| **SSID**       | 目标 Wi‑Fi 网络的 SSID       |
| **Password**   | 目标 Wi‑Fi 网络的密码        |
| **NTP Server** | NTP 服务器地址               |
| **ProfileName** | 配置文件名称（默认使用 SSID）|

<div align="center">
  <img src="https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/captive_portal.png" alt="强制门户截图">
  <p><em>强制门户</em></p>
</div>

保存配置后，设备会回到网络空闲状态。请在 OLED 上进入 **NETWORK → PROFILES**，选择刚保存的配置并执行 **LOAD**。ESP32 随后加入目标 Wi‑Fi，通过 mDNS 发现 `argos._mqtt._tcp` 服务，并订阅 `argos/info`。

> 当前限制：NTP 地址会保存到配置文件，但固件目前仍固定使用 `pool.ntp.org`；保存后自动加载新配置的功能也尚未实现。

### 原型阶段限制

- 当前组件清单不会自动获取全部固件依赖，干净检出后仍需手动准备组件。
- 强制门户保存处理器仍需补充请求长度、字段、路径和写入结果校验。
- SoftAP 密码硬编码在固件中，MQTT Broker 允许局域网客户端匿名连接；请仅在可信开发网络中使用。
- `argosctl/` 仍是 TUI 原型，Start/Stop 目前只改变界面状态，不会管理真实的目标代理进程。
- 当前没有固件或 Go 自动化测试；预编译目标中仅 Linux amd64 被记录为经过硬件验证。

---

## PCB

### Prototype

Prototype 版本用于功能验证，存在以下已知问题：

- 板载 USB 不应直接暴露，双路供电可能损坏 SoC 及其他元件。
- 外部 USB 应连接至板载 USB 的 `D+`/`D-` 线路，以便通过外部接口烧录固件。
- 充电及充电完成 LED 表现异常。仅电池供电时无 LED 指示状态。
- ETA9697 的 5 V 升压电路未使用，1.7 V 压降导致 ME6231 LDO 产生额外热量。
- 显示模块应替换为定制 PCB，以提高通用性和可维护性。
- ADC 电池测量精度不足。
- 丝印质量较差。

<div align="center">
  <img src="https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/argos_top.png" alt="PCB 正面">
  <p><em>正面视图</em></p>
  <br/>
  <img src="https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/argos_bottom.png" alt="PCB 背面">
  <p><em>背面视图</em></p>
</div>

### BOM

<div align="center">

| 名称                    | 类型               | 规格               | 数量 |
| ----------------------- | ------------------ | ------------------ | ---- |
| ESP32C3SuperMini        | MCU / SoC          | ESP32‑C3           | 1    |
| Encoder                 | 输入设备           | SIQ-02FVS3         | 1    |
| Pin Header              | 连接器             | 2×8p               | 1    |
| Power Switch            | 开关               | MSKT-12D14         | 1    |
| USB                     | 连接器             | USB Type‑C 16p     | 1    |
| LED                     | LED                | 0603               | 2    |
| R1, R2                  | 电阻               | 0603 1 kΩ          | 2    |
| R3, R4                  | 电阻               | 0603 100 kΩ        | 2    |
| R10, R11                | 电阻               | 0603 5.1 kΩ        | 2    |
| L1                      | 电感               | 2.2 µH             | 1    |
| C1–C5                   | 电容               | 0603 10 µF         | 5    |
| C6, C7                  | 电容               | 0603 0.1 µF        | 2    |
| C8                      | 电容               | 0603 1 µF          | 1    |
| ETA9697                 | 电池充电 IC        | ETA9697E8A         | 1    |
| ME6231                  | LDO                | ME6231C33M5G       | 1    |
| Battery                 | 电池               | 锂聚合物 60×30×48 mm | 1 |
| 接线端子                 | 连接器             | KF128-2P           | 1    |
| Display                 | 显示模组           | SER3.12‑D, SSD1322 | 1    |

<p><em>BOM</em></p>

</div>

---

## Roadmap

- [x] 系统信息采集
- [x] 平台迁移至 ESP32‑C3
- [x] PCB 设计与验证
- [x] 编码器驱动
- [x] LittleFS 文件系统集成
- [x] AP 模式下强制门户
- [x] 通过强制门户配置 Wi‑Fi
- [x] 多配置文件支持
- [x] mDNS 自动发现目标设备
- [x] 基于堆栈的 UI 状态机
- [x] 网络任务状态机重写
- [x] 用 Go 重写 PC 端服务
- [x] MQTT 遥测传输
- [ ] ADC 电池监测
- [ ] UI 重写
- [ ] 离线时间获取
- [ ] 可复现的固件依赖与 CI
- [ ] 将 `argosctl` 接入真实代理生命周期
- [ ] 加固强制门户输入与配置存储
- [ ] …
