import 'ble_uart.dart';
import 'dart:typed_data';
import 'dart:async';

class PoiHardware {
  final BLEUart uart;
  
  // Command codes
  static const int CMD_MODE = 0x01;
  static const int CMD_IMAGE = 0x02;
  static const int CMD_PATTERN = 0x03;
  static const int CMD_LIVE_FRAME = 0x05;
  static const int CMD_BRIGHTNESS = 0x06;
  static const int CMD_FRAMERATE = 0x07;
  static const int CMD_STATUS = 0x10;

  PoiHardware(this.uart);

  Future<void> uploadImage(Uint8List imageData, int width, int height) async {
    // Simple protocol: [0xFF][CMD][LEN_H][LEN_L][DATA...][0xFE]
    List<int> packet = [];
    packet.add(0xFF); // Start marker
    packet.add(CMD_IMAGE);
    
    // Width and height (2 bytes each)
    packet.add((width >> 8) & 0xFF);
    packet.add(width & 0xFF);
    packet.add((height >> 8) & 0xFF);
    packet.add(height & 0xFF);
    
    // Image data
    packet.addAll(imageData);
    packet.add(0xFE); // End marker
    
    // Send in chunks if needed (BLE MTU limit)
    const int maxChunkSize = 509;
    for (int i = 0; i < packet.length; i += maxChunkSize) {
      int end = (i + maxChunkSize < packet.length) ? i + maxChunkSize : packet.length;
      await uart.write(packet.sublist(i, end));
      await Future.delayed(Duration(milliseconds: 50)); // Small delay between chunks
    }
  }

  Future<void> setMode(int mode, int index) async {
    List<int> packet = [
      0xFF,
      CMD_MODE,
      mode,
      index,
      0xFE
    ];
    await uart.write(packet);
  }

  Future<void> setBrightness(int brightness) async {
    List<int> packet = [
      0xFF,
      CMD_BRIGHTNESS,
      brightness,
      0xFE
    ];
    await uart.write(packet);
  }

  Future<void> setFramerate(int framerate) async {
    List<int> packet = [
      0xFF,
      CMD_FRAMERATE,
      framerate,
      0xFE
    ];
    await uart.write(packet);
  }

  Future<void> uploadPattern(int type, int color1, int color2, int speed) async {
    List<int> packet = [
      0xFF,
      CMD_PATTERN,
      type,
      (color1 >> 16) & 0xFF, // R
      (color1 >> 8) & 0xFF,  // G
      color1 & 0xFF,         // B
      (color2 >> 16) & 0xFF, // R
      (color2 >> 8) & 0xFF,  // G
      color2 & 0xFF,         // B
      speed,
      0xFE
    ];
    await uart.write(packet);
  }

  Future<void> disconnect() async {
    await uart.disconnect();
  }

  bool get isConnected => uart.isConnected;
}
