# PUTM EV LAPTIMER

## About
This is standalone laptimer for Formula Student made on ESP32-S3 with ESP-IDF and LVGL. It provides simple way to monitor your lap times during trainings and vehicle tests.

## Features
- Measure lap time using one or two measure gates
- Save lap times on SD card
- Watch lap times on LCD or webpage
- Download .csv files with sorted lap times
- Add penalties from FSG Rules during laptime

## How to use

## How to install
1. It's recommended to install ESP-IDF for Visual Studio Code with [this guide](https://github.com/espressif/vscode-esp-idf-extension/blob/master/README.md).
2. After cloning repository you should configure ESP-IDF project using
```
idf.py menuconfig
```
in CLI with activated ESP-IDF or 
```
>ESP-IDF: SDK Configuration Editor (Menuconfig)
```
in VSCode Command Palette, pin assignment and some other settings can be set in (Component config)/(PUTM EV Laptimer).
Default pin assignment is for ESP32-S3 and matches shield hardware used in this project.
<br/><br/>
3. Run
```
idf.py build
```
in CLI or
```
>ESP-IDF: Build Your Project
```
in Command Palette to build project
4. Easy way to quickly rebuild and flash project is to use command
```
>ESP-IDF: Build, Flash and Start a Monitor on Your Device
```
also available from VSCode status bar. You will also see program output in CLI. 
<br/>
