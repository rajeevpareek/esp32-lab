# ESP32 Memory Map & Boot Process

Reference notes for the ESP32 DevKit (WROOM-32, chip ESP32-D0WD-V3).
Captured while building the `01-board-report` experiment.

---

## 1. Memory Types

The ESP32 separates memory by purpose rather than using one flat space.

| Type | Size | Volatile | Holds |
|---|---|---|---|
| **SRAM** (internal) | 520 KB | Yes | Stack, heap, variables, cached code |
| **Flash** (external SPI) | 4 MB | No | Program, bootloader, filesystem, config |
| **ROM** (mask ROM) | — | No (fixed) | 1st-stage bootloader, low-level routines |

SRAM is split into three blocks (SRAM0 192 KB / SRAM1 128 KB / SRAM2 200 KB).
After the Wi-Fi/BT stack and cache claim their share, ~370 KB is usable as heap.

---

## 2. Flash Partition Map

Layout for the "Default 4MB with spiffs" scheme.

| Address | Partition | Size | Purpose |
|---|---|---|---|
| `0x000000` | Bootloader | ~28 KB | 2nd-stage bootloader, runs on power-up |
| `0x008000` | Partition table | 3 KB | The map itself |
| `0x009000` | NVS | 20 KB | Key-value store (Wi-Fi creds, calibration, settings) |
| `0x00E000` | OTA data | 8 KB | Tracks which app slot is active |
| `0x010000` | **APP** | 1.2 MB | **Your sketch + all linked libraries run from here** |
| `0x150000` | SPIFFS | 1.5 MB | On-board filesystem |

The uploaded binary is **statically linked**: your code, the Arduino core,
and the ESP-IDF libraries are bundled into one image that lives entirely in
the APP partition. Nothing is loaded separately at runtime.

---

## 3. CPU Address Space

The MMU maps flash and RAM into one address space the CPU sees.

| Region | Maps to |
|---|---|
| `0x3F40_0000+` | External flash, read-mapped (code & constants, via cache) |
| `0x3FC0_0000+` | Internal SRAM (data) |
| `0x4000_0000+` | Internal SRAM (instructions) + ROM |
| `0x5000_0000+` | RTC memory (survives deep sleep) |

Code executes from flash via **XIP** (execute-in-place) through an instruction
cache; it is not fully copied into RAM. Initialised variables are copied from
flash to SRAM at startup.

---

## 4. Boot Sequence

```
Power on
  │
  ▼
ROM bootloader (factory-fixed, in chip ROM)
  │   prints the "ESP-ROM:..." banner
  ▼
2nd-stage bootloader (from flash @ 0x0000)
  │   reads partition table, sets up flash mode/clock
  ▼
Application (from APP partition @ 0x010000)
  │   your sketch + linked libraries
  ▼
setup() → loop()
```

---

## 5. Annotated Boot Trace

Raw serial output at boot (115200 baud), with each line explained.

```
ESP-ROM:esp32s3-20210327      ← ROM bootloader banner + build date
Build:Mar 27 2021
rst:0x1 (POWERON)             ← reset reason: power-on
boot:0x2b (SPI_FAST_FLASH_BOOT)  ← booting from SPI flash, fast mode
SPIWP:0xee                    ← SPI write-protect config
mode:DIO, clock div:1         ← flash in Dual I/O mode, full clock
load:0x3fff0030,len:4876      ← loading a segment into SRAM (data @ 0x3fff...)
load:0x40078000,len:16532     ← loading a segment into IRAM (instructions @ 0x4007...)
load:0x40080400,len:3500      ← another instruction segment
entry 0x400805b4              ← jump to app entry point → setup() runs
```

Note the `load:` addresses: `0x3fff...` targets are **data in SRAM**,
`0x4007.../0x4008...` targets are **instructions in IRAM** — the
instruction/data split of the architecture, visible at boot.

---

## 6. Live Board Report

Output of the `01-board-report` sketch on this unit.

```
CHIP
  Model          : ESP32-D0WD-V3
  Revision       : 301          (= silicon rev 3.1)
  Cores          : 2
  CPU freq       : 240 MHz
  Features       : WiFi BT BLE   (Classic BT present — unlike the S3)

FLASH
  Flash size     : 4 MB
  Flash speed    : 80 MHz

MEMORY
  Total heap     : 370036 bytes
  Free heap      : 323396 bytes
  Min free heap  : 291384 bytes  (lowest point reached — leak indicator)
  PSRAM size     : 0 bytes       (WROOM-32 has none)

FIRMWARE
  ESP-IDF ver    : v5.5.5
  Sketch size    : 352608 bytes  (mostly the linked-in Wi-Fi/BT stack)
  Free sketch sp : 1310720 bytes (APP partition capacity)

IDENTITY
  WiFi STA MAC   : 20:50:0D:CF:65:AC
  Bluetooth MAC  : 20:50:0D:CF:65:AE   (= WiFi MAC + 2)
  eFuse MAC (ID) : AC65CF0D5020        (factory chip ID, byte-reversed)
```

The three addresses derive from **one** factory eFuse ID:
Wi-Fi = base, Bluetooth = base + 2, with other offsets for Ethernet/AP.

---

## 7. Key Takeaways

- **Flash holds the program; SRAM holds the running state.** Sketch size and
  free heap are two different memories doing two different jobs.
- **Libraries are statically linked into the app image** — not pre-loaded on
  the board and not loaded at runtime. The only factory-resident code is the ROM.
- **Code runs from flash via cache (XIP)**, not by copying the whole program
  into RAM, which is why a 4 MB flash board runs happily with 520 KB of SRAM.
- **One factory eFuse ID** is the root of every MAC address the chip uses.

---

*Captured on the ESP32 DevKit (WROOM-32) as part of the `esp32-lab` learning project.*
