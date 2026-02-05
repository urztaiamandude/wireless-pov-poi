import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../model.dart';
import '../config.dart';
import '../database/patterndb.dart';

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  _HomePageState createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  final ValueNotifier<bool> loading = ValueNotifier<bool>(false);
  final PatternDB patternDB = PatternDB();

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: GestureDetector(
          onLongPress: () {
            // Secret settings page - placeholder for now
            ScaffoldMessenger.of(context).showSnackBar(
              SnackBar(content: Text("Settings (to be implemented)")),
            );
          },
          child: const Text("Wireless POV Poi"), // Updated title
        ),
        actions: [
          ...Provider.of<Model>(context)
              .connectedPoi!
              .map((e) => ConnectionStateIndicator(
                  Provider.of<Model>(context).connectedPoi!.indexOf(e)))
        ],
      ),
      body: Stack(
        children: [
          ListView(
            children: [
              getDeviceInfoCard(),
              getImagesList(),
            ],
          ),
          ValueListenableBuilder<bool>(
            valueListenable: loading,
            builder: (context, isLoading, child) {
              return isLoading ? getLoadingOverlay() : Container();
            },
          ),
        ],
      ),
    );
  }

  // Add device info card
  Widget getDeviceInfoCard() {
    return Card(
      margin: EdgeInsets.all(16.0),
      elevation: 5,
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              "Device Info",
              style: TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
                color: Colors.blue,
              ),
            ),
            SizedBox(height: 8),
            Text("LED Type: ${WirelessPOVConfig.LED_TYPE}"),
            Text("LED Count: ${WirelessPOVConfig.LED_COUNT}"),
            Text("Max Pattern Size: ${WirelessPOVConfig.MAX_PATTERN_WIDTH}×${WirelessPOVConfig.MAX_PATTERN_HEIGHT}"),
            Text("Connected Devices: ${Provider.of<Model>(context).connectedPoi?.length ?? 0}"),
          ],
        ),
      ),
    );
  }

  Widget getImagesList() {
    return Card(
      margin: EdgeInsets.all(16.0),
      elevation: 5,
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              "Pattern Library",
              style: TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
                color: Colors.blue,
              ),
            ),
            SizedBox(height: 8),
            Text("Pattern management coming soon..."),
            SizedBox(height: 16),
            ElevatedButton.icon(
              onPressed: () {
                ScaffoldMessenger.of(context).showSnackBar(
                  SnackBar(content: Text("Import pattern (to be implemented)")),
                );
              },
              icon: Icon(Icons.add),
              label: Text("Import Pattern"),
            ),
          ],
        ),
      ),
    );
  }

  Widget getLoadingOverlay() {
    return Container(
      color: Colors.black54,
      child: Center(
        child: CircularProgressIndicator(),
      ),
    );
  }
}

class ConnectionStateIndicator extends StatelessWidget {
  final int index;
  
  const ConnectionStateIndicator(this.index, {super.key});

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.all(8.0),
      child: Icon(
        Icons.bluetooth_connected,
        color: Colors.green,
      ),
    );
  }
}
