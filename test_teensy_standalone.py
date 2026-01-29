#!/usr/bin/env python3
"""
Test script for Teensy POV Poi without ESP32
Sends commands via USB Serial to test LED patterns and images
"""

import serial
import time
import sys

# Serial port configuration
# Windows: 'COM3', 'COM4', etc.
# Linux/Mac: '/dev/ttyACM0', '/dev/ttyUSB0', etc.
SERIAL_PORT = 'COM3'  # CHANGE THIS to match your Teensy port
BAUD_RATE = 115200

def send_command(ser, cmd_bytes, description=""):
    """Send command bytes to Teensy and print response"""
    if description:
        print(f"\n{description}")
    print(f"Sending: {' '.join(f'{b:02X}' for b in cmd_bytes)}")
    
    ser.write(bytes(cmd_bytes))
    time.sleep(0.2)  # Wait for processing
    
    # Read any response
    if ser.in_waiting:
        response = ser.read(ser.in_waiting)
        print(f"Response: {response.hex().upper()}")
    else:
        print("(No response)")

def main():
    print("=" * 50)
    print("Teensy POV Poi Standalone Test")
    print("=" * 50)
    
    # Try to open serial port
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        print(f"\nConnected to {SERIAL_PORT} at {BAUD_RATE} baud")
        time.sleep(2)  # Wait for Teensy to initialize
    except serial.SerialException as e:
        print(f"\nERROR: Could not open serial port {SERIAL_PORT}")
        print(f"Error: {e}")
        print("\nPlease:")
        print("1. Check that Teensy is connected via USB")
        print("2. Find the correct COM port (Windows Device Manager or 'ls /dev/tty*' on Linux/Mac)")
        print("3. Update SERIAL_PORT in this script")
        sys.exit(1)
    
    try:
        # Clear any existing data
        ser.reset_input_buffer()
        
        print("\n" + "=" * 50)
        print("TEST 1: Set Brightness to 50%")
        print("=" * 50)
        send_command(ser, [0xFF, 0x06, 0x01, 0x80, 0xFE], 
                    "Setting brightness to 128 (50%)...")
        time.sleep(1)
        
        print("\n" + "=" * 50)
        print("TEST 2: Pattern 0 - Rainbow")
        print("=" * 50)
        send_command(ser, [0xFF, 0x01, 0x02, 0x02, 0x00, 0xFE],
                    "Activating Pattern 0 (Rainbow)...")
        print("Watch LEDs for 5 seconds...")
        time.sleep(5)
        
        print("\n" + "=" * 50)
        print("TEST 3: Pattern 1 - Fire")
        print("=" * 50)
        send_command(ser, [0xFF, 0x01, 0x02, 0x02, 0x01, 0xFE],
                    "Activating Pattern 1 (Fire)...")
        print("Watch LEDs for 5 seconds...")
        time.sleep(5)
        
        print("\n" + "=" * 50)
        print("TEST 4: Pattern 2 - Comet")
        print("=" * 50)
        send_command(ser, [0xFF, 0x01, 0x02, 0x02, 0x02, 0xFE],
                    "Activating Pattern 2 (Comet)...")
        print("Watch LEDs for 5 seconds...")
        time.sleep(5)
        
        print("\n" + "=" * 50)
        print("TEST 5: Pattern 3 - Breathing")
        print("=" * 50)
        send_command(ser, [0xFF, 0x01, 0x02, 0x02, 0x03, 0xFE],
                    "Activating Pattern 3 (Breathing)...")
        print("Watch LEDs for 5 seconds...")
        time.sleep(5)
        
        print("\n" + "=" * 50)
        print("TEST 6: Pattern 4 - Plasma")
        print("=" * 50)
        send_command(ser, [0xFF, 0x01, 0x02, 0x02, 0x04, 0xFE],
                    "Activating Pattern 4 (Plasma)...")
        print("Watch LEDs for 5 seconds...")
        time.sleep(5)
        
        print("\n" + "=" * 50)
        print("TEST 7: Image 0 - Smiley Face")
        print("=" * 50)
        send_command(ser, [0xFF, 0x01, 0x02, 0x01, 0x00, 0xFE],
                    "Activating Image 0 (Smiley Face)...")
        print("Watch LEDs for 5 seconds (spin the poi to see image)...")
        time.sleep(5)
        
        print("\n" + "=" * 50)
        print("TEST 8: Image 1 - Rainbow Gradient")
        print("=" * 50)
        send_command(ser, [0xFF, 0x01, 0x02, 0x01, 0x01, 0xFE],
                    "Activating Image 1 (Rainbow Gradient)...")
        print("Watch LEDs for 5 seconds...")
        time.sleep(5)
        
        print("\n" + "=" * 50)
        print("TEST 9: Image 2 - Heart")
        print("=" * 50)
        send_command(ser, [0xFF, 0x01, 0x02, 0x01, 0x02, 0xFE],
                    "Activating Image 2 (Heart)...")
        print("Watch LEDs for 5 seconds...")
        time.sleep(5)
        
        print("\n" + "=" * 50)
        print("TEST 10: Sequence 0 - Auto Cycle")
        print("=" * 50)
        send_command(ser, [0xFF, 0x01, 0x02, 0x03, 0x00, 0xFE],
                    "Activating Sequence 0 (cycles through images/patterns)...")
        print("Watch LEDs for 15 seconds (sequence cycles every 2 seconds)...")
        time.sleep(15)
        
        print("\n" + "=" * 50)
        print("TEST 11: Request Status")
        print("=" * 50)
        send_command(ser, [0xFF, 0x10, 0x00, 0xFE],
                    "Requesting status...")
        time.sleep(1)
        
        print("\n" + "=" * 50)
        print("TEST 12: Turn Off (Idle Mode)")
        print("=" * 50)
        send_command(ser, [0xFF, 0x01, 0x02, 0x00, 0x00, 0xFE],
                    "Setting to Idle mode (LEDs off)...")
        time.sleep(1)
        
        print("\n" + "=" * 50)
        print("All tests complete!")
        print("=" * 50)
        
    except KeyboardInterrupt:
        print("\n\nTest interrupted by user")
    except Exception as e:
        print(f"\n\nERROR: {e}")
    finally:
        ser.close()
        print("\nSerial port closed")

if __name__ == "__main__":
    main()
