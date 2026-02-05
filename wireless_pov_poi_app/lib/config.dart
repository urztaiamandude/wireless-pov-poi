class WirelessPOVConfig {
  // Hardware specifications
  static const int LED_COUNT = 31;  // APA102 strip length
  static const String LED_TYPE = "APA102";
  
  // Pattern constraints
  static const int MAX_PATTERN_WIDTH = 400;
  static const int MAX_PATTERN_HEIGHT = 31;
  static const int MAX_PATTERN_PIXELS = 40000; // width * height <= 40000
  
  // Pattern storage
  static const int PATTERN_BANKS = 3;  // Banks A, B, C
  static const int PATTERNS_PER_BANK = 5;
  static const int TOTAL_PATTERN_SLOTS = 15; // 3 banks × 5 patterns
  
  // APA102 specific
  static const int MIN_BRIGHTNESS = 0;
  static const int MAX_BRIGHTNESS = 31; // APA102 5-bit global brightness
  static const int DEFAULT_BRIGHTNESS = 15;
  
  // Speed settings (milliseconds between frames)
  static const int MIN_SPEED = 0;
  static const int MAX_SPEED = 2000;
  static const int DEFAULT_SPEED = 500;
  
  // Sequencer
  static const int MAX_SEQUENCER_SEGMENTS = 70;
  static const int MAX_SEGMENT_DURATION = 20000; // milliseconds
  
  // BLE settings
  static const String DEVICE_NAME_FILTER = "Wireless POV";
  static const int BLE_MTU = 512;
  static const int MAX_PACKET_SIZE = 509;
  
  // UI settings
  static const String APP_NAME = "Wireless POV Poi";
  static const String APP_VERSION = "1.0.0";
}
