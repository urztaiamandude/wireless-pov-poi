import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../model.dart';
import '../config.dart';
import '../database/patterndb.dart';
import 'procedural_art_page.dart';
import 'pattern_library_page.dart';
import 'advanced_settings_page.dart';
import 'fleet_management_page.dart';
import 'sequence_editor_page.dart';

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
            // Navigate to advanced settings page on long press
            Navigator.push(
              context,
              MaterialPageRoute(builder: (context) => const AdvancedSettingsPage()),
            );
          },
          child: const Text("Wireless POV Poi"), // Updated title
        ),
        actions: [
          IconButton(
            icon: const Icon(Icons.devices),
            onPressed: () {
              Navigator.push(
                context,
                MaterialPageRoute(builder: (context) => const FleetManagementPage()),
              );
            },
            tooltip: 'Fleet Management',
          ),
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
              getQuickActionsCard(),
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

  Widget getQuickActionsCard() {
    return Card(
      margin: const EdgeInsets.all(16.0),
      elevation: 5,
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              "Quick Actions",
              style: TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
                color: Colors.purple,
              ),
            ),
            const SizedBox(height: 16),
            GridView.count(
              shrinkWrap: true,
              physics: const NeverScrollableScrollPhysics(),
              crossAxisCount: 2,
              mainAxisSpacing: 12,
              crossAxisSpacing: 12,
              childAspectRatio: 2.5,
              children: [
                _buildQuickActionButton(
                  icon: Icons.brush,
                  label: 'Generate Art',
                  color: Colors.purple,
                  onPressed: () {
                    Navigator.push(
                      context,
                      MaterialPageRoute(builder: (context) => const ProceduralArtPage()),
                    );
                  },
                ),
                _buildQuickActionButton(
                  icon: Icons.photo_library,
                  label: 'Pattern Library',
                  color: Colors.blue,
                  onPressed: () {
                    Navigator.push(
                      context,
                      MaterialPageRoute(builder: (context) => const PatternLibraryPage()),
                    );
                  },
                ),
                _buildQuickActionButton(
                  icon: Icons.playlist_play,
                  label: 'Sequences',
                  color: Colors.deepPurple,
                  onPressed: () {
                    Navigator.push(
                      context,
                      MaterialPageRoute(builder: (context) => const SequenceEditorPage()),
                    );
                  },
                ),
                _buildQuickActionButton(
                  icon: Icons.settings,
                  label: 'Settings',
                  color: Colors.orange,
                  onPressed: () {
                    Navigator.push(
                      context,
                      MaterialPageRoute(builder: (context) => const AdvancedSettingsPage()),
                    );
                  },
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildQuickActionButton({
    required IconData icon,
    required String label,
    required Color color,
    required VoidCallback onPressed,
  }) {
    return ElevatedButton(
      onPressed: onPressed,
      style: ElevatedButton.styleFrom(
        backgroundColor: color,
        foregroundColor: Colors.white,
        shape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(12),
        ),
        padding: const EdgeInsets.symmetric(vertical: 12),
      ),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(icon, size: 20),
          const SizedBox(width: 8),
          Text(
            label,
            style: const TextStyle(fontSize: 13, fontWeight: FontWeight.bold),
          ),
        ],
      ),
    );
  }

  Widget getImagesList() {
    return Card(
      margin: const EdgeInsets.all(16.0),
      elevation: 5,
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              "Recent Activity",
              style: TextStyle(
                fontSize: 20,
                fontWeight: FontWeight.bold,
                color: Colors.blue,
              ),
            ),
            const SizedBox(height: 8),
            const Text("No recent activity"),
            const SizedBox(height: 16),
            Row(
              children: [
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () {
                      Navigator.push(
                        context,
                        MaterialPageRoute(builder: (context) => const PatternLibraryPage()),
                      );
                    },
                    icon: const Icon(Icons.photo_library),
                    label: const Text("View Library"),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: ElevatedButton.icon(
                    onPressed: () {
                      Navigator.push(
                        context,
                        MaterialPageRoute(builder: (context) => const ProceduralArtPage()),
                      );
                    },
                    icon: const Icon(Icons.add),
                    label: const Text("Create New"),
                  ),
                ),
              ],
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
