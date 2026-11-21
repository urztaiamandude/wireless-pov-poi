# Hardware BOM — one pair

Bill of materials for **one pair** (two poi). Bring-up uses one shared bench supply. Everything else is per poi, so the pair quantity is two unless noted.

Prices are not listed. Buy current parts; do not hunt for old part numbers from earlier notes.

## Per poi (quantity 2 for the pair)

| Qty / poi | Part | Role |
|---|---|---|
| 1 | Teensy 4.1 | POV engine. USB for flashing. |
| 2 | 8 MB PSRAM, Teensy 4.1 footprint (IPS6404L-SQ-SPN or APS6404L-3SQR-SN) | **Required.** Firmware halts without both chips. See [PSRAM_INSTALLATION.md](PSRAM_INSTALLATION.md). |
| 1 | ESP32-S3-DevKitC-1, N16R8 (16 MB flash, 8 MB PSRAM) | WiFi, BLE, web UIs. |
| 32 LEDs | APA102 (DotStar) strip, **60 LEDs/m**, cut to 32 | ~16.7 mm pitch. About 534 mm of strip. |
| 2 | SN74AHCT1G125, SOT-23-5 | One gate for APA102 data, one for clock. |
| 1 | microSD card, FAT32 | Teensy 4.1 built-in slot. Images and pattern presets. |
| 2 | 1000 µF electrolytic, 10 V or higher, low ESR | One across 5 V at the buck output, one at the strip power inlet. |
| 1 | Pololu D24V50F5 | 5 V, 5 A buck. Input from the 3S pack (about 9–12.6 V). There is no Pololu D24V25F5S; do not search for that number. |
| 1 | INA219 breakout | Voltage and current. Mount on the **handle side** of the XT60 so it stays with the poi when the pack is swapped. Not read by firmware yet. |
| 1 | 10 A switch | Battery positive, in the handle, between the XT60 and the buck. |
| 1 | 3S 1300 mAh drone LiPo, XT60 | Quick-swap pod on the spin axis. **No BMS on the poi.** |
| 1 | XT60 pair | Pack side and handle side. |

Optional, per poi:

| Qty / poi | Part | Role |
|---|---|---|
| 1 | MAX9814 electret module | Music-reactive patterns. VCC from Teensy 3.3 V, OUT to Teensy A0, GND common. Gain pin: open = 60 dB, GND = 50 dB, VDD = 40 dB. |

## Shared (not doubled)

| Qty | Part | Role |
|---|---|---|
| 1 | 5 V, 4 A regulated bench supply | Bring-up and bench testing. Enough for one poi at full white (32 × 60 mA ≈ 1.9 A, plus the two boards). Use two supplies only if both poi are on the bench at full brightness together. |
| 1 | Balance charger for 3S LiPo | Charge packs **off** the poi. Do not charge through the buck or the XT60 pod. |

Wire, heat-shrink, and a second XT60 pigtail are assumed.

## Level shifter wiring (each poi)

Two SN74AHCT1G125 devices. TI SOT-23-5 (DBV) pinout, confirm pin 1 on the datasheet that ships with the parts you bought:

| Pin | Name | Connect |
|---|---|---|
| 1 | OE | GND (output enabled) |
| 2 | A | Teensy signal (pin 11 for data, pin 13 for clock) |
| 3 | GND | Common ground |
| 4 | Y | APA102 DI or CI |
| 5 | VCC | 5 V |

Do not power the LED strip from the Teensy 5 V pin. Take strip 5 V from the buck (or the bench supply) with its own wires.

## Portable power order

```
3S LiPo in the spin-axis pod
    XT60
    10 A switch (battery +)
    INA219 high-side shunt (handle)
    Pololu D24V50F5
    5 V rail → Teensy VIN, ESP32 5 V, APA102 5 V
    1000 µF at the buck output and at the strip
```

No 3S BMS in this build. The pack is balance-charged, then unplugged and flown. Undervoltage cutoff is the operator's job until battery monitoring is actually in the firmware.

## Fallback only

If a buffer is not available, LED 0 can be left dark and used as a sacrificial level-shift pixel. Set sacrificial LEDs to 1 in the web UI. That is not the default and not the plan for this pair.
