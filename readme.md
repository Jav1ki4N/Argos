# Argos

[中文](./readme_zh.md) | **English**

![Static Badge](https://img.shields.io/badge/ESP--IDF-5.5.4-none?logo=espressif&color=%23E7352C)

## About this project

**Argos** (*Ἄργος*) is a system monitor that displays host system information on an `256*64` OLED screen:

- **Hostname** 
- **CPU Info**: Core Frequency & Temperature / Threads & Cores
- **Memory Info**: Total / Used / Usage Percentage
- **Disk Info**: Total / Used / Usage Percentage
- **OS**: Type / Distro & Version
- **Time**: UTC / Local

<div align="center">
  <img src="https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/Argos.jpg" width="600">
  <img src="https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/argos_example.gif" width="600">
</div>



## How it works

Argos has two components:

1. **A PC-side agent** (`./server`) — a binary that collects system metrics (CPU, memory, disk, OS info) and exposes them as a `JSON` HTTP endpoint on port `8080`.

2. **An ESP32 device** — connects to the same Wi-Fi network, polls the agent's `/api/info` endpoint every second, parses the JSON response, and renders the data on the OLED display.



## Deployment

### 1. PC Agent (Server)

```bash
git clone https://github.com/Jav1ki4N/Argos.git
cd Argos
./server
```

Once running, verify the endpoint is reachable:

```bash
curl http://$(hostname -I | awk '{print $1}'):8080/api/info
```

### 2. Configuration

ESP32 will be intialized as a `softAP`(access point) that allows your phone or other devices to connected to.

Once connected to the AP, a captive portal service will be launched and the user can input a configuration profile via a web page that stroaged inside ESP32's flash: 

- **`SSID`** - ssid of the wifi target device connected to
- **`Password`** - password of the wifi target device connected to
- **`ProfileName`** - the name of this configuration profile, default using **`SSID`**

After that, ESP32 will switch to `STA` mode and locate the target device's ip via mDNS, and thus the target URL.

By sending `GET`, the system info will be as sent back as `.json` and ESP32 will start to parse it and fill a information structure.



## PCB

### Prototype

The prototype version of this project and only used for testment. Some issues are:

- Internal on board USB should not be exposed to user in case dual powering path damages the soc and other components.
- External USB should be connected to the on board USB's `D+` and `D-` so that programs can be flashed via external.
- Charing & Charge fininsh LED seems to not behave properly. Also, when powered only by the battery there's no LED to indicate any states.
- The 5V boost circuit of ETA9697 is not used and the `1.7`v voltage drop cause extra heat generation on ME6231.
- Consider replacing the display module with a custom PCB, as it's hard to be universally used and maintain.
- ADC is not precise for getting battery state
- Terrible silkscreen printing..

![](https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/argos_top.png)
<div align="center">
<p style="font-style: italic;">Top view</p>
</div>

![](https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/argos_bottom.png)
<div align="center">
<p style="font-style: italic;">Bottom view</p>
</div>

| Name             | Type               | Value            | Quantity |
| ---------------- | ------------------ | ---------------- | -------- |
| ESP32C3SuperMini | MCU/SOC            | ESP32C3          | 1        |
| Encoder          | Physical Input     | SIQ-02FVS3       | 1        |
| Pin Header       | Connector          | 2*8p             | 1        |
| Power Switch     | Switch             | MSKT-12D14       | 1        |
| USB              | USB                | USB Type-C 16p   | 1        |
| LED              | LED                | 0603             | 2        |
| R1,R2            | Resistor           | 0603 1kohm       | 2        |
| R3,R4            | Resistor           | 0603 100kohm     | 2        |
| R11,R10          | Resistor           | 0603 5.1kohm     | 2        |
| L1               | Inductor           | 2R2 2.2uh        | 1        |
| C1,C2,C3,C4,C5   | Capacitor          | 0603 10uf        | 5        |
| C6,C7            | Capacitor          | 0603 0.1uf       | 2        |
| C8               | Capacitor          | 0603 1uf         | 1        |
| ETA9697          | Battery Charing IC | ETA9697E8A       | 1        |
| ME6231           | LDO                | ME6231C33M5G     | 1        |
| Battery          | Battery            | Li-Po,60x30x48mm | 1        |
<div align="center">
<p style="font-style: italic;">BOM</p>
</div>


## TODO

- [x] Get system information correctly
- [x] Switch platform to ESP32C3
- [x] Design a PCB and verify it functional
- [x] Write a driver for encoder
- [x] Implant LittleFS as file system
- [x] Redirect to a captive portal on AP mode
- [x] Get configuration profile from captive portal & Connect to target Wi-Fi
- [ ] Support multiple profiles
- [x] Use mDNS to auto-get target device's ip
- [ ] Battey detection via ADC
- [ ] ...

