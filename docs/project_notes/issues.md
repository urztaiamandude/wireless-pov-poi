# POV Poi — Work Log & Issue Tracker

Current issue tracker aligned to the present repository structure.

---

## Status Legend

| Symbol | Meaning |
|--------|---------|
| ✅ | Complete |
| 🔄 | In Progress |
| ⏳ | Planned / Backlog |
| 🐛 | Known Issue |

---

## Current Repository Layout (Verified)

- `firmware/esp32_firmware/`
- `firmware/teensy_firmware/`
- `docs/`
- `examples/`

---

## Completed

- ✅ Teensy + ESP32 firmware communication over UART (115200)
- ✅ Web-based control flow from ESP32 firmware endpoints to Teensy commands
- ✅ BLE bridge implementation for ESP32 firmware (`src/ble_bridge.*`)
- ✅ Python image conversion and test utilities in `examples/`

---

## Active

- 🔄 Hardware validation of recent firmware and documentation changes
- 🔄 Continued stabilization of ESP32/Teensy integration on physical devices

---

## Backlog

- ⏳ OTA update workflow hardening
- ⏳ Additional runtime diagnostics for field troubleshooting
- ⏳ Expanded hardware-in-the-loop test coverage

---

## Known Issues

- 🐛 mDNS hostname reliability can vary by client OS/network setup (fallback: direct IP)
- 🐛 Serial link currently has no flow-control lines; packet framing remains required

---

## Notes

- This tracker intentionally excludes references to removed/nonexistent trees.
- BUG-007 was removed because the referenced PlatformIO Teensy firmware tree is not present on main.
