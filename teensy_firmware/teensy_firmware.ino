/*
 * Nebula Poi - Teensy 4.1 Firmware
 * 
 * This firmware controls a 32 LED APA102 strip for POV (Persistence of Vision) display.
 * By default, LED 0 is sacrificial (3.3V→5V level shifting) and LEDs 1-31 are the 31 display pixels.
 * The display range is runtime-configurable via the web UI without recompiling.
 * Communicates with ESP32 via Serial1 to receive images, patterns, and sequences.
 * 
 * Hardware:
 * - Teensy 4.1
 * - APA102 LED Strip (32 physical LEDs; default: LED 0 sacrificial, LEDs 1-31 display)
 * - MAX9814 Microphone Amplifier Module (for audio-reactive patterns)
 * - ESP32 connected via Serial1 (RX=0, TX=1)
 * - Optional: microSD card in Teensy 4.1 built-in slot (for SD_SUPPORT)
 * - Optional: 2x 8MB PSRAM chips for 16MB external RAM (for PSRAM support)
 */

#include <FastLED.h>
#include <EEPROM.h>

// Teensy 4.1 PSRAM support - declare external_psram_size if not already declared
#ifdef ARDUINO_TEENSY41
  #ifndef external_psram_size
    extern "C" uint32_t external_psram_size;
  #endif
#endif

// SD Card Support - Enabled: 64GB microSD card installed in Teensy 4.1 built-in slot
#define SD_SUPPORT

#ifdef SD_SUPPORT
  #include <SD.h>
  #include <SPI.h>
#endif

// LED Configuration
// By default, LED 0 is sacrificial (3.3V→5V level shifting); LEDs 1-31 are the 31 display pixels.
// The display range (g_displayLeds, g_displayLedStart) is runtime-configurable via the web UI
// (POST /api/hardware/leds on ESP32 → serial command 0x09 → EEPROM) without recompiling.
#define NUM_LEDS 32  // Total physical LEDs (compile-time max for array sizing)
static_assert(NUM_LEDS == 32, "NUM_LEDS must be 32 — includes sacrificial LED(s) for level shifting");
#define DATA_PIN 11
#define CLOCK_PIN 13
#define LED_TYPE APA102
#define COLOR_ORDER BGR
// Default LED display range — stored in EEPROM and loaded at runtime.
// Change via web UI (Advanced Settings → LED Hardware Configuration) without recompiling.
#define DEFAULT_NUM_LEDS 32         // Physical LEDs total
#define DEFAULT_SACRIFICIAL_LEDS 1  // LEDs 0..N-1 used only for level shifting (default: LED 0)
// g_displayLeds and g_displayLedStart are runtime variables initialized in setup().

// Audio Input Configuration (MAX9814 Microphone Amplifier Module)
// MAX9814 output connects through level shifter to Teensy analog input.
// Gain: no connect = 60dB, GND = 50dB, VDD = 40dB
// Output: biased at VDD/2 (~1.65V), swings +/- based on audio level
#define AUDIO_PIN A0         // Analog input from MAX9814 via level shifter
#define AUDIO_SAMPLES 64     // Samples for running average
#define AUDIO_NOISE_FLOOR 50 // Minimum threshold to filter ambient noise

// Communication
#define SERIAL_BAUD 115200
#define SERIAL_TX_PIN 0   // DO NOT CHANGE: ESP32-S3 serial link
#define SERIAL_RX_PIN 1   // DO NOT CHANGE: ESP32-S3 serial link
static_assert(SERIAL_TX_PIN == 0, "SERIAL_TX_PIN must remain 0 for ESP32-S3 serial link");
static_assert(SERIAL_RX_PIN == 1, "SERIAL_RX_PIN must remain 1 for ESP32-S3 serial link");
#define ESP32_SERIAL Serial1

// Display Configuration
// IMAGE_HEIGHT is the compile-time array dimension (= NUM_LEDS = max possible display height).
// g_displayLeds (runtime variable, loaded from EEPROM) controls how many rows are displayed.
#ifdef ARDUINO_TEENSY41
  // With 16MB PSRAM: 200 images at up to 32x400
  // Without PSRAM: 10 images at up to 32x200
  #define MAX_IMAGES 200
  #define IMAGE_MAX_WIDTH 400
#else
  #define MAX_IMAGES 10
  #define IMAGE_MAX_WIDTH 200
#endif
#define IMAGE_HEIGHT 32  // Compile-time array dimension (max = NUM_LEDS)
#define MAX_PATTERNS 19
#define MAX_SEQUENCES 5
const uint8_t kPatternSpeedDivisor = 20;

#ifdef SD_SUPPORT
  // SD Card Configuration - 64GB microSD installed
  #define SD_IMAGE_DIR "/poi_images"
  #define SD_PATTERN_DIR "/poi_patterns"
  #define MAX_FILENAME_LEN 32
  #define MAX_SD_FILES 100   // 64GB card can hold thousands; 100 files visible in UI
  #define MAX_FILEPATH_LEN 64
#endif

// LED Array
CRGB leds[NUM_LEDS];

// Runtime LED display configuration — loaded from EEPROM in setup()
uint8_t g_displayLeds = DEFAULT_NUM_LEDS - DEFAULT_SACRIFICIAL_LEDS;
uint8_t g_displayLedStart = DEFAULT_SACRIFICIAL_LEDS;
// Pattern state that must reset when LED config changes
uint8_t g_cometPos = DEFAULT_SACRIFICIAL_LEDS;
uint8_t g_wipePos  = DEFAULT_SACRIFICIAL_LEDS;

// Image storage structure
// Note: With PSRAM, pixels array can be much larger (IMAGE_MAX_WIDTH x IMAGE_HEIGHT)
struct POVImage {
  uint16_t width;   // Changed to uint16_t to support IMAGE_MAX_WIDTH up to 400
  uint16_t height;  // Changed to uint16_t for consistency
  CRGB pixels[IMAGE_MAX_WIDTH][IMAGE_HEIGHT];  // Max size based on PSRAM availability
  bool active;
};

// Pattern structure
// Pattern types (0-18):
//   Basic:  0=rainbow, 1=wave, 2=gradient, 3=sparkle, 4=fire, 5=comet
//           6=breathing, 7=strobe, 8=meteor, 9=wipe, 10=plasma
//   Audio (MAX9814): 11=VU meter, 12=pulse, 13=rainbow, 14=center burst, 15=sparkle
//   Extra:  16=split spin, 17=theater chase, 18=retro strobe (temporal color interleaving)
struct Pattern {
  uint8_t type;   // Pattern type (0-18), see types above
  CRGB color1;    // Primary color for pattern
  CRGB color2;    // Secondary color for pattern
  uint8_t speed;  // Animation speed (1-255): higher = faster animation
                  //   Typical: 20-40 slow, 50-80 medium, 100+ fast
                  //   For strobe: controls flash rate
                  //   For sparkle: controls sparkle density
  bool active;    // Whether this pattern slot is in use
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
// EXTMEM places these large arrays in external PSRAM (if installed)
// Without PSRAM, they will be in regular RAM (may cause issues if too large)
#ifdef ARDUINO_TEENSY41
EXTMEM POVImage images[MAX_IMAGES];
#else
POVImage images[MAX_IMAGES];
#endif
Pattern patterns[MAX_PATTERNS];
Sequence sequences[MAX_SEQUENCES];

// Display state
uint8_t currentMode = 0;  // 0=idle, 1=image, 2=pattern, 3=sequence, 4=live
uint8_t currentIndex = 0;
uint32_t lastUpdate = 0;
uint32_t frameDelay = 20;  // 50 FPS default
uint8_t currentColumn = 0;
bool displaying = false;

// Multi-poi sync time offset (in milliseconds)
// When synced with a peer, this offset adjusts pattern timing so both poi
// animate in phase. Positive means peer clock is ahead of ours.
int32_t syncTimeOffset = 0;

// Sequence state tracking
uint8_t currentSequenceItem = 0;
uint32_t sequenceStartTime = 0;
bool sequencePlaying = false;

// Next image upload slot (default 0; set via command 0x0B before command 0x02).
// Allows the ESP32 to store uploaded images in specific slots for multi-image sequences.
// Automatically resets to 0 after each image upload so the slot must be re-sent each time.
uint8_t g_nextImageSlot = 0;

// SD card initialization state
#ifdef SD_SUPPORT
bool sdInitialized = false;
#endif

// Live mode buffer
CRGB liveBuffer[NUM_LEDS];  // Sized at NUM_LEDS (max); g_displayLeds entries are used

// Retro Strobe (pattern type 18) — temporal color interleaving
// strobeMicros: duration of each phase in microseconds (default 300μs → ~3333 Hz show rate)
// Controls the apparent 'width' of bars and gaps when poi is spinning.
#define RETRO_STROBE_PATTERN_TYPE 18
uint16_t strobeMicros = 300;

// Serial command buffer
// Buffer size calculation for larger images:
//   Max image: IMAGE_MAX_WIDTH (400) × IMAGE_HEIGHT*2 (64, max accepted) × 3 (RGB) = 76,800 bytes
//   Plus protocol overhead (~100 bytes): 0xFF start, cmd, len, 0xFE end markers
//   Rounded up to 80,000 for safety margin
// With PSRAM (16MB installed): buffer placed in EXTMEM; without PSRAM: reduced buffer
#ifdef ARDUINO_TEENSY41
  #define CMD_BUFFER_SIZE 80000
  EXTMEM uint8_t cmdBuffer[CMD_BUFFER_SIZE];
#else
  #define CMD_BUFFER_SIZE 6400
  uint8_t cmdBuffer[CMD_BUFFER_SIZE];
#endif
uint32_t cmdBufferIndex = 0;

// ── LED Config EEPROM Persistence ─────────────────────────────────────────
// Layout: addr 0 = magic (0xA5), addr 1 = numLeds, addr 2 = sacrificialLeds
#define LED_CFG_EEPROM_ADDR 0
#define LED_CFG_MAGIC 0xA5

void loadLEDConfig() {
  if (EEPROM.read(LED_CFG_EEPROM_ADDR) == LED_CFG_MAGIC) {
    uint8_t n = EEPROM.read(LED_CFG_EEPROM_ADDR + 1);
    uint8_t s = EEPROM.read(LED_CFG_EEPROM_ADDR + 2);
    if (n >= 2 && n <= NUM_LEDS && s < n) {
      g_displayLedStart = s;
      g_displayLeds = n - s;
      Serial.printf("LED config (EEPROM): %u physical, %u sacrificial → %u display (LED %u–%u)\n",
                    n, s, g_displayLeds, g_displayLedStart,
                    g_displayLedStart + g_displayLeds - 1);
      return;
    }
  }
  // Defaults
  g_displayLeds    = DEFAULT_NUM_LEDS - DEFAULT_SACRIFICIAL_LEDS;
  g_displayLedStart = DEFAULT_SACRIFICIAL_LEDS;
  Serial.printf("LED config (default): %u display LEDs (LED %u–%u)\n",
                g_displayLeds, g_displayLedStart,
                g_displayLedStart + g_displayLeds - 1);
}

void saveLEDConfig(uint8_t numLeds, uint8_t sacrificial) {
  EEPROM.update(LED_CFG_EEPROM_ADDR,     LED_CFG_MAGIC);
  EEPROM.update(LED_CFG_EEPROM_ADDR + 1, numLeds);
  EEPROM.update(LED_CFG_EEPROM_ADDR + 2, sacrificial);
  Serial.printf("LED config saved: %u physical, %u sacrificial\n", numLeds, sacrificial);
}

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  Serial.println("Teensy 4.1 Nebula Poi Initializing...");
  
  // Check for PSRAM (Teensy 4.1 only)
  // EXTMEM arrays (images[], cmdBuffer[]) are placed in external PSRAM by the linker.
  // Accessing them without PSRAM physically installed causes a hard fault, so we halt
  // here with a clear diagnostic if PSRAM is not detected.
  #ifdef ARDUINO_TEENSY41
    uint32_t psram_size = external_psram_size;
    Serial.print("PSRAM detected: ");
    if (psram_size > 0) {
      Serial.print(psram_size / (1024*1024));
      Serial.println(" MB");
      Serial.print("Image capacity: ");
      Serial.print(MAX_IMAGES);
      Serial.print(" images at ");
      Serial.print(IMAGE_MAX_WIDTH);
      Serial.print("x");
      Serial.print(IMAGE_HEIGHT);
      Serial.println(" max");
      uint32_t extmem_used = (uint32_t)sizeof(images) + sizeof(cmdBuffer);
      Serial.print("EXTMEM usage: ~");
      Serial.print(extmem_used / (1024 * 1024));
      Serial.print(" MB of ");
      Serial.print(psram_size / (1024 * 1024));
      Serial.println(" MB PSRAM");
    } else {
      Serial.println("NONE");
      Serial.println("FATAL: This firmware requires PSRAM (2x 8MB chips on Teensy 4.1 upgrade pads).");
      Serial.println("The image array and command buffer are placed in external RAM (EXTMEM).");
      Serial.println("Accessing them without PSRAM installed will cause a hard fault.");
      Serial.println("See docs/PSRAM_INSTALLATION.md for installation instructions.");
      Serial.println("Halting.");
      while (true) {
        delay(1000);  // Halt in low-power manner — firmware cannot run safely without PSRAM
      }
    }
  #endif
  
  // Initialize ESP32 Serial
  ESP32_SERIAL.begin(SERIAL_BAUD);
  
  // Initialize FastLED
  loadLEDConfig();
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
  
  Serial.println("Teensy 4.1 Nebula Poi Ready!");
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
  
  // Retro Strobe (pattern 18) needs >1500 Hz FastLED.show() rate.
  // Bypass the normal frameDelay gate and use microsecond-precision timing.
  if (currentMode == 2 && currentIndex < MAX_PATTERNS &&
      patterns[currentIndex].active && patterns[currentIndex].type == RETRO_STROBE_PATTERN_TYPE) {
    displayRetroStrobe();
    return;
  }

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
  
  // Initialize with default patterns
  patterns[0].active = true;
  patterns[0].type = 0;  // Rainbow
  patterns[0].color1 = CRGB::Red;
  patterns[0].color2 = CRGB::Blue;
  patterns[0].speed = 50;
  
  patterns[1].active = true;
  patterns[1].type = 4;  // Fire
  patterns[1].color1 = CRGB::OrangeRed;
  patterns[1].color2 = CRGB::Yellow;
  patterns[1].speed = 120;
  
  patterns[2].active = true;
  patterns[2].type = 5;  // Comet
  patterns[2].color1 = CRGB::Cyan;
  patterns[2].color2 = CRGB::Blue;
  patterns[2].speed = 80;
  
  patterns[3].active = true;
  patterns[3].type = 6;  // Breathing
  patterns[3].color1 = CRGB::Purple;
  patterns[3].color2 = CRGB::Black;
  patterns[3].speed = 60;
  
  patterns[4].active = true;
  patterns[4].type = 10;  // Plasma
  patterns[4].color1 = CRGB::Green;
  patterns[4].color2 = CRGB::Magenta;
  patterns[4].speed = 40;

  patterns[5].active = true;
  patterns[5].type = 16;  // Split Spin
  patterns[5].color1 = CRGB::Blue;
  patterns[5].color2 = CRGB::Red;
  patterns[5].speed = 70;

  patterns[6].active = true;
  patterns[6].type = 17;  // Theater Chase
  patterns[6].color1 = CRGB::White;
  patterns[6].color2 = CRGB::Black;
  patterns[6].speed = 90;
  
  // Initialize with default demo images
  createDemoImages();
  
  // Initialize with default demo sequence
  createDemoSequence();
}

// Create default POV images
// These are real display-ready images sized for g_displayLeds LEDs.
// Each image is wider than tall (typical for POV), so when the poi
// spins it traces a detailed ring of light.
void createDemoImages() {
  const int W0 = 64;   // smiley width
  const int W1 = 100;  // rainbow width
  const int W2 = 64;   // heart width
  const int W3 = 80;   // starburst width
  const int W4 = 100;  // nebula spiral width
  const int H  = IMAGE_HEIGHT;  // 32

  // ── Image 0: Smiley Face (64×32) ──────────────────────────
  images[0].active = true;
  images[0].width  = W0;
  images[0].height = H;
  for (int x = 0; x < W0; x++)
    for (int y = 0; y < H; y++)
      images[0].pixels[x][y] = CRGB::Black;

  float cx0 = W0 / 2.0;
  float cy0 = H / 2.0;
  float r0  = min(W0, H) / 2.0 - 1.0;
  for (int x = 0; x < W0; x++) {
    for (int y = 0; y < H; y++) {
      float dx = x - cx0;
      float dy = y - cy0;
      float d  = sqrt(dx * dx + dy * dy);
      // Filled yellow circle
      if (d <= r0) {
        images[0].pixels[x][y] = CRGB(255, 200, 0);  // warm yellow
      }
      // Dark outline
      if (d > r0 - 1.2 && d <= r0) {
        images[0].pixels[x][y] = CRGB(180, 140, 0);
      }
    }
  }
  // Eyes (dark circles)
  float eyeR = 2.5;
  float leyX = cx0 - 7, leyY = cy0 - 4;
  float reyX = cx0 + 7, reyY = cy0 - 4;
  for (int x = 0; x < W0; x++) {
    for (int y = 0; y < H; y++) {
      float dl = sqrt((x - leyX) * (x - leyX) + (y - leyY) * (y - leyY));
      float dr = sqrt((x - reyX) * (x - reyX) + (y - reyY) * (y - reyY));
      if (dl <= eyeR || dr <= eyeR)
        images[0].pixels[x][y] = CRGB(60, 40, 0);
    }
  }
  // Smile arc
  for (int x = (int)(cx0 - 9); x <= (int)(cx0 + 9); x++) {
    if (x < 0 || x >= W0) continue;
    float t = (x - cx0) / 9.0;
    int y = (int)(cy0 + 4 + 4.0 * t * t);
    for (int dy = 0; dy <= 1; dy++) {
      if (y + dy >= 0 && y + dy < H)
        images[0].pixels[x][y + dy] = CRGB(60, 40, 0);
    }
  }
  Serial.println("Default image 0: Smiley Face (64x32)");

  // ── Image 1: Full Rainbow Spectrum (100×32) ──────────────
  images[1].active = true;
  images[1].width  = W1;
  images[1].height = H;
  for (int x = 0; x < W1; x++) {
    uint8_t hue = (uint8_t)(x * 255L / W1);
    for (int y = 0; y < H; y++) {
      // Smooth vertical brightness fade: full at centre, dimmer at edges
      uint8_t val = 255 - abs(y - (int)(H / 2.0)) * 8;
      if (val < 80) val = 80;
      images[1].pixels[x][y] = CHSV(hue, 240, val);
    }
  }
  Serial.println("Default image 1: Rainbow Spectrum (100x32)");

  // ── Image 2: Heart (64×32) ───────────────────────────────
  images[2].active = true;
  images[2].width  = W2;
  images[2].height = H;
  for (int x = 0; x < W2; x++)
    for (int y = 0; y < H; y++)
      images[2].pixels[x][y] = CRGB::Black;

  float cxH = W2 / 2.0;
  float cyH = H / 2.0;
  for (int x = 0; x < W2; x++) {
    for (int y = 0; y < H; y++) {
      // Parametric heart: map pixel to normalised coordinates
      float nx = (x - cxH) / 14.0;
      float ny = -(y - cyH + 2) / 14.0;  // flip y, shift centre up
      float eq = (nx * nx + ny * ny - 1.0);
      eq = eq * eq * eq - nx * nx * ny * ny * ny;
      if (eq <= 0.0) {
        // Distance from centre for gradient
        float d = sqrt(nx * nx + ny * ny);
        uint8_t r = 255;
        uint8_t g = (uint8_t)max(0.0, 40.0 - d * 40.0);
        uint8_t b = (uint8_t)max(0.0, 60.0 - d * 50.0);
        images[2].pixels[x][y] = CRGB(r, g, b);
      }
    }
  }
  Serial.println("Default image 2: Heart (64x32)");

  // ── Image 3: Starburst (80×32) ──────────────────────────
  images[3].active = true;
  images[3].width  = W3;
  images[3].height = H;
  float cx3 = W3 / 2.0;
  float cy3 = H / 2.0;
  for (int x = 0; x < W3; x++) {
    for (int y = 0; y < H; y++) {
      float dx = x - cx3;
      float dy = y - cy3;
      float angle = atan2(dy, dx);
      float dist  = sqrt(dx * dx + dy * dy);
      // 8 rays modulated by angle
      float ray = (sin(angle * 8.0) + 1.0) * 0.5;
      float brightness = ray * max(0.0, 1.0 - dist / 18.0);
      uint8_t hue = (uint8_t)((angle / 6.2832 + 0.5) * 255);
      uint8_t val = (uint8_t)(brightness * 255);
      images[3].pixels[x][y] = CHSV(hue, 200, val);
    }
  }
  Serial.println("Default image 3: Starburst (80x32)");

  // ── Image 4: Nebula Spiral (100×32) ─────────────────────
  images[4].active = true;
  images[4].width  = W4;
  images[4].height = H;
  float cx4 = W4 / 2.0;
  float cy4 = H / 2.0;
  for (int x = 0; x < W4; x++) {
    for (int y = 0; y < H; y++) {
      float dx = x - cx4;
      float dy = y - cy4;
      float angle = atan2(dy, dx);
      float dist  = sqrt(dx * dx + dy * dy);
      // Spiral arms
      float spiral = sin(angle * 3.0 - dist * 0.4);
      float glow   = max(0.0, 1.0 - dist / 20.0);
      float v = (spiral * 0.5 + 0.5) * glow;
      uint8_t hue = (uint8_t)(180 + angle * 20 + dist * 3);
      uint8_t sat = 180 + (uint8_t)(glow * 75);
      uint8_t val = (uint8_t)(v * 255);
      images[4].pixels[x][y] = CHSV(hue, sat, val);
    }
  }
  Serial.println("Default image 4: Nebula Spiral (100x32)");
}

// Create demo sequence
void createDemoSequence() {
  // Sequence 0: Cycle through all default images and patterns
  sequences[0].active = true;
  sequences[0].count = 7;
  sequences[0].loop = true;
  
  // Item 0: Smiley face image for 3 seconds
  sequences[0].items[0] = 0;  // Image 0
  sequences[0].durations[0] = 3000;
  
  // Item 1: Rainbow pattern for 2 seconds (bit 7 set = pattern)
  sequences[0].items[1] = 0x80 | 0;  // Pattern 0 (rainbow)
  sequences[0].durations[1] = 2000;
  
  // Item 2: Heart image for 3 seconds
  sequences[0].items[2] = 2;  // Image 2
  sequences[0].durations[2] = 3000;
  
  // Item 3: Fire pattern for 2 seconds (bit 7 set = pattern)
  sequences[0].items[3] = 0x80 | 1;  // Pattern 1 (fire)
  sequences[0].durations[3] = 2000;
  
  // Item 4: Starburst image for 3 seconds
  sequences[0].items[4] = 3;  // Image 3
  sequences[0].durations[4] = 3000;
  
  // Item 5: Rainbow spectrum image for 3 seconds
  sequences[0].items[5] = 1;  // Image 1
  sequences[0].durations[5] = 3000;
  
  // Item 6: Nebula spiral image for 3 seconds
  sequences[0].items[6] = 4;  // Image 4
  sequences[0].durations[6] = 3000;
  
  Serial.println("Default sequence created: Image/Pattern cycle");
}

void startupAnimation() {
  // Rainbow sweep startup animation (only active display LEDs)
  for (int hue = 0; hue < 256; hue += 4) {
    for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++) {
      leds[i] = CHSV(hue + ((i - g_displayLedStart) * 8), 255, 255);
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
      
    case 0x07:  // Set frame rate (uint16_t FPS, big-endian)
      if (dataLen >= 2) {
        // New 2-byte protocol: FPS as uint16_t big-endian
        uint16_t fps = ((uint16_t)cmdBuffer[3] << 8) | cmdBuffer[4];
        if (fps > 0) {
          frameDelay = 1000 / fps;
          if (frameDelay == 0) frameDelay = 1;  // Cap at 1ms minimum
        }
        Serial.print("Frame rate set to: ");
        Serial.print(fps);
        Serial.print(" FPS (delay=");
        Serial.print(frameDelay);
        Serial.println("ms)");
      } else if (dataLen == 1) {
        // Legacy 1-byte protocol: raw delay in ms (backward compat)
        frameDelay = cmdBuffer[3];
        Serial.print("Frame delay set to: ");
        Serial.println(frameDelay);
      }
      sendAck(cmd);
      break;

    case 0x08:  // Sync time offset (multi-poi phase alignment)
      if (dataLen >= 4) {
        syncTimeOffset = ((int32_t)cmdBuffer[3] << 24) |
                         ((int32_t)cmdBuffer[4] << 16) |
                         ((int32_t)cmdBuffer[5] << 8) |
                         (int32_t)cmdBuffer[6];
        Serial.print("Sync time offset set to: ");
        Serial.print(syncTimeOffset);
        Serial.println(" ms");
      }
      sendAck(cmd);
      break;

    case 0x09:  // Set LED config: [numLeds:1][sacrificialLeds:1]
      if (dataLen >= 2) {
        uint8_t newN = cmdBuffer[3];
        uint8_t newS = cmdBuffer[4];
        if (newN >= 2 && newN <= NUM_LEDS && newS < newN) {
          g_displayLedStart = newS;
          g_displayLeds     = newN - newS;
          g_cometPos = g_displayLedStart;
          g_wipePos  = g_displayLedStart;
          saveLEDConfig(newN, newS);
          FastLED.clear();
          FastLED.show();
          Serial.printf("LED config applied: %u display LEDs starting at LED %u\n",
                        g_displayLeds, g_displayLedStart);
        } else {
          Serial.println("LED config rejected: values out of range");
        }
      }
      sendAck(cmd);
      break;

    case 0x0A:  // Get LED config → [0xFF][0xBD][numLeds][sacrificialLeds][displayLeds][0xFE]
      {
        uint8_t numLeds = (uint8_t)(g_displayLedStart + g_displayLeds);
        ESP32_SERIAL.write(0xFF);
        ESP32_SERIAL.write(0xBD);
        ESP32_SERIAL.write(numLeds);
        ESP32_SERIAL.write(g_displayLedStart);  // = sacrificialLeds
        ESP32_SERIAL.write(g_displayLeds);
        ESP32_SERIAL.write(0xFE);
      }
      break;

    case 0x0B:  // Set next image upload slot: [slotIndex:1]
      // Allows the ESP32 to specify which image slot the next 0x02 command stores into.
      // The slot resets to 0 after each image upload.
      if (dataLen >= 1) {
        uint8_t reqSlot = cmdBuffer[3];
        if (reqSlot < MAX_IMAGES) {
          g_nextImageSlot = reqSlot;
          Serial.printf("Next image upload slot set to: %u\n", g_nextImageSlot);
        } else {
          Serial.println("Set slot rejected: index out of range");
        }
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
      
    case 0x21:  // List SD images
      listSDImages();
      break;
      
    case 0x22:  // Delete image from SD
      deleteSDImage();
      break;
      
    case 0x23:  // SD card info
      sendSDInfo();
      break;
      
    case 0x24:  // Load image from SD
      loadImageFromSD();
      break;
      
    case 0x30:  // Pattern preset commands (save/load/list/delete)
      handlePatternSDCommand();
      break;
    #endif
      
    default:
      Serial.println("Unknown command");
      break;
  }
}

void receiveImage() {
  // Parse image header from command buffer
  // Protocol: 0xFF 0x02 dataLen_high dataLen_low width_low width_high height_low height_high [RGB data...] 0xFE
  // Updated to support 16-bit width/height values
  uint16_t dataLen = (cmdBuffer[2] << 8) | cmdBuffer[3];
  uint16_t srcWidth = cmdBuffer[4] | (cmdBuffer[5] << 8);   // 16-bit width
  uint16_t srcHeight = cmdBuffer[6] | (cmdBuffer[7] << 8);  // 16-bit height
  
  // Use g_nextImageSlot (set by command 0x0B) then reset to 0 for the next upload.
  // This allows the ESP32 to direct images into specific slots for multi-image sequences.
  uint8_t imgIndex = g_nextImageSlot;
  g_nextImageSlot = 0;  // Reset so a forgotten 0x0B defaults back to slot 0
  
  // Calculate expected data size
  // Cast to uint32_t to prevent overflow: max is 400*64*3 = 76,800 bytes
  uint32_t expectedBytes = 8 + ((uint32_t)srcWidth * (uint32_t)srcHeight * 3) + 1; // header + pixels + end marker
  
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
  
  // Check if image fits within limits
  if (srcWidth > IMAGE_MAX_WIDTH || srcHeight > IMAGE_HEIGHT * 2) {
    Serial.print("Image too large. Max: ");
    Serial.print(IMAGE_MAX_WIDTH);
    Serial.print("x");
    Serial.println(IMAGE_HEIGHT * 2);
    Serial.println("Resizing to fit...");
    
    // Resize to fit within limits
    uint16_t targetWidth = srcWidth;
    uint16_t targetHeight = srcHeight;
    
    if (srcWidth > IMAGE_MAX_WIDTH) {
      targetWidth = IMAGE_MAX_WIDTH;
      targetHeight = (uint32_t)srcHeight * IMAGE_MAX_WIDTH / srcWidth;
    }
    if (targetHeight > IMAGE_HEIGHT * 2) {
      targetHeight = IMAGE_HEIGHT * 2;
      targetWidth = (uint32_t)srcWidth * (IMAGE_HEIGHT * 2) / srcHeight;
      if (targetWidth > IMAGE_MAX_WIDTH) targetWidth = IMAGE_MAX_WIDTH;
    }
    
    srcWidth = targetWidth;
    srcHeight = targetHeight;
  }
  
  // Store image with original dimensions (no forced resize to IMAGE_WIDTH)
  Serial.print("Storing image at: ");
  Serial.print(srcWidth);
  Serial.print("x");
  Serial.println(srcHeight);
  
  images[imgIndex].width = srcWidth;
  images[imgIndex].height = srcHeight;
  images[imgIndex].active = true;
  
  // Read pixel data directly
  uint32_t pixelCount = (uint32_t)srcWidth * srcHeight;
  for (uint32_t i = 0; i < pixelCount; i++) {
    uint32_t bufferPos = 8 + i * 3;  // 8-byte header
    // Ensure we have all 3 bytes for this pixel
    if (bufferPos + 2 < (uint32_t)(cmdBufferIndex - 1)) { // -1 for end marker
      uint16_t x = i % srcWidth;
      uint16_t y = i / srcWidth;
      if (x < IMAGE_MAX_WIDTH && y < IMAGE_HEIGHT * 2) {  // Safety bounds check
        images[imgIndex].pixels[x][y] = CRGB(
          cmdBuffer[bufferPos],
          cmdBuffer[bufferPos + 1],
          cmdBuffer[bufferPos + 2]
        );
      }
    } else {
      // Fill remaining with black if data is incomplete
      uint16_t x = i % srcWidth;
      uint16_t y = i / srcWidth;
      if (x < IMAGE_MAX_WIDTH && y < IMAGE_HEIGHT * 2) {
        images[imgIndex].pixels[x][y] = CRGB::Black;
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
  
  // Persist updated pattern set to SD so it survives power cycles
  #ifdef SD_SUPPORT
  if (sdInitialized) {
    savePatternPreset("default");
  }
  #endif
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
  // Receive live frame data for immediate display (g_displayLeds * 3 bytes RGB)
  for (int i = 0; i < g_displayLeds && (3 + (i + 1) * 3 - 1) < CMD_BUFFER_SIZE; i++) {
    liveBuffer[i] = CRGB(cmdBuffer[3 + i * 3], cmdBuffer[4 + i * 3], cmdBuffer[5 + i * 3]);
  }
}

void updateDisplay() {
  
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

  // Ensure sacrificial LEDs (indices 0..g_displayLedStart-1) stay black
  if (g_displayLedStart > 0) {
    for (int i = 0; i < g_displayLedStart; i++) {
      leds[i] = CRGB::Black;
    }
  }
  // Clear rows beyond the image height within the display range
  for (int i = 0; i < g_displayLeds; i++) {
    leds[i + g_displayLedStart] = (i < img.height) ? img.pixels[currentColumn][i] : CRGB::Black;
  }
  
  currentColumn = (currentColumn + 1) % img.width;
}

void displayPattern() {
  if (currentIndex >= MAX_PATTERNS || !patterns[currentIndex].active) {
    FastLED.clear();
    return;
  }

  Pattern& pat = patterns[currentIndex];
  // Use millis-based time with sync offset so paired poi animate in phase.
  // Dividing by frameDelay approximates the old frame-counter behavior
  // while being clock-aligned across devices.
  uint32_t patternTime = (uint32_t)((int32_t)millis() + syncTimeOffset) / max((uint32_t)1, frameDelay);
  
  switch (pat.type) {
    case 0:  // Rainbow
      for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++) {
        uint8_t hue = (patternTime * pat.speed / 10 + (i - g_displayLedStart) * 255 / g_displayLeds) % 256;
        leds[i] = CHSV(hue, 255, 255);
      }
      break;
      
    case 1:  // Wave
      for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++) {
        uint8_t brightness = (sin8(patternTime * pat.speed / 10 + (i - g_displayLedStart) * 255 / g_displayLeds));
        leds[i] = pat.color1;
        leds[i].nscale8(brightness);
      }
      break;
      
    case 2:  // Gradient - scrolling blend between two colors
      {
        uint32_t gradMillis = (uint32_t)((int32_t)millis() + syncTimeOffset);
        // Use 8-bit math so the phase wraps naturally and avoids 32-bit overflow
        uint8_t timeOffset = (uint8_t)((uint8_t)(gradMillis / 500u) * pat.speed);
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++) {
          // sin8 gives a smooth 0-255 wave that wraps naturally (no hard snap)
          uint8_t phase = (uint8_t)((i - g_displayLedStart) * 255 / g_displayLeds) + timeOffset;
          leds[i] = blend(pat.color1, pat.color2, sin8(phase));
        }
      }
      break;
      
    case 3:  // Sparkle
      if (random8() < pat.speed) {
        leds[random8(g_displayLedStart, g_displayLedStart + g_displayLeds)] = pat.color1;
      }
      fadeToBlackBy(leds + g_displayLedStart, g_displayLeds, 20);
      break;
      
    case 4:  // Fire - heat rises from bottom upward
      {
        static uint8_t heat[NUM_LEDS];
        const int displayEnd = g_displayLedStart + g_displayLeds;
        // Cool down every cell in display range
        for (int i = g_displayLedStart; i < displayEnd; i++) {
          heat[i] = qsub8(heat[i], random8(0, ((55 * 10) / g_displayLeds) + 2));
        }
        // Heat rises - drift heat upward within display range
        for (int i = displayEnd - 1; i >= g_displayLedStart + 2; i--) {
          heat[i] = (heat[i - 1] + heat[i - 2] + heat[i - 2]) / 3;
        }
        // Random ignition at the bottom (clamped to display range)
        if (random8() < pat.speed) {
          int y = random8(g_displayLedStart, min(g_displayLedStart + 3, displayEnd));
          heat[y] = qadd8(heat[y], random8(160, 255));
        }
        // Map heat to colors for display range only
        for (int i = g_displayLedStart; i < displayEnd; i++) {
          leds[i] = HeatColor(heat[i]);
        }
      }
      break;
      
    case 5:  // Comet - single bright head with fading tail
      {
        const int displayEnd = g_displayLedStart + g_displayLeds;
        static int8_t direction = 1;
        fadeToBlackBy(leds + g_displayLedStart, g_displayLeds, 60);  // Fade creates tail
        g_cometPos += direction;
        if (g_cometPos >= displayEnd - 1 || g_cometPos <= g_displayLedStart) {
          direction = -direction;
        }
        leds[g_cometPos] = pat.color1;
        int8_t tailPos = g_cometPos - direction;
        if (tailPos >= g_displayLedStart && tailPos < displayEnd) {
          leds[tailPos] = pat.color1;
          leds[tailPos].nscale8(128);
        }
      }
      break;
      
    case 6:  // Breathing - smooth pulse on/off
      {
        uint8_t breath = beatsin8(pat.speed / 4, 20, 255);
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++) {
          leds[i] = pat.color1;
          leds[i].nscale8(breath);
        }
      }
      break;
      
    case 7:  // Strobe - quick flashes using wall-clock time
      {
        static bool strobeOn = false;
        static uint32_t lastStrobeMs = 0;
        uint32_t strobeDelayMs = map(pat.speed, 1, 255, 500, 10);
        uint32_t nowMs = (uint32_t)((int32_t)millis() + syncTimeOffset);
        if (nowMs - lastStrobeMs >= strobeDelayMs) {
          strobeOn = !strobeOn;
          lastStrobeMs = nowMs;
        }
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++) {
          leds[i] = strobeOn ? pat.color1 : CRGB::Black;
        }
      }
      break;
      
    case 8:  // Meteor - falling with random decay
      {
        const int displayEnd = g_displayLedStart + g_displayLeds;
        // 255 is used as a sentinel for "not initialized yet"
        static uint8_t meteorPos = 255;
        if (meteorPos == 255 || meteorPos < g_displayLedStart || meteorPos >= displayEnd) {
          meteorPos = (uint8_t)(displayEnd - 1);
        }
        // Fade all display LEDs randomly for sparkly tail
        for (int i = g_displayLedStart; i < displayEnd; i++) {
          if (random8() < 80) {
            leds[i].fadeToBlackBy(64);
          }
        }
        // Draw meteor head
        for (int i = 0; i < 4; i++) {
          int16_t pos = (int16_t)meteorPos - i;
          if (pos >= g_displayLedStart && pos < displayEnd) {
            leds[pos] = pat.color1;
            leds[pos].nscale8(255 - (i * 60));
          }
        }
        if (meteorPos <= g_displayLedStart) {
          meteorPos = (uint8_t)(displayEnd - 1);
        } else {
          meteorPos--;
        }
      }
      break;
      
    case 9:  // Color Wipe - progressive fill then clear
      {
        const int displayEnd = g_displayLedStart + g_displayLeds;
        static bool filling = true;
        // Clamp g_wipePos in case the display range changed at runtime (via cmd 0x09)
        if (g_wipePos < g_displayLedStart || g_wipePos >= displayEnd) {
          g_wipePos = g_displayLedStart;
        }
        leds[g_wipePos] = filling ? pat.color1 : CRGB::Black;
        g_wipePos++;
        if (g_wipePos >= displayEnd) {
          g_wipePos = g_displayLedStart;
          filling = !filling;
        }
      }
      break;
      
    case 10:  // Plasma - organic color mixing
      for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++) {
        uint8_t idx = i - g_displayLedStart;
        uint8_t hue = sin8(idx * 10 + patternTime * pat.speed / 20) + 
                      sin8(idx * 15 - patternTime * pat.speed / 15) +
                      sin8(patternTime * pat.speed / 10);
        leds[i] = CHSV(hue, 255, 255);
      }
      break;
      
    case 11:  // Music Reactive - VU meter style with beat detection (MAX9814)
      {
        static uint16_t audioSamples[AUDIO_SAMPLES];
        static uint8_t sampleIndex = 0;
        static uint8_t peakLevel = 0;
        static uint8_t peakDecay = 0;
        static uint8_t beatHue = 0;
        
        // Read audio sample from MAX9814
        uint16_t rawSample = analogRead(AUDIO_PIN);
        audioSamples[sampleIndex] = rawSample;
        sampleIndex = (sampleIndex + 1) % AUDIO_SAMPLES;
        
        // Calculate average and peak
        uint32_t sum = 0;
        uint16_t maxVal = 0;
        for (int i = 0; i < AUDIO_SAMPLES; i++) {
          sum += audioSamples[i];
          if (audioSamples[i] > maxVal) maxVal = audioSamples[i];
        }
        uint16_t avg = sum / AUDIO_SAMPLES;
        
        // Calculate audio level (0-255)
        int16_t level = abs((int16_t)rawSample - (int16_t)avg);
        level = constrain(level - AUDIO_NOISE_FLOOR, 0, 512);
        uint8_t audioLevel = map(level, 0, 512, 0, 255);
        
        // Beat detection - sudden increase in level
        if (audioLevel > peakLevel + 30) {
          beatHue += 32;  // Shift color on beat
        }
        
        // Update peak with decay
        if (audioLevel > peakLevel) {
          peakLevel = audioLevel;
          peakDecay = 0;
        } else {
          peakDecay++;
          if (peakDecay > 5) {
            peakLevel = qsub8(peakLevel, 3);
          }
        }
        
        // Map audio level to number of LEDs to light
        uint8_t ledsToLight = map(audioLevel, 0, 255, 0, g_displayLeds);
        
        // Draw VU meter with color gradient (display LEDs)
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++) {
          uint8_t ledIndex = i - g_displayLedStart;
          if (ledIndex < ledsToLight) {
            // Gradient from green to yellow to red based on position
            uint8_t hue;
            if (ledIndex < g_displayLeds / 3) {
              hue = 96;  // Green
            } else if (ledIndex < 2 * g_displayLeds / 3) {
              hue = 64;  // Yellow
            } else {
              hue = 0;   // Red
            }
            // Add beat color shift
            hue = (hue + beatHue) % 256;
            leds[i] = CHSV(hue, 255, 255);
          } else {
            leds[i].fadeToBlackBy(50);  // Smooth fade
          }
        }
        
        // Draw peak indicator
        uint8_t peakPos = map(peakLevel, 0, 255, g_displayLedStart, g_displayLedStart + g_displayLeds - 1);
        if (peakPos >= g_displayLedStart && peakPos < g_displayLedStart + g_displayLeds) {
          leds[peakPos] = CRGB::White;
        }
      }
      break;
      
    case 12:  // Music Pulse - whole strip pulses with beat (MAX9814)
      {
        static uint16_t audioSamples[AUDIO_SAMPLES];
        static uint8_t sampleIndex = 0;
        static uint8_t pulseVal = 0;
        static uint8_t lastLevel = 0;
        
        // Read audio sample from MAX9814
        uint16_t rawSample = analogRead(AUDIO_PIN);
        audioSamples[sampleIndex] = rawSample;
        sampleIndex = (sampleIndex + 1) % AUDIO_SAMPLES;
        
        // Calculate average
        uint32_t sum = 0;
        for (int i = 0; i < AUDIO_SAMPLES; i++) sum += audioSamples[i];
        uint16_t avg = sum / AUDIO_SAMPLES;
        
        // Calculate audio level
        int16_t level = abs((int16_t)rawSample - (int16_t)avg);
        level = constrain(level - AUDIO_NOISE_FLOOR, 0, 512);
        uint8_t audioLevel = map(level, 0, 512, 0, 255);
        
        // Beat detection - pulse up on beat
        if (audioLevel > lastLevel + 20 && audioLevel > 100) {
          pulseVal = 255;
        }
        lastLevel = audioLevel;
        
        // Apply pulse to display LEDs
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++) {
          leds[i] = pat.color1;
          leds[i].nscale8(pulseVal);
        }
        
        // Decay pulse
        pulseVal = scale8(pulseVal, 220);
      }
      break;
      
    case 13:  // Music Rainbow - audio controls rainbow speed (MAX9814)
      {
        static uint16_t audioSamples[AUDIO_SAMPLES];
        static uint8_t sampleIndex = 0;
        static uint16_t rainbowOffset = 0;
        
        // Read audio sample from MAX9814
        uint16_t rawSample = analogRead(AUDIO_PIN);
        audioSamples[sampleIndex] = rawSample;
        sampleIndex = (sampleIndex + 1) % AUDIO_SAMPLES;
        
        // Calculate average
        uint32_t sum = 0;
        for (int i = 0; i < AUDIO_SAMPLES; i++) sum += audioSamples[i];
        uint16_t avg = sum / AUDIO_SAMPLES;
        
        // Calculate audio level
        int16_t level = abs((int16_t)rawSample - (int16_t)avg);
        level = constrain(level - AUDIO_NOISE_FLOOR, 0, 512);
        uint8_t audioLevel = map(level, 0, 512, 0, 255);
        
        // Audio level controls rainbow speed
        rainbowOffset += map(audioLevel, 0, 255, 1, 20);
        
        // Draw rainbow with audio-controlled speed (display LEDs)
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++) {
          uint8_t hue = (rainbowOffset / 4 + (i - g_displayLedStart) * 255 / g_displayLeds) % 256;
          uint8_t brightness = constrain(audioLevel + 50, 50, 255);
          leds[i] = CHSV(hue, 255, brightness);
        }
      }
      break;
      
    case 14:  // Music Center - expands from center based on audio (MAX9814)
      {
        static uint16_t audioSamples[AUDIO_SAMPLES];
        static uint8_t sampleIndex = 0;
        
        // Read audio sample from MAX9814
        uint16_t rawSample = analogRead(AUDIO_PIN);
        audioSamples[sampleIndex] = rawSample;
        sampleIndex = (sampleIndex + 1) % AUDIO_SAMPLES;
        
        // Calculate average
        uint32_t sum = 0;
        for (int i = 0; i < AUDIO_SAMPLES; i++) sum += audioSamples[i];
        uint16_t avg = sum / AUDIO_SAMPLES;
        
        // Calculate audio level
        int16_t level = abs((int16_t)rawSample - (int16_t)avg);
        level = constrain(level - AUDIO_NOISE_FLOOR, 0, 512);
        uint8_t audioLevel = map(level, 0, 512, 0, 255);
        
        // Map level to expansion from center
        uint8_t expansion = map(audioLevel, 0, 255, 0, g_displayLeds / 2);
        uint8_t center = g_displayLedStart + g_displayLeds / 2;
        
        // Fade all display LEDs first
        fadeToBlackBy(leds + g_displayLedStart, g_displayLeds, 80);
        
        // Draw expanding from center (display LEDs)
        for (int i = 0; i <= expansion; i++) {
          uint8_t hue = patternTime * pat.speed / 20 + i * 10;
          if (center + i < g_displayLedStart + g_displayLeds) leds[center + i] = CHSV(hue, 255, 255);
          if ((int16_t)center - i >= g_displayLedStart) leds[center - i] = CHSV(hue, 255, 255);
        }
      }
      break;
      
    case 15:  // Music Sparkle - sparkles intensity based on audio (MAX9814)
      {
        static uint16_t audioSamples[AUDIO_SAMPLES];
        static uint8_t sampleIndex = 0;
        
        // Read audio sample from MAX9814
        uint16_t rawSample = analogRead(AUDIO_PIN);
        audioSamples[sampleIndex] = rawSample;
        sampleIndex = (sampleIndex + 1) % AUDIO_SAMPLES;
        
        // Calculate average
        uint32_t sum = 0;
        for (int i = 0; i < AUDIO_SAMPLES; i++) sum += audioSamples[i];
        uint16_t avg = sum / AUDIO_SAMPLES;
        
        // Calculate audio level
        int16_t level = abs((int16_t)rawSample - (int16_t)avg);
        level = constrain(level - AUDIO_NOISE_FLOOR, 0, 512);
        uint8_t audioLevel = map(level, 0, 512, 0, 255);
        
        // Fade existing display LEDs
        fadeToBlackBy(leds + g_displayLedStart, g_displayLeds, 40);
        
        // Add sparkles based on audio level (display LEDs)
        uint8_t numSparkles = map(audioLevel, 0, 255, 0, 8);
        for (int s = 0; s < numSparkles; s++) {
          uint8_t pos = random8(g_displayLedStart, g_displayLedStart + g_displayLeds);
          uint8_t hue = patternTime * 2 + random8(64);  // Shifting colors
          leds[pos] = CHSV(hue, 255, 255);
        }
      }
      break;

    case 16:  // Split Spin - rotating two-color halves
      {
        uint8_t offset = (patternTime * pat.speed / kPatternSpeedDivisor) % g_displayLeds;
        uint8_t splitPoint = g_displayLeds / 2;
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++) {
          uint8_t pos = ((i - g_displayLedStart) + offset) % g_displayLeds;
          leds[i] = (pos < splitPoint) ? pat.color1 : pat.color2;
        }
      }
      break;

    case 17:  // Theater Chase - dotted chase with background
      {
        uint8_t chaseOffset = (patternTime * pat.speed / kPatternSpeedDivisor) % 3;
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++) {
          uint8_t phase = ((i - g_displayLedStart) + chaseOffset) % 3;
          leds[i] = (phase == 0) ? pat.color1 : pat.color2;
        }
      }
      break;
      
    default:
      FastLED.clear();
      break;
  }
}

// ── Retro Strobe (pattern type 18) — Temporal Color Interleaving ──────────
// High-speed state machine that cycles through individual color components
// separated by black gap frames. At rest the rapid flashing blends into a
// single perceived color (white for RGB mode, mixed for dual-color mode).
// When spun, POV reveals distinct solid bars of each component color.
//
// Sub-mode selection via the 'speed' byte:
//   speed >= 128  →  RGB (White) mode:  R → Black → G → Black → B → Black  (6 phases)
//   speed <  128  →  Dual-Color mode:   Color A → Black → Color B → Black   (4 phases)
  static uint8_t  strobePhase  = 0;
  static uint8_t  lastIndex    = 255;   // impossible value
  if (currentIndex != lastIndex) {
    strobePhase = 0;
    lastIndex = currentIndex;
  }
//   strobeMicros = (speed & 0x7F) * 5 + 100   (range 100–735 μs)
//   Default speed 168 (RGB) or 40 (Dual) → 300 μs → 3333 Hz show rate.
void displayRetroStrobe() {
  static uint32_t lastStrobeUs = 0;
  static uint8_t  strobePhase  = 0;
  static uint8_t  lastSpeed    = 0;   // cached to avoid re-decoding every call

  uint32_t now = micros();
  if (now - lastStrobeUs < strobeMicros) return;
  lastStrobeUs = now;

  Pattern& pat = patterns[currentIndex];

  // Only re-decode timing when the speed byte actually changes
  if (pat.speed != lastSpeed) {
    lastSpeed = pat.speed;
    strobeMicros = (uint16_t)(pat.speed & 0x7F) * 5 + 100;
    // Ensure sacrificial LEDs are black whenever config changes
    for (int i = 0; i < g_displayLedStart; i++) {
      leds[i] = CRGB::Black;
    }
  }

  bool rgbMode = (pat.speed & 0x80) != 0;

  if (rgbMode) {
    // RGB (White) mode — 6-phase cycle: R, Black, G, Black, B, Black
    switch (strobePhase) {
      case 0:  // Red
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++)
          leds[i] = CRGB(255, 0, 0);
        break;
      case 1:  // Black (gap)
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++)
          leds[i] = CRGB::Black;
        break;
      case 2:  // Green
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++)
          leds[i] = CRGB(0, 255, 0);
        break;
      case 3:  // Black (gap)
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++)
          leds[i] = CRGB::Black;
        break;
      case 4:  // Blue
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++)
          leds[i] = CRGB(0, 0, 255);
        break;
      case 5:  // Black (gap)
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++)
          leds[i] = CRGB::Black;
        break;
    }
    strobePhase = (strobePhase + 1) % 6;
  } else {
    // Dual-Color mode — 4-phase cycle: Color A, Black, Color B, Black
    switch (strobePhase) {
      case 0:  // Color A
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++)
          leds[i] = pat.color1;
        break;
      case 1:  // Black (gap)
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++)
          leds[i] = CRGB::Black;
        break;
      case 2:  // Color B
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++)
          leds[i] = pat.color2;
        break;
      case 3:  // Black (gap)
        for (int i = g_displayLedStart; i < g_displayLedStart + g_displayLeds; i++)
          leds[i] = CRGB::Black;
        break;
    }
    strobePhase = (strobePhase + 1) % 4;
  }

  FastLED.show();
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
  // Display the live buffer (g_displayLeds LEDs starting at g_displayLedStart)
  for (int i = 0; i < g_displayLeds; i++) {
    leds[i + g_displayLedStart] = liveBuffer[i];
  }
}

void sendAck(uint8_t cmd) {
  ESP32_SERIAL.write(0xFF);
  ESP32_SERIAL.write(0xAA);  // ACK
  ESP32_SERIAL.write(cmd);
  ESP32_SERIAL.write(0xFE);
}

void sendStatus() {
  // Response frame (6 bytes total):
  //   0xFF 0xBB mode index sd_present 0xFE
  // ESP32 checkTeensyConnection() must read and validate the trailing 0xFE.
  ESP32_SERIAL.write(0xFF);
  ESP32_SERIAL.write(0xBB);  // Status response
  ESP32_SERIAL.write(currentMode);
  ESP32_SERIAL.write(currentIndex);
  #ifdef SD_SUPPORT
  ESP32_SERIAL.write(sdInitialized ? (uint8_t)1 : (uint8_t)0);
  #else
  ESP32_SERIAL.write((uint8_t)0);
  #endif
  ESP32_SERIAL.write(0xFE);
}

// ==================== SD CARD FUNCTIONS ====================
#ifdef SD_SUPPORT

void autoLoadImagesFromSD();
void autoLoadPatternPreset();

void initSDCard() {
  Serial.print("Initializing SD card...");
  
  if (!SD.begin(BUILTIN_SDCARD)) {
    Serial.println("Failed!");
    Serial.println("Check that SD card is inserted");
    sdInitialized = false;
    return;
  }
  
  Serial.println("OK");
  sdInitialized = true;
  
  // Create image directory if it doesn't exist
  if (!SD.exists(SD_IMAGE_DIR)) {
    Serial.print("Creating directory: ");
    Serial.println(SD_IMAGE_DIR);
    SD.mkdir(SD_IMAGE_DIR);
  }
  
  // Create pattern directory if it doesn't exist
  if (!SD.exists(SD_PATTERN_DIR)) {
    Serial.print("Creating directory: ");
    Serial.println(SD_PATTERN_DIR);
    SD.mkdir(SD_PATTERN_DIR);
  }
  
  Serial.println("SD Card: Ready");
  
  // Auto-load saved images from SD into PSRAM slots (slots 5-MAX_IMAGES)
  autoLoadImagesFromSD();
  
  // Auto-load saved pattern preset from SD into RAM
  autoLoadPatternPreset();
}

// Auto-load all .pov image files from SD into PSRAM slots on boot.
// User upload slots start at 5 (0-4 are reserved for preloaded demo images).
void autoLoadImagesFromSD() {
  if (!sdInitialized) return;
  
  File dir = SD.open(SD_IMAGE_DIR);
  if (!dir) {
    Serial.println("autoLoadImages: cannot open image dir");
    return;
  }
  
  // Slots 0-4 reserved for demo images; user-uploaded files start at slot 5.
  uint8_t slot = 5;
  int loaded = 0;
  
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    
    String name = String(entry.name());
    entry.close();
    
    if (!name.endsWith(".pov")) continue;
    
    // SD library returns just the base filename (no path separators)
    String stem = name.substring(0, name.length() - 4);  // remove ".pov"
    
    if (stem.length() == 0 || stem.length() > MAX_FILENAME_LEN) continue;
    
    if (slot >= MAX_IMAGES) {
      Serial.println("autoLoadImages: all image slots full, remaining SD files skipped");
      break;
    }
    
    // Build full path
    char filepath[MAX_FILEPATH_LEN];
    snprintf(filepath, sizeof(filepath), "%s/%s.pov", SD_IMAGE_DIR, stem.c_str());
    
    File file = SD.open(filepath, FILE_READ);
    if (!file) continue;
    
    // Read 4-byte header: width (2 bytes little-endian) + height (2 bytes little-endian)
    int b0 = file.read(), b1 = file.read(), b2 = file.read(), b3 = file.read();
    if (b0 < 0 || b1 < 0 || b2 < 0 || b3 < 0) {
      file.close();
      Serial.print("autoLoadImages: skipping ");
      Serial.print(filepath);
      Serial.println(" (truncated header)");
      continue;
    }
    uint16_t width  = (uint16_t)b0 | ((uint16_t)b1 << 8);
    uint16_t height = (uint16_t)b2 | ((uint16_t)b3 << 8);
    
    if (width == 0 || width > IMAGE_MAX_WIDTH || height == 0 || height > IMAGE_HEIGHT) {
      file.close();
      Serial.print("autoLoadImages: skipping ");
      Serial.print(filepath);
      Serial.println(" (invalid dimensions)");
      continue;
    }
    
    // Validate that the file contains the full RGB payload: 4-byte header + width*height*3 bytes
    uint32_t expectedSize = 4UL + (uint32_t)width * (uint32_t)height * 3UL;
    if ((uint32_t)file.size() < expectedSize) {
      file.close();
      Serial.print("autoLoadImages: skipping ");
      Serial.print(filepath);
      Serial.println(" (truncated payload)");
      continue;
    }
    
    images[slot].width  = width;
    images[slot].height = height;
    
    bool readError = false;
    for (int x = 0; x < width && !readError; x++) {
      for (int y = 0; y < height; y++) {
        int r = file.read();
        int g = file.read();
        int b = file.read();
        if (r < 0 || g < 0 || b < 0) {
          readError = true;
          break;
        }
        images[slot].pixels[x][y].r = (uint8_t)r;
        images[slot].pixels[x][y].g = (uint8_t)g;
        images[slot].pixels[x][y].b = (uint8_t)b;
      }
    }
    
    file.close();
    
    if (readError) {
      Serial.print("autoLoadImages: skipping ");
      Serial.print(filepath);
      Serial.println(" (short read while loading pixels)");
      continue;
    }
    
    // Only mark slot active after a complete, successful read
    images[slot].active = true;
    
    Serial.print("autoLoadImages: loaded ");
    Serial.print(filepath);
    Serial.print(" -> slot ");
    Serial.println(slot);
    
    slot++;
    loaded++;
  }
  
  dir.close();
  
  Serial.print("autoLoadImages: ");
  Serial.print(loaded);
  Serial.println(" image(s) restored from SD");
}

// Auto-load the "default" pattern preset from SD on boot so patterns
// survive power cycles without requiring a manual load from the web UI.
void autoLoadPatternPreset() {
  if (!sdInitialized) return;
  
  char filepath[MAX_FILEPATH_LEN];
  snprintf(filepath, sizeof(filepath), "%s/default.pat", SD_PATTERN_DIR);
  
  if (!SD.exists(filepath)) {
    Serial.println("autoLoadPattern: no default.pat, skipping");
    return;
  }
  
  if (loadPatternPreset("default")) {
    Serial.println("autoLoadPattern: default preset restored from SD");
  } else {
    Serial.println("autoLoadPattern: failed to load default.pat");
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
  char filepath[MAX_FILEPATH_LEN];
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
  
  // Write header: width (2 bytes), height (2 bytes) for supporting larger images
  // Note: This changes the file format. Old files (1 byte width/height) won't be compatible.
  file.write((uint8_t)(img.width & 0xFF));        // Low byte
  file.write((uint8_t)((img.width >> 8) & 0xFF)); // High byte
  file.write((uint8_t)(img.height & 0xFF));       // Low byte
  file.write((uint8_t)((img.height >> 8) & 0xFF));// High byte
  
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
  // Protocol: 0xFF 0x24 dataLen [filenameLen] [filename] [imgIndex] 0xFE
  // Load image from SD card into the specified slot
  // cmdBuffer[3] = filenameLen, cmdBuffer[4..] = filename bytes
  
  uint8_t filenameLen = cmdBuffer[3];
  if (filenameLen == 0 || filenameLen > MAX_FILENAME_LEN) {
    Serial.println("Invalid filename length");
    sendAck(0x24);
    return;
  }
  
  // Extract filename from cmdBuffer[4] onwards
  char filename[MAX_FILENAME_LEN + 1];
  memcpy(filename, &cmdBuffer[4], filenameLen);
  filename[filenameLen] = '\0';
  
  // Image slot follows filename
  uint8_t imgIndex = cmdBuffer[4 + filenameLen];
  
  // Build full path
  char filepath[MAX_FILEPATH_LEN];
  snprintf(filepath, sizeof(filepath), "%s/%s.pov", SD_IMAGE_DIR, filename);
  
  Serial.print("Loading image from: ");
  Serial.println(filepath);
  
  // Open file for reading
  File file = SD.open(filepath, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file");
    sendAck(0x24);
    return;
  }
  
  // Read header: width (2 bytes), height (2 bytes)
  uint16_t width = file.read();           // Low byte
  width |= (file.read() << 8);            // High byte
  uint16_t height = file.read();          // Low byte
  height |= (file.read() << 8);           // High byte
  
  if (width > IMAGE_MAX_WIDTH || height > IMAGE_HEIGHT) {
    Serial.println("Image dimensions too large");
    file.close();
    sendAck(0x24);
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
  sendAck(0x24);
}

void listSDImages() {
  // Protocol: 0xFF 0x21 0 0xFE
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
  // Protocol: 0xFF 0x22 dataLen [filenameLen] [filename] 0xFE
  // cmdBuffer[3] = filenameLen, cmdBuffer[4..] = filename bytes
  
  uint8_t filenameLen = cmdBuffer[3];
  if (filenameLen == 0 || filenameLen > MAX_FILENAME_LEN) {
    Serial.println("Invalid filename length");
    sendAck(0x22);
    return;
  }
  
  // Extract filename from cmdBuffer[4] onwards
  char filename[MAX_FILENAME_LEN + 1];
  memcpy(filename, &cmdBuffer[4], filenameLen);
  filename[filenameLen] = '\0';
  
  // Build full path
  char filepath[MAX_FILEPATH_LEN];
  snprintf(filepath, sizeof(filepath), "%s/%s.pov", SD_IMAGE_DIR, filename);
  
  Serial.print("Deleting image: ");
  Serial.println(filepath);
  
  if (SD.remove(filepath)) {
    Serial.println("Image deleted successfully");
    sendAck(0x22);
  } else {
    Serial.println("Failed to delete image");
    sendAck(0x22);
  }
}

void sendSDInfo() {
  // Protocol: 0xFF 0x23 0 0xFE
  // Response: 0xFF 0xDD [present:1][totalSpace:8][freeSpace:8] 0xFE
  
  Serial.println("Sending SD card info...");
  
  ESP32_SERIAL.write(0xFF);
  ESP32_SERIAL.write(0xDD);  // SD info response marker
  
  // Get card info using Teensy SD library methods
  uint64_t totalSpace = SD.totalSize();
  uint64_t usedSpace = SD.usedSize();
  bool present = (totalSpace > 0);
  uint64_t freeSpace = present ? (totalSpace - usedSpace) : 0;
  
  // Present flag
  ESP32_SERIAL.write(present ? (uint8_t)1 : (uint8_t)0);
  
  // Total space (8 bytes, big-endian)
  for (int i = 7; i >= 0; i--) {
    ESP32_SERIAL.write((uint8_t)((totalSpace >> (i * 8)) & 0xFF));
  }
  
  // Free space (8 bytes, big-endian)
  for (int i = 7; i >= 0; i--) {
    ESP32_SERIAL.write((uint8_t)((freeSpace >> (i * 8)) & 0xFF));
  }
  
  ESP32_SERIAL.write(0xFE);
  
  Serial.print("SD Info: present=");
  Serial.print(present);
  Serial.print(" total=");
  Serial.print((uint32_t)(totalSpace / 1048576));
  Serial.print("MB free=");
  Serial.print((uint32_t)(freeSpace / 1048576));
  Serial.println("MB");
}

// ==================== PATTERN PRESET FUNCTIONS ====================
#define PATTERN_FILE_MAGIC 0x50415431  // "PAT1" in hex

void savePatternPreset(const char* presetName) {
  // Save all active patterns to a preset file
  
  // Create pattern directory if it doesn't exist
  if (!SD.exists(SD_PATTERN_DIR)) {
    SD.mkdir(SD_PATTERN_DIR);
  }
  
  // Build full path
  char filepath[MAX_FILEPATH_LEN];
  snprintf(filepath, sizeof(filepath), "%s/%s.pat", SD_PATTERN_DIR, presetName);
  
  Serial.print("Saving pattern preset to: ");
  Serial.println(filepath);
  
  // Open file for writing
  File file = SD.open(filepath, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to create pattern file");
    return;
  }
  
  // Write magic number
  uint32_t magic = PATTERN_FILE_MAGIC;
  file.write((uint8_t*)&magic, sizeof(magic));
  
  // Write number of patterns
  file.write(MAX_PATTERNS);
  
  // Write each pattern
  for (int i = 0; i < MAX_PATTERNS; i++) {
    Pattern& pat = patterns[i];
    file.write(pat.active ? 1 : 0);
    file.write(pat.type);
    file.write(pat.color1.r);
    file.write(pat.color1.g);
    file.write(pat.color1.b);
    file.write(pat.color2.r);
    file.write(pat.color2.g);
    file.write(pat.color2.b);
    file.write(pat.speed);
  }
  
  file.close();
  Serial.println("Pattern preset saved successfully");
}

bool loadPatternPreset(const char* presetName) {
  // Load patterns from a preset file
  
  // Build full path
  char filepath[MAX_FILEPATH_LEN];
  snprintf(filepath, sizeof(filepath), "%s/%s.pat", SD_PATTERN_DIR, presetName);
  
  Serial.print("Loading pattern preset from: ");
  Serial.println(filepath);
  
  // Open file for reading
  File file = SD.open(filepath, FILE_READ);
  if (!file) {
    Serial.println("Failed to open pattern file");
    return false;
  }
  
  // Validate minimum file size: magic(4) + patCount(1) = 5 bytes (0 patterns is valid)
  const uint32_t MIN_FILE_SIZE = 4 + 1;
  if ((uint32_t)file.size() < MIN_FILE_SIZE) {
    Serial.println("Pattern file too short");
    file.close();
    return false;
  }
  
  // Read and verify magic number
  uint32_t magic = 0;
  if (file.read((uint8_t*)&magic, sizeof(magic)) != (int)sizeof(magic)) {
    Serial.println("Failed to read pattern file magic");
    file.close();
    return false;
  }
  if (magic != PATTERN_FILE_MAGIC) {
    Serial.println("Invalid pattern file format");
    file.close();
    return false;
  }
  
  // Read number of patterns
  int patCountRaw = file.read();
  if (patCountRaw < 0) {
    Serial.println("Failed to read pattern count");
    file.close();
    return false;
  }
  uint8_t patCount = (uint8_t)patCountRaw;
  
  // Validate that the file holds all declared patterns (9 bytes each)
  uint32_t requiredSize = 4UL + 1UL + (uint32_t)patCount * 9UL;
  if ((uint32_t)file.size() < requiredSize) {
    Serial.println("Pattern file truncated");
    file.close();
    return false;
  }
  
  // Read each pattern (up to our MAX_PATTERNS)
  for (int i = 0; i < patCount && i < MAX_PATTERNS; i++) {
    Pattern& pat = patterns[i];
    int fActive = file.read(), fType = file.read();
    int fR1 = file.read(), fG1 = file.read(), fB1 = file.read();
    int fR2 = file.read(), fG2 = file.read(), fB2 = file.read();
    int fSpeed = file.read();
    if (fActive < 0 || fType < 0 || fR1 < 0 || fG1 < 0 || fB1 < 0 ||
        fR2 < 0 || fG2 < 0 || fB2 < 0 || fSpeed < 0) {
      Serial.println("Short read while loading pattern fields");
      file.close();
      return false;
    }
    pat.active  = (fActive != 0);
    pat.type    = (uint8_t)fType;
    pat.color1.r = (uint8_t)fR1;
    pat.color1.g = (uint8_t)fG1;
    pat.color1.b = (uint8_t)fB1;
    pat.color2.r = (uint8_t)fR2;
    pat.color2.g = (uint8_t)fG2;
    pat.color2.b = (uint8_t)fB2;
    pat.speed   = (uint8_t)fSpeed;
  }
  
  file.close();
  Serial.println("Pattern preset loaded successfully");
  return true;
}

void listPatternPresets() {
  // List all pattern preset files on SD card
  
  if (!SD.exists(SD_PATTERN_DIR)) {
    Serial.println("No pattern presets directory");
    ESP32_SERIAL.write(0xFF);
    ESP32_SERIAL.write(0xCD);
    ESP32_SERIAL.write((uint8_t)0);
    ESP32_SERIAL.write(0xFE);
    return;
  }
  
  File root = SD.open(SD_PATTERN_DIR);
  if (!root) {
    Serial.println("Failed to open pattern directory");
    return;
  }
  
  char filenames[MAX_SD_FILES][MAX_FILENAME_LEN];
  int count = 0;
  
  while (count < MAX_SD_FILES) {
    File entry = root.openNextFile();
    if (!entry) break;
    
    String name = entry.name();
    if (name.endsWith(".pat")) {
      // Remove extension and store
      name = name.substring(0, name.length() - 4);
      strncpy(filenames[count], name.c_str(), MAX_FILENAME_LEN - 1);
      filenames[count][MAX_FILENAME_LEN - 1] = '\0';
      count++;
      Serial.print("Found preset: ");
      Serial.println(filenames[count - 1]);
    }
    entry.close();
  }
  root.close();
  
  Serial.print("Total pattern presets: ");
  Serial.println(count);
  
  // Send list to ESP32 — framing: 0xFF 0xCD [count][nameLen][name...]...0xFE
  // (0xFF prefix so readTeensyResponse(0xCD,...) on the ESP32 can find it)
  ESP32_SERIAL.write(0xFF);
  ESP32_SERIAL.write(0xCD);  // Pattern list response
  ESP32_SERIAL.write(count);
  
  for (int i = 0; i < count; i++) {
    uint8_t nameLen = strlen(filenames[i]);
    ESP32_SERIAL.write(nameLen);
    ESP32_SERIAL.write(filenames[i], nameLen);
  }
  
  ESP32_SERIAL.write(0xFE);
}

void handlePatternSDCommand() {
  // Protocol for pattern commands:
  // Save: 0xFF 0x30 len 0x01 name_len [name] 0xFE
  // Load: 0xFF 0x30 len 0x02 name_len [name] 0xFE
  // List: 0xFF 0x30 len 0x03 0xFE
  // Delete: 0xFF 0x30 len 0x04 name_len [name] 0xFE
  
  uint8_t subCmd = cmdBuffer[3];
  
  switch (subCmd) {
    case 0x01: {  // Save
      uint8_t nameLen = cmdBuffer[4];
      if (nameLen > 0 && nameLen < MAX_FILENAME_LEN) {
        char name[MAX_FILENAME_LEN];
        memcpy(name, &cmdBuffer[5], nameLen);
        name[nameLen] = '\0';
        savePatternPreset(name);
      }
      sendAck(0x30);
      break;
    }
    case 0x02: {  // Load
      uint8_t nameLen = cmdBuffer[4];
      if (nameLen > 0 && nameLen < MAX_FILENAME_LEN) {
        char name[MAX_FILENAME_LEN];
        memcpy(name, &cmdBuffer[5], nameLen);
        name[nameLen] = '\0';
        loadPatternPreset(name);
      }
      sendAck(0x30);
      break;
    }
    case 0x03:  // List
      listPatternPresets();
      break;
    case 0x04: {  // Delete
      uint8_t nameLen = cmdBuffer[4];
      if (nameLen > 0 && nameLen < MAX_FILENAME_LEN) {
        char name[MAX_FILENAME_LEN];
        memcpy(name, &cmdBuffer[5], nameLen);
        name[nameLen] = '\0';
        char filepath[MAX_FILEPATH_LEN];
        snprintf(filepath, sizeof(filepath), "%s/%s.pat", SD_PATTERN_DIR, name);
        SD.remove(filepath);
        Serial.print("Deleted pattern preset: ");
        Serial.println(name);
      }
      sendAck(0x30);
      break;
    }
    default:
      Serial.println("Unknown pattern SD command");
      sendAck(0x30);
  }
}

#endif  // SD_SUPPORT
