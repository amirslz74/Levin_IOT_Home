# 🏠 Smart Home Automation Module (STM32, ESP8266, ESP32)

An easy and affordable smart relay system for DIY home automation, designed to be simple to build and use at home. The project uses ESP8266, ESP32, or STM32 microcontrollers to schedule and control devices like lights or appliances over WiFi.

## 🔧 Key Features

- 🖥️ **Web Interface**  
  Easily control your devices through a responsive web UI on your phone or PC.

- 🕒 **Time-Based Scheduling**  
  Automatically turn devices ON or OFF at specific times using a simple JSON schedule.

- 📂 **File Manager (SPIFFS)**  
  Upload, view, or delete schedule files directly through the browser.

- 🔌 **Relay Control**  
  Manual ON/OFF switching or scheduled control of relays (lights, fans, etc.).

- 🌐 **NTP Time Sync**  
  Automatically gets accurate time using NTP servers (with Iran timezone support).

- 🧩 **Modular Design**  
  Use with ESP8266, ESP32, or extend it to STM32 for advanced use cases.

## 🧱 Project Goals

- Easy to build at home
- Affordable and uses common components
- Simple code and clear structure
- Practical for real-world use (light automation, timers, etc.)

## ⚙️ Hardware Used

- **ESP8266** (NodeMCU, Wemos D1 mini) or **ESP32**
- Optional: **STM32** (for advanced control or multiple devices)
- **Relay module** (1 or more channels)
- 5V power supply (e.g., phone charger)
- Basic PCB or breadboard for testing

## 🖨️ PCB Design

- Designed with **home assembly in mind**
- Easy-to-solder components
- Compact footprint (can be mounted behind a wall switch or in a power box)
- Gerber files provided (coming soon)
