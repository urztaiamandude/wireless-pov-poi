#ifndef ESP32_INTERFACE_H
#define ESP32_INTERFACE_H

#include <Arduino.h>
#include "config.h"

// Forward declarations
class SDStorageManager;
class POVEngine;
class LEDDriver;

// Message types for communication protocol
enum MessageType {
    MSG_IMAGE_DATA = 0x01,
    MSG_COMMAND = 0x02,
    MSG_STATUS = 0x03,
    MSG_ACK = 0x04,
    MSG_NACK = 0x05,
    MSG_SD_SAVE_IMAGE = 0x10,
    MSG_SD_LIST_IMAGES = 0x11,
    MSG_SD_DELETE_IMAGE = 0x12,
    MSG_SD_GET_INFO = 0x13,
    MSG_SD_LOAD_IMAGE = 0x14
};

// Command types
enum CommandType {
    CMD_PLAY = 0x01,
    CMD_PAUSE = 0x02,
    CMD_STOP = 0x03,
    CMD_SET_BRIGHTNESS = 0x04,
    CMD_SET_MODE = 0x05
};

class ESP32Interface {
public:
    ESP32Interface();
    
    // Initialize serial communication with ESP32
    void begin();
    
    // Set references to SD storage, POV engine, and LED driver for command processing
    void setSDStorage(SDStorageManager* sd);
    void setPOVEngine(POVEngine* pov);
    void setLEDDriver(LEDDriver* led);
    
    // Check if data is available from ESP32
    bool available();
    
    // Read incoming message from ESP32
    bool readMessage(uint8_t* buffer, size_t maxLen, size_t& bytesRead, MessageType& msgType);
    
    // Send message to ESP32
    bool sendMessage(MessageType type, const uint8_t* data, size_t len);
    
    // Send acknowledgment
    void sendAck();
    
    // Send negative acknowledgment
    void sendNack();
    
    // Process incoming commands
    bool processCommand(uint8_t command, const uint8_t* data, size_t len);
    
    // Process incoming messages (determines type and routes accordingly)
    bool processMessage(MessageType type, const uint8_t* data, size_t len);

private:
    HardwareSerial& serial;
    SDStorageManager* sdStorage;
    POVEngine* povEngine;
    LEDDriver* ledDriver;
    
    // Calculate checksum for message
    uint8_t calculateChecksum(const uint8_t* data, size_t len);
    
    // Verify message checksum
    bool verifyChecksum(const uint8_t* data, size_t len, uint8_t checksum);
    
    // Image data handler
    bool handleImageData(const uint8_t* data, size_t len);
    
    // SD card message handlers
    bool handleSDSaveImage(const uint8_t* data, size_t len);
    bool handleSDListImages(const uint8_t* data, size_t len);
    bool handleSDDeleteImage(const uint8_t* data, size_t len);
    bool handleSDGetInfo(const uint8_t* data, size_t len);
    bool handleSDLoadImage(const uint8_t* data, size_t len);
};

#endif // ESP32_INTERFACE_H
