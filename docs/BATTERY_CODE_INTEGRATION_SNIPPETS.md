# Battery code snippets

Do not paste these into `teensy_firmware.ino`.

`firmware/teensy_firmware/BatteryMonitor.h` is intentionally not included. The line numbers and the text-style hooks in the old snippet file do not match the current sketch, which speaks binary `0xFF … 0xFE` frames and already uses the I2C pins only as documented in that header's comment.

Status and the hardware plan: [BATTERY_IMPLEMENTATION_ROADMAP.md](BATTERY_IMPLEMENTATION_ROADMAP.md) and [BATTERY_POWER_INTEGRATION.md](BATTERY_POWER_INTEGRATION.md).

When the monitor is integrated, do it as a reviewed change to the sketch and the serial protocol together, not as a copy from this page.
