# Testing Mode - Use USB Serial Instead of ESP32

To test the Teensy without ESP32, you can temporarily enable USB Serial command input.

## Quick Modification

Add this at the top of `teensy_firmware.ino` (after the includes, around line 45):

```cpp
// TEST MODE: Uncomment to use USB Serial instead of ESP32 Serial
// #define TEST_MODE_USB_SERIAL
```

Then modify `processSerialCommands()` function (around line 372):

**Find:**
```cpp
void processSerialCommands() {
  while (ESP32_SERIAL.available()) {
    uint8_t byte = ESP32_SERIAL.read();
```

**Replace with:**
```cpp
void processSerialCommands() {
  #ifdef TEST_MODE_USB_SERIAL
    #define CMD_SERIAL Serial
  #else
    #define CMD_SERIAL ESP32_SERIAL
  #endif
  
  while (CMD_SERIAL.available()) {
    uint8_t byte = CMD_SERIAL.read();
```

Also update `sendAck()` function (around line 1162):

**Find:**
```cpp
void sendAck(uint8_t cmd) {
  ESP32_SERIAL.write(0xFF);
  ESP32_SERIAL.write(0xAA);  // ACK
  ESP32_SERIAL.write(cmd);
  ESP32_SERIAL.write(0xFE);
}
```

**Replace with:**
```cpp
void sendAck(uint8_t cmd) {
  #ifdef TEST_MODE_USB_SERIAL
    #define CMD_SERIAL Serial
  #else
    #define CMD_SERIAL ESP32_SERIAL
  #endif
  
  CMD_SERIAL.write(0xFF);
  CMD_SERIAL.write(0xAA);  // ACK
  CMD_SERIAL.write(cmd);
  CMD_SERIAL.write(0xFE);
}
```

And update `sendStatus()` function (around line 1169):

**Find:**
```cpp
void sendStatus() {
  ESP32_SERIAL.write(0xFF);
  ESP32_SERIAL.write(0xBB);  // Status response
  ESP32_SERIAL.write(currentMode);
  ESP32_SERIAL.write(currentIndex);
  ESP32_SERIAL.write(0xFE);
}
```

**Replace with:**
```cpp
void sendStatus() {
  #ifdef TEST_MODE_USB_SERIAL
    #define CMD_SERIAL Serial
  #else
    #define CMD_SERIAL ESP32_SERIAL
  #endif
  
  CMD_SERIAL.write(0xFF);
  CMD_SERIAL.write(0xBB);  // Status response
  CMD_SERIAL.write(currentMode);
  CMD_SERIAL.write(currentIndex);
  CMD_SERIAL.write(0xFE);
}
```

## Usage

1. **Enable test mode**: Uncomment `#define TEST_MODE_USB_SERIAL` at the top
2. **Upload firmware** to Teensy
3. **Open Serial Monitor** at 115200 baud
4. **Use Python script** or send hex commands manually
5. **Disable test mode**: Comment out `#define TEST_MODE_USB_SERIAL` when done

## Testing

Use the `test_teensy_standalone.py` script in the project root, or send commands manually via Serial Monitor.

**Remember to disable test mode before using with ESP32!**
