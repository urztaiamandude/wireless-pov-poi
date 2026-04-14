#ifndef _BMPIMAGESEQUENCE_H
#define _BMPIMAGESEQUENCE_H

#include <Arduino.h>

/*
 * BMPImageSequence - Manage sequences of BMP images with durations
 * 
 * This class works with BMPImageReader to create sequences of images
 * that can be displayed with specified durations. Perfect for POV displays
 * where you want to show multiple images in a sequence.
 * 
 * Format of imagelist file:
 *   filename1.bmp 20
 *   filename2.bmp 15
 *   filename3.bmp
 * 
 * Each line contains:
 *   - Image filename (required)
 *   - Duration in seconds (optional, 0 if not specified)
 * 
 * Usage example:
 * 
 *   #include <SD.h>
 *   #include "BMPImageSequence.h"
 *   
 *   BMPImageSequence sequence;
 *   
 *   // Load sequence from file
 *   File listFile = SD.open("imagelist.txt");
 *   int count = sequence.loadFromFile(listFile);
 *   listFile.close();
 *   
 *   // Access images in sequence
 *   const char* filename = sequence.getCurrentFilename();
 *   uint16_t duration = sequence.getCurrentDuration();
 *   
 *   // Move to next image
 *   sequence.next();
 */

#ifndef MAX_SEQUENCE_FILES
#define MAX_SEQUENCE_FILES 50
#endif

#ifndef MAX_SEQUENCE_FILENAME
#define MAX_SEQUENCE_FILENAME 31
#endif

#ifndef MAX_SEQUENCE_LINE_LENGTH
#define MAX_SEQUENCE_LINE_LENGTH 64
#endif

class BMPImageSequence {
public:
    BMPImageSequence() : _currentIndex(0), _numImages(0) {}
    
    // Load sequence from imagelist file
    // File format: one image per line, optional duration in seconds
    // Example: "image1.bmp 20" or just "image2.bmp"
    // Returns number of images loaded
    template<typename FileType>
    int loadFromFile(FileType& file);
    
    // Add a single image to sequence
    // Returns true if successfully added
    bool addImage(const char* filename, uint16_t durationSeconds = 0);
    
    // Get current image filename
    const char* getCurrentFilename() const;
    
    // Get current image duration in seconds (0 means no duration set)
    uint16_t getCurrentDuration() const;
    
    // Move to next image in sequence (wraps around to first)
    void next();
    
    // Move to first image in sequence
    void first();
    
    // Get total number of images in sequence
    int count() const { return _numImages; }
    
    // Check if sequence is empty
    bool isEmpty() const { return _numImages == 0; }
    
    // Get current image index (0-based)
    int getCurrentIndex() const { return _currentIndex; }
    
    // Get filename at specific index
    const char* getFilename(int index) const;
    
    // Get duration at specific index
    uint16_t getDuration(int index) const;
    
    // Clear all images from sequence
    void clear();
    
    // Print sequence info to Serial (for debugging)
    void print() const;

private:
    char _filenames[MAX_SEQUENCE_FILES][MAX_SEQUENCE_FILENAME];
    uint16_t _durations[MAX_SEQUENCE_FILES];
    int _currentIndex;
    int _numImages;
    
    // Helper function to read a line from file
    template<typename FileType>
    int readLine(FileType& f, char* buffer, int maxLength);
};

// Template implementations

template<typename FileType>
int BMPImageSequence::loadFromFile(FileType& file) {
    if (!file) {
        Serial.println(F("BMPImageSequence: file not open"));
        return 0;
    }
    
    char line[MAX_SEQUENCE_LINE_LENGTH + 1];
    int numLoaded = 0;
    
    // Read file line by line
    while (file.available() && _numImages < MAX_SEQUENCE_FILES) {
        int lineLength = readLine(file, line, MAX_SEQUENCE_LINE_LENGTH);
        
        if (lineLength == 0) continue; // Skip empty lines
        
        // Parse line: filename [duration]
        char* filename = strtok(line, " \r\t\n");
        char* durationStr = strtok(NULL, " \r\t\n");
        
        if (filename != NULL && filename[0] != '#') { // Skip comments starting with #
            uint16_t duration = 0;
            if (durationStr != NULL) {
                duration = atoi(durationStr);
            }
            
            if (addImage(filename, duration)) {
                numLoaded++;
            }
        }
    }
    
    if (numLoaded > 0) {
        _currentIndex = 0; // Start from first image
    }
    
    return numLoaded;
}

template<typename FileType>
int BMPImageSequence::readLine(FileType& f, char* buffer, int maxLength) {
    int pos = 0;
    
    while (f.available() && pos < maxLength) {
        char c = f.read();
        
        if (c == '\n') {
            buffer[pos] = '\0';
            return pos;
        }
        
        if (c != '\r') { // Skip carriage returns
            buffer[pos++] = c;
        }
    }
    
    buffer[pos] = '\0';
    return pos;
}

inline bool BMPImageSequence::addImage(const char* filename, uint16_t durationSeconds) {
    if (_numImages >= MAX_SEQUENCE_FILES) {
        Serial.println(F("BMPImageSequence: sequence full"));
        return false;
    }
    
    if (strlen(filename) >= MAX_SEQUENCE_FILENAME) {
        Serial.println(F("BMPImageSequence: filename too long"));
        return false;
    }
    
    strcpy(_filenames[_numImages], filename);
    _durations[_numImages] = durationSeconds;
    _numImages++;
    
    return true;
}

inline const char* BMPImageSequence::getCurrentFilename() const {
    if (_numImages == 0) return NULL;
    return _filenames[_currentIndex];
}

inline uint16_t BMPImageSequence::getCurrentDuration() const {
    if (_numImages == 0) return 0;
    return _durations[_currentIndex];
}

inline void BMPImageSequence::next() {
    if (_numImages == 0) return;
    _currentIndex = (_currentIndex + 1) % _numImages;
}

inline void BMPImageSequence::first() {
    _currentIndex = 0;
}

inline const char* BMPImageSequence::getFilename(int index) const {
    if (index < 0 || index >= _numImages) return NULL;
    return _filenames[index];
}

inline uint16_t BMPImageSequence::getDuration(int index) const {
    if (index < 0 || index >= _numImages) return 0;
    return _durations[index];
}

inline void BMPImageSequence::clear() {
    _numImages = 0;
    _currentIndex = 0;
}

inline void BMPImageSequence::print() const {
    Serial.print(F("Image sequence: "));
    Serial.print(_numImages);
    Serial.println(F(" images"));
    
    for (int i = 0; i < _numImages; i++) {
        Serial.print(F("  ["));
        Serial.print(i);
        Serial.print(F("] "));
        Serial.print(_filenames[i]);
        Serial.print(F(" - "));
        Serial.print(_durations[i]);
        Serial.println(F(" seconds"));
    }
}

#endif // _BMPIMAGESEQUENCE_H
