import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../model.dart';
import '../hardware/poi_hardware.dart';

class FleetManagementPage extends StatefulWidget {
  const FleetManagementPage({super.key});

  @override
  _FleetManagementPageState createState() => _FleetManagementPageState();
}

class _FleetManagementPageState extends State<FleetManagementPage> {
  bool _isSyncMode = true;
  int _globalBrightness = 15;
  int _globalSpeed = 500;
  Set<int> _selectedDevices = {};
  bool _isSelectionMode = false;

  @override
  Widget build(BuildContext context) {
    final model = Provider.of<Model>(context);
    final devices = model.connectedPoi ?? [];

    return Scaffold(
      appBar: AppBar(
        title: _isSelectionMode
            ? Text('${_selectedDevices.length} selected')
            : const Text('Fleet Management'),
        leading: _isSelectionMode
            ? IconButton(
                icon: const Icon(Icons.close),
                onPressed: () {
                  setState(() {
                    _isSelectionMode = false;
                    _selectedDevices.clear();
                  });
                },
              )
            : null,
        actions: [
          if (!_isSelectionMode)
            IconButton(
              icon: const Icon(Icons.refresh),
              onPressed: () {
                // Refresh device list
                ScaffoldMessenger.of(context).showSnackBar(
                  const SnackBar(content: Text('Refreshing devices...')),
                );
              },
            ),
        ],
        backgroundColor: Colors.blue,
      ),
      body: devices.isEmpty
          ? _buildEmptyState()
          : Column(
              children: [
                _buildSyncModeCard(devices),
                _buildGlobalControlsCard(devices),
                const Divider(height: 1),
                Expanded(
                  child: _buildDeviceList(devices),
                ),
              ],
            ),
      floatingActionButton: devices.isNotEmpty
          ? FloatingActionButton.extended(
              onPressed: () {
                _showSyncAllDialog(devices);
              },
              icon: const Icon(Icons.sync),
              label: const Text('Sync All'),
              backgroundColor: Colors.green,
            )
          : null,
    );
  }

  Widget _buildEmptyState() {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          const Icon(Icons.devices_other, size: 64, color: Colors.grey),
          const SizedBox(height: 16),
          const Text(
            'No devices connected',
            style: TextStyle(fontSize: 18, color: Colors.grey),
          ),
          const SizedBox(height: 8),
          const Text(
            'Connect to devices to manage them',
            style: TextStyle(fontSize: 14, color: Colors.grey),
          ),
          const SizedBox(height: 24),
          ElevatedButton.icon(
            onPressed: () {
              Navigator.pop(context);
            },
            icon: const Icon(Icons.bluetooth_searching),
            label: const Text('Scan for Devices'),
          ),
        ],
      ),
    );
  }

  Widget _buildSyncModeCard(List<PoiHardware> devices) {
    return Card(
      margin: const EdgeInsets.all(16),
      elevation: 5,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(
                  _isSyncMode ? Icons.sync : Icons.person,
                  color: _isSyncMode ? Colors.green : Colors.blue,
                ),
                const SizedBox(width: 8),
                const Text(
                  'Control Mode',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const SizedBox(height: 8),
            SwitchListTile(
              title: const Text('Synchronized Control'),
              subtitle: Text(
                _isSyncMode
                    ? 'Commands sent to all devices'
                    : 'Control devices individually',
              ),
              value: _isSyncMode,
              onChanged: (value) {
                setState(() {
                  _isSyncMode = value;
                });
              },
              activeColor: Colors.green,
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildGlobalControlsCard(List<PoiHardware> devices) {
    return Card(
      margin: const EdgeInsets.symmetric(horizontal: 16),
      elevation: 5,
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Row(
              children: [
                Icon(Icons.tune, color: Colors.orange),
                SizedBox(width: 8),
                Text(
                  'Global Controls',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(),
            const SizedBox(height: 8),
            Text(
              'Brightness: $_globalBrightness',
              style: const TextStyle(fontWeight: FontWeight.bold),
            ),
            Slider(
              value: _globalBrightness.toDouble(),
              min: 0,
              max: 31,
              divisions: 31,
              label: _globalBrightness.toString(),
              onChanged: (value) {
                setState(() {
                  _globalBrightness = value.toInt();
                });
              },
              onChangeEnd: (value) {
                _applyGlobalBrightness(devices);
              },
            ),
            const SizedBox(height: 8),
            Text(
              'Speed: $_globalSpeed ms',
              style: const TextStyle(fontWeight: FontWeight.bold),
            ),
            Slider(
              value: _globalSpeed.toDouble(),
              min: 100,
              max: 2000,
              divisions: 19,
              label: '$_globalSpeed ms',
              onChanged: (value) {
                setState(() {
                  _globalSpeed = value.toInt();
                });
              },
              onChangeEnd: (value) {
                _applyGlobalSpeed(devices);
              },
            ),
            const SizedBox(height: 16),
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceEvenly,
              children: [
                ElevatedButton.icon(
                  onPressed: () => _sendGlobalCommand(devices, 'play'),
                  icon: const Icon(Icons.play_arrow),
                  label: const Text('Play'),
                  style: ElevatedButton.styleFrom(backgroundColor: Colors.green),
                ),
                ElevatedButton.icon(
                  onPressed: () => _sendGlobalCommand(devices, 'pause'),
                  icon: const Icon(Icons.pause),
                  label: const Text('Pause'),
                  style: ElevatedButton.styleFrom(backgroundColor: Colors.orange),
                ),
                ElevatedButton.icon(
                  onPressed: () => _sendGlobalCommand(devices, 'stop'),
                  icon: const Icon(Icons.stop),
                  label: const Text('Stop'),
                  style: ElevatedButton.styleFrom(backgroundColor: Colors.red),
                ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildDeviceList(List<PoiHardware> devices) {
    return ListView.builder(
      padding: const EdgeInsets.all(16),
      itemCount: devices.length,
      itemBuilder: (context, index) {
        final device = devices[index];
        final isSelected = _selectedDevices.contains(index);
        return _buildDeviceCard(device, index, isSelected);
      },
    );
  }

  Widget _buildDeviceCard(PoiHardware device, int index, bool isSelected) {
    return Card(
      elevation: isSelected ? 8 : 3,
      color: isSelected ? Colors.blue[50] : null,
      margin: const EdgeInsets.only(bottom: 12),
      child: ListTile(
        leading: Stack(
          children: [
            CircleAvatar(
              backgroundColor: _getDeviceStatusColor(device),
              child: Text(
                '${index + 1}',
                style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold),
              ),
            ),
            if (isSelected)
              const Positioned(
                right: -2,
                top: -2,
                child: Icon(Icons.check_circle, color: Colors.blue, size: 20),
              ),
          ],
        ),
        title: Text(
          device.name ?? 'Device ${index + 1}',
          style: const TextStyle(fontWeight: FontWeight.bold),
        ),
        subtitle: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('ID: ${device.id ?? "Unknown"}'),
            Text(
              _getDeviceStatusText(device),
              style: TextStyle(
                color: _getDeviceStatusColor(device),
                fontWeight: FontWeight.bold,
              ),
            ),
          ],
        ),
        trailing: PopupMenuButton<String>(
          onSelected: (value) => _handleDeviceAction(device, value),
          itemBuilder: (context) => [
            const PopupMenuItem(value: 'info', child: Text('Device Info')),
            const PopupMenuItem(value: 'test', child: Text('Test Pattern')),
            const PopupMenuItem(value: 'reset', child: Text('Reset Device')),
            const PopupMenuItem(value: 'disconnect', child: Text('Disconnect')),
          ],
        ),
        onTap: () {
          if (_isSelectionMode) {
            setState(() {
              if (isSelected) {
                _selectedDevices.remove(index);
                if (_selectedDevices.isEmpty) {
                  _isSelectionMode = false;
                }
              } else {
                _selectedDevices.add(index);
              }
            });
          } else {
            _showDeviceDetails(device, index);
          }
        },
        onLongPress: () {
          if (!_isSelectionMode) {
            setState(() {
              _isSelectionMode = true;
              _selectedDevices.add(index);
            });
          }
        },
      ),
    );
  }

  Color _getDeviceStatusColor(PoiHardware device) {
    // In real implementation, check actual device status
    return Colors.green; // Assuming connected
  }

  String _getDeviceStatusText(PoiHardware device) {
    // In real implementation, check actual device status
    return 'Connected';
  }

  void _applyGlobalBrightness(List<PoiHardware> devices) {
    final targets = _isSyncMode ? devices : _getSelectedDevices(devices);
    for (final device in targets) {
      // Apply brightness command to device
      // device.setBrightness(_globalBrightness);
    }
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Brightness set to $_globalBrightness for ${targets.length} device(s)')),
    );
  }

  void _applyGlobalSpeed(List<PoiHardware> devices) {
    final targets = _isSyncMode ? devices : _getSelectedDevices(devices);
    for (final device in targets) {
      // Apply speed command to device
      // device.setSpeed(_globalSpeed);
    }
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Speed set to $_globalSpeed ms for ${targets.length} device(s)')),
    );
  }

  void _sendGlobalCommand(List<PoiHardware> devices, String command) {
    final targets = _isSyncMode ? devices : _getSelectedDevices(devices);
    for (final device in targets) {
      // Send command to device
      // device.sendCommand(command);
    }
    ScaffoldMessenger.of(context).showSnackBar(
      SnackBar(content: Text('Command "$command" sent to ${targets.length} device(s)')),
    );
  }

  List<PoiHardware> _getSelectedDevices(List<PoiHardware> devices) {
    return _selectedDevices.map((index) => devices[index]).toList();
  }

  void _showDeviceDetails(PoiHardware device, int index) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text(device.name ?? 'Device ${index + 1}'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text('Device ID: ${device.id ?? "Unknown"}'),
            const SizedBox(height: 8),
            Text('Status: ${_getDeviceStatusText(device)}'),
            const SizedBox(height: 8),
            const Text('Patterns: 0/15'),
            const SizedBox(height: 8),
            const Text('Battery: N/A'),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Close'),
          ),
        ],
      ),
    );
  }

  void _showSyncAllDialog(List<PoiHardware> devices) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Sync All Devices'),
        content: Text('Synchronize settings across all ${devices.length} devices?'),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(context);
              _applyGlobalBrightness(devices);
              _applyGlobalSpeed(devices);
            },
            child: const Text('Sync'),
          ),
        ],
      ),
    );
  }

  void _handleDeviceAction(PoiHardware device, String action) {
    switch (action) {
      case 'info':
        _showDeviceDetails(device, 0);
        break;
      case 'test':
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Testing ${device.name ?? "device"}...')),
        );
        break;
      case 'reset':
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Resetting ${device.name ?? "device"}...')),
        );
        break;
      case 'disconnect':
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Disconnecting ${device.name ?? "device"}...')),
        );
        break;
    }
  }
}
