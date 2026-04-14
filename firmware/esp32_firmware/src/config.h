/*
 * Configuration for ESP32 Wireless POV Poi
 * 
 * Central configuration file for WiFi, BLE, and communication settings.
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// WiFi Configuration
// ============================================================================

// WiFi Access Point Settings
#define WIFI_SSID "POV-POI-WiFi"
#define WIFI_PASSWORD "povpoi123"
#define WIFI_CHANNEL 1

// mDNS Settings
#define MDNS_ENABLED true
#define MDNS_SERVICE_NAME "povpoi"

// ============================================================================
// BLE Configuration
// ============================================================================

// Enable/Disable BLE Support
// Set to true to enable BLE bridge functionality
// Set to false to disable BLE and save memory
#ifndef BLE_ENABLED
#define BLE_ENABLED true
#endif

// BLE Device Name (appears during scanning)
#define BLE_DEVICE_NAME "Wireless POV Poi"

// BLE Maximum Packet Size
// Standard BLE MTU is 23 bytes, but many devices support up to 512 bytes.
// We use 509 to account for 3 bytes of protocol overhead (0xD0, cmd, 0xD1).
// This ensures we stay within the 512-byte limit while maximizing data payload.
#define BLE_MAX_PACKET_SIZE 509

// BLE Command Protocol Markers
#define BLE_CMD_START 0xD0
#define BLE_CMD_END   0xD1

// ============================================================================
// Serial Communication (ESP32 ↔ Teensy)
// ============================================================================

// Serial Port Configuration
#define TEENSY_SERIAL Serial2
#define SERIAL_BAUD 115200

// For ESP32-S3 (USB CDC enabled)
#define TEENSY_RX_PIN 44  // GPIO 44 (U0RXD)
#define TEENSY_TX_PIN 43  // GPIO 43 (U0TXD)

// For standard ESP32
// #define TEENSY_RX_PIN 16  // GPIO 16
// #define TEENSY_TX_PIN 17  // GPIO 17

// Internal Protocol Markers (ESP32 ↔ Teensy)
#define INTERNAL_CMD_START 0xFF
#define INTERNAL_CMD_END   0xFE

// ============================================================================
// Sync Configuration (Peer-to-Peer)
// ============================================================================

#define AUTO_SYNC_ENABLED false
#define AUTO_SYNC_INTERVAL 30000      // 30 seconds
#define PEER_DISCOVERY_INTERVAL 60000 // 60 seconds
#define PEER_TIMEOUT 120000           // 2 minutes
#define MAX_PEERS 5

// ============================================================================
// System Configuration
// ============================================================================

// Debug Output
#define DEBUG_SERIAL_ENABLED true
#define DEBUG_BLE_COMMANDS true

// Teensy Connection Check
#define TEENSY_CHECK_INTERVAL 5000  // 5 seconds

// Response Timeout
#define RESPONSE_TIMEOUT 500  // milliseconds

#endif // CONFIG_H
