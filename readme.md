# Argos

[中文](./readme_zh.md) | **English**

![ESP-IDF](https://img.shields.io/badge/ESP--IDF-5.5.4-none?logo=espressif&color=%23E7352C)

---

## About

**Argos** (*Ἄργος*) is an ESP32-C3-powered system monitor that displays real‑time host metrics on a `256×64` OLED screen:

| Category     | Metrics                                      |
| ------------ | -------------------------------------------- |
| **Hostname** | System hostname                              |
| **CPU**      | Frequency, temperature, threads, core count  |
| **Memory**   | Total, used, usage %                         |
| **Disk**     | Total, used, usage %                         |
| **OS**       | Type, distro, version                        |
| **Clock**    | UTC and local time                           |

### Profiles

Save named configuration profiles to Argos and load them on demand. Each profile stores Wi‑Fi credentials, NTP server settings, and more. Delete profiles you no longer need directly from the device.

---

<div align="center">
  <img src="./assets/gallery/argos_info.png" width="600" alt="Argos device">
  <img src="./assets/gallery/example.gif" width="600" alt="Argos demo">
  <p><em>Argos in action</em></p>
</div>

---

## How It Works

Argos has two components:

1. **PC-side agent** (`./run_server`) — a binary that collects system metrics (CPU, memory, disk, OS) and exposes them as a JSON HTTP endpoint on port `8080`.

2. **ESP32 device** — connects to the same Wi‑Fi network, polls `/api/info` every second, parses the JSON response, and renders the data on the OLED display.

```mermaid
graph TD
  A["Target Device<br/>Run Server"]
  B["Argos: SoftAP<br/>& HTTP Server"]
  C["Captive Portal"]
  D["Argos: STA<br/>& HTTP Client"]
  E["Argos: UI"]
  F["Argos: Encoder"]

  B -->|"launch"| C
  C -->|"POST config"| B
  B -->|"switch to STA"| D
  D -->|"GET /api/info"| A
  A -->|"JSON"| D
  D -->|"parsed data"| E
  F -->|"input events"| E
```

---

## Components

### Firmware (ESP32)

**[`ESP-DDC`](./components/ESP-DDC)** is the project's own custom component — it contains all of Argos' firmware logic and is the only one tracked in git. The remaining components are third‑party, **gitignored**, and must be obtained before building:

| Component                                                    | Source                                                                 | Purpose                         |
| ------------------------------------------------------------ | ---------------------------------------------------------------------- | ------------------------------- |
| [u8g2](https://github.com/olikraus/u8g2)                     | `git clone https://github.com/olikraus/u8g2.git components/u8g2`       | OLED graphics (SSD1322 via SPI) |
| [esp_littlefs](https://github.com/joltwallet/esp_littlefs)   | `git clone https://github.com/joltwallet/esp_littlefs.git components/esp_littlefs` | Fail‑safe flash filesystem      |
| [espressif__mdns](https://github.com/espressif/esp-protocols)| `git clone https://github.com/espressif/esp-protocols.git components/espressif__mdns` | mDNS service discovery          |

```bash
# One‑liner to clone all missing components:
git clone https://github.com/olikraus/u8g2.git components/u8g2
git clone https://github.com/joltwallet/esp_littlefs.git components/esp_littlefs
git clone https://github.com/espressif/esp-protocols.git components/espressif__mdns
```

**ESP‑IDF framework** — provided by the SDK:

`driver` · `esp_wifi` · `nvs_flash` · `esp_http_client` · `esp_http_server` · `esp_timer` · `esp_netif` · `mdns` · `cJSON` · `esp_event` · `freertos` · `lwip`

> Requires **ESP‑IDF ≥ 5.0**.

### PC Agent

Cross‑platform Python server; see [Deployment](#1-pc-agent-server) for setup.

| Package                                                           | Version |
| ----------------------------------------------------------------- | ------- |
| [Flask](https://github.com/pallets/flask)                         | 3.1.3   |
| [psutil](https://github.com/giampaolo/psutil)                     | 7.2.2   |
| [zeroconf](https://github.com/jstasiak/python-zeroconf)           | 0.149.7 |

---

## Deployment

### 1. PC Agent (Server)

A `run_server.py` script is provided under [`./linux`](./linux) to launch the server on the target machine.

```bash
git clone https://github.com/Jav1ki4N/Argos.git
cd linux
./run_server.py
```

A pre‑compiled `run_server` binary is also available for Linux:

```bash
./run_server
```

Windows users must compile from source for now — a pre‑built Windows binary is planned.

**Verify the endpoint:**

```bash
curl http://$(hostname -I | awk '{print $1}'):8080/api/info
```

See [Components](#pc-agent) for Python dependency details.

### 2. Captive Portal

The ESP32 starts as a SoftAP. Connect your device to it and a DNS server redirects all queries to a **captive portal** hosted on the ESP32's flash. From there you fill in a configuration profile:

| Field          | Description                              |
| -------------- | ---------------------------------------- |
| **SSID**       | Target Wi‑Fi network SSID                |
| **Password**   | Target Wi‑Fi network password            |
| **NTP Server** | Network Time Protocol server             |
| **ProfileName**| Profile name (defaults to SSID)          |

<div align="center">
  <img src="https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/captive_portal.png" alt="Captive Portal screenshot">
  <p><em>Captive Portal</em></p>
</div>

After saving, the ESP32 switches to STA mode and locates the target device via **mDNS**. A `GET` request fetches the system info JSON, which is parsed and rendered on the display.

---

## PCB

### Prototype

The prototype revision — built for testing — has several known issues:

- On‑board USB should not be exposed to the user; dual power paths risk damaging the SoC.
- External USB should be wired to the on‑board USB `D+`/`D-` lines so firmware can be flashed externally.
- Charge and charge‑complete LEDs behave incorrectly. No LED indicates state when running on battery alone.
- The ETA9697 5 V boost circuit is unused; the 1.7 V dropout causes excess heat on the ME6231 LDO.
- The display module should be replaced with a custom PCB for better maintainability and fit.
- ADC battery measurement is imprecise.
- Silkscreen quality is poor.

<div align="center">
  <img src="https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/argos_top.png" alt="PCB top view">
  <p><em>Top view</em></p>
  <br/>
  <img src="https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/argos_bottom.png" alt="PCB bottom view">
  <p><em>Bottom view</em></p>
</div>

### Bill of Materials

<div align="center">

| Name                    | Type                 | Value              | Qty |
| ----------------------- | -------------------- | ------------------ | --- |
| ESP32C3SuperMini        | MCU / SoC            | ESP32-C3           | 1   |
| Encoder                 | Input                | SIQ-02FVS3         | 1   |
| Pin Header              | Connector            | 2×8p               | 1   |
| Power Switch            | Switch               | MSKT-12D14         | 1   |
| USB                     | Connector            | USB Type‑C 16p     | 1   |
| LED                     | LED                  | 0603               | 2   |
| R1, R2                  | Resistor             | 0603 1 kΩ          | 2   |
| R3, R4                  | Resistor             | 0603 100 kΩ        | 2   |
| R10, R11                | Resistor             | 0603 5.1 kΩ        | 2   |
| L1                      | Inductor             | 2.2 µH             | 1   |
| C1–C5                   | Capacitor            | 0603 10 µF         | 5   |
| C6, C7                  | Capacitor            | 0603 0.1 µF        | 2   |
| C8                      | Capacitor            | 0603 1 µF          | 1   |
| ETA9697                 | Battery Charging IC  | ETA9697E8A         | 1   |
| ME6231                  | LDO                  | ME6231C33M5G       | 1   |
| Battery                 | Battery              | Li‑Po 60×30×48 mm | 1   |
| Display                 | Display Module       | SER3.12‑D, SSD1322 | 1   |

<p><em>Bill of Materials</em></p>

</div>

---

## Roadmap

- [x] System information collection
- [x] Platform migration to ESP32‑C3
- [x] PCB design and validation
- [x] Encoder driver
- [x] LittleFS integration
- [x] Captive portal on AP mode
- [x] Configuration profiles via captive portal
- [x] Multi‑profile support
- [x] mDNS auto‑discovery of target device
- [x] Stack‑based UI FSM
- [x] Network task FSM rewrite
- [ ] Battery monitoring via ADC
- [ ] UI rewrite
- [ ] Offline time acquisition
- [ ] …
