import 'package:flutter/foundation.dart';
import 'hardware/poi_hardware.dart';

class Model extends ChangeNotifier {
  List<PoiHardware>? _connectedPoi;

  List<PoiHardware>? get connectedPoi => _connectedPoi;
  set connectedPoi(List<PoiHardware>? value) {
    _connectedPoi = value;
    notifyListeners();
  }

  Model();
}
