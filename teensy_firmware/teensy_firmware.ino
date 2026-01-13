/*
 * Wireless POV POI - Teensy 4.1 Firmware
 * 
 * This firmware controls a 32 LED APA102 strip for POV (Persistence of Vision) display.
 * LED 0 is used for level shifting, LEDs 1-31 are used for display.
 * Communicates with ESP32 via Serial1 to receive images, patterns, and sequences.
 * 
 * Hardware:
 * - Teensy 4.1
 * - APA102 LED Strip (32 LEDs)
 * - ESP32 connected via Serial1 (RX1=0, TX1=1)
 * - Optional: microSD card in Teensy 4.1 built-in slot (for SD_SUPPORT)
 */

#include <FastLED.h>

// SD Card Support - Uncomment to enable SD card features
// Requires microSD card in Teensy 4.1's built-in SD card slot
//#define SD_SUPPORT

#ifdef SD_SUPPORT
  #include <SD.h>
  #include <SPI.h>
#endif

// LED Configuration
#define NUM_LEDS 32
#define DATA_PIN 11
#define CLOCK_PIN 13
#define LED_TYPE APA102
#define COLOR_ORDER BGR
#define DISPLAY_LEDS 31  // LEDs 1-31 used for display (LED 0 for level shifting)

// Communication
#define SERIAL_BAUD 115200
#define ESP32_SERIAL Serial1

// Display Configuration
#define MAX_IMAGES 10
#define IMAGE_WIDTH 31
#define IMAGE_HEIGHT 64
#define MAX_PATTERNS 5
#define MAX_SEQUENCES 5

#ifdef SD_SUPPORT
  // SD Card Configuration
  #define SD_IMAGE_DIR "/poi_images"
  #define MAX_FILENAME_LEN 32
  #define MAX_SD_FILES 10
#endif

// LED Array
CRGB leds[NUM_LEDS];

// Image storage structure
struct POVImage {
  uint8_t width;
  uint8_t height;
  CRGB pixels[IMAGE_WIDTH][IMAGE_HEIGHT];  // Max 31x64 image
  bool active;
};

// Pattern structure
struct Pattern {
  uint8_t type;  // 0=rainbow, 1=wave, 2=gradient, 3=sparkle, 4=custom
  CRGB color1;
  CRGB color2;
  uint8_t speed;
  bool active;
};

// Sequence structure
struct Sequence {
  uint8_t items[10];  // Image/pattern indices
  uint16_t durations[10];  // Duration in ms
  uint8_t count;
  bool active;
  bool loop;
};

// Storage arrays
POVImage images[MAX_IMAGES];
Pattern patterns[MAX_PATTERNS];
Sequence sequences[MAX_SEQUENCES];

// Display state
uint8_t currentMode = 0;  // 0=idle, 1=image, 2=pattern, 3=sequence, 4=live
uint8_t currentIndex = 0;
uint32_t lastUpdate = 0;
uint32_t frameDelay = 20;  // 50 FPS default
uint8_t currentColumn = 0;
bool displaying = false;

// Sequence state tracking
uint8_t currentSequenceItem = 0;
uint32_t sequenceStartTime = 0;
bool sequencePlaying = false;

// Live mode buffer
CRGB liveBuffer[DISPLAY_LEDS];

// Serial command buffer
#define CMD_BUFFER_SIZE 6144  // Large enough for 31x64 RGB image + protocol overhead
uint8_t cmdBuffer[CMD_BUFFER_SIZE];
uint16_t cmdBufferIndex = 0;

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  Serial.println("Teensy 4.1 POV POI Initializing...");
  
  // Initialize ESP32 Serial
  ESP32_SERIAL.begin(SERIAL_BAUD);
  
  // Initialize FastLED
  FastLED.addLeds<LED_TYPE, DATA_PIN, CLOCK_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(128);
  FastLED.clear();
  FastLED.show();
  
  // Initialize storage
  initStorage();
  
  // Initialize SD card (if enabled)
  #ifdef SD_SUPPORT
    initSDCard();
  #endif
  
  // Startup animation
  startupAnimation();
  
  Serial.println("Teensy 4.1 POV POI Ready!");
  Serial.println("Commands: IMAGE, PATTERN, SEQUENCE, LIVE, STATUS");
  #ifdef SD_SUPPORT
    Serial.println("SD Card support: ENABLED");
  #else
    Serial.println("SD Card support: DISABLED");
  #endif
}

void loop() {
  // Process serial commands from ESP32
  processSerialCommands();
  
  // Update display based on current mode
  if (millis() - lastUpdate >= frameDelay) {
    lastUpdate = millis();
    updateDisplay();
  }
}

void initStorage() {
  // Clear all storage
  for (int i = 0; i < MAX_IMAGES; i++) {
    images[i].active = false;
    images[i].width = 0;
    images[i].height = 0;
  }
  
  for (int i = 0; i < MAX_PATTERNS; i++) {
    patterns[i].active = false;
    patterns[i].type = 0;
    patterns[i].speed = 50;
  }
  
  for (int i = 0; i < MAX_SEQUENCES; i++) {
    sequences[i].active = false;
    sequences[i].count = 0;
    sequences[i].loop = false;
  }
  
  // Initialize with default pattern
  patterns[0].active = true;
  patterns[0].type = 0;  // Rainbow
  patterns[0].color1 = CRGB::Red;
  patterns[0].color2 = CRGB::Blue;
  patterns[0].speed = 50;
}

void startupAnimation() {
  // Rainbow sweep animation
  for (int hue = 0; hue < 256; hue += 4) {
    for (int i = 1; i < NUM_LEDS; i++) {
      leds[i] = CHSV(hue + (i * 8), 255, 255);
    }
    FastLED.show();
    delay(10);
  }
  FastLED.clear();
  FastLED.show();
}

void processSerialCommands() {
  while (ESP32_SERIAL.available()) {
    uint8_t byte = ESP32_SERIAL.read();
    
    // Start of command marker
    if (byte == 0xFF && cmdBufferIndex == 0) {
      cmdBuffer[cmdBufferIndex++] = byte;
    }
    // Continue building command
    else if (cmdBufferIndex > 0) {
      cmdBuffer[cmdBufferIndex++] = byte;
      
      // Prevent buffer overflow
      if (cmdBufferIndex >= CMD_BUFFER_SIZE) {
        Serial.println("WARNING: Command buffer overflow, resetting");
        cmdBufferIndex = 0;
        continue;
      }
      
      // Check if we have end marker (commands vary in length)
      if (byte == 0xFE && cmdBufferIndex >= 4) {
        parseCommand();
        cmdBufferIndex = 0;
      }
    }
  }
}

void parseCommand() {
  if (cmdBuffer[0] != 0xFF) return;
  
  uint8_t cmd = cmdBuffer[1];
  
  // Note: dataLen interpretation depends on command type
  // For image command (0x02), bytes 2-3 are 16-bit length
  // For other commands, byte 2 is 8-bit length
  uint16_t dataLen = cmdBuffer[2];  // Used for simple commands
  
  Serial.print("Command received: 0x");
  Serial.println(cmd, HEX);
  
  switch (cmd) {
    case 0x01:  // Set mode
      if (dataLen >= 2) {
        uint8_t newMode = cmdBuffer[3];
        uint8_t newIndex = cmdBuffer[4];
        
        // Reset sequence state when changing modes
        if (newMode != currentMode || newIndex != currentIndex) {
          sequencePlaying = false;
          currentSequenceItem = 0;
          sequenceStartTime = 0;
        }
        
        currentMode = newMode;
        currentIndex = newIndex;
        Serial.print("Mode set to: ");
        Serial.println(currentMode);
      }
      sendAck(cmd);
      break;
      
    case 0x02:  // Upload image (uses 16-bit length in bytes 2-3)
      receiveImage();
      sendAck(cmd);
      break;
      
    case 0x03:  // Upload pattern
      receivePattern();
      sendAck(cmd);
      break;
      
    case 0x04:  // Upload sequence
      receiveSequence();
      sendAck(cmd);
      break;
      
    case 0x05:  // Live frame data
      receiveLiveFrame();
      break;
      
    case 0x06:  // Set brightness
      if (dataLen >= 1) {
        FastLED.setBrightness(cmdBuffer[3]);
        Serial.print("Brightness set to: ");
        Serial.println(cmdBuffer[3]);
      }
      sendAck(cmd);
      break;
      
    case 0x07:  // Set frame rate
      if (dataLen >= 1) {
        frameDelay = cmdBuffer[3];
        Serial.print("Frame delay set to: ");
        Serial.println(frameDelay);
      }
      sendAck(cmd);
      break;
      
    case 0x10:  // Status request
      sendStatus();
      break;
      
    #ifdef SD_SUPPORT
    case 0x20:  // Save image to SD
      saveImageToSD();
      break;
      
    case 0x21:  // Load image from SD
      loadImageFromSD();
      break;
      
    case 0x22:  // List SD images
      listSDImages();
      break;
      
    case 0x23:  // Delete image from SD
      deleteSDImage();
      break;
    #endif
      
    default:
      Serial.println("Unknown command");
      break;
  }
}

void receiveImage() {
  // Parse image header from command buffer
  // Protocol: 0xFF 0x02 dataLen_high dataLen_low width height [RGB data...] 0xFE
  uint16_t dataLen = (cmdBuffer[2] << 8) | cmdBuffer[3];
  uint8_t srcWidth = cmdBuffer[4];
  uint8_t srcHeight = cmdBuffer[5];
  
  // Always store uploaded images in slot 0 (most recent upload)
  // This simplifies the web/app interface - they don't need to manage slots
  uint8_t imgIndex = 0;
  
  // Calculate expected data size
  uint16_t expectedBytes = 6 + srcWidth * srcHeight * 3 + 1; // header + pixels + end marker
  
  if (imgIndex >= MAX_IMAGES) {
    Serial.println("Error: Invalid image index");
    return;
  }
  
  if (cmdBufferIndex < expectedBytes) {
    Serial.print("Warning: Incomplete image data. Expected ");
    Serial.print(expectedBytes);
    Serial.print(", got ");
    Serial.println(cmdBufferIndex);
    // Continue anyway with what we have
  }
  
  Serial.print("Receiving image, source size: ");
  Serial.print(srcWidth);
  Serial.print("x");
  Serial.print(srcHeight);
  Serial.print(" (buffer has ");
  Serial.print(cmdBufferIndex);
  Serial.println(" bytes)");
  
  // If image is already 31 pixels wide, use it directly
  if (srcWidth == IMAGE_WIDTH && srcHeight <= 64) {
    Serial.println("Image is already POV-compatible size");
    images[imgIndex].width = srcWidth;
    images[imgIndex].height = srcHeight;
    images[imgIndex].active = true;
    
    // Read pixel data directly
    uint16_t pixelCount = srcWidth * srcHeight;
    for (uint16_t i = 0; i < pixelCount; i++) {
      uint16_t bufferPos = 6 + i * 3;
      // Ensure we have all 3 bytes for this pixel
      if (bufferPos + 2 < cmdBufferIndex - 1) { // -1 for end marker
        uint8_t x = i % srcWidth;
        uint8_t y = i / srcWidth;
        images[imgIndex].pixels[x][y] = CRGB(
          cmdBuffer[bufferPos],
          cmdBuffer[bufferPos + 1],
          cmdBuffer[bufferPos + 2]
        );
      } else {
        // Fill remaining with black if data is incomplete
        uint8_t x = i % srcWidth;
        uint8_t y = i / srcWidth;
        images[imgIndex].pixels[x][y] = CRGB::Black;
      }
    }
  } else {
    // Image needs conversion - resize to 31 pixels wide
    Serial.println("Converting image to POV format (31 pixels wide)");
    
    // Calculate target height maintaining aspect ratio
    uint8_t targetHeight = (uint16_t)srcHeight * IMAGE_WIDTH / srcWidth;
    if (targetHeight > 64) targetHeight = 64;
    if (targetHeight < 1) targetHeight = 1;
    
    Serial.print("Target size: ");
    Serial.print(IMAGE_WIDTH);
    Serial.print("x");
    Serial.println(targetHeight);
    
    images[imgIndex].width = IMAGE_WIDTH;
    images[imgIndex].height = targetHeight;
    images[imgIndex].active = true;
    
    // Perform nearest-neighbor resize
    for (uint8_t ty = 0; ty < targetHeight; ty++) {
      for (uint8_t tx = 0; tx < IMAGE_WIDTH; tx++) {
        // Map target pixel to source pixel
        uint8_t sx = (uint16_t)tx * srcWidth / IMAGE_WIDTH;
        uint8_t sy = (uint16_t)ty * srcHeight / targetHeight;
        
        // Get source pixel index
        uint16_t srcIndex = sy * srcWidth + sx;
        uint16_t bufferPos = 6 + srcIndex * 3;
        
        // Read and store pixel (with bounds checking)
        if (bufferPos + 2 < cmdBufferIndex - 1) { // -1 for end marker
          images[imgIndex].pixels[tx][ty] = CRGB(
            cmdBuffer[bufferPos],
            cmdBuffer[bufferPos + 1],
            cmdBuffer[bufferPos + 2]
          );
        } else {
          images[imgIndex].pixels[tx][ty] = CRGB::Black;
        }
      }
    }
  }
  
  Serial.println("Image received and processed successfully");
}

void receivePattern() {
  uint8_t patIndex = cmdBuffer[3];
  
  if (patIndex >= MAX_PATTERNS) return;
  
  patterns[patIndex].active = true;
  patterns[patIndex].type = cmdBuffer[4];
  patterns[patIndex].color1 = CRGB(cmdBuffer[5], cmdBuffer[6], cmdBuffer[7]);
  patterns[patIndex].color2 = CRGB(cmdBuffer[8], cmdBuffer[9], cmdBuffer[10]);
  patterns[patIndex].speed = cmdBuffer[11];
  
  Serial.print("Pattern ");
  Serial.print(patIndex);
  Serial.println(" received");
}

void receiveSequence() {
  uint8_t seqIndex = cmdBuffer[3];
  
  if (seqIndex >= MAX_SEQUENCES) return;
  
  sequences[seqIndex].active = true;
  sequences[seqIndex].count = cmdBuffer[4];
  sequences[seqIndex].loop = cmdBuffer[5];
  
  // Receive sequence items and durations
  for (int i = 0; i < sequences[seqIndex].count && i < 10; i++) {
    sequences[seqIndex].items[i] = cmdBuffer[6 + i * 3];
    sequences[seqIndex].durations[i] = (cmdBuffer[7 + i * 3] << 8) | cmdBuffer[8 + i * 3];
  }
  
  Serial.print("Sequence ");
  Serial.print(seqIndex);
  Serial.println(" received");
}

void receiveLiveFrame() {
  // Receive live frame data for immediate display
  for (int i = 0; i < DISPLAY_LEDS && (5 + i * 3) < CMD_BUFFER_SIZE; i++) {
    liveBuffer[i] = CRGB(cmdBuffer[3 + i * 3], cmdBuffer[4 + i * 3], cmdBuffer[5 + i * 3]);
  }
}

void updateDisplay() {
  // Clear LED 0 (level shifter)
  leds[0] = CRGB::Black;
  
  switch (currentMode) {
    case 0:  // Idle - off
      FastLED.clear();
      break;
      
    case 1:  // Display image
      displayImage();
      break;
      
    case 2:  // Display pattern
      displayPattern();
      break;
      
    case 3:  // Display sequence
      displaySequence();
      break;
      
    case 4:  // Live mode
      displayLive();
      break;
  }
  
  FastLED.show();
}

void displayImage() {
  if (currentIndex >= MAX_IMAGES || !images[currentIndex].active) {
    FastLED.clear();
    return;
  }
  
  POVImage& img = images[currentIndex];
  
  // Display current column of the image
  for (int i = 0; i < DISPLAY_LEDS && i < img.height; i++) {
    leds[i + 1] = img.pixels[currentColumn][i];
  }
  
  currentColumn = (currentColumn + 1) % img.width;
}

void displayPattern() {
  if (currentIndex >= MAX_PATTERNS || !patterns[currentIndex].active) {
    FastLED.clear();
    return;
  }
  
  Pattern& pat = patterns[currentIndex];
  static uint32_t patternTime = 0;
  patternTime++;
  
  switch (pat.type) {
    case 0:  // Rainbow
      for (int i = 1; i < NUM_LEDS; i++) {
        uint8_t hue = (patternTime * pat.speed / 10 + i * 255 / DISPLAY_LEDS) % 256;
        leds[i] = CHSV(hue, 255, 255);
      }
      break;
      
    case 1:  // Wave
      for (int i = 1; i < NUM_LEDS; i++) {
        uint8_t brightness = (sin8(patternTime * pat.speed / 10 + i * 255 / DISPLAY_LEDS));
        leds[i] = pat.color1;
        leds[i].nscale8(brightness);
      }
      break;
      
    case 2:  // Gradient
      for (int i = 1; i < NUM_LEDS; i++) {
        uint8_t blend = (i * 255) / DISPLAY_LEDS;
        leds[i] = blend8(pat.color1, pat.color2, blend);
      }
      break;
      
    case 3:  // Sparkle
      if (random8() < pat.speed) {
        leds[random8(1, NUM_LEDS)] = pat.color1;
      }
      fadeToBlackBy(leds, NUM_LEDS, 20);
      break;
      
    default:
      FastLED.clear();
      break;
  }
}

void displaySequence() {
  // Get current sequence
  if (currentIndex >= MAX_SEQUENCES || !sequences[currentIndex].active) {
    FastLED.clear();
    return;
  }
  
  Sequence& seq = sequences[currentIndex];
  
  // Check if sequence has items
  if (seq.count == 0) {
    FastLED.clear();
    return;
  }
  
  // Initialize sequence playback on first call or mode switch
  if (!sequencePlaying) {
    currentSequenceItem = 0;
    sequenceStartTime = millis();
    sequencePlaying = true;
    Serial.print("Starting sequence ");
    Serial.print(currentIndex);
    Serial.print(", items: ");
    Serial.println(seq.count);
  }
  
  // Check if current item duration has elapsed
  uint32_t elapsedTime = millis() - sequenceStartTime;
  if (elapsedTime >= seq.durations[currentSequenceItem]) {
    // Move to next item
    currentSequenceItem++;
    sequenceStartTime = millis();
    
    Serial.print("Sequence item ");
    Serial.print(currentSequenceItem);
    Serial.print(" of ");
    Serial.println(seq.count);
    
    // Check if sequence is complete
    if (currentSequenceItem >= seq.count) {
      if (seq.loop) {
        // Loop back to start
        currentSequenceItem = 0;
        Serial.println("Sequence looping...");
      } else {
        // Sequence complete, stop playing
        sequencePlaying = false;
        currentSequenceItem = seq.count - 1;  // Stay on last item
        Serial.println("Sequence complete");
      }
    }
  }
  
  // Get the current item index
  uint8_t itemIndex = seq.items[currentSequenceItem];
  
  // Determine if item is an image or pattern (MSB indicates type)
  // Bit 7: 0 = image, 1 = pattern
  bool isPattern = (itemIndex & 0x80) != 0;
  uint8_t actualIndex = itemIndex & 0x7F;  // Remove type bit
  
  // Display current item
  if (isPattern) {
    // Display pattern
    if (actualIndex < MAX_PATTERNS && patterns[actualIndex].active) {
      // Temporarily set currentIndex for pattern display
      uint8_t savedIndex = currentIndex;
      currentIndex = actualIndex;
      displayPattern();
      currentIndex = savedIndex;
    } else {
      FastLED.clear();
    }
  } else {
    // Display image
    if (actualIndex < MAX_IMAGES && images[actualIndex].active) {
      // Temporarily set currentIndex for image display
      uint8_t savedIndex = currentIndex;
      currentIndex = actualIndex;
      displayImage();
      currentIndex = savedIndex;
    } else {
      FastLED.clear();
    }
  }
}

void displayLive() {
  // Display the live buffer
  for (int i = 0; i < DISPLAY_LEDS; i++) {
    leds[i + 1] = liveBuffer[i];
  }
}

void sendAck(uint8_t cmd) {
  ESP32_SERIAL.write(0xFF);
  ESP32_SERIAL.write(0xAA);  // ACK
  ESP32_SERIAL.write(cmd);
  ESP32_SERIAL.write(0xFE);
}

void sendStatus() {
  ESP32_SERIAL.write(0xFF);
  ESP32_SERIAL.write(0xBB);  // Status response
  ESP32_SERIAL.write(currentMode);
  ESP32_SERIAL.write(currentIndex);
  ESP32_SERIAL.write(0xFE);
}

// ==================== SD CARD FUNCTIONS ====================
#ifdef SD_SUPPORT

void initSDCard() {
  Serial.print("Initializing SD card...");
  
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("Failed!");
    Serial.println("Check that SD card is inserted");
    return;
  }
  
  Serial.println("OK");
  
  // Create image directory if it doesn't exist
  if (!SD.exists(SD_IMAGE_DIR)) {
    Serial.print("Creating directory: ");
    Serial.println(SD_IMAGE_DIR);
    SD.mkdir(SD_IMAGE_DIR);
  }
  
  // Print card info (with error handling)
  Serial.print("SD Card Type: ");
  if (SD.card) {
    switch (SD.card.type()) {
      case 0: Serial.println("UNKNOWN"); break;
      case 1: Serial.println("SD1"); break;
      case 2: Serial.println("SD2"); break;
      case 3: Serial.println("SDHC/SDXC"); break;
      default: Serial.println("ERROR"); break;
    }
  } else {
    Serial.println("ERROR: Cannot read card type");
  }
}

void saveImageToSD() {
  // Protocol: 0xFF 0x20 len filename_len [filename] img_index 0xFE
  // Save the specified image slot to SD card with given filename
  
  uint8_t filenameLen = cmdBuffer[3];
  if (filenameLen == 0 || filenameLen > MAX_FILENAME_LEN) {
    Serial.println("Invalid filename length");
    sendAck(0x20);
    return;
  }
  
  // Extract filename
  char filename[MAX_FILENAME_LEN + 1];
  memcpy(filename, &cmdBuffer[4], filenameLen);
  filename[filenameLen] = '\0';
  
  // Get image index
  uint8_t imgIndex = cmdBuffer[4 + filenameLen];
  
  if (imgIndex >= MAX_IMAGES || !images[imgIndex].active) {
    Serial.println("Invalid image index");
    sendAck(0x20);
    return;
  }
  
  // Build full path
  char filepath[64];
  snprintf(filepath, sizeof(filepath), "%s/%s.pov", SD_IMAGE_DIR, filename);
  
  Serial.print("Saving image to: ");
  Serial.println(filepath);
  
  // Open file for writing
  File file = SD.open(filepath, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create file");
    sendAck(0x20);
    return;
  }
  
  POVImage& img = images[imgIndex];
  
  // Write header: width (1 byte), height (1 byte)
  file.write(img.width);
  file.write(img.height);
  
  // Write pixel data (RGB, row by row)
  for (int x = 0; x < img.width; x++) {
    for (int y = 0; y < img.height; y++) {
      file.write(img.pixels[x][y].r);
      file.write(img.pixels[x][y].g);
      file.write(img.pixels[x][y].b);
    }
  }
  
  file.close();
  Serial.println("Image saved successfully");
  sendAck(0x20);
}

void loadImageFromSD() {
  // Protocol: 0xFF 0x21 len filename_len [filename] img_index 0xFE
  // Load image from SD card into specified image slot
  
  uint8_t filenameLen = cmdBuffer[3];
  if (filenameLen == 0 || filenameLen > MAX_FILENAME_LEN) {
    Serial.println("Invalid filename length");
    sendAck(0x21);
    return;
  }
  
  // Extract filename
  char filename[MAX_FILENAME_LEN + 1];
  memcpy(filename, &cmdBuffer[4], filenameLen);
  filename[filenameLen] = '\0';
  
  // Get image index
  uint8_t imgIndex = cmdBuffer[4 + filenameLen];
  
  if (imgIndex >= MAX_IMAGES) {
    Serial.println("Invalid image index");
    sendAck(0x21);
    return;
  }
  
  // Build full path
  char filepath[64];
  snprintf(filepath, sizeof(filepath), "%s/%s.pov", SD_IMAGE_DIR, filename);
  
  Serial.print("Loading image from: ");
  Serial.println(filepath);
  
  // Open file for reading
  File file = SD.open(filepath, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file");
    sendAck(0x21);
    return;
  }
  
  // Read header
  uint8_t width = file.read();
  uint8_t height = file.read();
  
  if (width > IMAGE_WIDTH || height > IMAGE_HEIGHT) {
    Serial.println("Image dimensions too large");
    file.close();
    sendAck(0x21);
    return;
  }
  
  // Set image properties
  images[imgIndex].width = width;
  images[imgIndex].height = height;
  images[imgIndex].active = true;
  
  // Read pixel data
  for (int x = 0; x < width; x++) {
    for (int y = 0; y < height; y++) {
      images[imgIndex].pixels[x][y].r = file.read();
      images[imgIndex].pixels[x][y].g = file.read();
      images[imgIndex].pixels[x][y].b = file.read();
    }
  }
  
  file.close();
  Serial.print("Image loaded successfully (");
  Serial.print(width);
  Serial.print("x");
  Serial.print(height);
  Serial.println(")");
  sendAck(0x21);
}

void listSDImages() {
  // Protocol: 0xFF 0x22 len 0xFE
  // Response: 0xFF 0xCC count [name1_len name1 ...] 0xFE
  
  Serial.println("Listing SD images...");
  
  File dir = SD.open(SD_IMAGE_DIR);
  if (!dir) {
    Serial.println("Failed to open directory");
    // Send empty list
    ESP32_SERIAL.write(0xFF);
    ESP32_SERIAL.write(0xCC);  // List response
    ESP32_SERIAL.write(0);     // Count = 0
    ESP32_SERIAL.write(0xFE);
    return;
  }
  
  // Count .pov files
  uint8_t count = 0;
  char filenames[MAX_SD_FILES][MAX_FILENAME_LEN];  // Store up to MAX_SD_FILES filenames
  
  File entry;
  while ((entry = dir.openNextFile()) && count < MAX_SD_FILES) {
    if (!entry.isDirectory()) {
      const char* name = entry.name();
      // Check if file ends with .pov
      int len = strlen(name);
      if (len > 4 && strcmp(name + len - 4, ".pov") == 0) {
        // Copy filename without extension
        int nameLen = len - 4;
        if (nameLen > MAX_FILENAME_LEN - 1) nameLen = MAX_FILENAME_LEN - 1;
        strncpy(filenames[count], name, nameLen);
        filenames[count][nameLen] = '\0';
        count++;
      }
    }
    entry.close();
  }
  dir.close();
  
  Serial.print("Found ");
  Serial.print(count);
  Serial.println(" images");
  
  // Send response
  ESP32_SERIAL.write(0xFF);
  ESP32_SERIAL.write(0xCC);  // List response
  ESP32_SERIAL.write(count);
  
  for (int i = 0; i < count; i++) {
    uint8_t nameLen = strlen(filenames[i]);
    ESP32_SERIAL.write(nameLen);
    ESP32_SERIAL.write(filenames[i], nameLen);
  }
  
  ESP32_SERIAL.write(0xFE);
}

void deleteSDImage() {
  // Protocol: 0xFF 0x23 len filename_len [filename] 0xFE
  
  uint8_t filenameLen = cmdBuffer[3];
  if (filenameLen == 0 || filenameLen > MAX_FILENAME_LEN) {
    Serial.println("Invalid filename length");
    sendAck(0x23);
    return;
  }
  
  // Extract filename
  char filename[MAX_FILENAME_LEN + 1];
  memcpy(filename, &cmdBuffer[4], filenameLen);
  filename[filenameLen] = '\0';
  
  // Build full path
  char filepath[64];
  snprintf(filepath, sizeof(filepath), "%s/%s.pov", SD_IMAGE_DIR, filename);
  
  Serial.print("Deleting image: ");
  Serial.println(filepath);
  
  if (SD.remove(filepath)) {
    Serial.println("Image deleted successfully");
    sendAck(0x23);
  } else {
    Serial.println("Failed to delete image");
    sendAck(0x23);
  }
}

#endif  // SD_SUPPORT
