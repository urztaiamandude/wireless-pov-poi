import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'dart:async';

class BLEUart {
  static const String SERVICE_UUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
  static const String RX_UUID = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";
  static const String TX_UUID = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";

  final BluetoothDevice device;
  BluetoothCharacteristic? rxCharacteristic;
  BluetoothCharacteristic? txCharacteristic;
  
  late Future<void> isInitialized;
  final StreamController<List<int>> _dataStreamController = StreamController<List<int>>.broadcast();
  
  Stream<List<int>> get dataStream => _dataStreamController.stream;

  BLEUart(this.device) {
    isInitialized = _initialize();
  }

  Future<void> _initialize() async {
    try {
      await device.connect(timeout: Duration(seconds: 15));
      List<BluetoothService> services = await device.discoverServices();
      
      for (var service in services) {
        if (service.uuid.toString().toLowerCase() == SERVICE_UUID.toLowerCase()) {
          for (var characteristic in service.characteristics) {
            if (characteristic.uuid.toString().toLowerCase() == RX_UUID.toLowerCase()) {
              rxCharacteristic = characteristic;
            } else if (characteristic.uuid.toString().toLowerCase() == TX_UUID.toLowerCase()) {
              txCharacteristic = characteristic;
              await txCharacteristic!.setNotifyValue(true);
              txCharacteristic!.lastValueStream.listen((value) {
                _dataStreamController.add(value);
              });
            }
          }
        }
      }
      
      if (rxCharacteristic == null || txCharacteristic == null) {
        throw Exception("UART service not found");
      }
    } catch (e) {
      print("BLE initialization error: $e");
      rethrow;
    }
  }

  Future<void> write(List<int> data) async {
    if (rxCharacteristic == null) {
      throw Exception("RX characteristic not initialized");
    }
    await rxCharacteristic!.write(data, withoutResponse: false);
  }

  Future<void> disconnect() async {
    await device.disconnect();
    await _dataStreamController.close();
  }

  bool get isConnected => device.isConnected;
}
