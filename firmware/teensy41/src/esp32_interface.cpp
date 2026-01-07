#include "esp32_interface.h"

ESP32Interface::ESP32Interface() : serial(ESP32_SERIAL) {
}

void ESP32Interface::begin() {
    serial.begin(ESP32_BAUD);
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.println("ESP32 Interface initialized");
    DEBUG_SERIAL.print("Baud rate: ");
    DEBUG_SERIAL.println(ESP32_BAUD);
    #endif
}

bool ESP32Interface::available() {
    return serial.available() > 0;
}

bool ESP32Interface::readMessage(uint8_t* buffer, size_t maxLen, size_t& bytesRead) {
    bytesRead = 0;
    
    if (!available()) {
        return false;
    }
    
    // Simple message format: [TYPE][LENGTH_H][LENGTH_L][DATA...][CHECKSUM]
    uint8_t type = serial.read();
    if (serial.available() < 2) {
        return false;
    }
    
    uint8_t lenHigh = serial.read();
    uint8_t lenLow = serial.read();
    uint16_t dataLen = (lenHigh << 8) | lenLow;
    
    if (dataLen > maxLen - 4) {
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
    // Command processing will be implemented based on specific requirements
    #if DEBUG_ENABLED
    DEBUG_SERIAL.print("Received command: 0x");
    DEBUG_SERIAL.println(command, HEX);
    #endif
    
    return true;
}

uint8_t ESP32Interface::calculateChecksum(const uint8_t* data, size_t len) {
    uint8_t checksum = 0;
    for (size_t i = 0; i < len; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

bool ESP32Interface::verifyChecksum(const uint8_t* data, size_t len, uint8_t checksum) {
    return calculateChecksum(data, len) == checksum;
}
