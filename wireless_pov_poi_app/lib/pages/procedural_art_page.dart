import 'dart:math';
import 'dart:typed_data';
import 'dart:ui' as ui;
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../model.dart';
import '../config.dart';
import '../database/patterndb.dart';
import '../database/dbimage.dart';

class ProceduralArtPage extends StatefulWidget {
  const ProceduralArtPage({super.key});

  @override
  _ProceduralArtPageState createState() => _ProceduralArtPageState();
}

class _ProceduralArtPageState extends State<ProceduralArtPage> {
  String _patternType = 'organic'; // 'organic' or 'geometric'
  double _complexity = 8.0;
  double _colorSeed = 0.5;
  Uint8List? _generatedImage;
  bool _isGenerating = false;
  final PatternDB _patternDB = PatternDB();

  @override
  void initState() {
    super.initState();
    _generateArt();
  }

  Future<void> _generateArt() async {
    setState(() {
      _isGenerating = true;
    });

    try {
      final image = await _createProceduralImage();
      setState(() {
        _generatedImage = image;
        _isGenerating = false;
      });
    } catch (e) {
      setState(() {
        _isGenerating = false;
      });
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Error generating art: $e')),
        );
      }
    }
  }

  Future<Uint8List> _createProceduralImage() async {
    final int height = WirelessPOVConfig.LED_COUNT;
    final int width = height * 4; // Aspect ratio for POV display

    final recorder = ui.PictureRecorder();
    final canvas = Canvas(recorder);
    final paint = Paint();

    // Black background
    paint.color = Colors.black;
    canvas.drawRect(Rect.fromLTWH(0, 0, width.toDouble(), height.toDouble()), paint);

    final random = Random(_colorSeed.hashCode);
    final hueStart = _colorSeed * 360;

    if (_patternType == 'organic') {
      _drawOrganicPattern(canvas, width, height, hueStart, random);
    } else {
      _drawGeometricPattern(canvas, width, height, hueStart, random);
    }

    final picture = recorder.endRecording();
    final img = await picture.toImage(width, height);
    final byteData = await img.toByteData(format: ui.ImageByteFormat.png);
    return byteData!.buffer.asUint8List();
  }

  void _drawOrganicPattern(Canvas canvas, int width, int height, double hueStart, Random random) {
    final paint = Paint()..style = PaintingStyle.stroke;
    final complexity = _complexity.toInt();

    for (int i = 0; i < complexity; i++) {
      final xOffset = random.nextDouble() * width;
      final freq = 0.01 + (random.nextDouble() * 0.04);
      final amp = (height / 4) + random.nextDouble() * (height / 2);
      final hue = (hueStart + (i * (360 / complexity))) % 360;
      
      paint.color = HSVColor.fromAHSV(0.7, hue, 0.9, 0.6).toColor();
      paint.strokeWidth = 2 + random.nextDouble() * 8;

      final path = Path();
      bool first = true;
      for (int x = 0; x <= width; x++) {
        final y = (height / 2) + 
                  sin(x * freq + xOffset) * amp + 
                  cos(x * freq * 0.5) * (amp * 0.3);
        if (first) {
          path.moveTo(x.toDouble(), y);
          first = false;
        } else {
          path.lineTo(x.toDouble(), y);
        }
      }
      canvas.drawPath(path, paint);
    }
  }

  void _drawGeometricPattern(Canvas canvas, int width, int height, double hueStart, Random random) {
    final paint = Paint();
    final cols = max(4, (_complexity * 2).toInt());
    final cellSize = width / cols;
    final rows = (height / cellSize).floor().clamp(1, height);

    for (int x = 0; x < cols; x++) {
      for (int y = 0; y < rows; y++) {
        if (random.nextDouble() > 0.4) {
          final hue = (hueStart + (random.nextDouble() * 60)) % 360;
          paint.color = HSVColor.fromAHSV(0.9, hue, 0.9, 0.5).toColor();
          
          final px = x * cellSize;
          final py = y * cellSize;
          final size = cellSize * (0.5 + random.nextDouble() * 0.4);
          final shape = random.nextInt(3);

          switch (shape) {
            case 0: // Rectangle
              canvas.drawRect(
                Rect.fromLTWH(px, py, size, size),
                paint,
              );
              break;
            case 1: // Circle
              canvas.drawCircle(
                Offset(px + size / 2, py + size / 2),
                size / 2,
                paint,
              );
              break;
            case 2: // Triangle
              final path = Path()
                ..moveTo(px + size / 2, py)
                ..lineTo(px, py + size)
                ..lineTo(px + size, py + size)
                ..close();
              canvas.drawPath(path, paint);
              break;
          }
        }
      }
    }
  }

  Future<void> _savePattern() async {
    if (_generatedImage == null) return;

    final name = 'Procedural_${_patternType}_${DateTime.now().millisecondsSinceEpoch}';
    
    try {
      await _patternDB.insertPattern(DBImage(
        name: name,
        width: WirelessPOVConfig.LED_COUNT * 4,
        height: WirelessPOVConfig.LED_COUNT,
        data: _generatedImage!,
      ));

      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Pattern saved: $name')),
        );
        Navigator.pop(context, true); // Return to previous page
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Error saving pattern: $e')),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Procedural Art Generator'),
        actions: [
          IconButton(
            icon: const Icon(Icons.save),
            onPressed: _generatedImage != null ? _savePattern : null,
            tooltip: 'Save Pattern',
          ),
        ],
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(16),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // Preview Card
            Card(
              elevation: 5,
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  children: [
                    const Text(
                      'Preview',
                      style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                    ),
                    const SizedBox(height: 16),
                    Container(
                      height: 200,
                      decoration: BoxDecoration(
                        color: Colors.black,
                        border: Border.all(color: Colors.grey),
                      ),
                      child: _isGenerating
                          ? const Center(child: CircularProgressIndicator())
                          : _generatedImage != null
                              ? Image.memory(
                                  _generatedImage!,
                                  fit: BoxFit.contain,
                                )
                              : const Center(child: Text('No preview')),
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 16),

            // Pattern Type Card
            Card(
              elevation: 5,
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text(
                      'Pattern Type',
                      style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                    ),
                    const SizedBox(height: 8),
                    SegmentedButton<String>(
                      segments: const [
                        ButtonSegment(
                          value: 'organic',
                          label: Text('Organic'),
                          icon: Icon(Icons.waves),
                        ),
                        ButtonSegment(
                          value: 'geometric',
                          label: Text('Geometric'),
                          icon: Icon(Icons.grid_on),
                        ),
                      ],
                      selected: {_patternType},
                      onSelectionChanged: (Set<String> selected) {
                        setState(() {
                          _patternType = selected.first;
                        });
                        _generateArt();
                      },
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 16),

            // Complexity Slider
            Card(
              elevation: 5,
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      'Complexity: ${_complexity.toInt()}',
                      style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                    ),
                    Slider(
                      value: _complexity,
                      min: 2,
                      max: 20,
                      divisions: 18,
                      label: _complexity.toInt().toString(),
                      onChanged: (value) {
                        setState(() {
                          _complexity = value;
                        });
                      },
                      onChangeEnd: (value) {
                        _generateArt();
                      },
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 16),

            // Color Seed Slider
            Card(
              elevation: 5,
              child: Padding(
                padding: const EdgeInsets.all(16),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text(
                      'Color Palette',
                      style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
                    ),
                    Slider(
                      value: _colorSeed,
                      min: 0,
                      max: 1,
                      onChanged: (value) {
                        setState(() {
                          _colorSeed = value;
                        });
                      },
                      onChangeEnd: (value) {
                        _generateArt();
                      },
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 16),

            // Regenerate Button
            ElevatedButton.icon(
              onPressed: _isGenerating ? null : _generateArt,
              icon: const Icon(Icons.refresh),
              label: const Text('Regenerate'),
              style: ElevatedButton.styleFrom(
                padding: const EdgeInsets.all(16),
              ),
            ),
          ],
        ),
      ),
    );
  }
}
