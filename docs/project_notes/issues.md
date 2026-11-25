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
- 🐛 BLE `CC_SET_PATTERN` (`0x04`) is forwarded as Teensy `0x03` with the payload unchanged, so a type-first body is one field early (`slot` gets the type byte). Not fixed in this docs pass.
- 🐛 `CC_SET_PATTERN_ALL` sends pattern index `0xFF`. The Teensy clears the LEDs. Auto-cycle is not implemented.
- 🐛 `POST /api/sync/push` acknowledges JSON and does not apply it. `POST /api/sync/execute` still reports `settingsUpdated: true`.
- 🐛 ESP-NOW pairing is RAM-only. Mirror mode does not copy images or sequences. Incoming commands are applied even in independent mode.
- 🐛 Image slots 128–199 exist in the Teensy array and are not reachable from the ESP32 API or from sequence playback.

---

## Notes

- This tracker intentionally excludes references to removed/nonexistent trees.
- The stale PlatformIO Teensy-firmware issue entry was removed because that tree is not present on main.
