# Battery power

Current plan for each poi. This replaces earlier notes that required a 3S BMS and a Pololu **D24V25F5S**. That regulator part number is not a Pololu product. Do not buy it. The buck is a Pololu **D24V50F5** (5 V, 5 A).

## Pack

- 3S 1300 mAh drone LiPo, XT60.
- Sits in a quick-swap pod on the spin axis.
- **No BMS** on the poi and no BMS in the pod.
- Charge only on a balance charger, unplugged from the poi. Do not charge through the buck.

A 3S pack is about 9.0 V empty to 12.6 V full. The D24V50F5 accepts that range. Without a BMS nothing will disconnect the pack for you. Stop using a pack that sags near 3.0 V per cell (9.0 V pack) under load. 3.5 V per cell is a kinder floor. Measure with a meter until the INA219 is actually read by firmware.

## Wiring

Handle side of the XT60, in order:

1. XT60 mate (the pack unplugs here).
2. 10 A switch in the battery positive.
3. INA219 high-side current shunt, still in the handle, so the sensor stays with the poi.
4. Pololu D24V50F5 input.
5. 5 V output to Teensy VIN, ESP32-S3 5 V, and the APA102 5 V, with 1000 µF at the regulator and at the strip.

Diagram and pin notes: [BATTERY_WIRING_QUICK_REFERENCE.md](BATTERY_WIRING_QUICK_REFERENCE.md). Parts count for a pair: [HARDWARE_BOM.md](HARDWARE_BOM.md).

Bench work does not use the pack. Use a 5 V 4 A supply on the same 5 V rail.

## Firmware status

`firmware/teensy_firmware/BatteryMonitor.h` is **not** `#include`d by `teensy_firmware.ino`. The ESP32 firmware has no `/api/battery` route. The web UIs do not show pack voltage.

Do not paste the old integration snippets into the sketch. They target line numbers and a text protocol the current firmware does not use. See [BATTERY_CODE_INTEGRATION_SNIPPETS.md](BATTERY_CODE_INTEGRATION_SNIPPETS.md).

When someone does integrate the monitor, it belongs on the Teensy (I2C to the INA219: SDA pin 18, SCL pin 19, as the header comments say) and the ESP32 should only forward a reading. That work is not done.
