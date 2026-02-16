#ifndef CONFIG_H
#define CONFIG_H

// Hardware Configuration
#define LED_DATA_PIN 11       // APA102 data pin
#define LED_CLOCK_PIN 13      // APA102 clock pin
#define NUM_LEDS 32           // Number of LEDs in the strip (all 32 for display)
#define DISPLAY_LEDS NUM_LEDS // All LEDs are display LEDs (hardware level shifter used)

// ESP32 Communication
#define ESP32_SERIAL Serial1  // Hardware serial for ESP32
#define ESP32_BAUD 115200     // Baud rate for ESP32 communication
#define ESP32_RX_PIN 0        // RX pin for ESP32
#define ESP32_TX_PIN 1        // TX pin for ESP32

// Audio Input Configuration (MAX9814 Microphone Amplifier)
// The MAX9814 output connects through a level shifter to Teensy analog input.
// MAX9814 Gain: set via GAIN pin (no connect = 60dB, GND = 50dB, VDD = 40dB)
// MAX9814 A/R: set via A/R pin (no connect = default attack/release)
// Output: biased at VDD/2 (~1.65V), swings +/- based on audio level
#define AUDIO_PIN A0           // Analog input from MAX9814 via level shifter
#define AUDIO_SAMPLES 64       // Number of samples for running average
#define AUDIO_NOISE_FLOOR 50   // Minimum threshold to filter ambient noise
#define AUDIO_ENABLED true     // Set false to disable audio input reads

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
#define SD_IMAGE_DIR "/images"    // PlatformIO firmware uses /images (auto-created; Arduino IDE uses /poi_images/)
#define SD_CACHE_SIZE 2  // Number of images to cache in RAM
#define SD_FILE_MAGIC 0x504F5631  // "POV1" in hex
// Note: SDIO configured with SdioConfig(FIFO_SDIO) for best performance (~20-25 MB/s read)

// Image Storage Configuration
// PSRAM Support: With 16MB PSRAM (2x 8MB chips), we can store more and larger images
#ifdef ARDUINO_TEENSY41
  // With PSRAM: 50 images at variable sizes up to 400 pixels wide
  #define MAX_IMAGES 50  // Maximum number of images that can be stored in RAM
  #define IMAGE_MAX_WIDTH 400  // Maximum width for stored images (with PSRAM)
#else
  // Without PSRAM: Conservative limits
  #define MAX_IMAGES 10
  #define IMAGE_MAX_WIDTH 200
#endif
#define MAX_FILENAME_LEN 32  // Maximum length for SD image filenames

#endif // CONFIG_H
