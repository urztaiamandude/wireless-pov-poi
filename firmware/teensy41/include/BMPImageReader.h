#ifndef _BMPIMAGEREADER_H
#define _BMPIMAGEREADER_H
#include <Arduino.h>

/*
 * Generic BMP Image Reader class
 * 
 * This class provides standalone BMP image processing functionality that can be
 * used with any File-compatible object (SD card, SPI Flash, etc.).
 * 
 * It is designed to work with:
 * - Teensy SD library (SDClass/File)
 * - Arduino SD library
 * - Adafruit SPI Flash with FatFileSystem
 * - Any other storage that provides a File-compatible interface
 * 
 * Usage example with Teensy SD card:
 * 
 *   #include <SD.h>
 *   #include "BMPImageReader.h"
 *   
 *   BMPImageReader reader;
 *   File bmpFile = SD.open("image.bmp");
 *   
 *   if (reader.begin(bmpFile)) {
 *       Serial.print("Image size: ");
 *       Serial.print(reader.width());
 *       Serial.print(" x ");
 *       Serial.println(reader.height());
 *       
 *       // Load image into buffer
 *       uint8_t* buffer = new uint8_t[reader.bufferSize()];
 *       if (reader.loadToBuffer(bmpFile, buffer)) {
 *           // Process image line by line
 *           for (int y = 0; y < reader.height(); y++) {
 *               uint8_t* line = reader.getLine(buffer, y);
 *               // line contains RGB data: 3 bytes per pixel in BGR order
 *           }
 *       }
 *       delete[] buffer;
 *   }
 *   bmpFile.close();
 */

class BMPImageReader {
public:
    BMPImageReader() : _width(0), _height(0), _rowSize(0), _imageOffset(0), _valid(false) {}
    
    // Parse BMP header from file. Returns true if successful.
    // File must be opened before calling this method.
    template<typename FileType>
    bool begin(FileType& file);
    
    // Load image data into provided buffer. Returns true if successful.
    // Buffer must be at least bufferSize() bytes.
    template<typename FileType>
    bool loadToBuffer(FileType& file, uint8_t* buffer);
    
    // Get a pointer to a specific line in the loaded buffer
    // Returns pointer to line data (3 bytes per pixel in BGR order)
    // Returns NULL if line number is invalid
    uint8_t* getLine(uint8_t* buffer, uint16_t lineNumber);
    
    // Get color of a specific pixel from loaded buffer
    // Returns 32-bit color in format 0x00RRGGBB
    // Returns 0 if coordinates are invalid
    uint32_t getPixelColor(uint8_t* buffer, uint16_t x, uint16_t y);
    
    // Image dimensions
    int width()  const { return _width; }
    int height() const { return _height; }
    
    // Size of each row in bytes (includes padding)
    int rowSize() const { return _rowSize; }
    
    // Total buffer size needed to load entire image
    uint32_t bufferSize() const { return (uint32_t)_rowSize * _height; }
    
    // Check if header was successfully parsed
    bool isValid() const { return _valid; }
    
private:
    int _width;
    int _height;
    int _rowSize;
    int _imageOffset;
    bool _valid;
    
    // Helper functions to read BMP data
    template<typename FileType>
    uint16_t read16(FileType& f);
    
    template<typename FileType>
    uint32_t read32(FileType& f);
};

// Template implementations must be in header file

template<typename FileType>
bool BMPImageReader::begin(FileType& file) {
    _valid = false;
    
    if (!file) {
        Serial.println(F("BMPImageReader: file not open"));
        return false;
    }
    
    // Seek to beginning
    file.seek(0);
    
    // Check BMP signature
    if (read16(file) != 0x4D42) {
        Serial.println(F("BMPImageReader: not a BMP file"));
        return false;
    }
    
    // Skip file size
    read32(file);
    // Skip creator bytes
    read32(file);
    // Get image data offset
    _imageOffset = read32(file);
    
    // Read DIB header
    read32(file); // header size
    _width = read32(file);
    _height = read32(file);
    
    // Check planes (must be 1)
    if (read16(file) != 1) {
        Serial.println(F("BMPImageReader: invalid number of planes"));
        return false;
    }
    
    // Check bit depth (must be 24)
    uint16_t bmpDepth = read16(file);
    if (bmpDepth != 24) {
        Serial.print(F("BMPImageReader: unsupported bit depth: "));
        Serial.println(bmpDepth);
        return false;
    }
    
    // Check compression (must be 0 = uncompressed)
    if (read32(file) != 0) {
        Serial.println(F("BMPImageReader: compressed BMP not supported"));
        return false;
    }
    
    // Handle negative height (top-down BMP)
    if (_height < 0) {
        _height = -_height;
    }
    
    // Calculate row size (rows are padded to 4-byte boundary)
    _rowSize = (_width * 3 + 3) & ~3;
    
    _valid = true;
    return true;
}

template<typename FileType>
bool BMPImageReader::loadToBuffer(FileType& file, uint8_t* buffer) {
    if (!_valid) {
        Serial.println(F("BMPImageReader: call begin() first"));
        return false;
    }
    
    if (!file) {
        Serial.println(F("BMPImageReader: file not open"));
        return false;
    }
    
    // Seek to image data
    file.seek(_imageOffset);
    
    // Read entire image data
    uint32_t size = bufferSize();
    uint32_t bytesRead = file.read(buffer, size);
    
    if (bytesRead != size) {
        Serial.print(F("BMPImageReader: read error. Expected "));
        Serial.print(size);
        Serial.print(F(" bytes, got "));
        Serial.println(bytesRead);
        return false;
    }
    
    return true;
}

template<typename FileType>
uint16_t BMPImageReader::read16(FileType& f) {
    uint16_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read(); // MSB
    return result;
}

template<typename FileType>
uint32_t BMPImageReader::read32(FileType& f) {
    uint32_t result;
    ((uint8_t *)&result)[0] = f.read(); // LSB
    ((uint8_t *)&result)[1] = f.read();
    ((uint8_t *)&result)[2] = f.read();
    ((uint8_t *)&result)[3] = f.read(); // MSB
    return result;
}

inline uint8_t* BMPImageReader::getLine(uint8_t* buffer, uint16_t lineNumber) {
    if (!_valid || lineNumber >= _height) {
        return NULL;
    }
    return buffer + (lineNumber * _rowSize);
}

inline uint32_t BMPImageReader::getPixelColor(uint8_t* buffer, uint16_t x, uint16_t y) {
    if (!_valid || x >= _width || y >= _height) {
        return 0;
    }
    
    uint32_t pos = y * _rowSize + x * 3;
    // BMP uses BGR order
    uint8_t b = buffer[pos];
    uint8_t g = buffer[pos + 1];
    uint8_t r = buffer[pos + 2];
    
    return (r << 16) | (g << 8) | b;
}

#endif // _BMPIMAGEREADER_H
