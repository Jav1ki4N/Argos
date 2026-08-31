# Components

`ESP-DDC/` is Argos' header-oriented C++ abstraction component for ESP-IDF. It wraps GPIO, SPI, the SSD1322/u8g2 display, the rotary encoder, Wi-Fi, SNTP, DNS, HTTP, MQTT, mDNS, and LittleFS.

## Dependencies

Declared by `ESP-DDC/CMakeLists.txt`:

- ESP-IDF components: `driver`, `esp_wifi`, `nvs_flash`, `esp_http_client`, `esp_timer`, `esp_netif`, and `mqtt`.
- External components: [u8g2](https://github.com/olikraus/u8g2), [esp_littlefs](https://github.com/joltwallet/esp_littlefs), and [espressif/mdns](https://components.espressif.com/components/espressif/mdns).

The external components are currently gitignored and are not fetched by `main/idf_component.yml`. A clean checkout is therefore not self-contained; prepare compatible component directories before running `idf.py build`. The project lock file targets ESP-IDF 5.5.4 and ESP32-C3.

See [`../doc/ARCHITECTURE.md`](../doc/ARCHITECTURE.md#esp-ddc-component) for the class inventory and active MQTT data path.
