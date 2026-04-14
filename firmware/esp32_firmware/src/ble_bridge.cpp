/*
 * BLE Bridge Implementation for Wireless POV Poi
 * 
 * Implements Nordic UART Service to bridge BLE commands to Teensy via Serial.
 * Translates between BLE protocol (0xD0...0xD1) and internal protocol (0xFF...0xFE).
 */

#include "ble_bridge.h"

BLEBridge::BLEBridge(HardwareSerial* serial) {
    teensySerial = serial;
    pServer = nullptr;
    pRxCharacteristic = nullptr;
    pTxCharacteristic = nullptr;
    deviceConnected = false;
    oldDeviceConnected = false;
    bleCmdBufferIndex = 0;
    inBLECommand = false;
}

void BLEBridge::setup() {
    Serial.println("Initializing BLE Bridge...");
    
    // Create BLE Device
    BLEDevice::init(BLE_DEVICE_NAME);
    
    // Create BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks(this));
    
    // Create Nordic UART Service
    BLEService* pService = pServer->createService(SERVICE_UUID);
    
    // Create RX Characteristic (receive from app)
    pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_WRITE_NR
    );
    pRxCharacteristic->setCallbacks(new RxCallbacks(this));
    
    // Create TX Characteristic (send to app)
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic->addDescriptor(new BLE2902());
    
    // Start service
    pService->start();
    
    // Start advertising
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);  // Apple connection interval guidelines (min)
    pAdvertising->setMaxPreferred(0x12);  // Apple connection interval guidelines (max)
    BLEDevice::startAdvertising();
    
    Serial.println("BLE Bridge initialized and advertising as: " + String(BLE_DEVICE_NAME));
}

void BLEBridge::loop() {
    // Handle connection/disconnection events
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Give the bluetooth stack time to prepare
        pServer->startAdvertising(); // Restart advertising
        Serial.println("BLE: Restarting advertising");
        oldDeviceConnected = deviceConnected;
    }
    
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
    
    // Check for responses from Teensy and forward to BLE
    if (teensySerial->available() && deviceConnected) {
        uint8_t response[256];
        size_t len = 0;
        unsigned long startTime = millis();
        
        // Read available data (up to 256 bytes) with timeout
        while (teensySerial->available() && len < sizeof(response) && (millis() - startTime < 100)) {
            response[len++] = teensySerial->read();
            
            // Check if we have a complete response (0xFF...0xFE)
            if (len >= 4 && response[len-1] == 0xFE && response[0] == 0xFF) {
                // Translate internal protocol to BLE protocol and send
                // Internal: 0xFF [cmd] [len] [data...] 0xFE
                // BLE:      0xD0 [cmd] [data...] 0xD1
                
                if (len >= 4) {
                    uint8_t bleResponse[260];
                    bleResponse[0] = BLE_CMD_START;
                    bleResponse[1] = response[1];  // Command code
                    
                    // Copy data (skip 0xFF, cmd, len, and final 0xFE)
                    size_t dataLen = len - 4;
                    if (dataLen > 0) {
                        memcpy(&bleResponse[2], &response[3], dataLen);
                    }
                    
                    bleResponse[2 + dataLen] = BLE_CMD_END;
                    
                    // Send via BLE
                    sendResponse(bleResponse, 3 + dataLen);
                }
                
                len = 0; // Reset for next response
                startTime = millis(); // Reset timeout
            }
        }
    }
}

void BLEBridge::onBLEDataReceived(uint8_t* data, size_t length) {
    Serial.print("BLE: Received ");
    Serial.print(length);
    Serial.println(" bytes");
    
    // Process incoming BLE data byte by byte to handle protocol markers
    for (size_t i = 0; i < length; i++) {
        uint8_t byte = data[i];
        
        if (byte == BLE_CMD_START && bleCmdBufferIndex == 0) {
            // Start of BLE command
            inBLECommand = true;
            bleCmdBufferIndex = 0;
        } else if (byte == BLE_CMD_END && inBLECommand) {
            // End of BLE command - process it
            processBLECommand(bleCmdBuffer, bleCmdBufferIndex);
            inBLECommand = false;
            bleCmdBufferIndex = 0;
        } else if (inBLECommand && bleCmdBufferIndex < BLE_CMD_BUFFER_SIZE) {
            bleCmdBuffer[bleCmdBufferIndex++] = byte;
        }
    }
}

void BLEBridge::processBLECommand(uint8_t* cmd, size_t length) {
    if (length < 1) return;
    
    Serial.print("BLE: Processing command 0x");
    Serial.print(cmd[0], HEX);
    Serial.print(", length: ");
    Serial.println(length);
    
    // Translate BLE protocol to internal Teensy protocol
    translateBLEtoInternalProtocol(cmd, length);
}

void BLEBridge::translateBLEtoInternalProtocol(uint8_t* cmd, size_t length) {
    /*
     * Translate BLE protocol to internal protocol:
     * BLE:      0xD0 [command_code] [data...] 0xD1
     * Internal: 0xFF [command_code] [len] [data...] 0xFE
     * 
     * Command code mapping:
     * BLE Code -> Internal Code
     * 0x02 (CC_SET_BRIGHTNESS) -> 0x06 (Set brightness)
     * 0x03 (CC_SET_SPEED) -> 0x07 (Set frame rate)
     * 0x04 (CC_SET_PATTERN) -> 0x03 (Upload pattern)
     * 0x05 (CC_SET_PATTERN_SLOT) -> 0x01 (Set mode with pattern index)
     * 0x06 (CC_SET_PATTERN_ALL) -> 0x01 (Set mode to auto-cycle)
     * 0x0E (CC_SET_SEQUENCER) -> 0x04 (Upload sequence)
     * 0x0F (CC_START_SEQUENCER) -> 0x01 (Set mode to sequence)
     */
    
    if (length < 1) return;
    
    uint8_t bleCommandCode = cmd[0];
    size_t dataLen = length - 1;  // Exclude command code
    
    // Map BLE command to internal command
    uint8_t internalCommand = bleCommandCode;
    
    switch (bleCommandCode) {
        case CC_SET_BRIGHTNESS:      // 0x02 -> 0x06
            internalCommand = 0x06;
            break;
        case CC_SET_SPEED:           // 0x03 -> 0x07
            internalCommand = 0x07;
            break;
        case CC_SET_PATTERN:         // 0x04 -> 0x03
            internalCommand = 0x03;
            break;
        case CC_SET_PATTERN_SLOT:    // 0x05 -> 0x01 (mode 2 = pattern mode)
            internalCommand = 0x01;
            // Need to convert: [pattern_slot] -> [mode=2, pattern_index]
            if (dataLen >= 1) {
                uint8_t packet[8];
                packet[0] = 0xFF;
                packet[1] = 0x01;  // Set mode command
                packet[2] = 0x02;  // Data length = 2
                packet[3] = 0x02;  // Mode 2 = pattern mode
                packet[4] = cmd[1]; // Pattern slot/index
                packet[5] = 0xFE;
                
                Serial.println("BLE: Mapped SET_PATTERN_SLOT to SetMode(2, slot)");
                teensySerial->write(packet, 6);
                return;
            }
            break;
        case CC_SET_PATTERN_ALL:     // 0x06 -> 0x01 (mode for auto-cycling)
            internalCommand = 0x01;
            // Auto-cycle patterns - use mode 2 with index 255 (special)
            {
                uint8_t packet[8];
                packet[0] = 0xFF;
                packet[1] = 0x01;  // Set mode command
                packet[2] = 0x02;  // Data length = 2
                packet[3] = 0x02;  // Mode 2 = pattern mode
                packet[4] = 0xFF;  // 0xFF = auto-cycle all patterns
                packet[5] = 0xFE;
                
                Serial.println("BLE: Mapped SET_PATTERN_ALL to SetMode(2, 255)");
                teensySerial->write(packet, 6);
                return;
            }
            break;
        case CC_SET_SEQUENCER:       // 0x0E -> 0x04
            internalCommand = 0x04;
            break;
        case CC_START_SEQUENCER:     // 0x0F -> 0x01 (mode 3 = sequence mode)
            internalCommand = 0x01;
            // Start sequencer - use mode 3 with sequence index
            if (dataLen >= 1) {
                uint8_t packet[8];
                packet[0] = 0xFF;
                packet[1] = 0x01;  // Set mode command
                packet[2] = 0x02;  // Data length = 2
                packet[3] = 0x03;  // Mode 3 = sequence mode
                packet[4] = cmd[1]; // Sequence index
                packet[5] = 0xFE;
                
                Serial.println("BLE: Mapped START_SEQUENCER to SetMode(3, seq_idx)");
                teensySerial->write(packet, 6);
                return;
            }
            break;
        default:
            // For other commands, pass through as-is
            break;
    }
    
    // Build internal protocol packet for simple pass-through commands
    uint8_t packet[1024];
    int packetLen = 0;
    
    packet[packetLen++] = 0xFF;  // Start marker
    packet[packetLen++] = internalCommand;
    packet[packetLen++] = (uint8_t)dataLen;  // Length (8-bit for simple commands)
    
    // Copy data
    if (dataLen > 0) {
        memcpy(&packet[packetLen], &cmd[1], dataLen);
        packetLen += dataLen;
    }
    
    packet[packetLen++] = 0xFE;  // End marker
    
    // Send to Teensy
    Serial.print("BLE: Forwarding to Teensy: ");
    for (int i = 0; i < packetLen; i++) {
        Serial.print(packet[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    teensySerial->write(packet, packetLen);
}

bool BLEBridge::isConnected() {
    return deviceConnected;
}

void BLEBridge::sendResponse(uint8_t* data, size_t length) {
    if (!deviceConnected || !pTxCharacteristic) {
        return;
    }
    
    Serial.print("BLE: Sending response: ");
    for (size_t i = 0; i < length; i++) {
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
    
    // Split large packets if needed (BLE has MTU limitations)
    size_t offset = 0;
    while (offset < length) {
        size_t chunkSize = min((size_t)BLE_MAX_PACKET_SIZE, length - offset);
        pTxCharacteristic->setValue(&data[offset], chunkSize);
        pTxCharacteristic->notify();
        offset += chunkSize;
        
        if (offset < length) {
            delay(10);  // Small delay between chunks
        }
    }
}
