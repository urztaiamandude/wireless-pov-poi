# BMP Image Processing Guide

This guide explains how to use the BMPImageReader and BMPImageSequence classes for BMP image handling in the wireless-pov-poi project.

## Overview

The wireless-pov-poi firmware includes two powerful classes from the pov-library:

- **BMPImageReader**: Standalone BMP image processing
- **BMPImageSequence**: Playlist management for multiple images

## BMPImageReader

### Features

- Parses BMP headers automatically
- Line-by-line access for POV displays
- Memory efficient with user-provided buffers
- Works with Teensy 4.1 built-in SD card
- Template-based (works with any File-compatible storage)

### API Reference

**Methods:**
- `bool begin(FileType& file)` - Parse BMP header
- `bool loadToBuffer(FileType& file, uint8_t* buffer)` - Load image data
- `uint8_t* getLine(uint8_t* buffer, uint16_t lineNumber)` - Get line pointer
- `uint32_t getPixelColor(uint8_t* buffer, uint16_t x, uint16_t y)` - Get pixel color
- `int width()` - Get image width
- `int height()` - Get image height
- `uint32_t bufferSize()` - Get required buffer size
- `bool isValid()` - Check if header parsed successfully

### Example: Load and Display BMP

```cpp
#include <SD.h>
#include "BMPImageReader.h"

void setup() {
    Serial.begin(115200);
    
    // Initialize SD card
    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("SD card init failed!");
        return;
    }
    
    // Open BMP file
    BMPImageReader reader;
    File bmpFile = SD.open("image.bmp");
    
    if (!bmpFile) {
        Serial.println("Failed to open file!");
        return;
    }
    
    // Parse BMP header
    if (!reader.begin(bmpFile)) {
        Serial.println("Invalid BMP file!");
        bmpFile.close();
        return;
    }
    
    // Print image information
    Serial.print("Image size: ");
    Serial.print(reader.width());
    Serial.print(" x ");
    Serial.println(reader.height());
    
    // Allocate buffer for image data
    uint8_t* buffer = new uint8_t[reader.bufferSize()];
    
    // Load image data into buffer
    if (reader.loadToBuffer(bmpFile, buffer)) {
        Serial.println("Image loaded successfully!");
        
        // Access image line by line
        for (int y = 0; y < reader.height(); y++) {
            uint8_t* line = reader.getLine(buffer, y);
            
            // Each line contains RGB data: 3 bytes per pixel in BGR order
            for (int x = 0; x < reader.width(); x++) {
                uint8_t b = line[x * 3 + 0];
                uint8_t g = line[x * 3 + 1];
                uint8_t r = line[x * 3 + 2];
                
                // Display pixel on LED
                // leds[y] = CRGB(r, g, b);
            }
        }
    }
    
    delete[] buffer;
    bmpFile.close();
}
```

### Example: Access Individual Pixels

```cpp
#include <SD.h>
#include "BMPImageReader.h"

void displayPixel(int x, int y) {
    BMPImageReader reader;
    File bmpFile = SD.open("image.bmp");
    
    if (reader.begin(bmpFile)) {
        uint8_t* buffer = new uint8_t[reader.bufferSize()];
        
        if (reader.loadToBuffer(bmpFile, buffer)) {
            // Get specific pixel color
            uint32_t color = reader.getPixelColor(buffer, x, y);
            
            // Extract RGB components
            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8) & 0xFF;
            uint8_t b = color & 0xFF;
            
            Serial.print("Pixel (");
            Serial.print(x);
            Serial.print(", ");
            Serial.print(y);
            Serial.print("): R=");
            Serial.print(r);
            Serial.print(" G=");
            Serial.print(g);
            Serial.print(" B=");
            Serial.println(b);
        }
        
        delete[] buffer;
    }
    
    bmpFile.close();
}
```

## BMPImageSequence

### Features

- Load image playlists from text files
- Automatic duration management
- Support for up to 50 images per sequence
- Comment support in playlist files
- Auto-wrapping to first image

### API Reference

**Methods:**
- `int loadFromFile(FileType& file)` - Load sequence from file, returns count
- `bool addImage(const char* filename, uint16_t duration)` - Add image manually
- `const char* getCurrentFilename()` - Get current image filename
- `uint16_t getCurrentDuration()` - Get current image duration (seconds)
- `void next()` - Move to next image (wraps around)
- `void first()` - Reset to first image
- `int count()` - Get total number of images
- `bool isEmpty()` - Check if sequence is empty
- `const char* getFilename(int index)` - Get filename at index
- `uint16_t getDuration(int index)` - Get duration at index
- `void clear()` - Remove all images
- `void print()` - Debug print to Serial

### Example: Load and Display Sequence

```cpp
#include <SD.h>
#include "BMPImageSequence.h"
#include "BMPImageReader.h"

BMPImageSequence sequence;
unsigned long lastChangeTime = 0;

void setup() {
    Serial.begin(115200);
    
    // Initialize SD card
    if (!SD.begin(BUILTIN_SDCARD)) {
        Serial.println("SD card init failed!");
        return;
    }
    
    // Load sequence from file
    File listFile = SD.open("imagelist.txt");
    if (!listFile) {
        Serial.println("Failed to open imagelist.txt!");
        return;
    }
    
    int count = sequence.loadFromFile(listFile);
    listFile.close();
    
    Serial.print("Loaded ");
    Serial.print(count);
    Serial.println(" images");
    
    // Print sequence information
    sequence.print();
    
    lastChangeTime = millis();
}

void loop() {
    if (sequence.isEmpty()) {
        return;
    }
    
    // Get current image
    const char* filename = sequence.getCurrentFilename();
    uint16_t duration = sequence.getCurrentDuration();
    
    // Check if it's time to move to next image
    if (duration > 0 && millis() - lastChangeTime >= duration * 1000) {
        sequence.next();
        lastChangeTime = millis();
        
        Serial.print("Switching to: ");
        Serial.println(sequence.getCurrentFilename());
    }
    
    // Display current image
    displayImage(filename);
}

void displayImage(const char* filename) {
    BMPImageReader reader;
    File bmpFile = SD.open(filename);
    
    if (reader.begin(bmpFile)) {
        uint8_t* buffer = new uint8_t[reader.bufferSize()];
        
        if (reader.loadToBuffer(bmpFile, buffer)) {
            // Display image on LEDs
            for (int y = 0; y < reader.height() && y < NUM_LEDS; y++) {
                uint8_t* line = reader.getLine(buffer, y);
                // Process line...
            }
        }
        
        delete[] buffer;
    }
    
    bmpFile.close();
}
```

### Example: Manual Sequence Creation

```cpp
#include "BMPImageSequence.h"

BMPImageSequence sequence;

void setup() {
    // Add images manually
    sequence.addImage("logo.bmp", 20);
    sequence.addImage("pattern1.bmp", 15);
    sequence.addImage("pattern2.bmp", 10);
    sequence.addImage("text.bmp", 30);
    
    Serial.print("Sequence contains ");
    Serial.print(sequence.count());
    Serial.println(" images");
    
    // Navigate through sequence
    for (int i = 0; i < sequence.count(); i++) {
        Serial.print("Image ");
        Serial.print(i);
        Serial.print(": ");
        Serial.print(sequence.getFilename(i));
        Serial.print(" (");
        Serial.print(sequence.getDuration(i));
        Serial.println(" seconds)");
    }
}
```

## Image List File Format

Create an `imagelist.txt` file on your SD card with the following format:

```
# POV Image Sequence
# Format: filename.bmp duration_in_seconds
# Lines starting with # are comments

rainbow.bmp 20
fire.bmp 15
heart.bmp 10
logo.bmp 30
```

**Format Rules:**
- One image per line
- Filename followed by optional duration (seconds)
- Duration of 0 or omitted means manual control
- Lines starting with `#` are comments
- Empty lines are ignored
- Maximum 50 images per sequence
- Maximum 31 characters per filename

## Configuration Constants

You can customize these constants in your code:

```cpp
#define MAX_SEQUENCE_FILES 50          // Maximum images in sequence
#define MAX_SEQUENCE_FILENAME 31       // Maximum filename length
#define MAX_SEQUENCE_LINE_LENGTH 64    // Maximum line length in imagelist.txt
```

## BMP File Requirements

**Supported Format:**
- 24-bit uncompressed BMP (RGB, no compression)
- Any width and height (limited by available RAM)
- Bottom-up or top-down format

**For POV Displays:**
- Recommended height: 31 pixels (matches LED count)
- Width: As needed (typically 32-200 pixels)
- Use high contrast images for best visibility

**Not Supported:**
- Compressed BMP files
- Indexed color (8-bit) BMP
- 16-bit or 32-bit BMP formats

## Memory Considerations

Each image requires memory for:
- Image buffer: `width * height * 3` bytes (3 bytes per pixel)
- Example: 32x31 image = 2,976 bytes
- Example: 200x31 image = 18,600 bytes

The Teensy 4.1 has 1MB RAM, so memory is generally not a concern for typical POV images.

## SD Card Setup

### Arduino IDE Firmware

1. Uncomment `#define SD_SUPPORT` in `teensy_firmware.ino`
2. Insert SD card into Teensy 4.1's built-in SD slot
3. Format card as FAT32
4. Create directory `/poi_images/`
5. Copy BMP files to `/poi_images/`
6. Create `imagelist.txt` in root or `/poi_images/`

### PlatformIO Firmware

1. SD support is always enabled
2. Directory `/images/` is auto-created
3. Copy BMP files to `/images/`
4. Create `imagelist.txt` in `/images/`

## Error Handling

Both classes provide comprehensive error messages via Serial:

```cpp
BMPImageReader: file not open
BMPImageReader: not a BMP file
BMPImageReader: unsupported bit depth: 8
BMPImageReader: compressed BMP not supported
BMPImageReader: invalid number of planes
BMPImageReader: read error. Expected 2976 bytes, got 1024

BMPImageSequence: file not open
BMPImageSequence: sequence full
BMPImageSequence: filename too long
```

Always check return values:
```cpp
if (!reader.begin(file)) {
    Serial.println("Failed to parse BMP header");
    return;
}

if (!reader.loadToBuffer(file, buffer)) {
    Serial.println("Failed to load image data");
    return;
}
```

## Integration with FastLED

Example of displaying BMP image on APA102 LEDs:

```cpp
#include <FastLED.h>
#include <SD.h>
#include "BMPImageReader.h"

#define NUM_LEDS 32
#define DATA_PIN 11
#define CLOCK_PIN 13

CRGB leds[NUM_LEDS];

void displayBMP(const char* filename) {
    BMPImageReader reader;
    File bmpFile = SD.open(filename);
    
    if (reader.begin(bmpFile)) {
        uint8_t* buffer = new uint8_t[reader.bufferSize()];
        
        if (reader.loadToBuffer(bmpFile, buffer)) {
            // BMP is stored bottom-up, LED 0 is at bottom
            for (int y = 0; y < reader.height() && y < NUM_LEDS; y++) {
                uint8_t* line = reader.getLine(buffer, y);
                
                // Get first pixel of line (for single column POV)
                uint8_t b = line[0];
                uint8_t g = line[1];
                uint8_t r = line[2];
                
                // Note: LED 0 is level shifter, display starts at LED 1
                if (y + 1 < NUM_LEDS) {
                    leds[y + 1] = CRGB(r, g, b);
                }
            }
            
            FastLED.show();
        }
        
        delete[] buffer;
    }
    
    bmpFile.close();
}
```

## Troubleshooting

**"BMPImageReader: file not open"**
- Check SD card is inserted
- Verify SD.begin() was called successfully
- Ensure file exists on SD card

**"BMPImageReader: not a BMP file"**
- File may be corrupted
- File may be a different format (PNG, JPEG)
- Use image converter tools to create proper BMP

**"BMPImageReader: unsupported bit depth"**
- Only 24-bit BMP is supported
- Convert image using image editing software
- Export as "24-bit BMP" or "True Color BMP"

**"BMPImageSequence: sequence full"**
- Maximum 50 images per sequence
- Split into multiple sequences
- Or increase MAX_SEQUENCE_FILES constant

**Out of memory errors:**
- Reduce image size
- Display images one at a time
- Use streaming access with getLine()

## Performance Tips

1. **Use getLine() for streaming**: Access image line-by-line without loading entire image
2. **Cache image data**: Load once, display multiple times
3. **Optimize file access**: Keep files open during active use
4. **Use SDIO mode**: PlatformIO firmware uses high-speed SDIO (20-25 MB/s)
5. **Minimize seeks**: Sequential reads are faster than random access

## Examples Location

Additional examples can be found in:
- `teensy_firmware/teensy_firmware.ino` - Full firmware integration
- `examples/` - Image conversion tools and test files
- This documentation - Code snippets above

## See Also

- [SD Card Storage Guide](SD_CARD_STORAGE.md) - SD card setup details
- [Image Conversion Guide](IMAGE_CONVERSION.md) - Creating POV-compatible images
- [API Documentation](API.md) - REST API for image uploads
- [README.md](../README.md) - Main project documentation
