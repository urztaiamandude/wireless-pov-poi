import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../model.dart';
import '../config.dart';
import '../database/patterndb.dart';
import '../database/dbimage.dart';

class SequenceEditorPage extends StatefulWidget {
  const SequenceEditorPage({super.key});

  @override
  _SequenceEditorPageState createState() => _SequenceEditorPageState();
}

class SequenceSegment {
  String patternBank;
  int patternSlot;
  int brightness;
  int speed;
  int duration;

  SequenceSegment({
    this.patternBank = 'A',
    this.patternSlot = 1,
    this.brightness = 15,
    this.speed = 500,
    this.duration = 5000,
  });
}

class _SequenceEditorPageState extends State<SequenceEditorPage> {
  final List<SequenceSegment> _segments = [];
  final PatternDB _patternDB = PatternDB();
  bool _isPlaying = false;
  int _currentSegmentIndex = -1;
  int _totalDuration = 0;

  @override
  void initState() {
    super.initState();
    // Add initial segment
    _segments.add(SequenceSegment());
    _updateTotalDuration();
  }

  void _updateTotalDuration() {
    _totalDuration = _segments.fold(0, (sum, segment) => sum + segment.duration);
  }

  void _addSegment() {
    if (_segments.length < WirelessPOVConfig.MAX_SEQUENCER_SEGMENTS) {
      setState(() {
        _segments.add(SequenceSegment());
        _updateTotalDuration();
      });
    } else {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Maximum ${WirelessPOVConfig.MAX_SEQUENCER_SEGMENTS} segments allowed'),
        ),
      );
    }
  }

  void _removeSegment(int index) {
    setState(() {
      _segments.removeAt(index);
      _updateTotalDuration();
    });
  }

  void _duplicateSegment(int index) {
    if (_segments.length < WirelessPOVConfig.MAX_SEQUENCER_SEGMENTS) {
      setState(() {
        final original = _segments[index];
        _segments.insert(
          index + 1,
          SequenceSegment(
            patternBank: original.patternBank,
            patternSlot: original.patternSlot,
            brightness: original.brightness,
            speed: original.speed,
            duration: original.duration,
          ),
        );
        _updateTotalDuration();
      });
    }
  }

  void _moveSegment(int fromIndex, int toIndex) {
    setState(() {
      final segment = _segments.removeAt(fromIndex);
      _segments.insert(toIndex, segment);
    });
  }

  Future<void> _uploadSequence() async {
    final model = Provider.of<Model>(context, listen: false);
    if (model.connectedPoi == null || model.connectedPoi!.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('No devices connected')),
      );
      return;
    }

    try {
      // In real implementation, encode and upload sequence
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(
          content: Text('Uploading sequence (${_segments.length} segments)...'),
        ),
      );
      
      // Simulate upload
      await Future.delayed(const Duration(seconds: 1));
      
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(
            content: Text('Sequence uploaded successfully!'),
            backgroundColor: Colors.green,
          ),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Upload failed: $e')),
        );
      }
    }
  }

  void _togglePlayback() {
    setState(() {
      _isPlaying = !_isPlaying;
      if (_isPlaying) {
        _startPlayback();
      } else {
        _currentSegmentIndex = -1;
      }
    });
  }

  void _startPlayback() async {
    for (int i = 0; i < _segments.length && _isPlaying; i++) {
      setState(() {
        _currentSegmentIndex = i;
      });
      await Future.delayed(Duration(milliseconds: _segments[i].duration));
    }
    if (mounted) {
      setState(() {
        _isPlaying = false;
        _currentSegmentIndex = -1;
      });
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Sequence Editor'),
        actions: [
          IconButton(
            icon: Icon(_isPlaying ? Icons.stop : Icons.play_arrow),
            onPressed: _togglePlayback,
            tooltip: _isPlaying ? 'Stop Preview' : 'Preview Sequence',
          ),
          IconButton(
            icon: const Icon(Icons.upload),
            onPressed: _segments.isEmpty ? null : _uploadSequence,
            tooltip: 'Upload to Poi',
          ),
        ],
        backgroundColor: Colors.deepPurple,
      ),
      body: Column(
        children: [
          _buildSummaryCard(),
          Expanded(
            child: _segments.isEmpty
                ? _buildEmptyState()
                : ReorderableListView.builder(
                    padding: const EdgeInsets.all(16),
                    itemCount: _segments.length,
                    onReorder: _moveSegment,
                    itemBuilder: (context, index) {
                      return _buildSegmentCard(index, _segments[index]);
                    },
                  ),
          ),
        ],
      ),
      floatingActionButton: FloatingActionButton.extended(
        onPressed: _addSegment,
        icon: const Icon(Icons.add),
        label: const Text('Add Segment'),
        backgroundColor: Colors.deepPurple,
      ),
    );
  }

  Widget _buildSummaryCard() {
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
                const Icon(Icons.info_outline, color: Colors.deepPurple),
                const SizedBox(width: 8),
                const Text(
                  'Sequence Summary',
                  style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                ),
              ],
            ),
            const Divider(),
            Row(
              mainAxisAlignment: MainAxisAlignment.spaceAround,
              children: [
                _buildStat('Segments', _segments.length.toString()),
                _buildStat('Total Time', '${(_totalDuration / 1000).toStringAsFixed(1)}s'),
                _buildStat('Max', '${WirelessPOVConfig.MAX_SEQUENCER_SEGMENTS}'),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildStat(String label, String value) {
    return Column(
      children: [
        Text(
          value,
          style: const TextStyle(
            fontSize: 24,
            fontWeight: FontWeight.bold,
            color: Colors.deepPurple,
          ),
        ),
        Text(
          label,
          style: const TextStyle(fontSize: 12, color: Colors.grey),
        ),
      ],
    );
  }

  Widget _buildEmptyState() {
    return Center(
      child: Column(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          const Icon(Icons.playlist_add, size: 64, color: Colors.grey),
          const SizedBox(height: 16),
          const Text(
            'No segments yet',
            style: TextStyle(fontSize: 18, color: Colors.grey),
          ),
          const SizedBox(height: 8),
          const Text(
            'Tap + to add your first segment',
            style: TextStyle(fontSize: 14, color: Colors.grey),
          ),
        ],
      ),
    );
  }

  Widget _buildSegmentCard(int index, SequenceSegment segment) {
    final isActive = _currentSegmentIndex == index;
    
    return Card(
      key: ValueKey(index),
      elevation: isActive ? 8 : 3,
      color: isActive ? Colors.deepPurple[50] : null,
      margin: const EdgeInsets.only(bottom: 12),
      child: Padding(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                CircleAvatar(
                  backgroundColor: isActive ? Colors.deepPurple : Colors.grey,
                  child: Text(
                    '${index + 1}',
                    style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold),
                  ),
                ),
                const SizedBox(width: 12),
                Expanded(
                  child: Text(
                    'Segment ${index + 1}',
                    style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                  ),
                ),
                PopupMenuButton<String>(
                  onSelected: (value) {
                    switch (value) {
                      case 'duplicate':
                        _duplicateSegment(index);
                        break;
                      case 'delete':
                        _removeSegment(index);
                        break;
                    }
                  },
                  itemBuilder: (context) => [
                    const PopupMenuItem(value: 'duplicate', child: Text('Duplicate')),
                    const PopupMenuItem(value: 'delete', child: Text('Delete')),
                  ],
                ),
              ],
            ),
            const Divider(),
            
            // Pattern Selection
            Row(
              children: [
                const Text('Pattern: ', style: TextStyle(fontWeight: FontWeight.bold)),
                DropdownButton<String>(
                  value: segment.patternBank,
                  items: ['A', 'B', 'C']
                      .map((bank) => DropdownMenuItem(value: bank, child: Text('Bank $bank')))
                      .toList(),
                  onChanged: (value) {
                    setState(() {
                      segment.patternBank = value!;
                    });
                  },
                ),
                const SizedBox(width: 8),
                DropdownButton<int>(
                  value: segment.patternSlot,
                  items: List.generate(5, (i) => i + 1)
                      .map((slot) => DropdownMenuItem(value: slot, child: Text('Slot $slot')))
                      .toList(),
                  onChanged: (value) {
                    setState(() {
                      segment.patternSlot = value!;
                    });
                  },
                ),
              ],
            ),
            const SizedBox(height: 8),
            
            // Brightness Slider
            Row(
              children: [
                const Text('Brightness: ', style: TextStyle(fontWeight: FontWeight.bold)),
                Expanded(
                  child: Slider(
                    value: segment.brightness.toDouble(),
                    min: 0,
                    max: 31,
                    divisions: 31,
                    label: segment.brightness.toString(),
                    onChanged: (value) {
                      setState(() {
                        segment.brightness = value.toInt();
                      });
                    },
                  ),
                ),
                Text('${segment.brightness}'),
              ],
            ),
            
            // Speed Slider
            Row(
              children: [
                const Text('Speed: ', style: TextStyle(fontWeight: FontWeight.bold)),
                Expanded(
                  child: Slider(
                    value: segment.speed.toDouble(),
                    min: 100,
                    max: 2000,
                    divisions: 19,
                    label: '${segment.speed}ms',
                    onChanged: (value) {
                      setState(() {
                        segment.speed = value.toInt();
                      });
                    },
                  ),
                ),
                Text('${segment.speed}ms'),
              ],
            ),
            
            // Duration Slider
            Row(
              children: [
                const Text('Duration: ', style: TextStyle(fontWeight: FontWeight.bold)),
                Expanded(
                  child: Slider(
                    value: segment.duration.toDouble(),
                    min: 1000,
                    max: 20000,
                    divisions: 19,
                    label: '${(segment.duration / 1000).toStringAsFixed(1)}s',
                    onChanged: (value) {
                      setState(() {
                        segment.duration = value.toInt();
                        _updateTotalDuration();
                      });
                    },
                  ),
                ),
                Text('${(segment.duration / 1000).toStringAsFixed(1)}s'),
              ],
            ),
          ],
        ),
      ),
    );
  }
}
