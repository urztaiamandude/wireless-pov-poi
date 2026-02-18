import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../model.dart';
import '../config.dart';

class AdvancedSettingsPage extends StatefulWidget {
  const AdvancedSettingsPage({super.key});

  @override
  _AdvancedSettingsPageState createState() => _AdvancedSettingsPageState();
}

class _AdvancedSettingsPageState extends State<AdvancedSettingsPage> {
  // Brightness presets (0-31 for APA102)
  final List<int> _brightnessPresets = [5, 10, 15, 20, 25, 31];
  int _selectedBrightnessPreset = 2; // Index 2 = 15 (default)

  // Speed presets (milliseconds between frames)
  final List<int> _speedPresets = [100, 250, 500, 1000, 1500, 2000];
  int _selectedSpeedPreset = 2; // Index 2 = 500ms (default)

  // Pattern shuffle duration
  int _shuffleDuration = 10; // seconds

  // Device settings
  String _deviceName = 'Wireless POV Poi';
  int _ledCount = WirelessPOVConfig.LED_COUNT;
  String _ledType = WirelessPOVConfig.LED_TYPE;

  // Display settings
  bool _autoConnect = true;
  bool _keepScreenOn = true;
  bool _showDebugInfo = false;

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Advanced Settings'),
        backgroundColor: Colors.blue,
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          _buildDeviceInfoCard(),
          const SizedBox(height: 16),
          _buildBrightnessPresetsCard(),
          const SizedBox(height: 16),
          _buildSpeedPresetsCard(),
          const SizedBox(height: 16),
          _buildDisplaySettingsCard(),
          const SizedBox(height: 16),
          _buildPatternSettingsCard(),
          const SizedBox(height: 16),
          _buildAboutCard(),
        ],
      ),
    );
  }

  Widget _buildDeviceInfoCard() {
    return Card(
      elevation: 5,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Row(
              children: [
                Icon(Icons.devices, color: Colors.blue),
                SizedBox(width: 8),
                Text(
                  'Device Configuration',
                  style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(),
            const SizedBox(height: 8),
            ListTile(
              title: const Text('Device Name'),
              subtitle: Text(_deviceName),
              trailing: const Icon(Icons.edit),
              onTap: () {
                _showEditDialog(
                  'Device Name',
                  _deviceName,
                  (value) => setState(() => _deviceName = value),
                );
              },
            ),
            ListTile(
              title: const Text('LED Type'),
              subtitle: Text(_ledType),
              trailing: const Icon(Icons.info_outline),
            ),
            ListTile(
              title: const Text('LED Count'),
              subtitle: Text('$_ledCount LEDs'),
              trailing: const Icon(Icons.info_outline),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildBrightnessPresetsCard() {
    return Card(
      elevation: 5,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Row(
              children: [
                Icon(Icons.brightness_6, color: Colors.amber),
                SizedBox(width: 8),
                Text(
                  'Brightness Presets',
                  style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(),
            const SizedBox(height: 8),
            const Text(
              'Quick brightness levels (0-31 for APA102)',
              style: TextStyle(fontSize: 12, color: Colors.grey),
            ),
            const SizedBox(height: 16),
            Wrap(
              spacing: 8,
              runSpacing: 8,
              children: List.generate(_brightnessPresets.length, (index) {
                final preset = _brightnessPresets[index];
                final isSelected = _selectedBrightnessPreset == index;
                return ChoiceChip(
                  label: Text('$preset'),
                  selected: isSelected,
                  onSelected: (selected) {
                    setState(() {
                      _selectedBrightnessPreset = index;
                    });
                  },
                  selectedColor: Colors.amber[300],
                );
              }),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildSpeedPresetsCard() {
    return Card(
      elevation: 5,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Row(
              children: [
                Icon(Icons.speed, color: Colors.green),
                SizedBox(width: 8),
                Text(
                  'Speed Presets',
                  style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(),
            const SizedBox(height: 8),
            const Text(
              'Animation speed in milliseconds',
              style: TextStyle(fontSize: 12, color: Colors.grey),
            ),
            const SizedBox(height: 16),
            Wrap(
              spacing: 8,
              runSpacing: 8,
              children: List.generate(_speedPresets.length, (index) {
                final preset = _speedPresets[index];
                final isSelected = _selectedSpeedPreset == index;
                return ChoiceChip(
                  label: Text('${preset}ms'),
                  selected: isSelected,
                  onSelected: (selected) {
                    setState(() {
                      _selectedSpeedPreset = index;
                    });
                  },
                  selectedColor: Colors.green[300],
                );
              }),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildDisplaySettingsCard() {
    return Card(
      elevation: 5,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Row(
              children: [
                Icon(Icons.settings, color: Colors.purple),
                SizedBox(width: 8),
                Text(
                  'Display Settings',
                  style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(),
            SwitchListTile(
              title: const Text('Auto-connect to devices'),
              subtitle: const Text('Automatically connect when devices are found'),
              value: _autoConnect,
              onChanged: (value) {
                setState(() {
                  _autoConnect = value;
                });
              },
            ),
            SwitchListTile(
              title: const Text('Keep screen on'),
              subtitle: const Text('Prevent screen from sleeping'),
              value: _keepScreenOn,
              onChanged: (value) {
                setState(() {
                  _keepScreenOn = value;
                });
              },
            ),
            SwitchListTile(
              title: const Text('Show debug info'),
              subtitle: const Text('Display technical information'),
              value: _showDebugInfo,
              onChanged: (value) {
                setState(() {
                  _showDebugInfo = value;
                });
              },
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildPatternSettingsCard() {
    return Card(
      elevation: 5,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Row(
              children: [
                Icon(Icons.palette, color: Colors.orange),
                SizedBox(width: 8),
                Text(
                  'Pattern Settings',
                  style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(),
            const SizedBox(height: 8),
            ListTile(
              title: const Text('Pattern Shuffle Duration'),
              subtitle: Text('$_shuffleDuration seconds'),
              trailing: Row(
                mainAxisSize: MainAxisSize.min,
                children: [
                  IconButton(
                    icon: const Icon(Icons.remove),
                    onPressed: _shuffleDuration > 1
                        ? () => setState(() => _shuffleDuration--)
                        : null,
                  ),
                  IconButton(
                    icon: const Icon(Icons.add),
                    onPressed: _shuffleDuration < 60
                        ? () => setState(() => _shuffleDuration++)
                        : null,
                  ),
                ],
              ),
            ),
            const SizedBox(height: 8),
            Slider(
              value: _shuffleDuration.toDouble(),
              min: 1,
              max: 60,
              divisions: 59,
              label: '$_shuffleDuration seconds',
              onChanged: (value) {
                setState(() {
                  _shuffleDuration = value.toInt();
                });
              },
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildAboutCard() {
    return Card(
      elevation: 5,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Row(
              children: [
                Icon(Icons.info, color: Colors.blue),
                SizedBox(width: 8),
                Text(
                  'About',
                  style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(),
            const SizedBox(height: 8),
            ListTile(
              title: const Text('App Version'),
              subtitle: Text(WirelessPOVConfig.APP_VERSION),
            ),
            ListTile(
              title: const Text('Max Pattern Size'),
              subtitle: Text(
                '${WirelessPOVConfig.MAX_PATTERN_WIDTH}×${WirelessPOVConfig.MAX_PATTERN_HEIGHT} pixels',
              ),
            ),
            ListTile(
              title: const Text('Pattern Banks'),
              subtitle: Text('${WirelessPOVConfig.TOTAL_PATTERN_SLOTS} slots (3 banks × 5 patterns)'),
            ),
            const SizedBox(height: 8),
            ElevatedButton.icon(
              onPressed: () {
                showDialog(
                  context: context,
                  builder: (context) => AlertDialog(
                    title: const Text('Reset Settings'),
                    content: const Text('Reset all settings to default values?'),
                    actions: [
                      TextButton(
                        onPressed: () => Navigator.pop(context),
                        child: const Text('Cancel'),
                      ),
                      ElevatedButton(
                        onPressed: () {
                          Navigator.pop(context);
                          _resetSettings();
                        },
                        style: ElevatedButton.styleFrom(
                          backgroundColor: Colors.red,
                        ),
                        child: const Text('Reset'),
                      ),
                    ],
                  ),
                );
              },
              icon: const Icon(Icons.restore),
              label: const Text('Reset to Defaults'),
              style: ElevatedButton.styleFrom(
                backgroundColor: Colors.orange,
                minimumSize: const Size(double.infinity, 48),
              ),
            ),
          ],
        ),
      ),
    );
  }

  void _showEditDialog(String title, String currentValue, Function(String) onSave) {
    final controller = TextEditingController(text: currentValue);
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text('Edit $title'),
        content: TextField(
          controller: controller,
          decoration: InputDecoration(
            labelText: title,
            border: const OutlineInputBorder(),
          ),
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () {
              onSave(controller.text);
              Navigator.pop(context);
            },
            child: const Text('Save'),
          ),
        ],
      ),
    );
  }

  void _resetSettings() {
    setState(() {
      _selectedBrightnessPreset = 2;
      _selectedSpeedPreset = 2;
      _shuffleDuration = 10;
      _deviceName = 'Wireless POV Poi';
      _autoConnect = true;
      _keepScreenOn = true;
      _showDebugInfo = false;
    });
    ScaffoldMessenger.of(context).showSnackBar(
      const SnackBar(content: Text('Settings reset to defaults')),
    );
  }
}
