## Introduction
This is the Open Sourse WiFi DCC remote control for controlling turnouts and locomotives via command stations supporting Roco Z21 Wi-Fi protocol. 
The hardware is based on ESP8266 chip and ssd1106 based OLED.
The software is using Arduino IDE for output hex file building.
### Dependensies
The code dependant on ESP8266 library for Arduino IDE and Async UDP Library for ESP8266 Arduino.
### Helpers
There is a possibility to debug device menu using CodeBlocks IDE on the Windows. See Menu.cbp.

## Configuration
User should configure the default Wi-Fi net name and password in "src/config.h" file. See CL_SSID and CL_PASS defines.

## Hardware
The PCB project made in Kicad IDE could be fould under Mouse folder.
The PCB thickness is 1mm.
The case is borrowed from airconditioner remote control DG11J1-16 which could be found on aliexpress or ebay.  
![Top](/Mouse/top.PNG)
![Bottom](/Mouse/bottom.PNG)
