# ESP8266 Smart Relay with Time-Based Scheduling and SPIFFS File Manager

This project allows you to control a relay connected to an ESP8266 microcontroller with:

- Manual ON/OFF buttons via web UI
- Automatic ON/OFF based on time schedule (via NTP & `schedule.json`)
- File manager to upload/download/delete files stored in SPIFFS (useful for updating the schedule)

## Features

- 📅 **Time-based relay scheduling** using NTP + JSON config
- 🌐 **Web interface** for real-time clock display and relay control
- 📂 **SPIFFS file manager** to view/upload/delete/download files directly from browser
- 🕒 **Iran time zone (UTC+3:30)** support
- 🔌 **Relay control** (manual + scheduled)

## Schedule File Format (`schedule.json`)

```json
[
  {
    "day": "mo",
    "time": "08:00",
    "state": "on"
  },
  {
    "day": "mo",
    "time": "20:00",
    "state": "off"
  }
]
