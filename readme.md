# Argos


![Static Badge](https://img.shields.io/badge/ESP--IDF-5.5.4-none?logo=espressif&color=%23E7352C)

## What is it ?

**Argos** ( *Ἄργος* ) is a  system Info monitor that displays system infomation such as:

- Host name
- CPU Core & Thread numbers / Core temperature / Frequeny / Usage
- Memory Total size / Used size 
- Disk Total size / Used size
- Operating System type
- Local time

![](https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/Argos.jpg)

![](https://raw.githubusercontent.com/Jav1ki4N/Argos/refs/heads/master/assets/gallery/argos_example.gif)

----

## Deployment

### Linux

```bash
git clone https://github.com/Jav1ki4N/Argos.git
```

```bash
./server
```

If ESP-32 and the host machine are connected to the same Wi-Fi network, and target url's `x.x.x.x:8080/api/info` is set to address shown in

```bash
ip a
```
then in `INFO` page all information will be displayed.

If there's any probelm you can use the device monitor inside ESP-IDF to check logs reported by ESP32.

## Log

 - [x] Get it working
 - [x] Verified with ESP32C3
 - [x] Make a PCB 
 - [ ] Add a way that allows Wi-Fi SSID & Target URL to be modified while the device is running instead of hard-coded in code
    - web
    - app
 - [ ] Utlitize power consumption
 - [ ] Use MQTT instead of HTTP client
 - [ ] Support OTA via Wi-Fi
 - [ ] Use integrated ESP-32 instead of dev board


