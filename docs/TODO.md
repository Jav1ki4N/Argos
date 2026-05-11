
## Easy-to-configurate Parameters

## Cloud

Crucial parameters must have way to configurate other than being hard-coded inside source code.
- wifi ssid & password
- http target url
  
Soloutions:
- web, must be accessed when wifi not connected
- app
- serial
## WiFi

### ARP Scan & WiFi Connection

Add a arp scan list as a sub page under the network page. 
- Shows all available WIFI's ssid
- If selected, trying searching corresponding password in **library**
- Groups of `ssid-password` must be stroaged in a library inside the device's memory, should be modified via a web / app / and so.
- Not password input method is provided becasue it can be complicated on device itself 
- On power up the device will try to connect to the first or default group of `ssid-password` 
- Always capable of connecting to another existed wifi group after power up.