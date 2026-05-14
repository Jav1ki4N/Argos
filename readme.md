# Argos

[中文](./readme_zh.md) | **English**

![Static Badge](https://img.shields.io/badge/ESP--IDF-5.5.4-none?logo=espressif&color=%23E7352C)

## About this project

**Argos** (*Ἄργος*) is a system monitor that displays host system information on an ESP32-driven OLED screen:

- Host machine's name
- CPU Info: Core Freq / Temp / Threads
- Memory Info: Total / Used / Usage Percentage
- Disk Info: Total / Used / Usage Percentage
- OS: Type / Distro & Version
- Time: UTC / Local

<div align="center">
  <img src="https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/Argos.jpg" width="600">
  <img src="https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/argos_example.gif" width="600">
</div>

----

## How it works

Argos has two components:

1. **A PC-side agent** (`./server`) — a lightweight binary that collects system metrics (CPU, memory, disk, OS info) and exposes them as a JSON HTTP endpoint on port `8080`.

2. **An ESP32 device** — connects to the same Wi-Fi network, polls the agent's `/api/info` endpoint every second, parses the JSON response, and renders the data on an SSD1322 OLED display (256×64).

----

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

### 2. ESP32 Firmware

Before flashing, update the WiFi credentials and the server IP in [`main/network.cpp`](main/network.cpp):

- `TARGET_URL` — set to your PC's local IP (the one shown by the `curl` command above)
- SSID and password — set to your Wi-Fi credentials

Then build and flash with ESP-IDF:

```bash
idf.py build flash monitor
```

The ESP32 and the host machine must be on the same Wi-Fi network.

### Troubleshooting

Use `idf.py monitor` to view ESP32 logs if the display doesn't update.

----

## PCB

![]("https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/argos_top.png")

![]("https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/argos_bottom.png")

- **MCU**: ESP32-C3 Supermini
- **Display**: SSD1322 OLED, 256×64, SPI interface
- 
----

## TODO

- [x] Get it working
- [x] Verified with ESP32-C3
- [x] Make a PCB
- [ ] Cloud-configurable Wi-Fi SSID & target URL
- [ ] Lower power consumption
- [ ] MQTT transport instead of HTTP polling
- [ ] OTA firmware updates over Wi-Fi
- [ ] Use integrated ESP32-C3 module instead of dev board
