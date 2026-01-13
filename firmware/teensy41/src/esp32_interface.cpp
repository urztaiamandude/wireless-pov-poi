#include "esp32_interface.h"
#include "sd_storage.h"
#include "pov_engine.h"
#include "led_driver.h"

ESP32Interface::ESP32Interface() 
    : serial(ESP32_SERIAL), sdStorage(nullptr), povEngine(nullptr), ledDriver(nullptr) {
}

void ESP32Interface::begin() {
    serial.begin(ESP32_BAUD);
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.println("ESP32 Interface initialized");
    DEBUG_SERIAL.print("Baud rate: ");
    DEBUG_SERIAL.println(ESP32_BAUD);
    #endif
}

void ESP32Interface::setSDStorage(SDStorageManager* sd) {
    sdStorage = sd;
}

void ESP32Interface::setPOVEngine(POVEngine* pov) {
    povEngine = pov;
}

void ESP32Interface::setLEDDriver(LEDDriver* led) {
    ledDriver = led;
}

bool ESP32Interface::available() {
    return serial.available() > 0;
}

bool ESP32Interface::readMessage(uint8_t* buffer, size_t maxLen, size_t& bytesRead, MessageType& msgType) {
    bytesRead = 0;
    
    if (!available() || maxLen < 4) {
        return false;
    }
    
    // Simple message format: [TYPE][LENGTH_H][LENGTH_L][DATA...][CHECKSUM]
    uint8_t type = serial.read();
    msgType = (MessageType)type;
    
    if (serial.available() < 2) {
        return false;
    }
    
    uint8_t lenHigh = serial.read();
    uint8_t lenLow = serial.read();
    uint16_t dataLen = (lenHigh << 8) | lenLow;
    
    if (dataLen + 4 > maxLen) {
        return false;
    }
    
    // Read data
    uint32_t timeout = millis() + 1000;
    uint16_t idx = 0;
    while (idx < dataLen && millis() < timeout) {
        if (serial.available()) {
            buffer[idx++] = serial.read();
        }
    }
    
    if (idx != dataLen) {
        return false;
    }
    
    // Read checksum
    if (serial.available() < 1) {
        return false;
    }
    uint8_t checksum = serial.read();
    
    // Verify checksum
    if (!verifyChecksum(buffer, dataLen, checksum)) {
        sendNack();
        return false;
    }
    
    bytesRead = dataLen;
    sendAck();
    return true;
}

bool ESP32Interface::sendMessage(MessageType type, const uint8_t* data, size_t len) {
    if (len > 65535) {
        return false;
    }
    
    // Send message format: [TYPE][LENGTH_H][LENGTH_L][DATA...][CHECKSUM]
    serial.write((uint8_t)type);
    serial.write((uint8_t)(len >> 8));
    serial.write((uint8_t)(len & 0xFF));
    
    if (data && len > 0) {
        serial.write(data, len);
    }
    
    uint8_t checksum = calculateChecksum(data, len);
    serial.write(checksum);
    
    return true;
}

void ESP32Interface::sendAck() {
    sendMessage(MSG_ACK, nullptr, 0);
}

void ESP32Interface::sendNack() {
    sendMessage(MSG_NACK, nullptr, 0);
}

bool ESP32Interface::processCommand(uint8_t command, const uint8_t* data, size_t len) {
    #if DEBUG_ENABLED
    DEBUG_SERIAL.print("Received command: 0x");
    DEBUG_SERIAL.print(command, HEX);
    DEBUG_SERIAL.print(", data length: ");
    DEBUG_SERIAL.println(len);
    #endif
    
    if (!povEngine) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("ERROR: POV engine not initialized");
        #endif
        sendNack();
        return false;
    }
    
    switch (command) {
        case CMD_PLAY:
            // Start/resume POV display
            #if DEBUG_ENABLED
            DEBUG_SERIAL.println("Command: PLAY");
            #endif
            povEngine->setEnabled(true);
            sendAck();
            return true;
            
        case CMD_PAUSE:
            // Pause POV display
            #if DEBUG_ENABLED
            DEBUG_SERIAL.println("Command: PAUSE");
            #endif
            povEngine->setEnabled(false);
            sendAck();
            return true;
            
        case CMD_STOP:
            // Stop POV display and clear
            #if DEBUG_ENABLED
            DEBUG_SERIAL.println("Command: STOP");
            #endif
            povEngine->setEnabled(false);
            // Clear LEDs through the engine
            sendAck();
            return true;
            
        case CMD_SET_BRIGHTNESS:
            // Set LED brightness (data[0] = brightness 0-255)
            if (len >= 1) {
                uint8_t brightness = data[0];
                #if DEBUG_ENABLED
                DEBUG_SERIAL.print("Command: SET_BRIGHTNESS = ");
                DEBUG_SERIAL.println(brightness);
                #endif
                if (ledDriver) {
                    ledDriver->setBrightness(brightness);
                    sendAck();
                    return true;
                } else {
                    #if DEBUG_ENABLED
                    DEBUG_SERIAL.println("ERROR: LED driver not initialized");
                    #endif
                    sendNack();
                    return false;
                }
            }
            #if DEBUG_ENABLED
            DEBUG_SERIAL.println("ERROR: Insufficient data for SET_BRIGHTNESS");
            #endif
            sendNack();
            return false;
            
        case CMD_SET_MODE:
            // Set display mode (data[0] = mode)
            if (len >= 1) {
                uint8_t mode = data[0];
                #if DEBUG_ENABLED
                DEBUG_SERIAL.print("Command: SET_MODE = ");
                DEBUG_SERIAL.println(mode);
                #endif
                povEngine->setMode(mode);
                sendAck();
                return true;
            }
            #if DEBUG_ENABLED
            DEBUG_SERIAL.println("ERROR: Insufficient data for SET_MODE");
            #endif
            sendNack();
            return false;
            
        default:
            #if DEBUG_ENABLED
            DEBUG_SERIAL.print("Unknown command: 0x");
            DEBUG_SERIAL.println(command, HEX);
            #endif
            sendNack();
            return false;
    }
}

bool ESP32Interface::processMessage(MessageType type, const uint8_t* data, size_t len) {
    #if DEBUG_ENABLED
    DEBUG_SERIAL.print("Processing message type: 0x");
    DEBUG_SERIAL.println((uint8_t)type, HEX);
    #endif
    
    switch (type) {
        case MSG_IMAGE_DATA:
            return handleImageData(data, len);
        
        case MSG_SD_SAVE_IMAGE:
            return handleSDSaveImage(data, len);
        
        case MSG_SD_LIST_IMAGES:
            return handleSDListImages(data, len);
        
        case MSG_SD_DELETE_IMAGE:
            return handleSDDeleteImage(data, len);
        
        case MSG_SD_GET_INFO:
            return handleSDGetInfo(data, len);
        
        case MSG_SD_LOAD_IMAGE:
            return handleSDLoadImage(data, len);
        
        case MSG_COMMAND:
            if (len > 0) {
                return processCommand(data[0], data + 1, len - 1);
            }
            return false;
        
        default:
            #if DEBUG_ENABLED
            DEBUG_SERIAL.println("Unknown message type");
            #endif
            return false;
    }
}

bool ESP32Interface::handleImageData(const uint8_t* data, size_t len) {
    if (!povEngine) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("ERROR: POV engine not available");
        #endif
        sendNack();
        return false;
    }
    
    // Message format: [width:2][height:2][image_data]
    if (len < 4) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("ERROR: Image data message too short");
        #endif
        sendNack();
        return false;
    }
    
    // Extract dimensions
    uint16_t width = (data[0] << 8) | data[1];
    uint16_t height = (data[2] << 8) | data[3];
    
    // Extract image data
    const uint8_t* imageData = data + 4;
    size_t imageDataLen = len - 4;
    
    // Verify data size
    size_t expectedSize = width * height * 3; // RGB data
    if (imageDataLen != expectedSize) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.print("ERROR: Image data size mismatch. Expected: ");
        DEBUG_SERIAL.print(expectedSize);
        DEBUG_SERIAL.print(", Got: ");
        DEBUG_SERIAL.println(imageDataLen);
        #endif
        sendNack();
        return false;
    }
    
    // Load image data into POV engine
    povEngine->loadImageData(imageData, width, height);
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.print("Image data loaded: ");
    DEBUG_SERIAL.print(width);
    DEBUG_SERIAL.print("x");
    DEBUG_SERIAL.println(height);
    #endif
    
    sendAck();
    return true;
}

bool ESP32Interface::handleSDSaveImage(const uint8_t* data, size_t len) {
    if (!sdStorage || !sdStorage->isInitialized()) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("SD storage not available");
        #endif
        sendNack();
        return false;
    }
    
    // Message format: [filename_len][filename][width:2][height:2][image_data]
    if (len < 5) {
        sendNack();
        return false;
    }
    
    uint8_t filenameLen = data[0];
    if (len < 1 + filenameLen + 4) {
        sendNack();
        return false;
    }
    
    // Extract filename
    char filename[64];
    memcpy(filename, data + 1, filenameLen);
    filename[filenameLen] = '\0';
    
    // Extract dimensions
    uint16_t width = (data[1 + filenameLen] << 8) | data[2 + filenameLen];
    uint16_t height = (data[3 + filenameLen] << 8) | data[4 + filenameLen];
    
    // Extract image data
    const uint8_t* imageData = data + 5 + filenameLen;
    size_t imageDataLen = len - 5 - filenameLen;
    
    // Verify data size
    if (imageDataLen != width * height * 3) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("Image data size mismatch");
        #endif
        sendNack();
        return false;
    }
    
    // Save image
    SDError error = sdStorage->saveImage(filename, imageData, width, height);
    
    if (error == SD_OK) {
        sendAck();
        return true;
    } else {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.print("Save failed: ");
        DEBUG_SERIAL.println(sdStorage->getErrorString(error));
        #endif
        sendNack();
        return false;
    }
}

bool ESP32Interface::handleSDListImages(const uint8_t* data, size_t len) {
    if (!sdStorage || !sdStorage->isInitialized()) {
        sendNack();
        return false;
    }
    
    char filenames[32][64];  // Support up to 32 files
    int count = sdStorage->listImages(filenames, 32);
    
    // Build response: [count][filename1_len][filename1][filename2_len][filename2]...
    uint8_t response[2048];
    size_t responseLen = 0;
    
    response[responseLen++] = (uint8_t)count;
    
    for (int i = 0; i < count && responseLen < sizeof(response) - 64; i++) {
        size_t nameLen = strlen(filenames[i]);
        response[responseLen++] = (uint8_t)nameLen;
        memcpy(response + responseLen, filenames[i], nameLen);
        responseLen += nameLen;
    }
    
    sendMessage(MSG_SD_LIST_IMAGES, response, responseLen);
    return true;
}

bool ESP32Interface::handleSDDeleteImage(const uint8_t* data, size_t len) {
    if (!sdStorage || !sdStorage->isInitialized()) {
        sendNack();
        return false;
    }
    
    // Message format: [filename_len][filename]
    if (len < 2) {
        sendNack();
        return false;
    }
    
    uint8_t filenameLen = data[0];
    if (len < 1 + filenameLen) {
        sendNack();
        return false;
    }
    
    char filename[64];
    memcpy(filename, data + 1, filenameLen);
    filename[filenameLen] = '\0';
    
    SDError error = sdStorage->deleteImage(filename);
    
    if (error == SD_OK) {
        sendAck();
        return true;
    } else {
        sendNack();
        return false;
    }
}

bool ESP32Interface::handleSDGetInfo(const uint8_t* data, size_t len) {
    if (!sdStorage || !sdStorage->isInitialized()) {
        sendNack();
        return false;
    }
    
    uint64_t totalSpace = sdStorage->getTotalSpace();
    uint64_t freeSpace = sdStorage->getFreeSpace();
    bool cardPresent = sdStorage->isCardPresent();
    
    // Build response: [card_present][total_space:8][free_space:8]
    uint8_t response[17];
    response[0] = cardPresent ? 1 : 0;
    
    // Pack 64-bit values (big-endian)
    for (int i = 0; i < 8; i++) {
        response[1 + i] = (totalSpace >> (56 - i * 8)) & 0xFF;
        response[9 + i] = (freeSpace >> (56 - i * 8)) & 0xFF;
    }
    
    sendMessage(MSG_SD_GET_INFO, response, sizeof(response));
    return true;
}

bool ESP32Interface::handleSDLoadImage(const uint8_t* data, size_t len) {
    if (!sdStorage || !sdStorage->isInitialized() || !povEngine) {
        sendNack();
        return false;
    }
    
    // Message format: [filename_len][filename]
    if (len < 2) {
        sendNack();
        return false;
    }
    
    uint8_t filenameLen = data[0];
    if (len < 1 + filenameLen) {
        sendNack();
        return false;
    }
    
    char filename[64];
    memcpy(filename, data + 1, filenameLen);
    filename[filenameLen] = '\0';
    
    // Load image from SD and pass to POV engine
    SDError error = povEngine->loadImageFromSD(filename, sdStorage);
    
    if (error == SD_OK) {
        sendAck();
        return true;
    } else {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.print("Load failed: ");
        DEBUG_SERIAL.println(sdStorage->getErrorString(error));
        #endif
        sendNack();
        return false;
    }
}

uint8_t ESP32Interface::calculateChecksum(const uint8_t* data, size_t len) {
    uint8_t checksum = 0;
    if (data != nullptr) {
        for (size_t i = 0; i < len; i++) {
            checksum ^= data[i];
        }
    }
    return checksum;
}

bool ESP32Interface::verifyChecksum(const uint8_t* data, size_t len, uint8_t checksum) {
    return calculateChecksum(data, len) == checksum;
}
