# Battery Monitoring - Code Integration Snippets

Copy-paste these exact code sections into `teensy_firmware.ino` at the specified locations.

---

## 1. Add Include Statement

**Location**: At the top with other includes (around line 18, after `#include <FastLED.h>`)

```cpp
#include "BatteryMonitor.h"
```

**Complete context:**
```cpp
#include <FastLED.h>
#include "BatteryMonitor.h"     // ← ADD THIS LINE

// Teensy 4.1 PSRAM support
#ifdef ARDUINO_TEENSY41
```

---

## 2. Add Global Variables

**Location**: In the "Display state" section (around line 137, after display state variables)

```cpp
// Battery monitoring
BatteryMonitor battery;
bool batteryWarningShown = false;
uint32_t lastBatteryCheck = 0;
```

**Complete context:**
```cpp
// Display state
uint8_t currentMode = 0;
uint8_t currentIndex = 0;
uint32_t lastUpdate = 0;
uint32_t frameDelay = 20;
uint8_t currentColumn = 0;
bool displaying = false;

// Battery monitoring          ← ADD THIS SECTION
BatteryMonitor battery;        ← ADD THIS
bool batteryWarningShown = false;  ← ADD THIS
uint32_t lastBatteryCheck = 0; ← ADD THIS

// Multi-poi sync time offset
int32_t syncTimeOffset = 0;
```

---

## 3. Add Setup Code

**Location**: In `setup()` function, after FastLED.begin() (around line 210)

```cpp
  // Initialize battery monitor
  if (!battery.begin()) {
    Serial.println("WARNING: Battery monitor not detected. Continuing without monitoring.");
    Serial.println("Check INA219 connections on I2C (pins 18/19)");
  } else {
    Serial.println("Battery monitor initialized");
    Serial.print("Voltage: ");
    Serial.print(battery.getVoltage());
    Serial.println("V");
  }
```

**Complete context:**
```cpp
void setup() {
  // ... previous setup code ...

  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  FastLED.show();

  // Initialize battery monitor    ← ADD THIS SECTION START
  if (!battery.begin()) {         ← ADD THIS
    Serial.println("WARNING: Battery monitor not detected. Continuing without monitoring.");  ← ADD THIS
    Serial.println("Check INA219 connections on I2C (pins 18/19)");  ← ADD THIS
  } else {                        ← ADD THIS
    Serial.println("Battery monitor initialized");  ← ADD THIS
    Serial.print("Voltage: ");   ← ADD THIS
    Serial.print(battery.getVoltage());  ← ADD THIS
    Serial.println("V");         ← ADD THIS
  }                              ← ADD THIS SECTION END

  // ... rest of setup code ...
}
```

---

## 4. Add Loop Code

**Location**: In `loop()` function, after `processSerialCommands();` (around line 231)

```cpp
  // Check battery every 5 seconds
  if (millis() - lastBatteryCheck >= 5000) {
    lastBatteryCheck = millis();
    checkBattery();
  }
```

**Complete context:**
```cpp
void loop() {
  // Process serial commands from ESP32
  processSerialCommands();

  // Check battery every 5 seconds        ← ADD THIS SECTION START
  if (millis() - lastBatteryCheck >= 5000) {  ← ADD THIS
    lastBatteryCheck = millis();         ← ADD THIS
    checkBattery();                      ← ADD THIS
  }                                      ← ADD THIS SECTION END

  // Update display based on current mode
  if (millis() - lastUpdate >= frameDelay) {
    lastUpdate = millis();
    updateDisplay();
  }
}
```

---

## 5. Add Battery Check Function

**Location**: At the end of file, before final closing brace (around line 1750)

```cpp
void checkBattery() {
  float voltage = battery.getVoltage();
  float percentage = battery.getPercentage();
  float current = battery.getCurrent();

  // Serial monitor output (for debugging)
  Serial.print("Battery: ");
  Serial.print(voltage);
  Serial.print("V (");
  Serial.print(percentage);
  Serial.print("%) | Current: ");
  Serial.print(current);
  Serial.print("A | Runtime: ");
  Serial.print(battery.getRuntimeMinutes());
  Serial.println(" min");

  // Critical battery warning
  if (battery.isCritical()) {
    Serial.println("🚨 CRITICAL BATTERY - SHUTTING DOWN");
    // Flash red pattern as visual warning
    for (int i = DISPLAY_LED_START; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Red;
    }
    FastLED.show();
    delay(100);
    for (int i = DISPLAY_LED_START; i < NUM_LEDS; i++) {
      leds[i] = CRGB::Black;
    }
    FastLED.show();
    // Stop displaying
    currentMode = 0;
    displaying = false;
  }
  // Low battery warning
  else if (battery.isLowBattery() && !batteryWarningShown) {
    Serial.println("⚠️ LOW BATTERY - Please recharge soon");
    batteryWarningShown = true;

    // Flash yellow pattern as visual warning
    for (int x = 0; x < 3; x++) {
      for (int i = DISPLAY_LED_START; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Yellow;
      }
      FastLED.show();
      delay(200);
      for (int i = DISPLAY_LED_START; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Black;
      }
      FastLED.show();
      delay(200);
    }
  }
  // Reset warning flag when battery recovers
  else if (!battery.isLowBattery() && batteryWarningShown) {
    batteryWarningShown = false;
  }
}
```

---

## Integration Checklist

- [ ] `#include "BatteryMonitor.h"` added at top (with other includes)
- [ ] Three global variables added (battery, batteryWarningShown, lastBatteryCheck)
- [ ] Battery initialization added to `setup()`
- [ ] Battery check call added to `loop()`
- [ ] `checkBattery()` function added at end of file
- [ ] File compiles without errors (click Verify)
- [ ] BatteryMonitor.h file is in same folder as teensy_firmware.ino
- [ ] INA219 module connected to I2C pins (18/19) before uploading

---

## Minimal Integration (Without INA219)

If you don't have an INA219 module, you can:

1. **Skip including BatteryMonitor.h** (comment out the #include)
2. **Skip battery initialization in setup()**
3. **Skip battery check call in loop()**
4. **Skip the checkBattery() function**

Then the system will work normally but without battery monitoring. You can add it later by simply uncommenting the code and adding the INA219 module.

---

## Verification Steps

### After Adding Code

1. **Open teensy_firmware.ino**
2. **Select Board**: Teensy 4.1
3. **Select USB Type**: Serial
4. **Click Verify** (compile without uploading)
5. **Look for errors** in the console

### Expected Output (No Errors)

```
Compiling sketch...
Sketch uses 12345 bytes (7%) of program storage space.
✓ Compilation successful!
```

### If There Are Errors

**Common errors:**
- `fatal error: BatteryMonitor.h: No such file or directory`
  - Solution: Make sure BatteryMonitor.h is in the same folder as teensy_firmware.ino

- `'battery' does not name a type`
  - Solution: Check that global variables are added before they're used in setup()

- `'checkBattery' was not declared in this scope`
  - Solution: Make sure checkBattery() function is completely added at end of file

---

## Serial Monitor Output

After uploading, open Serial Monitor (115200 baud) to see:

```
Battery monitor initialized
Voltage: 11.52V

Battery: 11.52V (100%) | Current: 0.45A | Runtime: 210 min
Battery: 11.48V (99%) | Current: 1.23A | Runtime: 124 min
Battery: 10.80V (21%) | Current: 1.15A | Runtime: 18 min
⚠️ LOW BATTERY - Please recharge soon
```

---

## Optional: Customize Battery Capacity

If your batteries are different capacity than 1500mAh default, edit BatteryMonitor.h:

**Line 56** (in BatteryMonitor.h):
```cpp
  const float batteryCapacity = 1500.0;  // ← Change this number
```

**Examples:**
- For 1000mAh: `const float batteryCapacity = 1000.0;`
- For 2000mAh: `const float batteryCapacity = 2000.0;`
- For 3000mAh: `const float batteryCapacity = 3000.0;`

This adjusts the runtime calculation to be more accurate for your battery.

---

## Optional: Add Serial Commands

You can optionally add these commands to `processSerialCommands()` to query battery status:

```cpp
// Add this in the command processing section:
else if (cmd[0] == 0xB0) {  // Battery status command
  Serial.print("BATTERY:");
  Serial.print(battery.getVoltage());
  Serial.print(",");
  Serial.print(battery.getPercentage());
  Serial.print(",");
  Serial.println(battery.getRuntimeMinutes());
}
```

This allows the ESP32 to request battery status and display it on the web interface.

---

**Last Updated**: 2026-02-19
**Status**: Ready to integrate
