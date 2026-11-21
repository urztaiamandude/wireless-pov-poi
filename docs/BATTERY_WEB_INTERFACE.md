# Battery web interface

Not implemented.

There is no `web/battery_dashboard.html`, no SPIFFS battery page, and no `/api/battery` handler in `firmware/esp32_firmware/esp32_firmware.ino`. The embedded page and the React app do not show pack voltage.

An older version of this document described an ASCII exchange (`BATTERY:voltage,percentage,runtime` and a `0xB0` query). The Teensy parser does not implement that. Do not add it as text. If a battery reading is added later, use the binary frame in [PROTOCOLS.md](PROTOCOLS.md) and return JSON from the ESP32 the same way the other routes do.

Until then, measure the pack with a meter. There is no BMS.
