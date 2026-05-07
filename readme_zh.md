# Argos

[English](./readme.md) | **中文**

![Static Badge](https://img.shields.io/badge/ESP--IDF-5.5.4-none?logo=espressif&color=%23E7352C)

## 这是什么？

**Argos**（*Ἄργος*）是一台系统监控器，通过 ESP32 驱动的 OLED 屏幕显示主机系统信息：

- 主机名
- CPU 核心 / 线程数、频率、温度、使用率
- 内存总量、已用、使用率
- 磁盘总量、已用、使用率
- 操作系统类型
- 本地时间（通过 NTP 同步）

![](https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/Argos.jpg)

![](https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/argos_example.gif)

![](https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/Top.png)

----

## 工作原理

Argos 由两部分组成：

1. **PC 端采集程序**（`./server`）—— 轻量级可执行文件，采集系统指标（CPU、内存、磁盘、操作系统信息），以 JSON HTTP 接口形式暴露在 `8080` 端口。

2. **ESP32 设备** —— 连接到同一 Wi-Fi 网络，每秒轮询采集程序的 `/api/info` 接口，解析 JSON 响应，将数据显示在 SSD1322 OLED 屏幕（256×64）上。

----

## 部署

### 1. PC 端采集程序

```bash
git clone https://github.com/Jav1ki4N/Argos.git
cd Argos
./server
```

启动后验证接口是否正常：

```bash
curl http://$(hostname -I | awk '{print $1}'):8080/api/info
```

### 2. ESP32 固件

烧录前，在 [`main/network.cpp`](main/network.cpp) 中修改 WiFi 凭证和服务器 IP：

- `TARGET_URL` —— 填写 PC 的局域网 IP（即上方 `curl` 命令中显示的地址）
- SSID 和密码 —— 填写 Wi-Fi 网络凭证

使用 ESP-IDF 构建并烧录：

```bash
idf.py build flash monitor
```

ESP32 和主机必须在同一 Wi-Fi 网络下。

### 故障排查

如果屏幕没有更新，使用 `idf.py monitor` 查看 ESP32 日志输出。

----

## 硬件

- **主控**: ESP32-C3（开发板或集成模块）
- **显示屏**: SSD1322 OLED，256×64，SPI 接口
- **PCB**: 见 [assets/](assets/) 目录下的 KiCad 文件和渲染图

----

## 待办

- [x] 跑通原型
- [x] 验证 ESP32-C3 兼容性
- [x] 制作 PCB
- [ ] WiFi SSID 和 URL 云端可配（不再硬编码）
- [ ] 优化功耗
- [ ] 用 MQTT 替代 HTTP 轮询
- [ ] 支持 Wi-Fi OTA 固件更新
- [ ] 使用集成式 ESP32-C3 模组替代开发板
