# Battery Monitoring Web Interface

Complete guide to adding battery status monitoring to your WiFi dashboard.

## Overview

The battery monitoring web interface displays:
- **Real-time voltage** (11.0V - 12.6V for 3S LiPo)
- **Battery percentage** (0-100% estimated)
- **Current draw** (in amps, updates every 5 seconds)
- **Estimated runtime** (minutes remaining at current draw)
- **Status alerts** (low battery, critical warnings)
- **Visual gauge** (shows percentage with color coding)

## Architecture

The system works like this:

```
Web Browser (on phone/laptop)
    ↓ HTTPS (wifi)
ESP32 Web Server (192.168.4.1)
    ↓ Serial @ 115200
Teensy 4.1
    ↓ I2C
INA219 Battery Monitor
    ↓
3S LiPo Battery
```

**Data flow for battery status:**
1. Web browser requests `/api/battery` endpoint
2. ESP32 receives request
3. ESP32 sends serial command to Teensy: `0xB0` (battery query)
4. Teensy reads INA219 and responds with: `BATTERY:voltage,percentage,runtime`
5. ESP32 parses and returns JSON: `{voltage, percentage, current, runtime, status}`
6. Web interface updates display every 5 seconds

## Part 1: Teensy Firmware Integration

### Add Battery Query Command Handler

Add this to `teensy_firmware.ino` in the `processSerialCommands()` function:

```cpp
// Add this in the command handling section (search for "else if (cmd[0]")
else if (cmd[0] == 0xB0) {  // Battery status query command
  // Send battery status in format: BATTERY:voltage,percentage,runtime
  float voltage = battery.getVoltage();
  float percentage = battery.getPercentage();
  int runtime = battery.getRuntimeMinutes();
  float current = battery.getCurrent();

  // Format: CMD_MARKER + "BATTERY:" + data + END_MARKER
  Serial1.write(0xFF);
  Serial1.print("BATTERY:");
  Serial1.print(voltage, 2);  // 2 decimal places
  Serial1.write(',');
  Serial1.print(percentage, 1);
  Serial1.write(',');
  Serial1.print(runtime);
  Serial1.write(',');
  Serial1.print(current, 2);
  Serial1.write(0xFE);

  if (DEBUG_ENABLED) {
    Serial.print("Battery query: ");
    Serial.print(voltage); Serial.print("V, ");
    Serial.print(percentage); Serial.print("%, ");
    Serial.print(current); Serial.print("A, ");
    Serial.print(runtime); Serial.println(" min");
  }
}
```

### Location in File

This should be added in the `processSerialCommands()` function, around line 900-1000. Find the section that starts with:

```cpp
void processSerialCommands() {
  // ... existing code ...

  while (ESP32_SERIAL.available()) {
    // ... existing command parsing ...

    if (cmd[0] == 0x???) {  // Existing commands
      // ...
    }
    // ← ADD BATTERY COMMAND HERE
  }
}
```

---

## Part 2: ESP32 Firmware Integration

### Add Battery API Endpoint

Add this to `esp32_firmware.ino` in the `setupWebServer()` function (around line 290):

```cpp
// Add this line with other server.on() routes:
server.on("/api/battery", HTTP_GET, handleBatteryStatus);
```

### Add Battery Status Handler

Add this function at the end of the file (before final closing brace):

```cpp
void handleBatteryStatus() {
  // Request battery status from Teensy
  sendTeensyCommand(0xB0, 0);  // Battery query command, no data bytes
  TEENSY_SERIAL.write(0xFE);  // End marker

  // Wait for response (max 500ms)
  String batteryData = "";
  unsigned long startTime = millis();
  bool responseReceived = false;

  while (millis() - startTime < 500) {
    if (TEENSY_SERIAL.available()) {
      char c = TEENSY_SERIAL.read();
      if (c == 0xFF) {
        // Start of response
        while (millis() - startTime < 500 && TEENSY_SERIAL.available()) {
          c = TEENSY_SERIAL.read();
          if (c == 0xFE) {
            // End of response
            responseReceived = true;
            break;
          }
          if (c >= 32 && c < 127) {  // Printable ASCII
            batteryData += c;
          }
        }
        break;
      }
    }
  }

  // Parse response: "BATTERY:voltage,percentage,runtime,current"
  JsonDocument doc;

  if (responseReceived && batteryData.startsWith("BATTERY:")) {
    // Extract data after "BATTERY:" prefix
    String data = batteryData.substring(8);

    // Parse CSV: voltage,percentage,runtime,current
    int commaPos1 = data.indexOf(',');
    int commaPos2 = data.indexOf(',', commaPos1 + 1);
    int commaPos3 = data.indexOf(',', commaPos2 + 1);

    if (commaPos1 > 0 && commaPos2 > 0 && commaPos3 > 0) {
      float voltage = data.substring(0, commaPos1).toFloat();
      float percentage = data.substring(commaPos1 + 1, commaPos2).toFloat();
      int runtime = data.substring(commaPos2 + 1, commaPos3).toInt();
      float current = data.substring(commaPos3 + 1).toFloat();

      doc["voltage"] = voltage;
      doc["percentage"] = percentage;
      doc["current"] = current;
      doc["runtime"] = runtime;

      // Determine status
      if (voltage < 9.6) {
        doc["status"] = "critical";  // Immediate shutdown required
      } else if (voltage < 10.5) {
        doc["status"] = "warning";   // Low battery warning
      } else if (voltage > 12.4) {
        doc["status"] = "full";      // Fully charged
      } else {
        doc["status"] = "normal";    // Operating normally
      }

      doc["connected"] = true;
    } else {
      // Parse error
      doc["error"] = "Parse failed";
      doc["connected"] = false;
    }
  } else {
    // No response or timeout
    doc["error"] = "No response from Teensy";
    doc["connected"] = false;
  }

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}
```

### Add sendTeensyCommand Declaration

Verify this function exists in your ESP32 firmware (it should, around line 56):

```cpp
void sendTeensyCommand(uint8_t cmd, uint8_t dataLen);
```

---

## Part 3: Web Interface HTML/CSS/JavaScript

Create a new file or integrate into your existing web interface:

```html
<!-- Add this to your web dashboard -->

<div id="battery-panel" class="panel">
  <h2>🔋 Battery Status</h2>

  <div class="battery-display">
    <div class="gauge">
      <svg viewBox="0 0 200 200" class="gauge-svg">
        <!-- Gauge background -->
        <circle cx="100" cy="100" r="90" fill="none" stroke="#e0e0e0" stroke-width="8"/>

        <!-- Percentage arc (animated) -->
        <circle id="gauge-fill" cx="100" cy="100" r="90" fill="none"
                stroke="#667eea" stroke-width="8" stroke-dasharray="565"
                stroke-dashoffset="565" stroke-linecap="round"/>

        <!-- Center circle with percentage -->
        <circle cx="100" cy="100" r="70" fill="white"/>
        <text id="percentage-text" x="100" y="105" text-anchor="middle"
              font-size="32" font-weight="bold" fill="#333">--%</text>
      </svg>
    </div>
  </div>

  <div class="battery-stats">
    <div class="stat">
      <label>Voltage</label>
      <div id="voltage-value" class="value">-- V</div>
    </div>

    <div class="stat">
      <label>Current Draw</label>
      <div id="current-value" class="value">-- A</div>
    </div>

    <div class="stat">
      <label>Est. Runtime</label>
      <div id="runtime-value" class="value">-- min</div>
    </div>
  </div>

  <div id="battery-alert" class="alert alert-hidden">
    <span id="alert-text"></span>
  </div>
</div>

<style>
#battery-panel {
  background: white;
  border-radius: 12px;
  padding: 20px;
  margin-bottom: 20px;
  box-shadow: 0 2px 8px rgba(0,0,0,0.1);
}

#battery-panel h2 {
  margin-top: 0;
  color: #333;
  font-size: 20px;
  margin-bottom: 20px;
}

.battery-display {
  display: flex;
  justify-content: center;
  margin-bottom: 20px;
}

.gauge {
  width: 180px;
  height: 180px;
  position: relative;
}

.gauge-svg {
  width: 100%;
  height: 100%;
  transform: rotate(-90deg);
}

#gauge-fill {
  transition: stroke-dashoffset 0.5s ease;
}

#percentage-text {
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto;
}

.battery-stats {
  display: grid;
  grid-template-columns: 1fr 1fr 1fr;
  gap: 15px;
  margin-bottom: 15px;
}

.stat {
  text-align: center;
  padding: 10px;
  background: #f8f9fa;
  border-radius: 6px;
}

.stat label {
  display: block;
  font-size: 12px;
  color: #999;
  margin-bottom: 5px;
  text-transform: uppercase;
  letter-spacing: 0.5px;
}

.stat .value {
  font-size: 18px;
  font-weight: 600;
  color: #333;
}

.alert {
  padding: 12px;
  border-radius: 6px;
  margin-top: 15px;
  font-weight: 500;
  transition: all 0.3s ease;
}

.alert-hidden {
  display: none;
}

.alert-normal {
  background: #d1fae5;
  color: #065f46;
  border-left: 4px solid #10b981;
}

.alert-warning {
  background: #fef3c7;
  color: #92400e;
  border-left: 4px solid #f59e0b;
  animation: pulse 1s infinite;
}

.alert-critical {
  background: #fee2e2;
  color: #7f1d1d;
  border-left: 4px solid #ef4444;
  animation: pulse 0.5s infinite;
}

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.7; }
}

@media (max-width: 600px) {
  .battery-stats {
    grid-template-columns: 1fr;
  }

  .gauge {
    width: 140px;
    height: 140px;
  }
}
</style>

<script>
// Battery monitoring
let batteryUpdateInterval;

async function updateBatteryStatus() {
  try {
    const response = await fetch('/api/battery');
    const data = await response.json();

    if (data.connected) {
      // Update gauge and percentage
      const circumference = 2 * Math.PI * 90;
      const offset = circumference * (1 - (data.percentage / 100));
      document.getElementById('gauge-fill').style.strokeDashoffset = offset;
      document.getElementById('percentage-text').textContent = Math.round(data.percentage) + '%';

      // Update stats
      document.getElementById('voltage-value').textContent = data.voltage.toFixed(2) + ' V';
      document.getElementById('current-value').textContent = data.current.toFixed(2) + ' A';
      document.getElementById('runtime-value').textContent = data.runtime + ' min';

      // Update alert based on status
      const alertDiv = document.getElementById('battery-alert');
      const alertText = document.getElementById('alert-text');

      alertDiv.classList.remove('alert-hidden', 'alert-normal', 'alert-warning', 'alert-critical');

      if (data.status === 'critical') {
        alertText.textContent = '🚨 CRITICAL - Battery below 9.6V. Stop immediately!';
        alertDiv.classList.add('alert-critical');
      } else if (data.status === 'warning') {
        alertText.textContent = '⚠️ LOW BATTERY - Please recharge soon (below 10.5V)';
        alertDiv.classList.add('alert-warning');
      } else if (data.status === 'full') {
        alertText.textContent = '✅ Fully charged and ready to use';
        alertDiv.classList.add('alert-normal');
      } else {
        alertText.textContent = '✅ Battery operating normally';
        alertDiv.classList.add('alert-normal');
      }
    } else {
      document.getElementById('battery-alert').classList.remove('alert-hidden');
      document.getElementById('alert-text').textContent = 'Battery monitor not detected';
    }
  } catch (error) {
    console.error('Battery fetch error:', error);
  }
}

// Update every 5 seconds
function startBatteryMonitoring() {
  updateBatteryStatus();  // Initial update
  batteryUpdateInterval = setInterval(updateBatteryStatus, 5000);
}

function stopBatteryMonitoring() {
  if (batteryUpdateInterval) {
    clearInterval(batteryUpdateInterval);
  }
}

// Start monitoring on page load
document.addEventListener('DOMContentLoaded', startBatteryMonitoring);
document.addEventListener('beforeunload', stopBatteryMonitoring);
</script>
```

---

## Part 4: Integration Steps

### Step 1: Modify Teensy Firmware

1. Open `teensy_firmware.ino`
2. Find the `processSerialCommands()` function (around line 900)
3. Add the battery query command handler (see code above)
4. Save and compile

### Step 2: Modify ESP32 Firmware

1. Open `esp32_firmware.ino`
2. In `setupWebServer()` function (around line 290), add: `server.on("/api/battery", HTTP_GET, handleBatteryStatus);`
3. At end of file, add the `handleBatteryStatus()` function (see code above)
4. Save and compile
5. Upload to ESP32

### Step 3: Update Web Interface

Option A: **Integrate into existing dashboard**
- If you have a main web page, add the HTML/CSS/JavaScript from Part 3

Option B: **Create standalone battery page**
- Create new file: `web/battery.html`
- Include the HTML/CSS/JavaScript from Part 3
- Link to it from main dashboard

Option C: **Use example web interface**
- See the complete example at `docs/BATTERY_DASHBOARD_EXAMPLE.html`

---

## Testing the Integration

### Test 1: Serial Communication

1. Upload Teensy firmware with battery command handler
2. Open Serial Monitor (115200 baud)
3. You should see: `Battery query: X.XXV, XX%, X.XXA, XXX min`

### Test 2: Web API

1. Upload ESP32 firmware with battery endpoint
2. Connect to WiFi: `POV-POI-WiFi` (password: `povpoi123`)
3. Go to: `http://192.168.4.1/api/battery`
4. Should see JSON response:
   ```json
   {
     "voltage": 11.52,
     "percentage": 85.5,
     "current": 0.45,
     "runtime": 180,
     "status": "normal",
     "connected": true
   }
   ```

### Test 3: Web Dashboard

1. If integrated into main dashboard
2. Go to: `http://192.168.4.1`
3. Battery panel should show:
   - Circular gauge with percentage
   - Voltage reading
   - Current draw
   - Estimated runtime
   - Status alert if needed

---

## API Reference

### Endpoint: `/api/battery`

**Method:** `GET`

**Response:**
```json
{
  "voltage": 11.52,           // 11-12.6V for 3S LiPo
  "percentage": 85.5,         // 0-100% estimated remaining
  "current": 0.45,            // Current draw in amps
  "runtime": 180,             // Minutes remaining at current draw
  "status": "normal",         // "normal", "warning", "critical", "full"
  "connected": true           // Is INA219 module responding
}
```

**Status values:**
- `"normal"`: 10.5V - 12.0V (operational)
- `"full"`: > 12.4V (fully charged)
- `"warning"`: 10.5V - 10.0V (low battery)
- `"critical"`: < 9.6V (stop immediately)

**Update frequency:** Every 5 seconds (adjustable in JavaScript)

---

## Troubleshooting

### Battery API returns "not connected"

**Problem:** INA219 module not responding

**Solutions:**
1. Check I2C wiring (pins 18/19 on Teensy)
2. Verify 5V power to INA219
3. Check for I2C address conflicts
4. INA219 module may not be installed (system still works without it)

### Web dashboard shows "--"

**Problem:** ESP32 not getting battery data from Teensy

**Solutions:**
1. Verify Serial1 connection (Teensy TX1 to ESP32 RX2)
2. Check baud rate: should be 115200
3. Verify Teensy firmware has battery command handler
4. Check serial monitor for any errors

### Voltage reading seems wrong

**Problem:** INA219 calibration issue

**Solution:**
- Manually measure voltage with multimeter
- If reading is off, adjust calibration in BatteryMonitor.h line 56
- See `BATTERY_POWER_INTEGRATION.md` section on calibration

---

## Advanced Configuration

### Change Update Frequency

In the JavaScript code, change:
```javascript
batteryUpdateInterval = setInterval(updateBatteryStatus, 5000);  // 5000ms = 5 seconds
```

To update every 2 seconds:
```javascript
batteryUpdateInterval = setInterval(updateBatteryStatus, 2000);
```

### Add Audio Alert

Add sound when battery goes critical:

```javascript
if (data.status === 'critical') {
  // Play warning sound (requires audio file)
  new Audio('/sounds/alert.mp3').play();
}
```

### Store Battery History

Track battery percentage over time:

```javascript
let batteryHistory = [];

async function updateBatteryStatus() {
  // ... existing code ...

  // Store history
  batteryHistory.push({
    timestamp: new Date(),
    voltage: data.voltage,
    percentage: data.percentage,
    runtime: data.runtime
  });

  // Keep only last 100 readings
  if (batteryHistory.length > 100) {
    batteryHistory.shift();
  }
}
```

---

**Last Updated**: 2026-02-19
**Status**: Ready for implementation
