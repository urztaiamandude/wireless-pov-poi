#include <Arduino.h>
#include "config.h"
#include "led_driver.h"
#include "esp32_interface.h"
#include "pov_engine.h"
#include "sd_storage.h"

// Global objects
LEDDriver ledDriver;
ESP32Interface esp32;
POVEngine povEngine(ledDriver);

#if SD_CARD_ENABLED
SDStorageManager sdStorage;
#endif

void setup() {
    // Initialize debug serial
    #if DEBUG_ENABLED
    DEBUG_SERIAL.begin(DEBUG_BAUD);
    while (!DEBUG_SERIAL && millis() < 3000) {
        // Wait for serial with timeout
    }
    DEBUG_SERIAL.println("=== Teensy 4.1 POV Poi System ===");
    DEBUG_SERIAL.println("Initializing...");
    #endif
    
    // Initialize LED driver
    ledDriver.begin();
    
    // Initialize ESP32 communication
    esp32.begin();
    
    // Initialize POV engine
    povEngine.begin();
    
    // Initialize SD card storage
    #if SD_CARD_ENABLED
    DEBUG_SERIAL.println("Initializing SD card storage...");
    if (sdStorage.begin()) {
        DEBUG_SERIAL.println("SD card ready");
        DEBUG_SERIAL.print("Total space: ");
        DEBUG_SERIAL.print(sdStorage.getTotalSpace() / (1024ULL * 1024ULL));
        DEBUG_SERIAL.println(" MB");
        DEBUG_SERIAL.print("Free space: ");
        DEBUG_SERIAL.print(sdStorage.getFreeSpace() / (1024ULL * 1024ULL));
        DEBUG_SERIAL.println(" MB");
        
        // Set SD storage reference in ESP32 interface
        esp32.setSDStorage(&sdStorage);
    } else {
        DEBUG_SERIAL.println("WARNING: SD card not available - running without SD storage");
    }
    #else
    DEBUG_SERIAL.println("SD card support disabled");
    #endif
    
    // Set POV engine reference in ESP32 interface
    esp32.setPOVEngine(&povEngine);
    
    // Set LED driver reference in ESP32 interface
    esp32.setLEDDriver(&ledDriver);
    
    // Startup animation
    ledDriver.clear();
    for (int i = 0; i < ledDriver.getNumLEDs(); i++) {
        ledDriver.setPixel(i, 0, 255, 0);
        ledDriver.show();
        delay(10);
    }
    delay(500);
    ledDriver.clear();
    ledDriver.show();
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.println("Initialization complete!");
    #endif
}

void loop() {
    // Process ESP32 communication
    if (esp32.available()) {
        uint8_t buffer[2048];  // Increased buffer for larger messages
        size_t bytesRead = 0;
        
        // Try reading simple protocol first (ESP32 sends 0xFF marker)
        uint8_t simpleCommand = 0;
        if (esp32.readSimpleMessage(buffer, sizeof(buffer), bytesRead, simpleCommand)) {
            #if DEBUG_ENABLED
            DEBUG_SERIAL.print("Received simple command: 0x");
            DEBUG_SERIAL.print(simpleCommand, HEX);
            DEBUG_SERIAL.print(", ");
            DEBUG_SERIAL.print(bytesRead);
            DEBUG_SERIAL.println(" bytes");
            #endif
            
            // Process simple protocol command
            esp32.processSimpleCommand(simpleCommand, buffer, bytesRead);
        } else {
            // Fall back to structured protocol
            MessageType msgType;
            if (esp32.readMessage(buffer, sizeof(buffer), bytesRead, msgType)) {
                #if DEBUG_ENABLED
                DEBUG_SERIAL.print("Received structured message, type: 0x");
                DEBUG_SERIAL.print((uint8_t)msgType, HEX);
                DEBUG_SERIAL.print(", ");
                DEBUG_SERIAL.print(bytesRead);
                DEBUG_SERIAL.println(" bytes");
                #endif
                
                // Process message based on type
                esp32.processMessage(msgType, buffer, bytesRead);
            }
        }
    }
    
    // Let POV engine handle its own frame timing via frameDelay
    povEngine.update();
    
    // Small yield to prevent watchdog issues
    yield();
}
