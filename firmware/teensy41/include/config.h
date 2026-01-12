#ifndef CONFIG_H
#define CONFIG_H

// Hardware Configuration
#define LED_DATA_PIN 11       // APA102 data pin
#define LED_CLOCK_PIN 13      // APA102 clock pin
#define NUM_LEDS 32           // Number of LEDs in the strip (31 for display, 1 for level shifting)

// ESP32 Communication
#define ESP32_SERIAL Serial1  // Hardware serial for ESP32
#define ESP32_BAUD 115200     // Baud rate for ESP32 communication
#define ESP32_RX_PIN 0        // RX pin for ESP32
#define ESP32_TX_PIN 1        // TX pin for ESP32

// POV Configuration
#define POV_FRAME_RATE 60     // Frames per second
#define POV_IMAGE_WIDTH 360   // Angular resolution (degrees)
#define POV_IMAGE_HEIGHT NUM_LEDS

// Performance Settings
#define LED_BRIGHTNESS 128    // Default brightness (0-255)
#define LED_COLOR_ORDER BGR   // APA102 color order

// Debug Settings
#define DEBUG_ENABLED true
#define DEBUG_SERIAL Serial
#define DEBUG_BAUD 115200

// SD Card Configuration
#define SD_CARD_ENABLED true
#define SD_CS_PIN BUILTIN_SDCARD  // Teensy 4.1 built-in SD slot
#define SD_IMAGE_DIR "/images"
#define SD_CACHE_SIZE 2  // Number of images to cache in RAM
#define SD_FILE_MAGIC 0x504F5631  // "POV1" in hex

// SD Card Performance Settings
#define SD_CONFIG SdioConfig(FIFO_SDIO)  // Use SDIO with FIFO for best performance

#endif // CONFIG_H
