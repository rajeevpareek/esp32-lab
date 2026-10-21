# esp32-lab

Hands-on ESP32 learning experiments — Wi-Fi, BLE, and breadboard sensors.
A personal log of working my way from "board bring-up" to small sensor projects.

## Hardware

| Board | Chip | Radios | Flash / PSRAM | Used for |
|---|---|---|---|---|
| **ESP32 DevKit** | ESP32-D0WD-V3 (rev 3.1) | Wi-Fi + Classic BT + BLE | 4 MB / none | Wi-Fi, BLE, breadboard sensors |

## Experiments

| # | Experiment | Concept demonstrated | Board |
|---|---|---|---|
| 01 | Board report | Read chip ID, flash, heap, MACs at runtime | DevKit |
| 02 | Wi-Fi scan | Scanning 2.4 GHz networks, RSSI, channels | DevKit |
| 03 | Captive portal | SoftAP + DNS hijack + login page | DevKit |
| 04 | BLE scan | Passive BLE advertisement scanning | DevKit |
| 05 | BLE advertisement decode | Parsing manufacturer data, MAC rotation | DevKit |
| 06 | LED blink | GPIO output, current-limiting resistor | DevKit |
| 07 | Button → LED | Digital input, internal pull-up | DevKit |

## Board config (Arduino IDE)

**ESP32 DevKit (WROOM-32)**
| Setting | Value |
|---|---|
| Board | ESP32 Dev Module |
| Flash Size | 4MB (32Mb) |
| PSRAM | Disabled |
| Upload Speed | 115200 |
| Port | /dev/cu.usbserial-* |

## Notes

- The DevKit uses a USB-UART bridge (`usbserial`); 115200 is the reliable upload speed.
- Each experiment lives in its own folder so the `.ino` matches the folder name (Arduino requirement).
- Wi-Fi credentials in the sketches are placeholders — replace with your own before uploading.

---
*A learning project. Built incrementally, one working sketch at a time.*
