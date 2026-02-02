# Home-control-panel
With the ESP32-C3 you can control your smart home and other things, all wirelessly!

![image](/user-attachments/blobs/proxy/eyJfcmFpbHMiOnsiZGF0YSI6OTcxODgsInB1ciI6ImJsb2JfaWQifX0=--bef1885b6b9a9bb103581271ee009071b32e2f8c/image.png)

## Why did I do this?

I was thinking about what project to do and I came up with this, I mainly wanted to create it because I needed some kind of device to control my smart home.

## Features

- 4 touch buttons
- OLED display as an indicator of e.g. time
- Rechargeable battery for carrying
- Wireless compatibility

## Wiring Diagram

![image](https://blueprint.hackclub.com/user-attachments/representations/redirect/eyJfcmFpbHMiOnsiZGF0YSI6OTcxMjMsInB1ciI6ImJsb2JfaWQifX0=--b2c4e6909dd5a191f0a0e8d11423718aeec311e2/eyJfcmFpbHMiOnsiZGF0YSI6eyJmb3JtYXQiOiJwbmciLCJyZXNpemVfdG9fbGltaXQiOlsyMDAwLDIwMDBdLCJjb252ZXJ0Ijoid2VicCIsInNhdmVyIjp7InF1YWxpdHkiOjgwLCJzdHJpcCI6dHJ1ZX19LCJwdXIiOiJ2YXJpYXRpb24ifX0=--0f85faa91c373105a0f317054e965c1f47e93a37/circuit_image%20(2).png)

## Scripts

The script is designed for ESP32-C3 and is just a simple script, so just upload it, adjust the names, API,... and you're done!

### How it works?

When you charge the battery, you turn on the system by holding down the 1st button (as far away from the OLED as possible) and the system will connect to the network and other peripherals. Then you just need to press the buttons and, for example, your light will turn on/off.

### Libraries:

```
WiFi.h
SinricPro.h
SinricProSwitch.h
Wire.h
Adafruit_GFX.h
Adafruit_SSD1306.h
WiFiUdp.h
NTPClient.h
```


## 3D models

Here is a view of the top and bottom of the case:

![image](https://github.com/mavory/Home-control-panel/blob/main/Photos/Sn%C3%ADmek%20obrazovky%202026-02-02%20204903.png?raw=true)

![image](https://github.com/mavory/Home-control-panel/blob/main/Photos/Sn%C3%ADmek%20obrazovky%202026-02-02%20205055.png?raw=true)

## Bill of Materials (BOM)

| Item | Quantity | Price | Link |
| :--- | :---: | :--- | :--- |
| **ESP32-C3 Super Mini** | 1 | $7.17 | [Laskakit](https://www.laskakit.cz/esp32-c3-super-mini-wifi-bluetooth-modul/) |
| **OLED display** | 1 | $3.05 | [AliExpress](https://www.aliexpress.com/item/1005006913366977.html) |
| **Touch buttons (TTP223)** | 4 | $1.21 | [Laskakit](https://www.laskakit.cz/arduino-kapacitni-dotykove-tlacitko-ttp223/) |
| **Wires (10cm, 20pcs)** | 1 | $2.71 | [Laskakit](https://www.laskakit.cz/propojovaci-vodice-10cm-20-kusu/) |
| **Battery Li-ion 18650** | 1 | $7.66 | [Laskakit](https://www.laskakit.cz/geb-li-ion-baterie-18650-3200mah-3-7v/) |
| **Charger module / Boost converter** | 1 | $1.36 | [Laskakit](https://www.laskakit.cz/mikro-nabijecka-boost-pro-usb-powerbank-5v--usb-c/) |
| *Shipping to CZ* | - | $3.49 | - |
| **Total** | | **$26.55** | |
