import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../model.dart';
import '../config.dart';
import '../database/patterndb.dart';
import '../database/dbimage.dart';

class PatternLibraryPage extends StatefulWidget {
  const PatternLibraryPage({super.key});

  @override
  _PatternLibraryPageState createState() => _PatternLibraryPageState();
}

class _PatternLibraryPageState extends State<PatternLibraryPage> {
  final PatternDB _patternDB = PatternDB();
  List<DBImage> _allPatterns = [];
  List<DBImage> _filteredPatterns = [];
  String _searchQuery = '';
  String _sortBy = 'name'; // 'name', 'date', 'size'
  bool _isGridView = true;
  Set<int> _selectedPatterns = {};
  bool _isSelectionMode = false;

  @override
  void initState() {
    super.initState();
    _loadPatterns();
  }

  Future<void> _loadPatterns() async {
    try {
      final patterns = await _patternDB.getAllPatterns();
      setState(() {
        _allPatterns = patterns;
        _applyFilterAndSort();
      });
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Error loading patterns: $e')),
        );
      }
    }
  }

  void _applyFilterAndSort() {
    var filtered = _allPatterns.where((pattern) {
      if (_searchQuery.isEmpty) return true;
      return pattern.name.toLowerCase().contains(_searchQuery.toLowerCase());
    }).toList();

    // Sort
    switch (_sortBy) {
      case 'name':
        filtered.sort((a, b) => a.name.compareTo(b.name));
        break;
      case 'date':
        filtered.sort((a, b) => b.id.compareTo(a.id)); // Newer first
        break;
      case 'size':
        filtered.sort((a, b) {
          final sizeA = a.width * a.height;
          final sizeB = b.width * b.height;
          return sizeB.compareTo(sizeA);
        });
        break;
    }

    setState(() {
      _filteredPatterns = filtered;
    });
  }

  Future<void> _deletePatterns(List<int> ids) async {
    try {
      for (final id in ids) {
        await _patternDB.deletePattern(id);
      }
      setState(() {
        _selectedPatterns.clear();
        _isSelectionMode = false;
      });
      await _loadPatterns();
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Deleted ${ids.length} pattern(s)')),
        );
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Error deleting patterns: $e')),
        );
      }
    }
  }

  void _toggleSelection(int id) {
    setState(() {
      if (_selectedPatterns.contains(id)) {
        _selectedPatterns.remove(id);
        if (_selectedPatterns.isEmpty) {
          _isSelectionMode = false;
        }
      } else {
        _selectedPatterns.add(id);
      }
    });
  }

  void _onPatternTap(DBImage pattern) {
    if (_isSelectionMode) {
      _toggleSelection(pattern.id);
    } else {
      // Show pattern details
      _showPatternDetails(pattern);
    }
  }

  void _onPatternLongPress(DBImage pattern) {
    if (!_isSelectionMode) {
      setState(() {
        _isSelectionMode = true;
        _selectedPatterns.add(pattern.id);
      });
    }
  }

  void _showPatternDetails(DBImage pattern) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text(pattern.name),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Image.memory(pattern.data, height: 200, fit: BoxFit.contain),
            const SizedBox(height: 16),
            Text('Dimensions: ${pattern.width} × ${pattern.height}'),
            Text('Total Pixels: ${pattern.width * pattern.height}'),
            Text('Size: ${(pattern.data.length / 1024).toStringAsFixed(1)} KB'),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Close'),
          ),
          ElevatedButton(
            onPressed: () {
              Navigator.pop(context);
              _uploadPattern(pattern);
            },
            child: const Text('Upload to Poi'),
          ),
        ],
      ),
    );
  }

  Future<void> _uploadPattern(DBImage pattern) async {
    final model = Provider.of<Model>(context, listen: false);
    if (model.connectedPoi == null || model.connectedPoi!.isEmpty) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('No devices connected')),
      );
      return;
    }

    // Upload to first connected device (could be enhanced for multi-device)
    try {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text('Uploading ${pattern.name}...')),
      );
      // Actual upload logic would go here
      // await model.connectedPoi![0].uploadPattern(pattern);
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Upload failed: $e')),
        );
      }
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: _isSelectionMode
            ? Text('${_selectedPatterns.length} selected')
            : const Text('Pattern Library'),
        leading: _isSelectionMode
            ? IconButton(
                icon: const Icon(Icons.close),
                onPressed: () {
                  setState(() {
                    _isSelectionMode = false;
                    _selectedPatterns.clear();
                  });
                },
              )
            : null,
        actions: [
          if (_isSelectionMode)
            IconButton(
              icon: const Icon(Icons.delete),
              onPressed: _selectedPatterns.isEmpty
                  ? null
                  : () {
                      showDialog(
                        context: context,
                        builder: (context) => AlertDialog(
                          title: const Text('Delete Patterns'),
                          content: Text(
                            'Delete ${_selectedPatterns.length} pattern(s)?',
                          ),
                          actions: [
                            TextButton(
                              onPressed: () => Navigator.pop(context),
                              child: const Text('Cancel'),
                            ),
                            ElevatedButton(
                              onPressed: () {
                                Navigator.pop(context);
                                _deletePatterns(_selectedPatterns.toList());
                              },
                              style: ElevatedButton.styleFrom(
                                backgroundColor: Colors.red,
                              ),
                              child: const Text('Delete'),
                            ),
                          ],
                        ),
                      );
                    },
            )
          else ...[
            IconButton(
              icon: Icon(_isGridView ? Icons.view_list : Icons.grid_view),
              onPressed: () {
                setState(() {
                  _isGridView = !_isGridView;
                });
              },
            ),
            PopupMenuButton<String>(
              icon: const Icon(Icons.sort),
              onSelected: (value) {
                setState(() {
                  _sortBy = value;
                  _applyFilterAndSort();
                });
              },
              itemBuilder: (context) => [
                const PopupMenuItem(value: 'name', child: Text('Sort by Name')),
                const PopupMenuItem(value: 'date', child: Text('Sort by Date')),
                const PopupMenuItem(value: 'size', child: Text('Sort by Size')),
              ],
            ),
          ],
        ],
      ),
      body: Column(
        children: [
          // Search Bar
          if (!_isSelectionMode)
            Padding(
              padding: const EdgeInsets.all(16),
              child: TextField(
                decoration: InputDecoration(
                  hintText: 'Search patterns...',
                  prefixIcon: const Icon(Icons.search),
                  suffixIcon: _searchQuery.isNotEmpty
                      ? IconButton(
                          icon: const Icon(Icons.clear),
                          onPressed: () {
                            setState(() {
                              _searchQuery = '';
                              _applyFilterAndSort();
                            });
                          },
                        )
                      : null,
                  border: OutlineInputBorder(
                    borderRadius: BorderRadius.circular(30),
                  ),
                ),
                onChanged: (value) {
                  setState(() {
                    _searchQuery = value;
                    _applyFilterAndSort();
                  });
                },
              ),
            ),

          // Pattern Grid/List
          Expanded(
            child: _filteredPatterns.isEmpty
                ? Center(
                    child: Column(
                      mainAxisAlignment: MainAxisAlignment.center,
                      children: [
                        Icon(Icons.image_not_supported,
                            size: 64, color: Colors.grey),
                        const SizedBox(height: 16),
                        Text(
                          _searchQuery.isEmpty
                              ? 'No patterns yet'
                              : 'No patterns found',
                          style: TextStyle(fontSize: 18, color: Colors.grey),
                        ),
                      ],
                    ),
                  )
                : _isGridView
                    ? GridView.builder(
                        padding: const EdgeInsets.all(16),
                        gridDelegate: const SliverGridDelegateWithFixedCrossAxisCount(
                          crossAxisCount: 2,
                          crossAxisSpacing: 16,
                          mainAxisSpacing: 16,
                          childAspectRatio: 1,
                        ),
                        itemCount: _filteredPatterns.length,
                        itemBuilder: (context, index) {
                          final pattern = _filteredPatterns[index];
                          final isSelected = _selectedPatterns.contains(pattern.id);
                          return _buildPatternCard(pattern, isSelected);
                        },
                      )
                    : ListView.builder(
                        padding: const EdgeInsets.all(16),
                        itemCount: _filteredPatterns.length,
                        itemBuilder: (context, index) {
                          final pattern = _filteredPatterns[index];
                          final isSelected = _selectedPatterns.contains(pattern.id);
                          return _buildPatternListTile(pattern, isSelected);
                        },
                      ),
          ),

          // Stats Bar
          if (!_isSelectionMode && _allPatterns.isNotEmpty)
            Container(
              padding: const EdgeInsets.all(8),
              decoration: BoxDecoration(
                color: Colors.grey[200],
                border: Border(top: BorderSide(color: Colors.grey[300]!)),
              ),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.spaceAround,
                children: [
                  Text(
                    '${_filteredPatterns.length} pattern(s)',
                    style: const TextStyle(fontSize: 12),
                  ),
                  Text(
                    'Total: ${_allPatterns.length}',
                    style: const TextStyle(fontSize: 12),
                  ),
                ],
              ),
            ),
        ],
      ),
    );
  }

  Widget _buildPatternCard(DBImage pattern, bool isSelected) {
    return GestureDetector(
      onTap: () => _onPatternTap(pattern),
      onLongPress: () => _onPatternLongPress(pattern),
      child: Card(
        elevation: isSelected ? 8 : 2,
        color: isSelected ? Colors.blue[50] : null,
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            Expanded(
              child: Stack(
                children: [
                  Container(
                    color: Colors.black,
                    child: Image.memory(
                      pattern.data,
                      fit: BoxFit.contain,
                    ),
                  ),
                  if (isSelected)
                    Positioned(
                      top: 8,
                      right: 8,
                      child: Icon(Icons.check_circle, color: Colors.blue, size: 32),
                    ),
                ],
              ),
            ),
            Padding(
              padding: const EdgeInsets.all(8),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(
                    pattern.name,
                    style: const TextStyle(
                      fontSize: 12,
                      fontWeight: FontWeight.bold,
                    ),
                    maxLines: 1,
                    overflow: TextOverflow.ellipsis,
                  ),
                  Text(
                    '${pattern.width}×${pattern.height}',
                    style: TextStyle(fontSize: 10, color: Colors.grey[600]),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildPatternListTile(DBImage pattern, bool isSelected) {
    return Card(
      elevation: isSelected ? 8 : 2,
      color: isSelected ? Colors.blue[50] : null,
      margin: const EdgeInsets.only(bottom: 8),
      child: ListTile(
        leading: Container(
          width: 60,
          height: 60,
          color: Colors.black,
          child: Image.memory(pattern.data, fit: BoxFit.contain),
        ),
        title: Text(pattern.name),
        subtitle: Text(
          '${pattern.width}×${pattern.height} • ${(pattern.data.length / 1024).toStringAsFixed(1)} KB',
        ),
        trailing: isSelected
            ? const Icon(Icons.check_circle, color: Colors.blue)
            : null,
        onTap: () => _onPatternTap(pattern),
        onLongPress: () => _onPatternLongPress(pattern),
      ),
    );
  }
}
