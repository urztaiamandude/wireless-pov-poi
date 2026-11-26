# Documentation map

The [repository README](../README.md) is the source of truth: hardware, power, UIs, build, and what is actually in the firmware. This page only says which other file to open.

If a document below conflicts with the README or the code, the README and the code win. Several older guides described a second Teensy tree, text serial commands, a sacrificial LED as the default, a Pololu part that does not exist, or files that were never added. Those claims are withdrawn.

## Use these

| Document | What it is for |
|---|---|
| [HARDWARE_BOM.md](HARDWARE_BOM.md) | Parts for one pair, including the level shifters and the LiPo plan. |
| [WIRING.md](WIRING.md) | Pinout and the SN74AHCT1G125 wiring. |
| [FLASHING.md](FLASHING.md) | PlatformIO, Arduino IDE, Teensy Loader, Raspberry Pi, ESP32 browser flash. |
| [PSRAM_INSTALLATION.md](PSRAM_INSTALLATION.md) | Soldering the two PSRAM chips. Required, not optional. |
| [TROUBLESHOOTING.md](TROUBLESHOOTING.md) | First checks when something is dark, wrong-colored, or offline. |
| [FIRST_PAIR_QUICKSTART.md](FIRST_PAIR_QUICKSTART.md) | Bring-up checklist for two poi. |
| [FIRST_PAIR_FIELD_CHECKLIST.md](FIRST_PAIR_FIELD_CHECKLIST.md) | Field checklist once a pair already runs. |
| [POV_DISPLAY_ORIENTATION_GUIDE.md](POV_DISPLAY_ORIENTATION_GUIDE.md) | Which LED is the bottom of the image. |
| [ORIENTATION_TROUBLESHOOTING.md](ORIENTATION_TROUBLESHOOTING.md) | Orientation symptoms. |
| [API.md](API.md) | HTTP API on the ESP32. Both UIs use it. |
| [PROTOCOLS.md](PROTOCOLS.md) | Binary Teensy↔ESP32 frames. |
| [BLE_PROTOCOL.md](BLE_PROTOCOL.md) | BLE bridge. It translates to the same binary frames. |
| [POI_PAIRING.md](POI_PAIRING.md) | ESP-NOW pairing (not persisted, mirror mode partial) and the legacy HTTP sync stub. Unproven on hardware. |
| [POWER_SUPPLY_DESIGN.md](POWER_SUPPLY_DESIGN.md) | Current power plan, then older explorations that are not the build. |
| [CIRCUIT_DIAGRAMS.md](CIRCUIT_DIAGRAMS.md) | Extra wiring drawings. Level-shift part is the one in the BOM. |
| [ESP32_S3_COMPATIBILITY.md](ESP32_S3_COMPATIBILITY.md) | Current ESP32-S3 DevKitC-1 N16R8 and the GPIO 16/17 Teensy UART. |
| [BUILDING_HEX.md](BUILDING_HEX.md) | Exporting a Teensy `.hex`. |
| [CLI_COMPILATION.md](CLI_COMPILATION.md) | Extra command-line build notes. |
| [IMAGE_CONVERSION.md](IMAGE_CONVERSION.md) | Turning pictures into POV frames. |
| [BMP_IMAGE_PROCESSING.md](BMP_IMAGE_PROCESSING.md) | BMP notes. The BMP helper headers are not linked into the sketch. |
| [PREFLIGHT_CHECKLIST.txt](PREFLIGHT_CHECKLIST.txt) | Plain-text preflight list. |
| [project_notes/key_facts.md](project_notes/key_facts.md) | Pin and path cheat sheet. |
| [project_notes/decisions.md](project_notes/decisions.md) | Architectural decisions. |
| [project_notes/bugs.md](project_notes/bugs.md) | Bug log. |
| [../TODO.md](../TODO.md) | Open work: bring-up, firmware follow-ups, and repo cleanup. |

## Power and battery

The portable plan is in the README and the BOM: 3S 1300 mAh, XT60, no BMS, Pololu D24V50F5, INA219 in the handle, 10 A switch. Bench power is 5 V 4 A.

| Document | Status |
|---|---|
| [BATTERY_POWER_INTEGRATION.md](BATTERY_POWER_INTEGRATION.md) | Current plan and the fact that the monitor code is not in the build. |
| [BATTERY_WIRING_QUICK_REFERENCE.md](BATTERY_WIRING_QUICK_REFERENCE.md) | Wiring for that plan. |
| [BATTERY_IMPLEMENTATION_ROADMAP.md](BATTERY_IMPLEMENTATION_ROADMAP.md) | What is and is not done. The old roadmap is withdrawn. |
| [BATTERY_WEB_INTERFACE.md](BATTERY_WEB_INTERFACE.md) | Not implemented. No dashboard file, no `/api/battery`. |
| [BATTERY_CODE_INTEGRATION_SNIPPETS.md](BATTERY_CODE_INTEGRATION_SNIPPETS.md) | Do not paste these into the sketch. |

## Background, not the running code

These were written against designs that are not in `firmware/teensy_firmware/teensy_firmware.ino`. Read them as notes, not as instructions.

| Document | Why it is background |
|---|---|
| [SD_CARD_STORAGE.md](SD_CARD_STORAGE.md) | Describes an `SDStorageManager` class and `/images/`. The sketch uses the SD library and `/poi_images`. |
| [SD_IMPLEMENTATION_SUMMARY.md](SD_IMPLEMENTATION_SUMMARY.md) | Same class, which is not in the tree. |
| [ARCHITECTURE_DESIGN.md](ARCHITECTURE_DESIGN.md) | Design discussion. Flow diagrams that still show `IMAGE:` / `LIVE:` text or 31 LEDs are old notes. The running link is the binary frame in [PROTOCOLS.md](PROTOCOLS.md). |
| [PERFORMANCE_OPTIMIZATIONS.md](PERFORMANCE_OPTIMIZATIONS.md) | Optimization notes, not a procedure. |
| [project_notes/issues.md](project_notes/issues.md) | Scratch issues. |

## Not in this repository

Do not look for these. They were linked from older docs and were never added:

- `QUICKSTART.md` (use [FIRST_PAIR_QUICKSTART.md](FIRST_PAIR_QUICKSTART.md))
- `TROUBLESHOOTING.md` at the repository root (use [TROUBLESHOOTING.md](TROUBLESHOOTING.md) in this folder)
- `FIRMWARE_ARCHITECTURE.md`, `ESP32_S3_PURCHASE_GUIDE.md`, `docs/DEVELOPMENT.md`, `ARCHITECTURE.md`, `TESTING.md`
- A `wiki/` directory
- `web/battery_dashboard.html`, `tools/poi_preflight_checklist.html`
- `firmware/teensy41/`
