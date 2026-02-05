/*
 * BLE Bridge for Wireless POV Poi
 * 
 * Implements Nordic UART Service (NUS) to enable BLE communication
 * with Flutter apps (Android/Windows/Web).
 * 
 * Architecture:
 *   Flutter App <--> BLE (NUS) <--> ESP32 <--> Serial UART <--> Teensy 4.1
 */

#ifndef BLE_BRIDGE_H
#define BLE_BRIDGE_H

#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "config.h"

// Nordic UART Service UUIDs
#define SERVICE_UUID           "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e"  // Receive from app
#define CHARACTERISTIC_UUID_TX "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  // Send to app

// Command codes (Open-Pixel-Poi compatible)
#define CC_SUCCESS           0x00
#define CC_ERROR             0x01
#define CC_SET_BRIGHTNESS    0x02
#define CC_SET_SPEED         0x03
#define CC_SET_PATTERN       0x04
#define CC_SET_PATTERN_SLOT  0x05
#define CC_SET_PATTERN_ALL   0x06
#define CC_SET_SEQUENCER     0x0E
#define CC_START_SEQUENCER   0x0F

class BLEBridge {
private:
    BLEServer* pServer;
    BLECharacteristic* pRxCharacteristic;
    BLECharacteristic* pTxCharacteristic;
    bool deviceConnected;
    bool oldDeviceConnected;
    HardwareSerial* teensySerial;
    
    // Command buffer for BLE protocol (0xD0...0xD1)
    static const int BLE_CMD_BUFFER_SIZE = 1024;
    uint8_t bleCmdBuffer[BLE_CMD_BUFFER_SIZE];
    int bleCmdBufferIndex;
    bool inBLECommand;
    
    // Server callback handler
    class ServerCallbacks : public BLEServerCallbacks {
    private:
        BLEBridge* bridge;
    public:
        ServerCallbacks(BLEBridge* b) : bridge(b) {}
        
        void onConnect(BLEServer* pServer) {
            bridge->deviceConnected = true;
            Serial.println("BLE: Device connected");
        }
        
        void onDisconnect(BLEServer* pServer) {
            bridge->deviceConnected = false;
            Serial.println("BLE: Device disconnected");
        }
    };
    
    // RX characteristic callback handler
    class RxCallbacks : public BLECharacteristicCallbacks {
    private:
        BLEBridge* bridge;
    public:
        RxCallbacks(BLEBridge* b) : bridge(b) {}
        
        void onWrite(BLECharacteristic* pCharacteristic) {
            std::string rxValue = pCharacteristic->getValue();
            
            if (rxValue.length() > 0) {
                bridge->onBLEDataReceived((uint8_t*)rxValue.c_str(), rxValue.length());
            }
        }
    };

public:
    BLEBridge(HardwareSerial* serial);
    void setup();
    void loop();
    void onBLEDataReceived(uint8_t* data, size_t length);
    bool isConnected();
    void sendResponse(uint8_t* data, size_t length);
    
private:
    void processBLECommand(uint8_t* cmd, size_t length);
    void translateBLEtoInternalProtocol(uint8_t* cmd, size_t length);
};

#endif // BLE_BRIDGE_H
