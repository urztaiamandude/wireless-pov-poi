# Battery implementation status

The previous roadmap is withdrawn. It specified a 3S BMS, a Pololu D24V25F5S (not a real Pololu part), and these files, which are not in the repository:

- `web/battery_dashboard.html`
- `tools/poi_preflight_checklist.html`

Do not create those paths to satisfy an old checklist. Preflight text that does exist is [PREFLIGHT_CHECKLIST.txt](PREFLIGHT_CHECKLIST.txt) and [FIRST_PAIR_FIELD_CHECKLIST.md](FIRST_PAIR_FIELD_CHECKLIST.md).

## Now

| Piece | State |
|---|---|
| 3S 1300 mAh, XT60, no BMS, D24V50F5, INA219 in the handle, 10 A switch | The hardware plan. See [HARDWARE_BOM.md](HARDWARE_BOM.md). |
| `BatteryMonitor.h` | Present, not compiled in. |
| ESP32 `/api/battery` | Not implemented. |
| Web dashboard for the pack | Not implemented. |

## Later, if the monitor is integrated

1. Include `BatteryMonitor.h` from the Teensy sketch and read the INA219 on I2C.
2. Add a binary serial command, `0xFF … 0xFE`, not a text line. Document it in [PROTOCOLS.md](PROTOCOLS.md) in the same change.
3. Add an ESP32 route that forwards that reading.
4. Show it in both UIs, since both already share the REST API.

None of those steps are done. Do not treat this file as a procedure for the current firmware.
