#!/usr/bin/env python3
"""
Capture serial output from Teensy (COM10) and ESP32 (COM15) to debug.log (NDJSON).
Run this while reproducing the Teensy-ESP32 communication issue.

Prerequisites: pip install pyserial
Usage: python scripts/capture_serial_debug.py
"""
import serial
import json
import time
import sys
from pathlib import Path

# User's ports
TEENSY_PORT = "COM10"
ESP32_PORT = "COM15"
BAUD = 115200
LOG_PATH = Path(__file__).resolve().parent.parent / ".cursor" / "debug.log"

def ensure_log_dir():
    LOG_PATH.parent.mkdir(parents=True, exist_ok=True)

def append_ndjson(record):
    ensure_log_dir()
    with open(LOG_PATH, "a", encoding="utf-8") as f:
        f.write(json.dumps(record, default=str) + "\n")

def main():
    print(f"Capturing Teensy ({TEENSY_PORT}) and ESP32 ({ESP32_PORT}) to {LOG_PATH}")
    print("Press Ctrl+C to stop.")
    print("TIP: Close Arduino IDE, PlatformIO monitor, and any Serial Monitor before running.\n")
    teensy = None
    esp32 = None
    try:
        teensy = serial.Serial(TEENSY_PORT, BAUD, timeout=0.1)
        print(f"  OK: {TEENSY_PORT} (Teensy)")
    except serial.SerialException as e:
        print(f"  SKIP {TEENSY_PORT}: {e}")
    try:
        esp32 = serial.Serial(ESP32_PORT, BAUD, timeout=0.1)
        print(f"  OK: {ESP32_PORT} (ESP32)")
    except serial.SerialException as e:
        print(f"  SKIP {ESP32_PORT}: {e}")
    if teensy is None and esp32 is None:
        print("\nCould not open any port. Close other programs using the ports and try again.")
        sys.exit(1)

    # Always create log with startup entry
    append_ndjson({
        "timestamp": int(time.time() * 1000),
        "source": "capture",
        "message": "Capture started",
        "sessionId": "debug-session",
        "runId": "comm-debug",
    })

    teensy_buf = []
    esp32_buf = []
    teensy_bytes = 0
    esp32_bytes = 0
    last_heartbeat = time.time()

    def process(port, buf, name):
        nonlocal teensy_bytes, esp32_bytes
        if port is None:
            return
        raw = port.read(256)
        if not raw:
            return
        n = len(raw)
        if name == "teensy":
            teensy_bytes += n
        else:
            esp32_bytes += n
        buf.append(raw.decode("utf-8", errors="replace"))
        text = "".join(buf)
        lines = text.replace("\r\n", "\n").replace("\r", "\n").split("\n")
        buf[:] = [lines[-1]]  # keep incomplete line
        for line in lines[:-1]:
            line = line.strip()
            if line:
                rec = {
                    "timestamp": int(time.time() * 1000),
                    "source": name,
                    "location": f"serial:{name}",
                    "message": line,
                    "sessionId": "debug-session",
                    "runId": "comm-debug",
                }
                append_ndjson(rec)
                print(f"[{name}] {line}")

    try:
        while True:
            if teensy:
                process(teensy, teensy_buf, "teensy")
            if esp32:
                process(esp32, esp32_buf, "esp32")
            # Heartbeat every 5s: log byte counts to verify data flow
            if time.time() - last_heartbeat >= 5:
                last_heartbeat = time.time()
                rec = {
                    "timestamp": int(time.time() * 1000),
                    "source": "capture",
                    "message": f"Heartbeat: Teensy={teensy_bytes} bytes, ESP32={esp32_bytes} bytes",
                    "data": {"teensy_bytes": teensy_bytes, "esp32_bytes": esp32_bytes},
                    "sessionId": "debug-session",
                    "runId": "comm-debug",
                }
                append_ndjson(rec)
                print(f"  [heartbeat] Teensy: {teensy_bytes} bytes, ESP32: {esp32_bytes} bytes")
            time.sleep(0.01)
    except KeyboardInterrupt:
        pass
    finally:
        if teensy:
            teensy.close()
        if esp32:
            esp32.close()
    append_ndjson({
        "timestamp": int(time.time() * 1000),
        "source": "capture",
        "message": "Capture stopped",
        "sessionId": "debug-session",
        "runId": "comm-debug",
    })
    print(f"\nCapture saved to {LOG_PATH}")

if __name__ == "__main__":
    main()
