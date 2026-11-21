# Battery wiring

One poi. The pair is two of these. No BMS.

```
Spin-axis pod                         Handle
┌──────────────────┐                  ┌──────────────────────────────────────┐
│ 3S 1300 mAh LiPo │                  │                                      │
│   no BMS         │                  │  XT60 ---- 10 A switch ---- INA219   │
│                  │     XT60         │                         |            │
│                  ├──────────────────┤                         v            │
│                  │                  │                  Pololu D24V50F5     │
└──────────────────┘                  │                    |     |           │
                                      │                   5 V   GND          │
                                      │                    |                 │
                                      │     Teensy VIN, ESP32 5 V, APA102 5 V│
                                      │     1000 µF at the buck and the strip│
                                      └──────────────────────────────────────┘
```

| Connection | Notes |
|---|---|
| XT60 | Quick-swap. Pack side lives in the pod. INA219 and the switch stay in the handle. |
| Switch | 10 A, battery positive. Opens the pack from the buck. |
| INA219 | High-side shunt between the switch and the buck input. VCC of the module from 5 V or 3.3 V per that module's silkscreen. SDA/SCL are unused until `BatteryMonitor.h` is integrated (Teensy pins 18 and 19). |
| D24V50F5 | VIN from the pack side, VOUT 5 V. Not a D24V25F5S. |
| Ground | Pack negative, buck GND, INA219 GND, Teensy GND, ESP32 GND, and LED GND are one net. |

Bring-up: disconnect the pack and feed the 5 V rail from a 5 V 4 A bench supply instead of the buck output.

Charge the LiPo on a balance charger, off the poi.
