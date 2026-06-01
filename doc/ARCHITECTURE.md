# Architecture

## Overview

Argos is an ESP32-C3 hardware monitoring dashboard. It displays real-time system information (CPU, memory, disk, temperature) from a remote target device on a 256x64 OLED display (SSD1322). Interaction is via a rotary encoder. WiFi connectivity, mDNS target discovery, HTTP-based telemetry polling, and a captive portal for profile provisioning are all handled on-device.


# Program Entrance

`/main` contains the application logic.

```
.
├── Argos
│   ├── Argos_framework.hpp
│   ├── Argos_global.hpp
│   ├── Argos_icons.hpp
│   └── Argos_Page.hpp
├── CMakeLists.txt
├── idf_component.yml
├── input.cpp
├── input.hpp
├── main.cpp
├── main.hpp
├── network.cpp
├── network.hpp
├── network_task.hpp
├── ui.cpp
├── ui.hpp
└── web
    ├── root.html
    └── root_html.hpp

3 directories, 17 files
```

`main.cpp` does nothing but create FreeRTOS tasks. `void app_main()` is wrapped by `extern "C"{}` because it is called directly from C (ESP-IDF startup).


# Task Architecture

Three FreeRTOS tasks are created in `main.cpp`, all started simultaneously under a scheduler lock (`vTaskSuspendAll` / `vTaskResumeAll`):

| Task | Priority | Stack | Period | File |
|---|---|---|---|---|
| Input | 4 (highest) | 2048 | 100 Hz | `input.cpp` |
| UI | 3 | 4096 | 50 Hz | `ui.cpp` |
| Network | 2 (lowest) | 4096 | 1 Hz | `network.cpp` |

Task handles are declared `extern` in `main.hpp` so any module can signal another via `xTaskNotifyGive` / `ulTaskNotifyTake`.

## Startup Synchronization

1. Network Task creates its queues, signals UI Task, then blocks on UI acknowledgment.
2. UI Task creates hardware (SPI, display), instantiates `ArgosFramework`, blocks waiting for Network notification.
3. UI Task receives notification, attaches encoder queue to framework, signals Network Task back.
4. Both enter their main loops. Input Task runs independently from the start.


# Input Task

`input.cpp` reads a rotary encoder (phase A, phase B, and push button). Rotation events are captured via GPIO interrupts (ISR-based quadrature decoding in the `Encoder` class). Button state is polled at 100 Hz with a debounce FSM:

```
Idle → SPressed (short press) → SHeld (hold ≥ 300ms)
```

Events are enqueued to `enc_task_q` (`QueueHandle_t`, extern in `input.hpp`) and consumed by the UI framework.

**Pins**: A=GPIO1, B=GPIO2, Button=GPIO10.


# UI Architecture

`ui.cpp` owns the display hardware and the `ArgosFramework`. The render loop runs at 50 Hz (`vTaskDelay(20ms)`).

## ArgosFramework (`Argos_framework.hpp`)

The central UI state machine. Each render cycle:

1. **`updateSystemState()`** — non-blocking reads from three queues:
   - `enc_task_q` — encoder events from Input Task
   - `network2ui_state_q` — WiFi state / chain stage / errors from Network Task
   - `network2ui_info_q` — parsed target system info from Network Task
   - Also reads RTC time and refreshes the profile list from LittleFS.

2. **`u8g2_ClearBuffer()`** → **`drawStatic()`** (frame & nav bar) → **`drawOverlay()`** (tabs & clock).

3. **Page rendering** — one of two modes:
   - **Preview mode** (not in stack): draws the root page for the currently focused tab. Encoder rotation switches tabs (INFO=0, NETWORK=1, ABOUT=2). Button press enters that tab's page stack.
   - **Stack mode** (in stack): delegates to the top page's `draw()`. Encoder events go to the page's `onEvent()`, which returns a `PageCommand`.

4. **`u8g2_SendBuffer()`** — flushes to display.

### Page Stack

Each root tab has its own `PageStack` (max depth 3). Pushing/popping is driven by `PageCommand`:

| Command | Effect |
|---|---|
| `PC_Enter` | Push next page in stack order |
| `PC_Exit` | Pop current page (or exit stack at root) |
| `PC_LoadProfile` | Forward to Network Task, pop page |
| `PC_AddProfile` | Forward to Network Task, pop page |
| `PC_DeleteProfile` | Delete profile from LittleFS directly |

`PC_LoadProfile`, `PC_AddProfile`, and `PC_DeleteProfile` carry a `Payload` (variant holding a `ProfilePayload` with the profile name).

### Pages (`Argos_Page.hpp`)

All pages inherit from `ArgosPage` and implement `draw()`, `onEvent()`, `onExit()`:

| Page | Tab | Description |
|---|---|---|
| **InfoPage** | 0 | Scrollable system info (hostname, CPU cores/threads/freq, usage%, temp, memory, disk) on the left; temperature sparkline on the right. |
| **NetworkPage** | 1 | WiFi status icon (animated scanning bars / checkmark / X), profile button, state text. |
| **AboutPage** | 2 | Static branding: GitHub icon, "Argos V1.0", project URL. |
| **ProfilePage** | subpage of Network | Two-column profile manager: 3 slots with bracket UI, LOAD/DEL for filled slots, ADD for empty slots. |

### Animation

`Animation<FRAME_COUNT>` template in `Argos_global.hpp` supports frame-based animation with per-frame durations. Used by `NetworkPage` for the WiFi scanning icon (3 frames, 300ms each).

### Shared Types (`Argos_global.hpp`)

- `SystemState` — aggregates all mutable UI state (system info, network state, encoder event, profile list, focus tab, clock string, stack flag).
- `UIMsg` — command struct: `PageCommand` + `Payload` variant.
- `HWINFO` / `STATIC` / `FONT` — display constants and base font (`u8g2_font_profont11_tr`).
- `PencilMode` — draw color wrapper (Hollow, Solid, Invert) for the monochrome display.


# Network Architecture

`network.cpp` wraps `NetworkTask` (defined in `network_task.hpp`) and runs its `tick()` at 1 Hz.

## Chain State Machine

`NetworkTask` uses a `std::variant`-based state machine. The active chain is stored as:

```cpp
std::variant<std::monostate, ChainIdle, ChainLoadProfile, ChainAddProfile,
             ChainTargetDiscovery, ChainReceiveInfo>
```

Each tick: `dispatcher()` processes incoming UI commands (which may set a pending chain), then `std::visit` with `execute()` runs the current chain. `applyChain()` defers state transitions to the end of the tick.

### Chains

```
                    ┌──────────────────────────────┐
                    │         ChainIdle             │
                    │  Sends idle state to UI once  │
                    └──────────┬───────────────────┘
                               │ UI sends PC_LoadProfile
                    ┌──────────▼───────────────────┐
                    │     ChainLoadProfile          │
                    │  Load profile from LittleFS   │
                    │  Connect WiFi STA             │
                    │  On success → TargetDiscovery │
                    │  On error   → Idle + error    │
                    └──────────┬───────────────────┘
                               │ WiFi connected
                    ┌──────────▼───────────────────┐
                    │   ChainTargetDiscovery        │
                    │  SNTP time sync               │
                    │  mDNS query: "argos-target"   │
                    │  Max 5 retries                │
                    │  On success → ReceiveInfo     │
                    │  On timeout → Idle + error    │
                    └──────────┬───────────────────┘
                               │ Target found
                    ┌──────────▼───────────────────┐
                    │    ChainReceiveInfo            │
                    │  HTTP GET /api/info            │
                    │  Parse JSON → SystemInfoMsg    │
                    │  Send to UI queue             │
                    │  Loops indefinitely           │
                    └──────────────────────────────┘

                    ┌──────────────────────────────┐
                    │     ChainAddProfile           │
                    │  WiFi SoftAP mode             │
                    │  DNS + HTTP captive portal    │
                    │  POST /save → write profile   │
                    │  On save → Idle               │
                    └──────────────────────────────┘
```

### Captive Portal (`ChainAddProfile`)

- Starts WiFi in SoftAP mode (SSID: `"Argos"`, password: `"clairvoyance"`).
- DNS server redirects all requests to the ESP32.
- HTTP server serves the HTML from `root_html.hpp` and handles `POST /save`.
- Max 3 profiles; writing a 4th returns `E_ProfileSlotFull`.

### Target Communication (`ChainReceiveInfo`)

Polls `http://<target-ip>:8080/api/info` and parses JSON fields:

host_name, os, os_version, cpu_percent, cpu_cores, cpu_threads, cpu_freq_mhz, cpu_temp, mem_total_mb, mem_used_mb, mem_percent, disk_total_mb, disk_used_mb, disk_percent.

### Profile Storage

Profiles are stored as text files in LittleFS under `/profile/<name>.txt`. Format is line-delimited key=value pairs (SSID, password, NTP server). The `LFS` class wraps the `esp_littlefs` component.

### Data Structures (`network.hpp`)

- `Profile` — fixed-size char arrays for FreeRTOS queue compatibility (SSID, password, NTP server, profile name).
- `SystemInfoMsg` — all parsed target system fields.
- `NetworkTaskStateMsg` — packs `ChainStage`, `ChainError`, `WiFiState`, and current SSID into one queue message.


# Message Passing

```
┌──────────────┐     enc_task_q       ┌──────────────┐
│  Input Task  │ ────────────────────→ │   UI Task    │
│  (Encoder)   │   EncoderMsg          │  (Framework) │
└──────────────┘                       └──────┬───────┘
                                              │
                              ui2network_command_q
                              (UIMsg: PageCommand + Payload)
                                              │
                                       ┌──────▼───────┐
                                       │ Network Task │
                                       │  (Chain SM)  │
                                       └──────┬───────┘
                                              │
                      network2ui_state_q       │
                      (NetworkTaskStateMsg)    │
                      network2ui_info_q        │
                      (SystemInfoMsg)          │
                                              ▼
                                       ┌──────────────┐
                                       │   UI Task    │
                                       │ updateSystem │
                                       │    State()   │
                                       └──────────────┘
```

All queue reads are non-blocking (0 timeout). The UI renders at 50 Hz regardless of whether new data arrived.


# ESP-DDC Component

**ESP-DDC** is a C++ OOP-style encapsulation of **ESP-IDF** and several third-party libraries, designed to be header-only and class-driven. It lives at `components/ESP-DDC/` and provides:

| Class | Header | Purpose |
|---|---|---|
| `SPI` | `general/ddc_spi.hpp` | RAII SPI bus wrapper (MOSI=6, MISO=8, SCLK=7) |
| `SSD1322` | `devices/display/ddc_ssd1322_u8g2.hpp` | Display driver using u8g2 (256x64, SPI, DC=5, RST=3, CS=4) |
| `Encoder` | `devices/ddc_encoder.hpp` | Quadrature encoder with ISR rotation + polled button |
| `WIFI` | `network/ddc_wifi.hpp` | WiFi management (STA/SoftAP, event-driven state) |
| `mDNS` | `network/ddc_mdns.hpp` | mDNS querier |
| `SNTP` | `network/ddc_sntp.hpp` | NTP time sync |
| `HttpClient` | `network/ddc_http_client.hpp` | HTTP client |
| `HttpServer` | `network/ddc_http_server.hpp` | HTTP server with captive portal mode |
| `DNServer` | `network/ddc_dns_server.hpp` | DNS server for captive portal redirection |
| `LFS` | `thirdparty/ddc_littlefs.hpp` | LittleFS wrapper |
| `Pin` | `general/ddc_io.hpp` | GPIO pin abstraction |

Component dependencies: `driver u8g2 esp_wifi nvs_flash esp_http_client esp_timer esp_netif esp_littlefs mdns`.


# Flash Layout

From `partitions.csv`:

| Name | Offset | Size |
|---|---|---|
| nvs | 0x9000 | 0x5000 |
| otadata | 0xE000 | 0x2000 |
| phy_init | 0x10000 | 0x1000 |
| app0 (OTA) | 0x20000 | 0x140000 (~1.25MB) |
| app1 (OTA) | 0x160000 | 0x140000 (~1.25MB) |
| spiffs (LittleFS) | 0x2A0000 | 0x160000 (~1.375MB) |


# Target Agent (`deploy/`)

The target agent is a companion Go program that runs on the monitored machine (Linux / Windows). It collects system telemetry and exposes it over HTTP, so the ESP32 device can poll it via `ChainReceiveInfo`.

```
deploy/
├── launch.go              # Entry point (calls cmd.Execute())
├── go.mod / go.sum        # Go module
├── cmd/
│   ├── root.go            # Root command (logo, version, help)
│   └── start.go           # start subcommand (HTTP/mDNS server)
├── linux/
│   ├── amd64/
│   │   └── argos-linux-amd64   # Pre-built binary (x86-64)
│   └── arm64/
│       └── argos-linux-arm64   # Pre-built binary (ARM64)
└── windows/
    └── argos-windows-amd64.exe # Pre-built binary (x86-64)
```

## How It Works

1. **mDNS registration** — uses `zeroconf` to broadcast as `argos-target._http._tcp.local` on port 8080, with a TXT record `path=/api/info`.
2. **HTTP endpoint** — `GET /api/info` returns a JSON payload with live system metrics.
3. The ESP32 `ChainTargetDiscovery` resolves the mDNS name, then `ChainReceiveInfo` polls `/api/info` at 1 Hz.

## System Info Collected

Uses `gopsutil/v4` to gather:

| Field | Source |
|---|---|
| `cpu_percent` | `cpu.Percent(500ms)` |
| `cpu_cores`, `cpu_threads` | `cpu.Counts(false/true)` |
| `cpu_freq_mhz` | `cpu.Info()[0].Mhz` |
| `cpu_temp` | `sensors.SensorsTemperatures()` — probes coretemp, k10temp, cpu, cpu_thermal, soc, package |
| `mem_total_mb`, `mem_used_mb`, `mem_percent` | `mem.VirtualMemory()` |
| `disk_total_gb`, `disk_used_gb`, `disk_percent` | `disk.Usage("/")` |
| `host_name`, `os`, `os_version`, `uptime_s` | `host.Info()` |

## Building

```bash
cd deploy
GOOS=linux GOARCH=amd64 go build -ldflags="-s -w" -o linux/amd64/argos-linux-amd64 .
GOOS=linux GOARCH=arm64 go build -ldflags="-s -w" -o linux/arm64/argos-linux-arm64 .
GOOS=windows GOARCH=amd64 go build -ldflags="-s -w" -o windows/argos-windows-amd64.exe .
```

## Deploy CLI

The `deploy/` agent is a command-line program built with `github.com/spf13/cobra`.

- **`argos`** (no args) — prints a centered, colorized ASCII logo/banner with version and attribution.
- **`argos start`** — starts the HTTP/mDNS server that the ESP32 polls.
- **`argos -v, --version`** — shows the agent version (via logo banner).
- **`argos -h, --help`** — shows usage and available commands.
- **`argos start -v, --verbose`** — starts the server with verbose logging (prints collected JSON on each request).



# Build System

- **Firmware framework**: ESP-IDF 5.5.4, CMake ≥ 3.16.
- **Target**: ESP32-C3.
- **Main component sources**: `ui.cpp`, `input.cpp`, `network.cpp`, `main.cpp` + globbed `*.cpp`.
- **Target agent**: Go 1.26.3, cross-compiled for Linux (amd64/arm64) and Windows (amd64).
