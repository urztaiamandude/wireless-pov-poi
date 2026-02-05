import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:provider/provider.dart';
import '../hardware/ble_uart.dart';
import '../hardware/poi_hardware.dart';
import '../model.dart';
import 'home.dart';
import 'dart:io' show Platform;
import 'package:flutter/foundation.dart' show kIsWeb;

class WelcomePage extends StatefulWidget {
  const WelcomePage({super.key});

  @override
  _WelcomePageState createState() => _WelcomePageState();
}

class _WelcomePageState extends State<WelcomePage> {
  List<ScanResult> scanResults = [];
  List<ScanResult> selectedDevices = [];
  bool scanning = false;

  @override
  void initState() {
    super.initState();
    
    // Request permissions on Android
    if (!kIsWeb && Platform.isAndroid) {
      FlutterBluePlus.turnOn();
    }
  }

  void scan() async {
    setState(() {
      scanning = true;
      scanResults.clear();
    });

    try {
      await FlutterBluePlus.startScan(
        withKeywords: ["Wireless POV"], // Updated device name filter
        webOptionalServices: [Guid(BLEUart.SERVICE_UUID)],
        timeout: Duration(seconds: 5),
        androidUsesFineLocation: false
      );

      FlutterBluePlus.scanResults.listen((results) {
        setState(() {
          scanResults = results;
        });
      });

      await Future.delayed(Duration(seconds: 5));
      await FlutterBluePlus.stopScan();
    } catch (e) {
      print("Scan error: $e");
    }

    setState(() {
      scanning = false;
    });
  }

  void connect() async {
    if (selectedDevices.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text("Please select at least one device"))
      );
      return;
    }

    List<PoiHardware> poiList = [];

    for (var result in selectedDevices) {
      try {
        BLEUart uart = BLEUart(result.device);
        await uart.isIntialized;
        PoiHardware poi = PoiHardware(uart);
        poiList.add(poi);
      } catch (e) {
        print("Connection failed for ${result.device.platformName}: $e");
      }
    }

    if (poiList.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text("Failed to connect to any devices"))
      );
      return;
    }

    Provider.of<Model>(context, listen: false).connectedPoi = poiList;

    Navigator.pushReplacement(
      context,
      MaterialPageRoute(builder: (context) => HomePage()),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text("Wireless POV Poi"),
      ),
      body: Column(
        children: [
          Padding(
            padding: const EdgeInsets.all(16.0),
            child: Text(
              "Scan for nearby Wireless POV Poi devices",
              style: TextStyle(fontSize: 18),
              textAlign: TextAlign.center,
            ),
          ),
          ElevatedButton(
            onPressed: scanning ? null : scan,
            child: Text(scanning ? "Scanning..." : "Scan for Devices"),
          ),
          Expanded(
            child: ListView.builder(
              itemCount: scanResults.length,
              itemBuilder: (context, index) {
                var result = scanResults[index];
                bool isSelected = selectedDevices.contains(result);
                
                return CheckboxListTile(
                  title: Text(result.device.platformName.isNotEmpty 
                    ? result.device.platformName 
                    : "Unknown Device"),
                  subtitle: Text(result.device.remoteId.toString()),
                  secondary: Icon(Icons.bluetooth),
                  value: isSelected,
                  onChanged: (bool? value) {
                    setState(() {
                      if (value == true) {
                        selectedDevices.add(result);
                      } else {
                        selectedDevices.remove(result);
                      }
                    });
                  },
                );
              },
            ),
          ),
          Padding(
            padding: const EdgeInsets.all(16.0),
            child: ElevatedButton(
              onPressed: selectedDevices.isEmpty ? null : connect,
              child: Text("Connect (${selectedDevices.length} selected)"),
            ),
          ),
        ],
      ),
    );
  }
}
