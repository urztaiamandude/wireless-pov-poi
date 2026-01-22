#ifndef SD_STORAGE_H
#define SD_STORAGE_H

#include <Arduino.h>
#include <SdFat.h>
#include "config.h"

// SD Card error codes
enum SDError {
    SD_OK = 0,
    SD_ERROR_NOT_INITIALIZED = 1,
    SD_ERROR_CARD_NOT_PRESENT = 2,
    SD_ERROR_FILE_NOT_FOUND = 3,
    SD_ERROR_FILE_OPEN_FAILED = 4,
    SD_ERROR_FILE_READ_FAILED = 5,
    SD_ERROR_FILE_WRITE_FAILED = 6,
    SD_ERROR_INVALID_FORMAT = 7,
    SD_ERROR_OUT_OF_MEMORY = 8,
    SD_ERROR_DISK_FULL = 9,
    SD_ERROR_INVALID_FILENAME = 10
};

// POV image file header structure
struct POVImageHeader {
    uint32_t magic;       // Magic number: 0x504F5631 ("POV1")
    uint32_t version;     // File format version (currently 1)
    uint16_t width;       // Image width in pixels
    uint16_t height;      // Image height in pixels
    uint32_t dataSize;    // Size of image data in bytes (width * height * 3)
    uint32_t reserved;    // Reserved for future use
};

class SDStorageManager {
public:
    SDStorageManager();
    
    // Initialize SD card and mount file system
    bool begin();
    
    // Check if SD card is initialized and ready
    bool isInitialized() const { return initialized; }
    
    // Image file operations
    SDError saveImage(const char* filename, const uint8_t* imageData, size_t width, size_t height);
    SDError loadImage(const char* filename, uint8_t* buffer, size_t maxBufferSize, size_t& width, size_t& height);
    SDError deleteImage(const char* filename);
    bool imageExists(const char* filename);
    
    // Directory operations
    int listImages(char filenames[][64], int maxFiles);
    SDError getImageInfo(const char* filename, size_t& width, size_t& height, size_t& fileSize);
    
    // Storage info
    uint64_t getTotalSpace();
    uint64_t getFreeSpace();
    bool isCardPresent();
    
    // Get last error
    SDError getLastError() const { return lastError; }
    const char* getErrorString(SDError error);

private:
    SdFat sd;
    bool initialized;
    SDError lastError;
    
    // Helper functions
    bool ensureImageDirectory();
    void buildImagePath(const char* filename, char* fullPath, size_t maxLen);
    bool validateFilename(const char* filename);
    bool validateHeader(const POVImageHeader& header);
};

#endif // SD_STORAGE_H
