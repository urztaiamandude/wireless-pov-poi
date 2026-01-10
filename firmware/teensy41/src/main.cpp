#include <Arduino.h>
#include "config.h"
#include "led_driver.h"
#include "esp32_interface.h"
#include "pov_engine.h"

// Global objects
LEDDriver ledDriver;
ESP32Interface esp32;
POVEngine povEngine(ledDriver);

// Timing variables
unsigned long lastFrameTime = 0;
const unsigned long frameInterval = 1000 / POV_FRAME_RATE;

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
    unsigned long currentTime = millis();
    
    // Process ESP32 communication
    if (esp32.available()) {
        uint8_t buffer[512];
        size_t bytesRead = 0;
        
        if (esp32.readMessage(buffer, sizeof(buffer), bytesRead)) {
            #if DEBUG_ENABLED
            DEBUG_SERIAL.print("Received ");
            DEBUG_SERIAL.print(bytesRead);
            DEBUG_SERIAL.println(" bytes from ESP32");
            #endif
            
            // Process message based on type
            // This would be expanded based on protocol requirements
        }
    }
    
    // Update POV engine at fixed frame rate
    if (currentTime - lastFrameTime >= frameInterval) {
        lastFrameTime = currentTime;
        povEngine.update();
    }
    
    // Small yield to prevent watchdog issues
    yield();
}
