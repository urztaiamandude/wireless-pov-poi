#include "sd_storage.h"
#include <cstring>

SDStorageManager::SDStorageManager() 
    : initialized(false), lastError(SD_OK) {
}

bool SDStorageManager::begin() {
    #if DEBUG_ENABLED
    DEBUG_SERIAL.println("Initializing SD card...");
    #endif
    
    // Try to initialize SD card with SDIO
    if (!sd.begin(SD_CONFIG)) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("SD card initialization failed!");
        if (sd.card()->errorCode()) {
            DEBUG_SERIAL.print("SD error code: 0x");
            DEBUG_SERIAL.print(sd.card()->errorCode(), HEX);
            DEBUG_SERIAL.print(", 0x");
            DEBUG_SERIAL.println(sd.card()->errorData(), HEX);
        }
        #endif
        lastError = SD_ERROR_CARD_NOT_PRESENT;
        initialized = false;
        return false;
    }
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.println("SD card initialized successfully");
    DEBUG_SERIAL.print("Card type: ");
    switch (sd.card()->type()) {
        case SD_CARD_TYPE_SD1:
            DEBUG_SERIAL.println("SD1");
            break;
        case SD_CARD_TYPE_SD2:
            DEBUG_SERIAL.println("SD2");
            break;
        case SD_CARD_TYPE_SDHC:
            DEBUG_SERIAL.println("SDHC");
            break;
        default:
            DEBUG_SERIAL.println("Unknown");
    }
    
    uint64_t cardSize = sd.card()->sectorCount() * 512ULL;
    DEBUG_SERIAL.print("Card size: ");
    DEBUG_SERIAL.print(cardSize / (1024ULL * 1024ULL));
    DEBUG_SERIAL.println(" MB");
    #endif
    
    // Ensure images directory exists
    if (!ensureImageDirectory()) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("Failed to create images directory");
        #endif
        lastError = SD_ERROR_FILE_OPEN_FAILED;
        initialized = false;
        return false;
    }
    
    initialized = true;
    lastError = SD_OK;
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.println("SD Storage Manager ready");
    #endif
    
    return true;
}

bool SDStorageManager::isCardPresent() {
    return initialized && sd.card() && sd.card()->errorCode() == 0;
}

SDError SDStorageManager::saveImage(const char* filename, const uint8_t* imageData, size_t width, size_t height) {
    if (!initialized) {
        lastError = SD_ERROR_NOT_INITIALIZED;
        return lastError;
    }
    
    if (!validateFilename(filename)) {
        lastError = SD_ERROR_INVALID_FILENAME;
        return lastError;
    }
    
    if (!imageData || width == 0 || height == 0) {
        lastError = SD_ERROR_INVALID_FORMAT;
        return lastError;
    }
    
    // Build full path
    char fullPath[128];
    buildImagePath(filename, fullPath, sizeof(fullPath));
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.print("Saving image to: ");
    DEBUG_SERIAL.println(fullPath);
    #endif
    
    // Open file for writing
    FsFile file;
    if (!file.open(fullPath, O_WRONLY | O_CREAT | O_TRUNC)) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("Failed to open file for writing");
        #endif
        lastError = SD_ERROR_FILE_OPEN_FAILED;
        return lastError;
    }
    
    // Create and write header
    POVImageHeader header;
    header.magic = SD_FILE_MAGIC;
    header.version = 1;
    header.width = width;
    header.height = height;
    header.dataSize = width * height * 3;
    header.reserved = 0;
    
    if (file.write(&header, sizeof(header)) != sizeof(header)) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("Failed to write header");
        #endif
        file.close();
        lastError = SD_ERROR_FILE_WRITE_FAILED;
        return lastError;
    }
    
    // Write image data in chunks for better performance
    const size_t chunkSize = 512;
    size_t totalBytes = width * height * 3;
    size_t bytesWritten = 0;
    
    while (bytesWritten < totalBytes) {
        size_t bytesToWrite = min(chunkSize, totalBytes - bytesWritten);
        size_t written = file.write(imageData + bytesWritten, bytesToWrite);
        
        if (written != bytesToWrite) {
            #if DEBUG_ENABLED
            DEBUG_SERIAL.println("Failed to write image data");
            #endif
            file.close();
            lastError = SD_ERROR_FILE_WRITE_FAILED;
            return lastError;
        }
        
        bytesWritten += written;
    }
    
    file.close();
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.print("Image saved successfully: ");
    DEBUG_SERIAL.print(width);
    DEBUG_SERIAL.print("x");
    DEBUG_SERIAL.print(height);
    DEBUG_SERIAL.print(" (");
    DEBUG_SERIAL.print(bytesWritten);
    DEBUG_SERIAL.println(" bytes)");
    #endif
    
    lastError = SD_OK;
    return SD_OK;
}

SDError SDStorageManager::loadImage(const char* filename, uint8_t* buffer, size_t maxBufferSize, size_t& width, size_t& height) {
    if (!initialized) {
        lastError = SD_ERROR_NOT_INITIALIZED;
        return lastError;
    }
    
    if (!validateFilename(filename)) {
        lastError = SD_ERROR_INVALID_FILENAME;
        return lastError;
    }
    
    if (!buffer || maxBufferSize == 0) {
        lastError = SD_ERROR_INVALID_FORMAT;
        return lastError;
    }
    
    // Build full path
    char fullPath[128];
    buildImagePath(filename, fullPath, sizeof(fullPath));
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.print("Loading image from: ");
    DEBUG_SERIAL.println(fullPath);
    #endif
    
    // Open file for reading
    FsFile file;
    if (!file.open(fullPath, O_RDONLY)) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("Failed to open file for reading");
        #endif
        lastError = SD_ERROR_FILE_NOT_FOUND;
        return lastError;
    }
    
    // Read and validate header
    POVImageHeader header;
    if (file.read(&header, sizeof(header)) != sizeof(header)) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("Failed to read header");
        #endif
        file.close();
        lastError = SD_ERROR_FILE_READ_FAILED;
        return lastError;
    }
    
    if (!validateHeader(header)) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("Invalid file header");
        #endif
        file.close();
        lastError = SD_ERROR_INVALID_FORMAT;
        return lastError;
    }
    
    // Check buffer size
    if (header.dataSize > maxBufferSize) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("Buffer too small for image data");
        #endif
        file.close();
        lastError = SD_ERROR_OUT_OF_MEMORY;
        return lastError;
    }
    
    // Read image data
    size_t bytesRead = file.read(buffer, header.dataSize);
    file.close();
    
    if (bytesRead != header.dataSize) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("Failed to read complete image data");
        #endif
        lastError = SD_ERROR_FILE_READ_FAILED;
        return lastError;
    }
    
    width = header.width;
    height = header.height;
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.print("Image loaded successfully: ");
    DEBUG_SERIAL.print(width);
    DEBUG_SERIAL.print("x");
    DEBUG_SERIAL.print(height);
    DEBUG_SERIAL.print(" (");
    DEBUG_SERIAL.print(bytesRead);
    DEBUG_SERIAL.println(" bytes)");
    #endif
    
    lastError = SD_OK;
    return SD_OK;
}

SDError SDStorageManager::deleteImage(const char* filename) {
    if (!initialized) {
        lastError = SD_ERROR_NOT_INITIALIZED;
        return lastError;
    }
    
    if (!validateFilename(filename)) {
        lastError = SD_ERROR_INVALID_FILENAME;
        return lastError;
    }
    
    // Build full path
    char fullPath[128];
    buildImagePath(filename, fullPath, sizeof(fullPath));
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.print("Deleting image: ");
    DEBUG_SERIAL.println(fullPath);
    #endif
    
    if (!sd.remove(fullPath)) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("Failed to delete file");
        #endif
        lastError = SD_ERROR_FILE_NOT_FOUND;
        return lastError;
    }
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.println("Image deleted successfully");
    #endif
    
    lastError = SD_OK;
    return SD_OK;
}

bool SDStorageManager::imageExists(const char* filename) {
    if (!initialized || !validateFilename(filename)) {
        return false;
    }
    
    char fullPath[128];
    buildImagePath(filename, fullPath, sizeof(fullPath));
    
    return sd.exists(fullPath);
}

int SDStorageManager::listImages(char filenames[][64], int maxFiles) {
    if (!initialized || maxFiles <= 0) {
        return 0;
    }
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.println("Listing images...");
    #endif
    
    // Open images directory
    FsFile dir;
    if (!dir.open(SD_IMAGE_DIR)) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.println("Failed to open images directory");
        #endif
        return 0;
    }
    
    int count = 0;
    FsFile entry;
    
    while (count < maxFiles && entry.openNext(&dir, O_RDONLY)) {
        // Skip directories
        if (!entry.isDirectory()) {
            char name[64];
            entry.getName(name, sizeof(name));
            
            // Check if it's a .pov file
            size_t len = strlen(name);
            if (len > 4 && strcmp(name + len - 4, ".pov") == 0) {
                strncpy(filenames[count], name, 63);
                filenames[count][63] = '\0';
                
                #if DEBUG_ENABLED
                DEBUG_SERIAL.print("Found: ");
                DEBUG_SERIAL.println(name);
                #endif
                
                count++;
            }
        }
        entry.close();
    }
    
    dir.close();
    
    #if DEBUG_ENABLED
    DEBUG_SERIAL.print("Found ");
    DEBUG_SERIAL.print(count);
    DEBUG_SERIAL.println(" images");
    #endif
    
    return count;
}

SDError SDStorageManager::getImageInfo(const char* filename, size_t& width, size_t& height, size_t& fileSize) {
    if (!initialized) {
        lastError = SD_ERROR_NOT_INITIALIZED;
        return lastError;
    }
    
    if (!validateFilename(filename)) {
        lastError = SD_ERROR_INVALID_FILENAME;
        return lastError;
    }
    
    // Build full path
    char fullPath[128];
    buildImagePath(filename, fullPath, sizeof(fullPath));
    
    // Open file
    FsFile file;
    if (!file.open(fullPath, O_RDONLY)) {
        lastError = SD_ERROR_FILE_NOT_FOUND;
        return lastError;
    }
    
    // Get file size
    fileSize = file.size();
    
    // Read header
    POVImageHeader header;
    if (file.read(&header, sizeof(header)) != sizeof(header)) {
        file.close();
        lastError = SD_ERROR_FILE_READ_FAILED;
        return lastError;
    }
    
    file.close();
    
    if (!validateHeader(header)) {
        lastError = SD_ERROR_INVALID_FORMAT;
        return lastError;
    }
    
    width = header.width;
    height = header.height;
    
    lastError = SD_OK;
    return SD_OK;
}

uint64_t SDStorageManager::getTotalSpace() {
    if (!initialized || !sd.card()) {
        return 0;
    }
    
    return sd.card()->sectorCount() * 512ULL;
}

uint64_t SDStorageManager::getFreeSpace() {
    if (!initialized) {
        return 0;
    }
    
    // Get free clusters
    uint32_t freeClusters = sd.freeClusterCount();
    uint32_t clusterSize = sd.sectorsPerCluster() * 512;
    
    return (uint64_t)freeClusters * clusterSize;
}

const char* SDStorageManager::getErrorString(SDError error) {
    switch (error) {
        case SD_OK: return "No error";
        case SD_ERROR_NOT_INITIALIZED: return "SD card not initialized";
        case SD_ERROR_CARD_NOT_PRESENT: return "SD card not present";
        case SD_ERROR_FILE_NOT_FOUND: return "File not found";
        case SD_ERROR_FILE_OPEN_FAILED: return "Failed to open file";
        case SD_ERROR_FILE_READ_FAILED: return "Failed to read file";
        case SD_ERROR_FILE_WRITE_FAILED: return "Failed to write file";
        case SD_ERROR_INVALID_FORMAT: return "Invalid file format";
        case SD_ERROR_OUT_OF_MEMORY: return "Out of memory";
        case SD_ERROR_DISK_FULL: return "Disk full";
        case SD_ERROR_INVALID_FILENAME: return "Invalid filename";
        default: return "Unknown error";
    }
}

// Private helper functions

bool SDStorageManager::ensureImageDirectory() {
    if (!sd.exists(SD_IMAGE_DIR)) {
        #if DEBUG_ENABLED
        DEBUG_SERIAL.print("Creating directory: ");
        DEBUG_SERIAL.println(SD_IMAGE_DIR);
        #endif
        
        if (!sd.mkdir(SD_IMAGE_DIR)) {
            return false;
        }
    }
    return true;
}

void SDStorageManager::buildImagePath(const char* filename, char* fullPath, size_t maxLen) {
    snprintf(fullPath, maxLen, "%s/%s", SD_IMAGE_DIR, filename);
}

bool SDStorageManager::validateFilename(const char* filename) {
    if (!filename || strlen(filename) == 0) {
        return false;
    }
    
    // Check for valid characters and length
    size_t len = strlen(filename);
    if (len > 60) {  // Leave room for directory path
        return false;
    }
    
    // Basic validation: no path separators
    for (size_t i = 0; i < len; i++) {
        char c = filename[i];
        if (c == '/' || c == '\\') {
            return false;
        }
    }
    
    return true;
}

bool SDStorageManager::validateHeader(const POVImageHeader& header) {
    // Check magic number
    if (header.magic != SD_FILE_MAGIC) {
        return false;
    }
    
    // Check version
    if (header.version != 1) {
        return false;
    }
    
    // Sanity check dimensions
    if (header.width == 0 || header.height == 0 || 
        header.width > 1024 || header.height > 1024) {
        return false;
    }
    
    // Check data size matches dimensions
    if (header.dataSize != header.width * header.height * 3) {
        return false;
    }
    
    return true;
}
